#pragma once

#include <algorithm>
#include <cmath>

#include <gcem.hpp>
#include <q/detail/fast_math.hpp>

class LinearMapping
{
public:
    // min < max
    constexpr LinearMapping(float min, float max) :
        _min(min),
        _max(max)
    {
    }

    // 0 <= x <= 1
    float operator()(float x) const
    {
        return std::lerp(_min, _max, x);
    }

private:
    const float _min;
    const float _max;
};

class LogMapping
{
public:
    // min < center < max
    // center < (min + max) / 2
    // min + center > 0
    constexpr LogMapping(float min, float center, float max) :
        offset(calculateOffset(min, center, max)),
        log_min(gcem::log(min + offset)),
        log_range(gcem::log(max + offset) - log_min)
    {
    }

    // 0 < min < max
    constexpr LogMapping(float min, float max) :
        offset(0),
        log_min(gcem::log(min)),
        log_range(gcem::log(max) - log_min)
    {
    }

    // 0 <= x <= 1
    float operator()(float x) const
    {
        return fasterexp(log_range*x + log_min) - offset;
    }

private:
    static constexpr float calculateCenter(float min, float max)
    {
        const auto log_min = gcem::log(min);
        const auto log_range = gcem::log(max) - log_min;
        return gcem::exp(log_range*0.5 + log_min);
    }

    static constexpr float calculateOffset(float min, float center, float max)
    {
        return (center*center - min*max) / ((min+max) - 2*center);
    }

    const float offset;
    const float log_min;
    const float log_range;
};
