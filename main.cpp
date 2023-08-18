#include <daisy_seed.h>
#include <q/fx/envelope.hpp>
#include <q/pitch/pitch_detector.hpp>
#include <q/support/pitch_names.hpp>
#include <q/synth/pulse_osc.hpp>

#include <util/Terrarium.h>

namespace q = cycfi::q;
using namespace q::literals;

struct EffectState
{
    float blend = 0.0f;
    float duty_cycle = 0.5f;
};

Terrarium terrarium;
EffectState interface_state;
bool enable_effect = false;

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

    static q::peak_envelope_follower envelope_follower(30_ms, sample_rate);
    static q::pitch_detector pd(min_freq, max_freq, sample_rate, hysteresis);
    static q::phase_iterator phase;
    static q::basic_pulse_osc pulse_synth;

    const auto wet_blend = interface_state.blend;
    const auto dry_blend = 1 - wet_blend;
    const auto duty_cycle = interface_state.duty_cycle;

    pulse_synth.width(duty_cycle);

    for (size_t i = 0; i < size; ++i)
    {
        const auto dry = in[0][i];

        const auto envelope = envelope_follower(std::abs(dry));
        if (pd(dry))
        {
            phase.set(pd.get_frequency(), sample_rate);
        }
        const auto wet = pulse_synth(phase++) * envelope;

        const auto mix = (dry * dry_blend) + (wet * wet_blend);
        out[0][i] = enable_effect ? mix : dry;
        out[1][i] = 0;
    }
}

//=============================================================================
int main()
{
    terrarium.Init();

    daisy::Parameter param_blend;
    daisy::Parameter param_duty_cycle;

    auto& knobs = terrarium.knobs;
    param_blend.Init(knobs[1], 0.0, 1.0, daisy::Parameter::LINEAR);
    param_duty_cycle.Init(knobs[2], 0.5, 1.0, daisy::Parameter::LINEAR);

    auto& stomp_bypass = terrarium.stomps[0];

    auto& led_enable = terrarium.leds[0];

    terrarium.seed.StartAudio(processAudioBlock);

    terrarium.Loop(100, [&](){
        interface_state.blend = param_blend.Process();
        interface_state.duty_cycle = param_duty_cycle.Process();

        if (stomp_bypass.RisingEdge())
        {
            enable_effect = !enable_effect;
        }
        led_enable.Set(enable_effect ? 1 : 0);
    });
}
