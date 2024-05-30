#pragma once

#include <algorithm>

#include <q/detail/fast_math.hpp>

#include <util/Mapping.h>

class EffectState
{
public:
    void setDryRatio(float r) { _dry_ratio = r; }
    void setSynthRatio(float r) { _synth_ratio = r; }
    void setWaveRatio(float r) { _wave_ratio = r; }
    void setFilterRatio(float r) { _filter_ratio = r; }
    void setResonanceRatio(float r) { _resonance_ratio = r; }
    void setNoiseEnabled(bool on) { _noise_ratio = on ? ratio_max : ratio_min; }
    void setEnvelopeEnabled(bool on) { _envelope_ratio = on ? ratio_max : ratio_min; }

    float dryLevel() const { return dry_mapping(_dry_ratio); }
    float synthLevel() const { return synth_mapping(_synth_ratio); }
    float waveShape() const { return wave_mapping(_wave_ratio); }
    float resonance() const { return resonance_mapping(_resonance_ratio); }
    float waveMix() const { return (1 - _noise_ratio); }
    float noiseMix() const { return _noise_ratio; }
    float envelopeInfluence() const { return _envelope_ratio; }

    float noiseSampleDuration(float frequency) const
    {
        const auto noise_freq = noise_freq_mapping(ratio_max - _wave_ratio);
        const auto x = noise_freq / frequency;
        return x * x;
    }

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

    float lowPassMix() const
    {
        return (_filter_ratio < 0.5) ? 1 : 0;
    }

    float highPassMix() const
    {
        return (_filter_ratio < 0.5) ? 0 : 1;
    }

    static constexpr float max_level = 20;

private:
    friend constexpr EffectState blended(
        const EffectState& s1, const EffectState& s2, float ratio);

    static constexpr LogMapping dry_mapping{0, 1, max_level};
    static constexpr LogMapping synth_mapping{0, 1, max_level};
    // wave_mapping goes a little beyond 3 to make sure we can reach a full
    // sawtooth wave even if the knob doesn't quite hit 1.0 when maxed out.
    static constexpr LinearMapping wave_mapping{0, 3.1};
    static constexpr LinearMapping noise_freq_mapping{64, 3200};
    static constexpr LogMapping low_pass_mapping{2, 200};
    static constexpr LogMapping high_pass_mapping{0, 6, 49};
    static constexpr LogMapping resonance_mapping{0.707, 6};

    static constexpr float ratio_min = 0.0;
    static constexpr float ratio_max = 1.0;

    float _dry_ratio = ratio_min;
    float _synth_ratio = ratio_min;
    float _wave_ratio = ratio_min;
    float _filter_ratio = (ratio_max + ratio_min) / 2;
    float _resonance_ratio = ratio_min;
    float _noise_ratio = ratio_min;
    float _envelope_ratio = ratio_min;
};

constexpr EffectState blended(
    const EffectState& s1, const EffectState& s2, float ratio)
{
    using std::lerp;
    EffectState s;
    s._dry_ratio = lerp(s1._dry_ratio, s2._dry_ratio, ratio);
    s._synth_ratio = lerp(s1._synth_ratio, s2._synth_ratio, ratio);
    s._wave_ratio = lerp(s1._wave_ratio, s2._wave_ratio, ratio);
    s._filter_ratio = lerp(s1._filter_ratio, s2._filter_ratio, ratio);
    s._resonance_ratio = lerp(s1._resonance_ratio, s2._resonance_ratio, ratio);
    s._noise_ratio = lerp(s1._noise_ratio, s2._noise_ratio, ratio);
    s._envelope_ratio = lerp(s1._envelope_ratio, s2._envelope_ratio, ratio);
    return s;
}
