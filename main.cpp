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
    static q::basic_pulse_osc pulse_synth;
    static TriangleSynth triangle_synth;
    static SvFilter filter;

    const auto& s = use_preset ? preset_state : interface_state;

    gate.onset_threshold(s.gate_onset);
    gate.release_threshold(q::lin_to_db(s.gate_onset) - 12_dB);

    pulse_synth.width(s.duty_cycle);
    triangle_synth.setSkew(s.duty_cycle);

    const auto frequency = pd.get_frequency();
    const auto corner = std::clamp(s.filter * frequency, 100.0f, 23000.0f );
    filter.config(corner, sample_rate, s.filter_q);

    for (size_t i = 0; i < size; ++i)
    {
        const auto dry_signal = in[0][i];
        if (pd(dry_signal))
        {
            phase.set(pd.get_frequency(), sample_rate);
        }

        const auto dry_envelope = envelope_follower(std::abs(dry_signal));
        const auto gate_level = ramp(gate(dry_envelope) ? 1 : 0);
        const auto synth_envelope = gate_level *
            (s.follow_envelope ? dry_envelope : 0.25f);

        filter.update(
            std::lerp(triangle_synth(phase), pulse_synth(phase), s.wave_blend));
        phase++;
        const auto synth_signal = synth_envelope * filter.lowPass();

        const auto mix =
            (dry_signal * s.dry_level) + (synth_signal * s.synth_level);
        out[0][i] = enable_effect ? mix : dry_signal;
        out[1][i] = 0;
    }
}

//=============================================================================
int main()
{
    terrarium.Init();

    // This offset keeps unity gain at noon. Assumes min == 0.
    constexpr auto dry_level_offset = 1 / (EffectState::dry_level_max - 2);

    daisy::Parameter param_dry_level;
    daisy::Parameter param_synth_level;
    daisy::Parameter param_duty_cycle;
    daisy::Parameter param_gate_onset;
    daisy::Parameter param_filter;
    daisy::Parameter param_filter_q;

    auto& knobs = terrarium.knobs;
    param_dry_level.Init(
        knobs[0],
        EffectState::dry_level_min + dry_level_offset,
        EffectState::dry_level_max + dry_level_offset,
        daisy::Parameter::LOGARITHMIC);
    param_synth_level.Init(
        knobs[1],
        EffectState::synth_level_min,
        EffectState::synth_level_max,
        daisy::Parameter::LINEAR);
    param_duty_cycle.Init(
        knobs[2],
        EffectState::duty_cycle_min,
        EffectState::duty_cycle_max,
        daisy::Parameter::LINEAR);
    param_gate_onset.Init(
        knobs[3],
        EffectState::gate_onset_min,
        EffectState::gate_onset_max,
        daisy::Parameter::LOGARITHMIC);
    param_filter.Init(
        knobs[4],
        EffectState::filter_min,
        EffectState::filter_max,
        daisy::Parameter::LOGARITHMIC);
    param_filter_q.Init(
        knobs[5],
        EffectState::filter_q_min,
        EffectState::filter_q_max,
        daisy::Parameter::LINEAR);

    auto& toggle_wave_shape = terrarium.toggles[0];
    auto& toggle_envelope = terrarium.toggles[1];

    auto& stomp_bypass = terrarium.stomps[0];
    auto& stomp_preset = terrarium.stomps[1];

    auto& led_enable = terrarium.leds[0];
    auto& preset_led = terrarium.leds[1];

    preset_state = saved_preset.clamped();
    bool preset_written = false;
    Blink blink;


    terrarium.seed.StartAudio(processAudioBlock);

    terrarium.Loop(100, [&](){
        if (stomp_bypass.RisingEdge())
        {
            enable_effect = !enable_effect;
        }

        if (stomp_preset.RisingEdge())
        {
            use_preset = !use_preset;
            preset_written = false;
        }

        if ((stomp_preset.TimeHeldMs() > 5000) && !preset_written)
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
        auto preset_led_on = blink.enabled() ? blink.process() : use_preset;
        preset_led.Set(preset_led_on ? 1 : 0);

        interface_state.dry_level =
            param_dry_level.Process() - dry_level_offset;
        interface_state.synth_level = param_synth_level.Process();
        interface_state.duty_cycle = param_duty_cycle.Process();
        interface_state.gate_onset = param_gate_onset.Process();
        interface_state.filter = param_filter.Process();
        interface_state.filter_q = param_filter_q.Process();
        interface_state.wave_blend = toggle_wave_shape.Pressed() ?
            EffectState::wave_blend_max : EffectState::wave_blend_min;
        interface_state.follow_envelope = toggle_envelope.Pressed();
    });
}
