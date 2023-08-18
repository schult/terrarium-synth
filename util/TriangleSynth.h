#include <q/support/phase.hpp>

class TriangleSynth
{
public:
    constexpr TriangleSynth(float skew = 0.5)
    {
        setSkew(skew);
    }

    constexpr void setSkew(float skew)
    {
        _inflection = cycfi::q::frac_to_phase(skew);
    }

    constexpr float operator()(cycfi::q::phase p) const
    {
        const bool rising = (p <= _inflection);
        if (rising)
        {
            const auto x = p.rep;
            const auto max = _inflection.rep;
            const float scale = 2.0f / max;
            return (x * scale) - 1;
        }
        else
        {
            const auto x = (p - _inflection).rep;
            const auto max = cycfi::q::phase::one_cyc - _inflection.rep;
            const float scale = 2.0f / max;
            return 1 - (x * scale);
        }
    }

    constexpr float operator()(cycfi::q::phase_iterator i) const
    {
        return (*this)(i._phase);
    }

private:
    cycfi::q::phase _inflection;
};
