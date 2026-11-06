#include "core/fps_counter.h"

#include <algorithm>
#include <cmath>

namespace vp {

FpsCounter::FpsCounter(int64_t ticksPerSecond, double intervalSeconds) noexcept
    : m_ticksPerSecond(std::max<int64_t>(1, ticksPerSecond)),
      m_interval(std::max<int64_t>(1, std::llround(static_cast<double>(m_ticksPerSecond) * intervalSeconds))) {}

bool FpsCounter::addFrame(int64_t now, int64_t cpuTicks) noexcept {
    if (m_intervalStart < 0) {
        // The first frame only marks the start of the first interval.
        m_intervalStart = now;
        return false;
    }
    ++m_frames;
    m_cpuTicksSum += cpuTicks;
    m_cpuTicksMax = std::max(m_cpuTicksMax, cpuTicks);

    const int64_t elapsed = now - m_intervalStart;
    if (elapsed < m_interval) {
        return false;
    }

    const double ticksPerMs = static_cast<double>(m_ticksPerSecond) / 1000.0;
    m_report.frames = m_frames;
    m_report.fps = static_cast<double>(m_frames) * static_cast<double>(m_ticksPerSecond) / static_cast<double>(elapsed);
    m_report.averageCpuMs = static_cast<double>(m_cpuTicksSum) / static_cast<double>(m_frames) / ticksPerMs;
    m_report.maxCpuMs = static_cast<double>(m_cpuTicksMax) / ticksPerMs;

    m_intervalStart = now;
    m_frames = 0;
    m_cpuTicksSum = 0;
    m_cpuTicksMax = 0;
    return true;
}

} // namespace vp
