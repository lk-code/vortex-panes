#pragma once

#include <cstdint>

namespace vp {

struct WindParams {
    float baseSpeed = 6.0f;   // px/s, prevailing drift (positive = to the right)
    float amplitude = 18.0f;  // px/s, maximum deviation from the base speed
    float slowPeriod = 8.0f;  // s between the noise keys of the slow octave
    float fastPeriod = 3.0f;  // s between the noise keys of the fast octave
    float fastWeight = 0.35f; // weight of the fast octave relative to the slow one
};

// Horizontal wind as smooth 1D value noise over the simulation time: random keys at fixed
// intervals, interpolated with smoothstep, two octaves. Stateless: the result depends only on
// the seed and the time, so it is deterministic and has no jumps.
class WindField {
public:
    WindField(uint64_t seed, const WindParams& params) noexcept;

    // Wind speed in px/s, within baseSpeed +- amplitude.
    float sample(double timeSeconds) const noexcept;

    const WindParams& params() const noexcept { return m_params; }

private:
    WindParams m_params;
    uint64_t m_slowSeed;
    uint64_t m_fastSeed;
};

} // namespace vp
