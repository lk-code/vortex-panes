#include "core/geometry.h"

#include <gtest/gtest.h>

TEST(RectI, WidthAndHeight) {
    constexpr vp::RectI r{-1920, 0, 0, 1080};
    EXPECT_EQ(r.width(), 1920);
    EXPECT_EQ(r.height(), 1080);
    EXPECT_FALSE(r.empty());
}

TEST(RectI, EmptyWhenDegenerate) {
    EXPECT_TRUE((vp::RectI{}).empty());
    EXPECT_TRUE((vp::RectI{10, 10, 10, 20}).empty());
    EXPECT_TRUE((vp::RectI{10, 10, 20, 5}).empty());
}

TEST(RectI, Equality) {
    EXPECT_EQ((vp::RectI{1, 2, 3, 4}), (vp::RectI{1, 2, 3, 4}));
    EXPECT_NE((vp::RectI{1, 2, 3, 4}), (vp::RectI{1, 2, 3, 5}));
}
