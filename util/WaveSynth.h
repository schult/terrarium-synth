#pragma once

#include <algorithm>
#include <cmath>

#include <q/support/phase.hpp>

class WaveSynth
{
public:
    constexpr WaveSynth(float shape = 1)
    {
        setShape(shape);
    }

    // 0.0 - 1.0: Pulse - Square
    // 1.0 - 2.0: Square - Triangle
    // 2.0 - 3.0: Triangle - Sawtooth
    constexpr void setShape(float shape)
    {
        using cycfi::q::frac_to_phase;
        using std::lerp;

        shape = std::clamp(shape, 0.0f, 3.0f);

        if (shape <= 1) // Pulse (0.0) to Square (1.0)
        {
            _low_end = frac_to_phase(lerp(0.47f, 0.25f, shape));
            _rise_end = _low_end;
            _high_end = frac_to_phase(lerp(0.53f, 0.75f, shape));
            _fall_end = _high_end;
        }
        else if (shape <= 2) // Square (1.0) to Triangle (2.0)
        {
            shape -= 1;
            _low_end = frac_to_phase(lerp(0.25f, 0.0f, shape));
            _rise_end = frac_to_phase(lerp(0.25f, 0.5f, shape));
            _high_end = frac_to_phase(lerp(0.75f, 0.5f, shape));
            _fall_end = frac_to_phase(lerp(0.75f, 1.0f, shape));
        }
        else // Triangle (2.0) to Sawtooth (3.0)
        {
            shape -= 2;
            _low_end = frac_to_phase(0.0f);
            _rise_end = frac_to_phase(lerp(0.5f, 1.0f, shape));
            _high_end = _rise_end;
            _fall_end = frac_to_phase(1.0f);
        }
    }

    constexpr float operator()(cycfi::q::phase p) const
    {
        // The wave shape is determined by the timing of the inflection points:
        //
        //  1 _|         ________         |
        //     |        /        \        |
        //  0 _|       /          \       |
        //     |      /            \      |
        // -1 _| ____/   |      |   \____ |
        //      0    A   B      C   D   End
        //
        // A: Low End / Rise Begin
        // B: Rise End / High Begin
        // C: High End / Fall Begin
        // D: Fall End / Low Begin

        if (p <= _low_end)
        {
            return -1;
        }

        if (p <= _rise_end)
        {
            const float value = (p - _low_end).rep;
            const float range = (_rise_end - _low_end).rep;
            const auto t = value / range;
            return std::lerp(-1.0f, 1.0f, t);
        }

        if (p <= _high_end)
        {
            return 1;
        }

        if (p <= _fall_end)
        {
            const float value = (p - _high_end).rep;
            const float range = (_fall_end - _high_end).rep;
            const auto t = value / range;
            return std::lerp(1.0f, -1.0f, t);
        }

        return -1;
    }

    constexpr float operator()(cycfi::q::phase_iterator i) const
    {
        return (*this)(i._phase);
    }

private:
    cycfi::q::phase _low_end;
    cycfi::q::phase _rise_end;
    cycfi::q::phase _high_end;
    cycfi::q::phase _fall_end;
};
