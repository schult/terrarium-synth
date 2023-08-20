#include <algorithm>

struct EffectState
{
    static constexpr float min_dry_level = 0;
    static constexpr float max_dry_level = 20;

    static constexpr float min_wet_level = 0;
    static constexpr float max_wet_level = 1;

    static constexpr float min_duty_cycle = 0.5;
    static constexpr float max_duty_cycle = 1.0;

    static constexpr float min_wave_blend = 0.0;
    static constexpr float max_wave_blend = 1.0;

    float dry_level = min_dry_level;
    float wet_level = min_wet_level;
    float duty_cycle = min_duty_cycle;
    float wave_blend = min_wave_blend;

    EffectState clamped() const
    {
        using std::clamp;
        return EffectState{
            .dry_level = clamp(dry_level, min_dry_level, max_dry_level),
            .wet_level = clamp(wet_level, min_wet_level, max_wet_level),
            .duty_cycle = clamp(duty_cycle, min_duty_cycle, max_duty_cycle),
            .wave_blend = clamp(wave_blend, min_wave_blend, max_wave_blend),
        };
    }
};
