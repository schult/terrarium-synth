#pragma once

#include <cassert>
#include <numbers>

#include <q/detail/fast_math.hpp>
#include <q/support/frequency.hpp>

// Algorithm source: https://arxiv.org/pdf/2111.05592
// "Improving the Chamberlin Digital State Variable Filter"
// by Victor Lazzarini and Joseph Timoney
class SvFilter
{
public:
    SvFilter() = default;

    SvFilter(cycfi::q::frequency corner, float sample_rate, float q=0.707)
    {
        config(corner, sample_rate, q);
    }

    void config(cycfi::q::frequency corner, float sample_rate, float q=0.707)
    {
        constexpr auto pi = std::numbers::pi_v<float>;
        const auto f = cycfi::q::as_float(corner) / sample_rate;
        assert(f <= 0.5); // fastertan expects input in [-pi/2, pi/2]
        const auto k = fastertan(pi * f);

        _k = k;
        _q_inv = 1 / q;
        _c1 = _q_inv + k;
        _c2 = 1 / (1 + (k * _q_inv) + (k * k));
    }

    void update(float sample)
    {
        _hp = (sample - (_c1 * _s1) - _s2) * _c2;
        _bp = (_k * _hp) + _s1;
        _s1 = (_k * _hp) + _bp;
        _lp = (_k * _bp) + _s2;
        _s2 = (_k * _bp) + _lp;
    }

    float highPass()
    {
        return _hp;
    }

    float lowPass()
    {
        return _lp;
    }

    float bandPass()
    {
        return _bp;
    }

    float bandStop()
    {
        return _hp + _lp;
    }

    float allPass()
    {
        return _hp + _lp + (_q_inv * _bp);
    }

private:
    float _k = 0;
    float _q_inv = 0;
    float _c1 = 0;
    float _c2 = 0;

    float _s1 = 0;
    float _s2 = 0;

    float _hp = 0;
    float _bp = 0;
    float _lp = 0;
};
