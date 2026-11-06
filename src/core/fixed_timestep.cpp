#include "core/fixed_timestep.h"

#include <algorithm>

namespace vp {

FixedTimestep::FixedTimestep(int64_t clockFrequency) noexcept : m_frequency(std::max<int64_t>(1, clockFrequency)) {}

void FixedTimestep::start(int64_t now) noexcept {
    m_last = now;
    m_accumulator = m_frequency / 2;
}

int32_t FixedTimestep::advance(int64_t now) noexcept {
    // A clock running backwards counts as no time; more than one second (e.g. after standby)
    // is clamped, which also rules out overflow in the multiplication below.
    const int64_t elapsed = std::clamp<int64_t>(now - m_last, 0, m_frequency);
    m_last = now;
    m_accumulator += elapsed * kSimulationRate;

    const int64_t ticks = m_accumulator / m_frequency;
    m_accumulator %= m_frequency;
    return static_cast<int32_t>(std::min<int64_t>(ticks, kMaxTicksPerFrame));
}

} // namespace vp
