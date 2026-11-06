#include "core/fixed_timestep.h"

#include <gtest/gtest.h>

#include <cstdint>

namespace {

constexpr int64_t kQpcFrequency = 10'000'000; // typical QueryPerformanceFrequency
constexpr int64_t kFramePeriod = 166'667;     // FramePacer period at 60 FPS

} // namespace

TEST(FixedTimestep, OneTickPerFrameDespiteJitter) {
    vp::FixedTimestep timestep(kQpcFrequency);
    timestep.start(0);
    // Wake-ups scatter by up to 4 ms around the frame deadlines.
    constexpr int64_t kJitter[] = {0, 40'000, -40'000, 25'000, -10'000, 39'000, -39'000, 5'000};
    for (int frame = 1; frame <= 6000; ++frame) {
        const int64_t now = frame * kFramePeriod + kJitter[frame % 8];
        ASSERT_EQ(timestep.advance(now), 1) << "frame " << frame;
    }
}

TEST(FixedTimestep, LongTermRateIsExact) {
    vp::FixedTimestep timestep(kQpcFrequency);
    timestep.start(0);
    int64_t ticks = 0;
    // Irregular frames over exactly 100 s.
    int64_t now = 0;
    int step = 0;
    while (now < 100 * kQpcFrequency) {
        now += 50'000 + (step++ % 7) * 30'000;
        if (now > 100 * kQpcFrequency) {
            now = 100 * kQpcFrequency;
        }
        ticks += timestep.advance(now);
    }
    // 6000 ticks, give or take the half-tick start phase.
    EXPECT_GE(ticks, 6000);
    EXPECT_LE(ticks, 6001);
}

TEST(FixedTimestep, NoTimeGivesNoTick) {
    vp::FixedTimestep timestep(kQpcFrequency);
    timestep.start(1000);
    EXPECT_EQ(timestep.advance(1000), 0);
    EXPECT_EQ(timestep.advance(500), 0); // clock running backwards
}

TEST(FixedTimestep, FirstTickIsDueAfterHalfATick) {
    vp::FixedTimestep timestep(kQpcFrequency);
    timestep.start(0);
    EXPECT_EQ(timestep.advance(kFramePeriod / 2 - 1'000), 0);
    EXPECT_EQ(timestep.advance(kFramePeriod / 2 + 1'000), 1);
}

TEST(FixedTimestep, BacklogIsCappedAndDropped) {
    vp::FixedTimestep timestep(kQpcFrequency);
    timestep.start(0);
    // Ten frames late: only the maximum is simulated, the rest is dropped.
    EXPECT_EQ(timestep.advance(10 * kFramePeriod), vp::FixedTimestep::kMaxTicksPerFrame);
    // Afterwards the timestep continues normally instead of catching up.
    EXPECT_EQ(timestep.advance(11 * kFramePeriod), 1);
    EXPECT_EQ(timestep.advance(12 * kFramePeriod), 1);
}

TEST(FixedTimestep, LargeJumpDoesNotOverflow) {
    vp::FixedTimestep timestep(kQpcFrequency);
    timestep.start(0);
    constexpr int64_t kHours = int64_t{24} * 3600 * kQpcFrequency * 365; // a year of standby
    EXPECT_EQ(timestep.advance(kHours), vp::FixedTimestep::kMaxTicksPerFrame);
    EXPECT_EQ(timestep.advance(kHours + kFramePeriod), 1);
}

TEST(FixedTimestep, TickDurationMatchesRate) {
    EXPECT_EQ(vp::kSimulationRate, 60);
    EXPECT_FLOAT_EQ(vp::kTickSeconds * vp::kSimulationRate, 1.0f);
}
