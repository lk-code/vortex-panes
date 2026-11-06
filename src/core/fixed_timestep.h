#pragma once

#include <cstdint>

namespace vp {

// Fixed simulation rate of the whole application.
inline constexpr int32_t kSimulationRate = 60;
inline constexpr float kTickSeconds = 1.0f / static_cast<float>(kSimulationRate);

// Converts elapsed time on a tick clock (e.g. QueryPerformanceCounter) into a number of fixed
// simulation ticks. The accumulator is integer-based, so the long-term rate is exact. It starts
// half a tick in, which keeps simulation ticks away from frame boundaries: at a frame rate equal
// to the simulation rate, wake-up jitter below half a frame still yields exactly one tick per
// frame instead of alternating between zero and two. A backlog beyond kMaxTicksPerFrame is
// dropped (no spiral of death); the sub-tick phase is kept.
class FixedTimestep {
public:
    static constexpr int32_t kMaxTicksPerFrame = 4;

    explicit FixedTimestep(int64_t clockFrequency) noexcept;

    // Resets the accumulator; the first tick is due half a tick after `now`.
    void start(int64_t now) noexcept;

    // Returns the number of ticks (0 to kMaxTicksPerFrame) to simulate up to `now`.
    int32_t advance(int64_t now) noexcept;

private:
    int64_t m_frequency;
    int64_t m_last = 0;
    int64_t m_accumulator = 0; // in clock ticks * kSimulationRate; one simulation tick = m_frequency
};

} // namespace vp
