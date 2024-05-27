#pragma once

#include <algorithm>
#include <cmath>

#include <gcem.hpp>
#include <q/detail/fast_math.hpp>

class LinearMapping
{
public:
    // min < max
    constexpr LinearMapping(float _min, float _max) :
        min(_min),
        max(_max)
    {
    }

    // 0 <= x <= 1
    float operator()(float x) const
    {
        return std::lerp(min, max, x);
    }

    constexpr float clamp(float y) const
    {
        return std::clamp(y, min, max);
    }

    const float min;
    const float max;
};

class LogMapping
{
public:
    // min < center < max
    // center < (min + max) / 2
    // min + center > 0
    constexpr LogMapping(float _min, float _center, float _max) :
        min(_min),
        max(_max),
        offset(calculateOffset(_min, _center, _max)),
        log_min(gcem::log(_min + offset)),
        log_range(gcem::log(_max + offset) - log_min)
    {
    }

    // 0 < min < max
    constexpr LogMapping(float _min, float _max) :
        min(_min),
        max(_max),
        offset(0),
        log_min(gcem::log(_min)),
        log_range(gcem::log(_max) - log_min)
    {
    }

    // 0 <= x <= 1
    float operator()(float x) const
    {
        return fasterexp(log_range*x + log_min) - offset;
    }

    constexpr float clamp(float y) const
    {
        return std::clamp(y, min, max);
    }

    const float min;
    const float max;

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
