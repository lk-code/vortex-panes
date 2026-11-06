#include "platform/monitors.h"

#include <gtest/gtest.h>

#include <vector>

namespace {

vp::MonitorInfo monitor(int32_t left, int32_t top, bool primary, uint64_t id) {
    return {{left, top, left + 1920, top + 1080}, primary, id};
}

std::vector<uint64_t> ids(const std::vector<vp::MonitorInfo>& monitors) {
    std::vector<uint64_t> result;
    for (const auto& m : monitors) {
        result.push_back(m.id);
    }
    return result;
}

} // namespace

TEST(Monitors, PrimaryComesFirst) {
    std::vector<vp::MonitorInfo> monitors{monitor(-1920, 0, false, 1), monitor(0, 0, true, 2)};
    vp::sortMonitors(monitors);
    EXPECT_EQ(ids(monitors), (std::vector<uint64_t>{2, 1}));
}

TEST(Monitors, SecondaryMonitorsLeftToRightThenTopToBottom) {
    std::vector<vp::MonitorInfo> monitors{monitor(1920, 1080, false, 1), monitor(3840, 0, false, 2),
                                          monitor(0, 0, true, 3), monitor(1920, 0, false, 4),
                                          monitor(-1920, 0, false, 5)};
    vp::sortMonitors(monitors);
    EXPECT_EQ(ids(monitors), (std::vector<uint64_t>{3, 5, 4, 1, 2}));
}

TEST(Monitors, EmptyAndSingle) {
    std::vector<vp::MonitorInfo> none;
    vp::sortMonitors(none);
    EXPECT_TRUE(none.empty());

    std::vector<vp::MonitorInfo> single{monitor(0, 0, true, 7)};
    vp::sortMonitors(single);
    EXPECT_EQ(ids(single), (std::vector<uint64_t>{7}));
}
