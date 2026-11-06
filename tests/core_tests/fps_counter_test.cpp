#include "core/fps_counter.h"

#include <gtest/gtest.h>

namespace {

constexpr int64_t kTicksPerSecond = 1'000'000; // 1 tick = 1 us

} // namespace

TEST(FpsCounter, ReportsOncePerInterval) {
    vp::FpsCounter counter(kTicksPerSecond, 1.0);
    int reports = 0;
    for (int i = 0; i < 120; ++i) {
        if (counter.addFrame(i * 16'667, 200)) {
            ++reports;
        }
    }
    // Frames 0..119 at 60 FPS span just under 2 s: one complete interval (after frame 60).
    EXPECT_EQ(reports, 1);
}

TEST(FpsCounter, ComputesRateAndCpuTimes) {
    vp::FpsCounter counter(kTicksPerSecond, 1.0);
    bool reported = false;
    for (int i = 0; i <= 60 && !reported; ++i) {
        const int64_t cpu = (i == 30) ? 1'500 : 300; // one slow frame
        reported = counter.addFrame(i * 16'667, cpu);
    }
    ASSERT_TRUE(reported);
    const auto& r = counter.report();
    EXPECT_EQ(r.frames, 60u);
    EXPECT_NEAR(r.fps, 60.0, 0.1);
    EXPECT_NEAR(r.maxCpuMs, 1.5, 1e-9);
    EXPECT_NEAR(r.averageCpuMs, (59 * 0.3 + 1.5) / 60.0, 1e-9);
}

TEST(FpsCounter, ResetsBetweenIntervals) {
    vp::FpsCounter counter(kTicksPerSecond, 0.5);
    int64_t t = 0;
    counter.addFrame(t, 0);
    while (!counter.addFrame(t += 10'000, 5'000)) {
    }
    while (!counter.addFrame(t += 20'000, 100)) {
    }
    const auto& r = counter.report();
    EXPECT_NEAR(r.fps, 50.0, 0.1);
    EXPECT_NEAR(r.maxCpuMs, 0.1, 1e-9);
}
