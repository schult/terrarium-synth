#pragma once

#include <q/synth/noise_gen.hpp>

class NoiseSynth
{
public:
    void setSampleDuration(int duration)
    {
        _sample_duration = duration;
    }

    float operator()()
    {
        _ticks++;
        if (_ticks >= _sample_duration)
        {
            _ticks = 0;
            _last_sample = _noise();
        }
        return _last_sample;
    }

private:
    cycfi::q::white_noise_gen _noise;
    float _last_sample = 0;
    int _sample_duration = 0;
    int _ticks = 0;
};
