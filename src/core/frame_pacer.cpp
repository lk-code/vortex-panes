#include "core/frame_pacer.h"

#include <algorithm>
#include <cmath>

namespace vp {

namespace {

int64_t periodFor(int64_t ticksPerSecond, double targetFps) noexcept {
    const double fps = std::clamp(targetFps, 1.0, 1000.0);
    return std::max<int64_t>(1, std::llround(static_cast<double>(ticksPerSecond) / fps));
}

} // namespace

FramePacer::FramePacer(int64_t ticksPerSecond, double targetFps) noexcept
    : m_ticksPerSecond(ticksPerSecond), m_period(periodFor(ticksPerSecond, targetFps)) {}

void FramePacer::setTargetFps(double targetFps, int64_t now) noexcept {
    m_period = periodFor(m_ticksPerSecond, targetFps);
    m_next = now + m_period;
}

void FramePacer::start(int64_t now) noexcept {
    m_next = now + m_period;
}

int64_t FramePacer::advance(int64_t now) noexcept {
    m_next += m_period;
    if (m_next <= now) {
        // Fell behind (e.g. a long frame or a suspended process): drop the missed frames.
        m_next = now + m_period;
    }
    return m_next;
}

int64_t FramePacer::waitIn100ns(int64_t deadline, int64_t now, int64_t ticksPerSecond) noexcept {
    if (deadline <= now || ticksPerSecond <= 0) {
        return 1;
    }
    const int64_t ticks = deadline - now;
    // Split to avoid overflow: ticks * 10'000'000 can exceed int64 for large values.
    const int64_t whole = ticks / ticksPerSecond;
    const int64_t rest = ticks % ticksPerSecond;
    const int64_t result = whole * 10'000'000 + rest * 10'000'000 / ticksPerSecond;
    return std::max<int64_t>(1, result);
}

} // namespace vp
