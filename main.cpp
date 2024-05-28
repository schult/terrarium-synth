#include <algorithm>
#include <cmath>

#include <daisy_seed.h>
#include <q/fx/edge.hpp>
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
#include <util/NoiseSynth.h>
#include <util/SvFilter.h>
#include <util/Terrarium.h>
#include <util/TriangleSynth.h>

namespace q = cycfi::q;
using namespace q::literals;

Terrarium terrarium;
EffectState interface_state;
EffectState preset_state;
EffectState DSY_QSPI_BSS saved_preset;
bool enable_effect = false;
bool use_preset = false;
bool use_modulate = false;
float mod_duration = 1000; // ms
float trigger_ratio = 1;

//=============================================================================
void processAudioBlock(
    daisy::AudioHandle::InputBuffer in,
    daisy::AudioHandle::OutputBuffer out,
    size_t size)
{
    static constexpr auto min_freq = q::pitch_names::E[2];
    static constexpr auto max_freq = q::pitch_names::C[7];
    static constexpr auto hysteresis = -35_dB;

    static const auto sample_rate = terrarium.seed.AudioSampleRate();

    static q::peak_envelope_follower envelope_follower(10_ms, sample_rate);
    static q::noise_gate gate(-120_dB);
    static q::rising_edge edge;
    static LinearRamp ramp(0, 0.008);
    static q::pitch_detector pd(min_freq, max_freq, sample_rate, hysteresis);
    static q::phase_iterator phase;
    static TriangleSynth triangle_synth;
    static q::basic_pulse_osc pulse_synth;
    static NoiseSynth noise_synth;
    static SvFilter low_pass;
    static SvFilter high_pass;
    static uint32_t mod_begin = terrarium.seed.system.GetNow();

    const auto now = terrarium.seed.system.GetNow();
    const auto mod_elapsed = (now - mod_begin);
    const auto mod_ratio =
        std::clamp((mod_elapsed / mod_duration), 0.0f, 1.0f);

    const auto& s =
        use_modulate ? blended(preset_state, interface_state, mod_ratio) :
        use_preset ? preset_state :
        interface_state;

    constexpr LogMapping trigger_mapping{0.0001, 0.1, 0.75};
    const auto trigger = trigger_mapping(trigger_ratio);
    gate.onset_threshold(trigger);
    gate.release_threshold(q::lin_to_db(trigger) - 12_dB);

    const auto shape = s.shape();
    triangle_synth.setSkew(shape);
    pulse_synth.width(shape);
    noise_synth.setSampleDuration(s.noiseSampleDuration(pd.get_frequency()));

    const auto resonance = s.resonance();
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
            if (pd.is_note_shift()) mod_begin = terrarium.seed.system.GetNow();
        }

        const auto no_envelope = 1 / EffectState::max_level;
        const auto dry_envelope = envelope_follower(std::abs(dry_signal));
        const auto gate_state = gate(dry_envelope);
        if (edge(gate_state)) mod_begin = terrarium.seed.system.GetNow();
        const auto gate_level = ramp(gate_state ? 1 : 0);
        const auto synth_envelope = gate_level *
            std::lerp(no_envelope, dry_envelope, s.envelopeInfluence());

        const auto oscillator_signal =
            (triangle_synth(phase) * s.triangleMix()) +
            (pulse_synth(phase) * s.pulseMix()) +
            (noise_synth() * s.noiseMix());
        low_pass.update(oscillator_signal);
        high_pass.update(oscillator_signal);
        const auto filtered_signal =
            (low_pass.lowPass() * s.lowPassMix()) +
            (high_pass.highPass() * s.highPassMix());
        const auto synth_signal = synth_envelope * filtered_signal;
        phase++;

        const auto mix =
            (dry_signal * s.dryLevel()) + (synth_signal * s.synthLevel());
        out[0][i] = enable_effect ? mix : dry_signal;
        out[1][i] = 0;
    }
}

//=============================================================================
int main()
{
    terrarium.Init(true);

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

        interface_state.setDryRatio(knob_dry.Process());
        interface_state.setSynthRatio(knob_synth.Process());
        trigger_ratio = knob_trigger.Process();
        interface_state.setShapeRatio(knob_shape.Process());
        interface_state.setFilterRatio(knob_filter.Process());
        interface_state.setResonanceRatio(knob_resonance.Process());

        const auto w1 = toggle_wave1.Pressed();
        const auto w2 = toggle_wave2.Pressed();
        interface_state.setTriangleEnabled(!w1 && !w2);
        interface_state.setPulseEnabled(!w1 && w2);
        interface_state.setNoiseEnabled(w1 && !w2);

        interface_state.setEnvelopeEnabled(toggle_envelope.Pressed());
        use_modulate = toggle_modulate.Pressed();

        if (stomp_bypass.RisingEdge())
        {
            enable_effect = !enable_effect;
        }

        led_enable.Set(enable_effect ? 1 : 0);


        if (use_modulate)
        {
            if (stomp_preset.RisingEdge())
            {
                if (elapsed < 2000)
                {
                    mod_duration = elapsed;
                }
                last_tap = now;
                preset_written = false;
            }

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
                float i = 0;
                const auto mod_ratio = std::modf(elapsed / mod_duration, &i);
                const auto brightness = std::abs(2*mod_ratio - 1);
                led_preset.Set(brightness);
            }
        }
        else
        {
            if (stomp_preset.RisingEdge())
            {
                use_preset = !use_preset;
                preset_written = false;
            }

            if (blink.enabled())
            {
                led_preset.Set(blink.process() ? 1 : 0);
            }
            else
            {
                led_preset.Set(use_preset ? 1 : 0);
            }
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
    });
}
