#include "core/frame_pacer.h"

#include <gtest/gtest.h>

namespace {

constexpr int64_t kQpcFrequency = 10'000'000; // typical QueryPerformanceFrequency

} // namespace

TEST(FramePacer, PeriodMatchesTargetRate) {
    const vp::FramePacer pacer(kQpcFrequency, 60.0);
    EXPECT_EQ(pacer.periodTicks(), 166'667);
}

TEST(FramePacer, AverageRateIsExactOverManyFrames) {
    vp::FramePacer pacer(kQpcFrequency, 60.0);
    pacer.start(0);
    int64_t deadline = pacer.nextDeadline();
    for (int i = 1; i < 600; ++i) {
        deadline = pacer.advance(deadline + 500); // each frame wakes slightly late
    }
    // 600 frames at 60 FPS = 10 s; rounding of the period stays below one tick per frame.
    EXPECT_NEAR(static_cast<double>(deadline), 10.0 * kQpcFrequency, 600.0);
}

TEST(FramePacer, LateWakeUpsDoNotAccumulateDrift) {
    vp::FramePacer pacer(kQpcFrequency, 50.0);
    pacer.start(0);
    EXPECT_EQ(pacer.nextDeadline(), 200'000);
    EXPECT_EQ(pacer.advance(201'000), 400'000); // 1 ms late: next deadline unchanged
}

TEST(FramePacer, FallingBehindDropsMissedFrames) {
    vp::FramePacer pacer(kQpcFrequency, 50.0);
    pacer.start(0);
    // The process was suspended for a second: schedule relative to now, no catch-up burst.
    EXPECT_EQ(pacer.advance(10'000'000), 10'200'000);
}

TEST(FramePacer, SetTargetFpsRestartsFromNow) {
    vp::FramePacer pacer(kQpcFrequency, 60.0);
    pacer.start(0);
    pacer.setTargetFps(30.0, 1'000'000);
    EXPECT_EQ(pacer.periodTicks(), 333'333);
    EXPECT_EQ(pacer.nextDeadline(), 1'333'333);
}

TEST(FramePacer, TargetRateIsClamped) {
    EXPECT_EQ(vp::FramePacer(kQpcFrequency, 0.0).periodTicks(), kQpcFrequency);
    EXPECT_EQ(vp::FramePacer(kQpcFrequency, 1e9).periodTicks(), kQpcFrequency / 1000);
}

TEST(FramePacer, WaitConversion) {
    EXPECT_EQ(vp::FramePacer::waitIn100ns(166'667, 0, kQpcFrequency), 166'667);
    EXPECT_EQ(vp::FramePacer::waitIn100ns(3'000, 1'000, 1'000), 20'000'000); // 2 s at 1 kHz
    EXPECT_EQ(vp::FramePacer::waitIn100ns(5, 10, kQpcFrequency), 1);         // already due
    // Large tick counts must not overflow.
    EXPECT_EQ(vp::FramePacer::waitIn100ns(int64_t{1} << 50, 0, int64_t{1} << 40), 1024 * 10'000'000LL);
}
