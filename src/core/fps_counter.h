#pragma once

#include <cstdint>

namespace vp {

// Frame statistics over fixed reporting intervals (used for the Debug output).
// Allocation-free; call addFrame() once per rendered frame.
class FpsCounter {
public:
    struct Report {
        double fps = 0.0;
        double averageCpuMs = 0.0;
        double maxCpuMs = 0.0;
        uint32_t frames = 0;
    };

    explicit FpsCounter(int64_t ticksPerSecond, double intervalSeconds = 1.0) noexcept;

    // Records a frame that finished at `now` and took `cpuTicks` of CPU work.
    // Returns true when an interval completed; the result is then available via report().
    bool addFrame(int64_t now, int64_t cpuTicks) noexcept;

    const Report& report() const noexcept { return m_report; }

private:
    int64_t m_ticksPerSecond;
    int64_t m_interval;
    int64_t m_intervalStart = -1;
    uint32_t m_frames = 0;
    int64_t m_cpuTicksSum = 0;
    int64_t m_cpuTicksMax = 0;
    Report m_report;
};

} // namespace vp
