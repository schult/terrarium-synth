#pragma once

#include <algorithm>

#include <q/detail/fast_math.hpp>

struct EffectState
{
    static constexpr float dry_level_min = 0;
    static constexpr float dry_level_max = 20;

    static constexpr float synth_level_min = 0;
    static constexpr float synth_level_max = 1;

    static constexpr float duty_cycle_min = 0.5;
    static constexpr float duty_cycle_max = 1.0;

    static constexpr float filter_min = -1;
    static constexpr float filter_max = 1;

    static constexpr float filter_q_min = 0.707;
    static constexpr float filter_q_max = 6;

    static constexpr float mix_min = 0.0;
    static constexpr float mix_max = 1.0;

    static constexpr float envelope_influence_min = 0.0;
    static constexpr float envelope_influence_max = 1.0;

    float dry_level = dry_level_min;
    float synth_level = synth_level_min;
    float duty_cycle = duty_cycle_min;
    float filter = 0;
    float filter_q = filter_q_min;
    float pulse_mix = mix_min;
    float triangle_mix = mix_min;
    float envelope_influence = envelope_influence_min;

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
            .duty_cycle = clamp(duty_cycle, duty_cycle_min, duty_cycle_max),
            .filter = clamp(filter, filter_min, filter_max),
            .filter_q = clamp(filter_q, filter_q_min, filter_q_max),
            .pulse_mix = clamp(pulse_mix, mix_min, mix_max),
            .triangle_mix = clamp(triangle_mix, mix_min, mix_max),
            .envelope_influence = clamp(envelope_influence,
                envelope_influence_min, envelope_influence_max),
        };
    }
};

inline EffectState blended(
    const EffectState& s1, const EffectState& s2, float ratio)
{
    EffectState result;
    result.dry_level = std::lerp(s1.dry_level, s2.dry_level, ratio);
    result.synth_level = std::lerp(s1.synth_level, s2.synth_level, ratio);
    result.duty_cycle = std::lerp(s1.duty_cycle, s2.duty_cycle, ratio);
    result.filter = std::lerp(s1.filter, s2.filter, ratio);
    result.filter_q = std::lerp(s1.filter_q, s2.filter_q, ratio);
    result.pulse_mix = std::lerp(s1.pulse_mix, s2.pulse_mix, ratio);
    result.triangle_mix = std::lerp(s1.triangle_mix, s2.triangle_mix, ratio);
    result.envelope_influence =
        std::lerp(s1.envelope_influence, s2.envelope_influence, ratio);
    return result;
}
