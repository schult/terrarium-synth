#pragma once

#include <algorithm>
#include <type_traits>

#include <q/detail/fast_math.hpp>

#include <util/Mapping.h>

struct EffectState
{
    static constexpr LogMapping dry_mapping{0, 1, 20};
    static constexpr LogMapping synth_mapping{0, 1, 4}; // TODO: Match dry mapping
    static constexpr LinearMapping duty_mapping{0.5, 1.0};
    static constexpr LogMapping low_pass_mapping{2, 200};
    static constexpr LogMapping high_pass_mapping{0, 6, 49};
    static constexpr LogMapping resonance_mapping{0.707, 6};

    static constexpr float mix_min = 0.0;
    static constexpr float mix_max = 1.0;

    static constexpr float envelope_influence_min = 0.0;
    static constexpr float envelope_influence_max = 1.0;

    float dry_level = 0;
    float synth_level = 0;
    float duty_cycle = 0;
    float filter = 0.5;
    float resonance = resonance_mapping.min;
    float pulse_mix = mix_min;
    float triangle_mix = mix_min;
    float envelope_influence = envelope_influence_min;

    float lowPassCorner(float frequency) const
    {
        const auto adjusted_filter = std::clamp((2*filter), 0.0f, 1.0f);
        const auto factor = low_pass_mapping(adjusted_filter);
        const auto corner = factor * frequency;
        return std::clamp(corner, 1.0f, 23900.0f );
    }

    float highPassCorner(float frequency) const
    {
        const auto adjusted_filter = std::clamp((2*filter - 1), 0.0f, 1.0f);
        const auto factor = high_pass_mapping(adjusted_filter);
        const auto corner = factor * frequency;
        return std::clamp(corner, 1.0f, 23900.0f );
    }

    float blendFilters(float low_pass_signal, float high_pass_signal) const
    {
        return (filter < 0.5) ? low_pass_signal : high_pass_signal;
    }

    EffectState clamped() const
    {
        using std::clamp;
        return EffectState{
            .dry_level = std::clamp(dry_level, 0.0f, 1.0f),
            .synth_level = std::clamp(synth_level, 0.0f, 1.0f),
            .duty_cycle = std::clamp(duty_cycle, 0.0f, 1.0f),
            .filter = std::clamp(filter, 0.0f, 1.0f),
            .resonance = std::clamp(resonance, 0.0f, 1.0f),
            .pulse_mix = clamp(pulse_mix, mix_min, mix_max),
            .triangle_mix = clamp(triangle_mix, mix_min, mix_max),
            .envelope_influence = clamp(envelope_influence,
                envelope_influence_min, envelope_influence_max),
        };
    }
};
static_assert(std::is_trivially_copyable_v<EffectState>);

constexpr EffectState blended(
    const EffectState& s1, const EffectState& s2, float ratio)
{
    EffectState result;
    result.dry_level = std::lerp(s1.dry_level, s2.dry_level, ratio);
    result.synth_level = std::lerp(s1.synth_level, s2.synth_level, ratio);
    result.duty_cycle = std::lerp(s1.duty_cycle, s2.duty_cycle, ratio);
    result.filter = std::lerp(s1.filter, s2.filter, ratio);
    result.resonance = std::lerp(s1.resonance, s2.resonance, ratio);
    result.pulse_mix = std::lerp(s1.pulse_mix, s2.pulse_mix, ratio);
    result.triangle_mix = std::lerp(s1.triangle_mix, s2.triangle_mix, ratio);
    result.envelope_influence =
        std::lerp(s1.envelope_influence, s2.envelope_influence, ratio);
    return result;
}
