#pragma once

#include <algorithm>

#include <q/detail/fast_math.hpp>

struct EffectState
{
    static constexpr float dry_level_min = 0;
    static constexpr float dry_level_max = 20;

    static constexpr float synth_level_min = 0;
    static constexpr float synth_level_max = 1;

    static constexpr float gate_onset_min = 0.000001;
    static constexpr float gate_onset_max = 0.75;

    static constexpr float duty_cycle_min = 0.5;
    static constexpr float duty_cycle_max = 1.0;

    static constexpr float filter_min = -1;
    static constexpr float filter_max = 1;

    static constexpr float filter_q_min = 0.707;
    static constexpr float filter_q_max = 6;

    static constexpr float wave_blend_min = 0.0;
    static constexpr float wave_blend_max = 1.0;

    float dry_level = dry_level_min;
    float synth_level = synth_level_min;
    float gate_onset = gate_onset_min;
    float duty_cycle = duty_cycle_min;
    float filter = 0;
    float filter_q = filter_q_min;
    float wave_blend = wave_blend_min;
    bool follow_envelope = false;

    float lowPassCorner(float frequency) const
    {
        const auto f = (filter < 0) ? (filter + 1) : 1;
        constexpr float max = 5.2983174f; // ln(200)
        constexpr float min = 0.6931472f; // ln(2)
        constexpr float range = max - min;
        const auto factor = fasterexp((f * range) + min);
        const auto corner = factor * frequency;
        return std::clamp(corner, 1.0f, 23900.0f );
    }

    float highPassCorner(float frequency) const
    {
        const auto f = (filter < 0) ? 0 : filter;
        constexpr float range = 3.9120230f; // ln(50)
        const auto factor = fasterexp(f * range) - 1;
        const auto corner = factor * frequency;
        return std::clamp(corner, 1.0f, 23900.0f );
    }

    float blendFilters(float low_pass_signal, float high_pass_signal) const
    {
        return (filter < 0) ? low_pass_signal : high_pass_signal;
    }

    EffectState clamped() const
    {
        using std::clamp;
        return EffectState{
            .dry_level = clamp(dry_level, dry_level_min, dry_level_max),
            .synth_level = clamp(synth_level, synth_level_min, synth_level_max),
            .gate_onset = clamp(gate_onset, gate_onset_min, gate_onset_max),
            .duty_cycle = clamp(duty_cycle, duty_cycle_min, duty_cycle_max),
            .filter = clamp(filter, filter_min, filter_max),
            .filter_q = clamp(filter_q, filter_q_min, filter_q_max),
            .wave_blend = clamp(wave_blend, wave_blend_min, wave_blend_max),
            .follow_envelope = follow_envelope,
        };
    }
};
