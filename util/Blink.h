#pragma once

#include <daisy_seed.h>

class Blink
{
public:
    bool enabled() const
    {
        return !_expired;
    }

    void reset()
    {
        _expired = false;
        _start_ms = daisy::System::GetNow();
    }

    bool process()
    {
        if (_expired) return false;
        const auto elapsed_ms = daisy::System::GetNow() - _start_ms;
        const auto count = elapsed_ms / interval_ms;
        _expired = (count > max_count);
        return !_expired && (elapsed_ms / interval_ms) % 2;
    }

private:
    static constexpr int max_count = 6;
    static constexpr uint32_t interval_ms = 125;

    bool _expired = true;
    uint32_t _start_ms = 0;
};
