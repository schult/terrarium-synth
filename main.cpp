#include <algorithm>
#include <cmath>

#include <daisy_seed.h>
#include <q/fx/envelope.hpp>
#include <q/fx/noise_gate.hpp>
#include <q/pitch/pitch_detector.hpp>
#include <q/support/literals.hpp>
#include <q/support/pitch_names.hpp>
#include <q/synth/pulse_osc.hpp>

#include <util/Blink.h>
#include <util/EffectState.h>
#include <util/LinearRamp.h>
#include <util/Mapping.h>
#include <util/SvFilter.h>
#include <util/Terrarium.h>
#include <util/TriangleSynth.h>

namespace q = cycfi::q;
using namespace q::literals;

constexpr LogMapping trigger_mapping{0.0001, 0.1, 0.75};

Terrarium terrarium;
EffectState interface_state;
EffectState preset_state;
EffectState DSY_QSPI_BSS saved_preset;
bool enable_effect = false;
bool use_preset = false;
bool use_modulate = false;
float blend_duration = 1000; // ms
float trigger_ratio = 1;

//=============================================================================
void processAudioBlock(
    daisy::AudioHandle::InputBuffer in,
    daisy::AudioHandle::OutputBuffer out,
    size_t size)
{
    static constexpr auto min_freq = q::pitch_names::Gb[2];
    static constexpr auto max_freq = q::pitch_names::C[7];
    static constexpr auto hysteresis = -35_dB;

    static const auto sample_rate = terrarium.seed.AudioSampleRate();

    static q::peak_envelope_follower envelope_follower(10_ms, sample_rate);
    static q::noise_gate gate(-120_dB);
    static LinearRamp ramp(0, 0.008);
    static q::pitch_detector pd(min_freq, max_freq, sample_rate, hysteresis);
    static q::phase_iterator phase;
    static TriangleSynth triangle_synth;
    static q::basic_pulse_osc pulse_synth;
    static SvFilter low_pass;
    static SvFilter high_pass;
    static uint32_t blend_begin = terrarium.seed.system.GetNow();

    const auto now = terrarium.seed.system.GetNow();
    const auto blend_elapsed = (now - blend_begin);
    const auto blend_ratio =
        std::clamp((blend_elapsed / blend_duration), 0.0f, 1.0f);

    const auto& s =
        use_modulate ? blended(preset_state, interface_state, blend_ratio) :
        use_preset ? preset_state :
        interface_state;

    const auto trigger = trigger_mapping(trigger_ratio);
    gate.onset_threshold(trigger);
    gate.release_threshold(q::lin_to_db(trigger) - 12_dB);

    const auto shape = s.shape_mapping(s.shape);
    triangle_synth.setSkew(shape);
    pulse_synth.width(shape);

    const auto resonance = s.resonance_mapping(s.resonance);
    const auto lp_corner = s.lowPassCorner(pd.get_frequency());
    const auto hp_corner = s.highPassCorner(pd.get_frequency());
    low_pass.config(lp_corner, sample_rate, resonance);
    high_pass.config(hp_corner, sample_rate, resonance);

    for (size_t i = 0; i < size; ++i)
    {
        const auto dry_signal = in[0][i];
        if (pd(dry_signal))
        {
            phase.set(pd.get_frequency(), sample_rate);
            if (pd.is_note_shift()) blend_begin = terrarium.seed.system.GetNow();
        }

        const auto dry_envelope = envelope_follower(std::abs(dry_signal));
        const auto gate_level = ramp(gate(dry_envelope) ? 1 : 0);
        const auto synth_envelope = gate_level *
            std::lerp(0.25f, dry_envelope, s.envelope_influence);

        const auto oscillator_signal =
            (triangle_synth(phase) * s.triangle_mix) +
            (pulse_synth(phase) * s.pulse_mix);
        low_pass.update(oscillator_signal);
        high_pass.update(oscillator_signal);
        const auto synth_signal = synth_envelope *
            s.blendFilters(low_pass.lowPass(), high_pass.highPass());
        phase++;

        const auto dry_level = s.dry_mapping(s.dry);
        const auto synth_level = s.synth_mapping(s.synth);
        const auto mix =
            (dry_signal * dry_level) + (synth_signal * synth_level);
        out[0][i] = enable_effect ? mix : dry_signal;
        out[1][i] = 0;
    }
}

//=============================================================================
int main()
{
    terrarium.Init();

    daisy::Parameter param_filter_q;

    auto& knob_dry = terrarium.knobs[0];
    auto& knob_synth = terrarium.knobs[1];
    auto& knob_trigger = terrarium.knobs[2];
    auto& knob_shape = terrarium.knobs[3];
    auto& knob_filter = terrarium.knobs[4];
    auto& knob_resonance = terrarium.knobs[5];

    auto& toggle_wave1 = terrarium.toggles[0];
    auto& toggle_wave2 = terrarium.toggles[1];
    auto& toggle_envelope = terrarium.toggles[2];
    auto& toggle_modulate = terrarium.toggles[3];

    auto& stomp_bypass = terrarium.stomps[0];
    auto& stomp_preset = terrarium.stomps[1];

    auto& led_enable = terrarium.leds[0];
    auto& led_preset = terrarium.leds[1];

    preset_state = saved_preset.clamped();
    bool preset_written = false;
    Blink blink;

    uint32_t last_tap = 0;


    terrarium.seed.StartAudio(processAudioBlock);

    terrarium.Loop(100, [&](){
        const auto now = terrarium.seed.system.GetNow();
        const auto elapsed = now - last_tap;

        if (stomp_bypass.RisingEdge())
        {
            enable_effect = !enable_effect;
        }

        use_modulate = toggle_modulate.Pressed();

        if (stomp_preset.RisingEdge())
        {
            if (use_modulate)
            {
                if (elapsed < 2000)
                {
                    blend_duration = elapsed;
                }
                last_tap = now;
            }
            else
            {
                use_preset = !use_preset;
            }

            preset_written = false;
        }

        if ((stomp_preset.TimeHeldMs() > 1000) && !preset_written)
        {
            preset_state = interface_state;

            const auto data = reinterpret_cast<uint8_t*>(&preset_state);
            const auto size = static_cast<uint32_t>(sizeof(preset_state));
            const auto start_addr = reinterpret_cast<uint32_t>(&saved_preset);
            terrarium.seed.qspi.Erase(start_addr, start_addr+size);
            terrarium.seed.qspi.Write(start_addr, size, data);

            preset_written = true;
            blink.reset();
        }

        led_enable.Set(enable_effect ? 1 : 0);

        if (use_modulate)
        {
            if (blink.enabled())
            {
                led_preset.Set(blink.process() ? 1 : 0);
            }
            else if (stomp_preset.Pressed())
            {
                led_preset.Set(1);
            }
            else
            {
                uint32_t duration = blend_duration;
                led_preset.Set(
                    std::abs((2 * (elapsed % duration) / blend_duration) - 1));
            }
        }
        else
        {
            auto preset_led_on = blink.enabled() ? blink.process() : use_preset;
            led_preset.Set(preset_led_on ? 1 : 0);
        }

        interface_state.dry = knob_dry.Process();
        interface_state.synth = knob_synth.Process();
        trigger_ratio = knob_trigger.Process();
        interface_state.shape = knob_shape.Process();
        interface_state.filter = knob_filter.Process();
        interface_state.resonance = knob_resonance.Process();

        const auto w1 = toggle_wave1.Pressed();
        const auto w2 = toggle_wave2.Pressed();
        interface_state.triangle_mix =
            ( !w1 && !w2 ) ? EffectState::ratio_max : EffectState::ratio_min;
        interface_state.pulse_mix =
            ( !w1 &&  w2 ) ? EffectState::ratio_max : EffectState::ratio_min;

        interface_state.envelope_influence = toggle_envelope.Pressed() ?
            EffectState::ratio_max : EffectState::ratio_min;
    });
}
