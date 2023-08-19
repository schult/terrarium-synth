#include <algorithm>
#include <cmath>

struct EffectState
{
    static constexpr float min_level = 0.05;
    static constexpr float max_level = 20.0;
    static constexpr float min_wet_blend = 0.0;
    static constexpr float max_wet_blend = 1.0;
    static constexpr float min_duty_cycle = 0.5;
    static constexpr float max_duty_cycle = 1.0;
    static constexpr float min_wave_blend = 0.0;
    static constexpr float max_wave_blend = 1.0;

    float level = min_level;
    float wet_blend = min_wet_blend;
    float duty_cycle = min_duty_cycle;
    float wave_blend = min_wave_blend;

    EffectState clamped() const
    {
        return EffectState{
            .level = std::clamp(level, min_level, max_level),
            .wet_blend = std::clamp(wet_blend, min_wet_blend, max_wet_blend),
            .duty_cycle = std::clamp(duty_cycle, min_duty_cycle, max_duty_cycle),
            .wave_blend = std::clamp(wave_blend, min_wave_blend, max_wave_blend),
        };
    }

    friend EffectState lerp(EffectState& a, EffectState& b, float t)
    {
        return EffectState{
            .level = std::lerp(a.level, b.level, t),
            .wet_blend = std::lerp(a.wet_blend, b.wet_blend, t),
            .duty_cycle = std::lerp(a.duty_cycle, b.duty_cycle, t),
            .wave_blend = std::lerp(a.wave_blend, b.wave_blend, t),
        };
    }
};
