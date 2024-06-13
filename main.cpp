#include <algorithm>
#include <cmath>

#include <daisy_seed.h>
#include <q/fx/edge.hpp>
#include <q/fx/envelope.hpp>
#include <q/fx/noise_gate.hpp>
#include <q/pitch/pitch_detector.hpp>
#include <q/support/literals.hpp>
#include <q/support/pitch_names.hpp>
#include <q/synth/sin_osc.hpp>

#include <util/Blink.h>
#include <util/EffectState.h>
#include <util/LinearRamp.h>
#include <util/Mapping.h>
#include <util/NoiseSynth.h>
#include <util/PersistentSettings.h>
#include <util/SvFilter.h>
#include <util/Terrarium.h>
#include <util/WaveSynth.h>

namespace q = cycfi::q;
using namespace q::literals;

Terrarium terrarium;
EffectState interface_state;
EffectState preset_state;
bool enable_effect = false;
bool use_preset = false;
bool apply_mod = false;
bool cycle_mod = false;
uint32_t mod_duration = 1000; // ms
float trigger_ratio = 1;

void processAudioBlock(
    daisy::AudioHandle::InputBuffer in,
    daisy::AudioHandle::OutputBuffer out,
    size_t size)
{
    static constexpr auto min_freq = q::pitch_names::Ds[2];
    static constexpr auto max_freq = q::pitch_names::F[6];
    static constexpr auto hysteresis = -35_dB;

    static const auto sample_rate = terrarium.seed.AudioSampleRate();

    static q::peak_envelope_follower envelope_follower(10_ms, sample_rate);
    static q::noise_gate gate(-120_dB);
    static q::rising_edge gate_rising;
    static LinearRamp gate_ramp(0, 0.008);
    static q::pitch_detector pd(min_freq, max_freq, sample_rate, hysteresis);
    static q::phase_iterator phase;
    static WaveSynth wave_synth;
    static NoiseSynth noise_synth;
    static SvFilter low_pass;
    static SvFilter high_pass;
    static uint32_t mod_begin = 0;
    static LinearRamp mod_ramp(0, 0.02);

    const auto now = terrarium.seed.system.GetNow();
    const auto mod_elapsed = (now - mod_begin);
    float meh = 0;
    const auto base_frac = static_cast<float>(mod_elapsed) / mod_duration;
    const auto one_shot_frac = std::clamp(base_frac, 0.0f, 0.5f);
    const auto cycle_frac = std::modf(base_frac, &meh);
    const auto mod_phase =
        q::frac_to_phase(cycle_mod ? cycle_frac : one_shot_frac) -
        q::frac_to_phase(0.25);
    const auto mod_ratio = mod_ramp((q::sin(mod_phase) + 1) / 2);

    const auto& s =
        apply_mod ? blended(preset_state, interface_state, mod_ratio) :
        use_preset ? preset_state :
        interface_state;

    constexpr LogMapping trigger_mapping{0.0001, 0.1, 0.75};
    const auto trigger = trigger_mapping(trigger_ratio);
    gate.onset_threshold(trigger);
    gate.release_threshold(q::lin_to_db(trigger) - 12_dB);

    wave_synth.setShape(s.waveShape());
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
            if (pd.is_note_shift())
            {
                mod_begin = terrarium.seed.system.GetNow();
            }
        }

        const auto no_envelope = 1 / EffectState::max_level;
        const auto dry_envelope = envelope_follower(std::abs(dry_signal));
        const auto gate_state = gate(dry_envelope);
        if (gate_rising(gate_state))
        {
            mod_begin = terrarium.seed.system.GetNow();
        }
        const auto gate_level = gate_ramp(gate_state ? 1 : 0);
        const auto synth_envelope = gate_level *
            std::lerp(no_envelope, dry_envelope, s.envelopeInfluence());

        const auto oscillator_signal =
            (wave_synth.compensated(phase) * s.waveMix()) +
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

int main()
{
    terrarium.Init(true);

    auto settings = loadSettings();
    preset_state = settings.preset;
    mod_duration = settings.mod_duration;

    auto& knob_dry = terrarium.knobs[0];
    auto& knob_synth = terrarium.knobs[1];
    auto& knob_trigger = terrarium.knobs[2];
    auto& knob_wave = terrarium.knobs[3];
    auto& knob_filter = terrarium.knobs[4];
    auto& knob_resonance = terrarium.knobs[5];

    auto& toggle_noise = terrarium.toggles[0];
    auto& toggle_envelope = terrarium.toggles[1];
    auto& toggle_modulate = terrarium.toggles[2];
    auto& toggle_cycle = terrarium.toggles[3];

    auto& stomp_bypass = terrarium.stomps[0];
    auto& stomp_preset = terrarium.stomps[1];

    auto& led_enable = terrarium.leds[0];
    auto& led_preset = terrarium.leds[1];

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
        interface_state.setWaveRatio(knob_wave.Process());
        interface_state.setFilterRatio(knob_filter.Process());
        interface_state.setResonanceRatio(knob_resonance.Process());

        interface_state.setNoiseEnabled(toggle_noise.Pressed());
        interface_state.setEnvelopeEnabled(toggle_envelope.Pressed());
        apply_mod = toggle_modulate.Pressed();
        cycle_mod = toggle_cycle.Pressed();

        if (stomp_bypass.RisingEdge())
        {
            enable_effect = !enable_effect;
        }

        led_enable.Set(enable_effect ? 1 : 0);


        if (apply_mod)
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
                const auto mod_ratio = std::modf((float)elapsed / mod_duration, &i);
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

            settings.preset = preset_state;
            settings.mod_duration = mod_duration;
            saveSettings(terrarium.seed.qspi, settings);

            preset_written = true;
            blink.reset();
        }

        if ((elapsed > 10000) && (mod_duration != settings.mod_duration))
        {
            settings.preset = preset_state;
            settings.mod_duration = mod_duration;
            saveSettings(terrarium.seed.qspi, settings);
        }
    });
}
