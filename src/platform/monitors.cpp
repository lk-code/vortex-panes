#include "platform/monitors.h"

#include <windows.h>

#include <algorithm>
#include <tuple>

namespace vp {

namespace {

BOOL CALLBACK collectMonitor(HMONITOR monitor, HDC /*dc*/, LPRECT /*clip*/, LPARAM context) {
    auto& result = *reinterpret_cast<std::vector<MonitorInfo>*>(context);
    MONITORINFO info{};
    info.cbSize = sizeof(info);
    if (::GetMonitorInfoW(monitor, &info)) {
        MonitorInfo entry;
        entry.bounds = {info.rcMonitor.left, info.rcMonitor.top, info.rcMonitor.right, info.rcMonitor.bottom};
        entry.primary = (info.dwFlags & MONITORINFOF_PRIMARY) != 0;
        entry.id = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(monitor));
        result.push_back(entry);
    }
    return TRUE;
}

} // namespace

std::vector<MonitorInfo> enumerateMonitors() {
    std::vector<MonitorInfo> result;
    ::EnumDisplayMonitors(nullptr, nullptr, &collectMonitor, reinterpret_cast<LPARAM>(&result));
    sortMonitors(result);
    return result;
}

void sortMonitors(std::span<MonitorInfo> monitors) noexcept {
    std::stable_sort(monitors.begin(), monitors.end(), [](const MonitorInfo& a, const MonitorInfo& b) {
        return std::make_tuple(!a.primary, a.bounds.left, a.bounds.top) <
               std::make_tuple(!b.primary, b.bounds.left, b.bounds.top);
    });
}

} // namespace vp
