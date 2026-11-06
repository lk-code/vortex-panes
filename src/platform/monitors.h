#pragma once

#include "core/geometry.h"

#include <cstdint>
#include <span>
#include <vector>

namespace vp {

struct MonitorInfo {
    RectI bounds; // physical pixels in virtual-screen coordinates (PerMonitorV2)
    bool primary = false;
    uint64_t id = 0; // HMONITOR as an integer
};

// Enumerates all monitors, sorted with sortMonitors().
std::vector<MonitorInfo> enumerateMonitors();

// Deterministic order: primary monitor first, then left to right, then top to bottom.
void sortMonitors(std::span<MonitorInfo> monitors) noexcept;

} // namespace vp
