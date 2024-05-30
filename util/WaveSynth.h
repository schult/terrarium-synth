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
        shape = std::clamp(shape, 0.0f, 3.0f);

        using cycfi::q::frac_to_phase;
        using std::lerp;

        if (shape <= 1) // Pulse (0.0) to Square (1.0)
        {
            const auto t = shape;
            _low_end = frac_to_phase(lerp(0.47f, 0.25f, t));
            _rise_end = _low_end;
            _high_end = frac_to_phase(lerp(0.53f, 0.75f, t));
            _fall_end = _high_end;
        }
        else if (shape <= 2) // Square (1.0) to Triangle (2.0)
        {
            const auto t = shape - 1;
            _low_end = frac_to_phase(lerp(0.25f, 0.0f, t));
            _rise_end = frac_to_phase(lerp(0.25f, 0.5f, t));
            _high_end = frac_to_phase(lerp(0.75f, 0.5f, t));
            _fall_end = frac_to_phase(lerp(0.75f, 1.0f, t));
        }
        else // Triangle (2.0) to Sawtooth (3.0)
        {
            const auto t = shape - 2;
            _low_end = frac_to_phase(0.0f);
            _rise_end = frac_to_phase(lerp(0.5f, 1.0f, t));
            _high_end = _rise_end;
            _fall_end = frac_to_phase(1.0f);
        }

        // Triangle waves are quieter than everything else, so boost them.
        const auto tri_boost = 1.8;
        const auto x = shape - 2;
        const auto tri_ratio = 1 - std::clamp(x*x, 0.0f, 1.0f);
        _boost = (tri_boost * tri_ratio) + 1;

        // Sawtooth needs a little boost too
        const auto saw_boost = 0.4;
        const auto y = shape - 3;
        const auto saw_ratio = 1 - std::clamp(y*y, 0.0f, 1.0f);
        _boost *= (saw_boost * saw_ratio) + 1;
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

    constexpr float compensated(cycfi::q::phase p) const
    {
        return (*this)(p) * _boost;
    }

    constexpr float compensated(cycfi::q::phase_iterator i) const
    {
        return (*this)(i) * _boost;
    }

private:
    cycfi::q::phase _low_end;
    cycfi::q::phase _rise_end;
    cycfi::q::phase _high_end;
    cycfi::q::phase _fall_end;
    float _boost;
};
