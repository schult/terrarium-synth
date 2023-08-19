#include <algorithm>

struct EffectState
{
    static constexpr float min_level = 0.05;
    static constexpr float max_level = 20.0;
    static constexpr float min_blend = 0.0;
    static constexpr float max_blend = 1.0;
    static constexpr float min_duty_cycle = 0.5;
    static constexpr float max_duty_cycle = 1.0;
    static constexpr float min_wave_blend = 0.0;
    static constexpr float max_wave_blend = 1.0;

    float level = min_level;
    float blend = min_blend;
    float duty_cycle = min_duty_cycle;
    float wave_blend = min_wave_blend;

    EffectState clamped() const
    {
        return EffectState{
            .level = std::clamp(level, min_level, max_level),
            .blend = std::clamp(blend, min_blend, max_blend),
            .duty_cycle = std::clamp(duty_cycle, min_duty_cycle, max_duty_cycle),
            .wave_blend = std::clamp(wave_blend, min_wave_blend, max_wave_blend),
        };
    }
};
