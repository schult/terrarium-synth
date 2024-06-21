#include <cstdint>

class TapTempo
{
public:
    explicit TapTempo(uint32_t interval, uint32_t max_interval = 2000) :
        _max_interval(max_interval),
        _interval(interval)
    {
    }

    void Update(uint32_t now)
    {
        _now = now;
    }

    void Tap()
    {
        const auto elapsed = SinceTap();
        if (elapsed < _max_interval)
        {
            _interval = elapsed;
            _start = _now;
        }
        _last_tap = _now;
    }

    uint32_t Interval() const
    {
        return _interval;
    }

    float Ratio() const
    {
        const auto elapsed = _now - _start;
        return static_cast<float>(elapsed % _interval) / _interval;
    }

    uint32_t SinceTap() const
    {
        return _now - _last_tap;
    }

private:
    const uint32_t _max_interval;
    uint32_t _interval;

    uint32_t _now = 0;
    uint32_t _last_tap = 0;
    uint32_t _start = 0;
};
