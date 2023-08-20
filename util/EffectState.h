#include <algorithm>

struct EffectState
{
    static constexpr float dry_level_min = 0;
    static constexpr float dry_level_max = 20;

    static constexpr float synth_level_min = 0;
    static constexpr float synth_level_max = 1;

    static constexpr float duty_cycle_min = 0.5;
    static constexpr float duty_cycle_max = 1.0;

    static constexpr float wave_blend_min = 0.0;
    static constexpr float wave_blend_max = 1.0;

    float dry_level = dry_level_min;
    float synth_level = synth_level_min;
    float duty_cycle = duty_cycle_min;
    float wave_blend = wave_blend_min;

    EffectState clamped() const
    {
        using std::clamp;
        return EffectState{
            .dry_level = clamp(dry_level, dry_level_min, dry_level_max),
            .synth_level = clamp(synth_level, synth_level_min, synth_level_max),
            .duty_cycle = clamp(duty_cycle, duty_cycle_min, duty_cycle_max),
            .wave_blend = clamp(wave_blend, wave_blend_min, wave_blend_max),
        };
    }
};
