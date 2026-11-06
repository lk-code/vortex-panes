#include "core/flake.h"

#include <gtest/gtest.h>

namespace {

vp::Flake flakeAt(float x) {
    vp::Flake flake;
    flake.x = x;
    return flake;
}

} // namespace

TEST(FlakePool, StartsEmptyWithFixedCapacity) {
    const vp::FlakePool pool(16);
    EXPECT_TRUE(pool.empty());
    EXPECT_EQ(pool.size(), 0u);
    EXPECT_EQ(pool.capacity(), 16u);
    EXPECT_FALSE(pool.full());
}

TEST(FlakePool, AddingNeverReallocates) {
    vp::FlakePool pool(100);
    ASSERT_TRUE(pool.tryAdd(flakeAt(0.0f)));
    const vp::Flake* const storage = pool.flakes().data();
    for (int i = 1; i < 100; ++i) {
        ASSERT_TRUE(pool.tryAdd(flakeAt(static_cast<float>(i))));
    }
    EXPECT_EQ(pool.flakes().data(), storage);
    EXPECT_EQ(pool.size(), 100u);
}

TEST(FlakePool, RejectsFlakesWhenFull) {
    vp::FlakePool pool(2);
    EXPECT_TRUE(pool.tryAdd(flakeAt(1.0f)));
    EXPECT_TRUE(pool.tryAdd(flakeAt(2.0f)));
    EXPECT_TRUE(pool.full());
    EXPECT_FALSE(pool.tryAdd(flakeAt(3.0f)));
    EXPECT_EQ(pool.size(), 2u);
    EXPECT_FLOAT_EQ(pool[1].x, 2.0f);
}

TEST(FlakePool, RemoveInTheMiddleSwapsInTheLastFlake) {
    vp::FlakePool pool(8);
    for (int i = 0; i < 4; ++i) {
        pool.tryAdd(flakeAt(static_cast<float>(i)));
    }
    pool.removeAt(1);
    ASSERT_EQ(pool.size(), 3u);
    EXPECT_FLOAT_EQ(pool[0].x, 0.0f);
    EXPECT_FLOAT_EQ(pool[1].x, 3.0f);
    EXPECT_FLOAT_EQ(pool[2].x, 2.0f);
}

TEST(FlakePool, RemoveLastAndOutOfRange) {
    vp::FlakePool pool(8);
    pool.tryAdd(flakeAt(0.0f));
    pool.tryAdd(flakeAt(1.0f));
    pool.removeAt(1);
    ASSERT_EQ(pool.size(), 1u);
    EXPECT_FLOAT_EQ(pool[0].x, 0.0f);
    pool.removeAt(5); // ignored
    EXPECT_EQ(pool.size(), 1u);
    pool.removeAt(0);
    EXPECT_TRUE(pool.empty());
}

TEST(FlakePool, ClearKeepsCapacityAndStorage) {
    vp::FlakePool pool(4);
    pool.tryAdd(flakeAt(0.0f));
    const vp::Flake* const storage = pool.flakes().data();
    pool.clear();
    EXPECT_TRUE(pool.empty());
    EXPECT_EQ(pool.capacity(), 4u);
    for (int i = 0; i < 4; ++i) {
        ASSERT_TRUE(pool.tryAdd(flakeAt(static_cast<float>(i))));
    }
    EXPECT_EQ(pool.flakes().data(), storage);
}
