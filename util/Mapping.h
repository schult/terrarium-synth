#pragma once

#include <algorithm>

#include <gcem.hpp>
#include <q/detail/fast_math.hpp>

class LogMapping
{
public:
    // max > center > min
    // (min + center) > 0
    constexpr LogMapping(float _min, float _center, float _max) :
        min(_min),
        center(_center),
        max(_max),
        offset(calculateOffset(_min, _center, _max)),
        log_min(gcem::log(_min + offset)),
        log_range(gcem::log(_max + offset) - log_min)
    {
    }

    // max > min > 0
    constexpr LogMapping(float _min, float _max) :
        min(_min),
        center(calculateCenter(_min, _max)),
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
    const float center;
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
