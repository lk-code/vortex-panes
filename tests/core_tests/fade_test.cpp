#include "core/fade.h"

#include <gtest/gtest.h>

TEST(FadeInOpacity, RisesSmoothlyFromZeroToOne) {
    EXPECT_FLOAT_EQ(vp::fadeInOpacity(0.0f, 2.0f), 0.0f);
    EXPECT_FLOAT_EQ(vp::fadeInOpacity(1.0f, 2.0f), 0.5f);
    EXPECT_FLOAT_EQ(vp::fadeInOpacity(2.0f, 2.0f), 1.0f);
    float previous = 0.0f;
    for (int step = 1; step <= 120; ++step) {
        const float value = vp::fadeInOpacity(static_cast<float>(step) / 60.0f, 2.0f);
        ASSERT_GT(value, previous);
        previous = value;
    }
}

TEST(FadeInOpacity, StartsAndEndsGently) {
    // Smoothstep: the first and last steps are much smaller than the one in the middle.
    const float first = vp::fadeInOpacity(0.02f, 2.0f);
    const float middle = vp::fadeInOpacity(1.01f, 2.0f) - vp::fadeInOpacity(0.99f, 2.0f);
    const float last = 1.0f - vp::fadeInOpacity(1.98f, 2.0f);
    EXPECT_LT(first, middle * 0.1f);
    EXPECT_LT(last, middle * 0.1f);
}

TEST(FadeInOpacity, IsClampedOutsideTheFade) {
    EXPECT_FLOAT_EQ(vp::fadeInOpacity(-1.0f, 2.0f), 0.0f);
    EXPECT_FLOAT_EQ(vp::fadeInOpacity(100.0f, 2.0f), 1.0f);
    EXPECT_FLOAT_EQ(vp::fadeInOpacity(0.0f, 0.0f), 1.0f);
}
