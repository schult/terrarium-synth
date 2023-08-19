#include <algorithm>

struct EffectState
{
    static constexpr float min_level = 0.05;
    static constexpr float max_level = 20.0;
    static constexpr float min_blend = 0.0;
    static constexpr float max_blend = 1.0;
    static constexpr float min_duty_cycle = 0.5;
    static constexpr float max_duty_cycle = 1.0;

    float level = min_level;
    float blend = min_blend;
    float duty_cycle = min_duty_cycle;
    bool wave_shape = true;

    EffectState clamped() const
    {
        EffectState copy = *this;
        copy.level = std::clamp(copy.level, min_level, max_level);
        copy.blend = std::clamp(copy.blend, min_blend, max_blend);
        copy.duty_cycle =
            std::clamp(copy.duty_cycle, min_duty_cycle, max_duty_cycle);
        return copy;
    }
};
