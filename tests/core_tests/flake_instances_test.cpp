#include "core/flake_instances.h"

#include <gtest/gtest.h>

#include <array>
#include <vector>

namespace {

constexpr vp::RectI kLeftMonitor{-2560, 0, 0, 1440};
constexpr vp::RectI kRightMonitor{0, 0, 2560, 1440};

vp::Flake flake(float x, float y, float radius = 2.0f, float alpha = 0.5f) {
    vp::Flake result;
    result.x = x;
    result.y = y;
    result.radius = radius;
    result.alpha = alpha;
    return result;
}

} // namespace

TEST(CollectVisibleFlakes, CopiesPositionRadiusAndAlpha) {
    const std::array flakes = {flake(100.0f, 200.0f, 3.5f, 0.75f)};
    std::array<vp::FlakeInstance, 4> out{};
    ASSERT_EQ(vp::collectVisibleFlakes(flakes, kRightMonitor, out), 1u);
    EXPECT_FLOAT_EQ(out[0].x, 100.0f);
    EXPECT_FLOAT_EQ(out[0].y, 200.0f);
    EXPECT_FLOAT_EQ(out[0].radius, 3.5f);
    EXPECT_FLOAT_EQ(out[0].alpha, 0.75f);
}

TEST(CollectVisibleFlakes, FlakeOnTheBorderAppearsOnBothMonitors) {
    const std::array flakes = {flake(-1.0f, 700.0f, 3.0f)};
    std::array<vp::FlakeInstance, 4> out{};
    EXPECT_EQ(vp::collectVisibleFlakes(flakes, kLeftMonitor, out), 1u);
    EXPECT_EQ(vp::collectVisibleFlakes(flakes, kRightMonitor, out), 1u);
}

TEST(CollectVisibleFlakes, FlakesOutsideAreSkipped) {
    // Radius 2 gives an extent of 3 px: just touching the edge is not visible.
    const std::array flakes = {
        flake(-3.0f, 100.0f),     // left of the right monitor, touching
        flake(2563.0f, 100.0f),   // right of it, touching
        flake(500.0f, -3.0f),     // above, touching
        flake(500.0f, 1443.0f),   // below, touching
        flake(-2.9f, 100.0f),     // overlaps by 0.1 px
        flake(500.0f, 1442.9f),   // overlaps by 0.1 px
        flake(-5000.0f, 100.0f),  // far away
    };
    std::array<vp::FlakeInstance, 8> out{};
    ASSERT_EQ(vp::collectVisibleFlakes(flakes, kRightMonitor, out), 2u);
    EXPECT_FLOAT_EQ(out[0].x, -2.9f);
    EXPECT_FLOAT_EQ(out[1].y, 1442.9f);
}

TEST(CollectVisibleFlakes, TinyFlakesUseTheMinimumExtent) {
    // A radius below one pixel is drawn with one pixel plus the edge (extent 2 px).
    const std::array flakes = {flake(-1.5f, 100.0f, 0.3f), flake(-2.0f, 100.0f, 0.3f)};
    std::array<vp::FlakeInstance, 4> out{};
    EXPECT_EQ(vp::collectVisibleFlakes(flakes, kRightMonitor, out), 1u);
}

TEST(CollectVisibleFlakes, StopsWhenTheOutputIsFull) {
    std::vector<vp::Flake> flakes;
    for (int i = 0; i < 10; ++i) {
        flakes.push_back(flake(10.0f * static_cast<float>(i), 50.0f));
    }
    std::array<vp::FlakeInstance, 4> out{};
    EXPECT_EQ(vp::collectVisibleFlakes(flakes, kRightMonitor, out), 4u);
    EXPECT_FLOAT_EQ(out[3].x, 30.0f);
    EXPECT_EQ(vp::collectVisibleFlakes(flakes, kRightMonitor, std::span<vp::FlakeInstance>{}), 0u);
}
