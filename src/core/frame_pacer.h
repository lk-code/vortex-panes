#pragma once

#include <cstdint>

namespace vp {

// Schedules frame deadlines at a fixed rate on a tick clock (e.g. QueryPerformanceCounter).
// Deadlines advance by exactly one period, so the average rate matches the target even if
// the timer granularity does not divide the period. When the loop falls behind, missed
// frames are dropped instead of being rendered in a burst.
class FramePacer {
public:
    FramePacer(int64_t ticksPerSecond, double targetFps) noexcept;

    // Changes the target rate; the next deadline is recomputed relative to `now`.
    void setTargetFps(double targetFps, int64_t now) noexcept;

    // Starts pacing: the first deadline is one period after `now`.
    void start(int64_t now) noexcept;

    // Advances to the next deadline after a frame and returns it.
    int64_t advance(int64_t now) noexcept;

    int64_t nextDeadline() const noexcept { return m_next; }
    int64_t periodTicks() const noexcept { return m_period; }

    // Relative wait from `now` until `deadline` in 100 ns units (at least 1).
    static int64_t waitIn100ns(int64_t deadline, int64_t now, int64_t ticksPerSecond) noexcept;

private:
    int64_t m_ticksPerSecond;
    int64_t m_period;
    int64_t m_next = 0;
};

} // namespace vp
