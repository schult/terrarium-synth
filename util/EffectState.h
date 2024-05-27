#pragma once

#include <algorithm>
#include <type_traits>

#include <q/detail/fast_math.hpp>

#include <util/Mapping.h>

class EffectState
{
public:
    void setDryRatio(float r) { _dry_ratio = r; }
    void setSynthRatio(float r) { _synth_ratio = r; }
    void setShapeRatio(float r) { _shape_ratio = r; }
    void setFilterRatio(float r) { _filter_ratio = r; }
    void setResonanceRatio(float r) { _resonance_ratio = r; }
    void setPulseEnabled(bool on) { _pulse_ratio = on ? ratio_max : ratio_min; }
    void setTriangleEnabled(bool on) { _triangle_ratio = on ? ratio_max : ratio_min; }
    void setEnvelopeEnabled(bool on) { _envelope_ratio = on ? ratio_max : ratio_min; }

    float dryLevel() const { return dry_mapping(_dry_ratio); }
    float synthLevel() const { return synth_mapping(_synth_ratio); }
    float shape() const { return shape_mapping(_shape_ratio); }
    float resonance() const { return resonance_mapping(_resonance_ratio); }
    float pulseMix() const { return _pulse_ratio; }
    float triangleMix() const { return _triangle_ratio; }
    float envelopeInfluence() const { return _envelope_ratio; }

    float lowPassCorner(float frequency) const
    {
        const auto adjusted_filter =
            std::clamp((2*_filter_ratio), ratio_min, ratio_max);
        const auto factor = low_pass_mapping(adjusted_filter);
        const auto corner = factor * frequency;
        return std::clamp(corner, 1.0f, 23900.0f );
    }

    float highPassCorner(float frequency) const
    {
        const auto adjusted_filter =
            std::clamp((2*_filter_ratio - 1), ratio_min, ratio_max);
        const auto factor = high_pass_mapping(adjusted_filter);
        const auto corner = factor * frequency;
        return std::clamp(corner, 1.0f, 23900.0f );
    }

    float blendFilters(float low_pass_signal, float high_pass_signal) const
    {
        return (_filter_ratio < 0.5) ? low_pass_signal : high_pass_signal;
    }

    EffectState clamped() const
    {
        using std::clamp;
        EffectState s;
        s._dry_ratio = std::clamp(_dry_ratio, ratio_min, ratio_max);
        s._synth_ratio = std::clamp(_synth_ratio, ratio_min, ratio_max);
        s._shape_ratio = std::clamp(_shape_ratio, ratio_min, ratio_max);
        s._filter_ratio = std::clamp(_filter_ratio, ratio_min, ratio_max);
        s._resonance_ratio = std::clamp(_resonance_ratio, ratio_min, ratio_max);
        s._pulse_ratio = clamp(_pulse_ratio, ratio_min, ratio_max);
        s._triangle_ratio = clamp(_triangle_ratio, ratio_min, ratio_max);
        s._envelope_ratio = clamp(_envelope_ratio, ratio_min, ratio_max);
        return s;
    }

private:
    friend constexpr EffectState blended(
        const EffectState& s1, const EffectState& s2, float ratio);

    static constexpr LogMapping dry_mapping{0, 1, 20};
    static constexpr LogMapping synth_mapping{0, 1, 4}; // TODO: Match dry mapping
    static constexpr LinearMapping shape_mapping{0.5, 1.0};
    static constexpr LogMapping low_pass_mapping{2, 200};
    static constexpr LogMapping high_pass_mapping{0, 6, 49};
    static constexpr LogMapping resonance_mapping{0.707, 6};

    static constexpr float ratio_min = 0.0;
    static constexpr float ratio_max = 1.0;

    float _dry_ratio = ratio_min;
    float _synth_ratio = ratio_min;
    float _shape_ratio = ratio_min;
    float _filter_ratio = (ratio_max + ratio_min) / 2;
    float _resonance_ratio = ratio_min;
    float _pulse_ratio = ratio_min;
    float _triangle_ratio = ratio_min;
    float _envelope_ratio = ratio_min;
};

static_assert(std::is_trivially_copyable_v<EffectState>);

constexpr EffectState blended(
    const EffectState& s1, const EffectState& s2, float ratio)
{
    EffectState s;
    s._dry_ratio = std::lerp(s1._dry_ratio, s2._dry_ratio, ratio);
    s._synth_ratio = std::lerp(s1._synth_ratio, s2._synth_ratio, ratio);
    s._shape_ratio = std::lerp(s1._shape_ratio, s2._shape_ratio, ratio);
    s._filter_ratio = std::lerp(s1._filter_ratio, s2._filter_ratio, ratio);
    s._resonance_ratio = std::lerp(s1._resonance_ratio, s2._resonance_ratio, ratio);
    s._pulse_ratio = std::lerp(s1._pulse_ratio, s2._pulse_ratio, ratio);
    s._triangle_ratio = std::lerp(s1._triangle_ratio, s2._triangle_ratio, ratio);
    s._envelope_ratio = std::lerp(s1._envelope_ratio, s2._envelope_ratio, ratio);
    return s;
}
