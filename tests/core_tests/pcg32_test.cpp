#include "core/pcg32.h"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>

TEST(Pcg32, MatchesReferenceSequence) {
    // First outputs of the PCG reference implementation for seed 42, stream 54.
    constexpr std::array<uint32_t, 6> kExpected = {0xa15c02b7u, 0x7b47f409u, 0xba1d3330u,
                                                   0x83d2f293u, 0xbfa4784bu, 0xcbed606eu};
    vp::Pcg32 rng(42u, 54u);
    for (const uint32_t expected : kExpected) {
        EXPECT_EQ(rng.nextU32(), expected);
    }
}

TEST(Pcg32, EqualSeedsGiveEqualSequences) {
    vp::Pcg32 a(1234u, 7u);
    vp::Pcg32 b(1234u, 7u);
    for (int i = 0; i < 1000; ++i) {
        ASSERT_EQ(a.nextU32(), b.nextU32());
    }
}

TEST(Pcg32, StreamsAndSeedsDiffer) {
    vp::Pcg32 base(1234u, 7u);
    vp::Pcg32 otherStream(1234u, 8u);
    vp::Pcg32 otherSeed(1235u, 7u);
    int sameStream = 0;
    int sameSeed = 0;
    for (int i = 0; i < 100; ++i) {
        const uint32_t value = base.nextU32();
        sameStream += value == otherStream.nextU32() ? 1 : 0;
        sameSeed += value == otherSeed.nextU32() ? 1 : 0;
    }
    EXPECT_LT(sameStream, 3);
    EXPECT_LT(sameSeed, 3);
}

TEST(Pcg32, FloatsAreInUnitIntervalAndCoverIt) {
    vp::Pcg32 rng(99u);
    float minimum = 1.0f;
    float maximum = 0.0f;
    double sum = 0.0;
    constexpr int kSamples = 100'000;
    for (int i = 0; i < kSamples; ++i) {
        const float value = rng.nextFloat();
        ASSERT_GE(value, 0.0f);
        ASSERT_LT(value, 1.0f);
        minimum = value < minimum ? value : minimum;
        maximum = value > maximum ? value : maximum;
        sum += value;
    }
    EXPECT_LT(minimum, 0.001f);
    EXPECT_GT(maximum, 0.999f);
    EXPECT_NEAR(sum / kSamples, 0.5, 0.01);
}

TEST(Pcg32, RangeStaysWithinBounds) {
    vp::Pcg32 rng(5u);
    for (int i = 0; i < 10'000; ++i) {
        const float value = rng.range(-3.0f, 7.5f);
        ASSERT_GE(value, -3.0f);
        ASSERT_LE(value, 7.5f);
    }
}
