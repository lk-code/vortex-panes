#include "core/test_scene.h"

#include <gtest/gtest.h>

namespace {

constexpr vp::RectI kLeftMonitor{-2560, 0, 0, 1440};

bool inside(const vp::ShapeInstance& s, const vp::RectI& m) {
    return s.left >= static_cast<float>(m.left) && s.right <= static_cast<float>(m.right) &&
           s.top >= static_cast<float>(m.top) && s.bottom <= static_cast<float>(m.bottom);
}

} // namespace

TEST(TestScene, ShapesLieWithinTheMonitor) {
    for (double t : {0.0, 1.0, 2.0, 4.0, 6.0, 7.99}) {
        const auto shapes = vp::buildTestScene(kLeftMonitor, t);
        for (const auto& s : shapes) {
            EXPECT_TRUE(inside(s, kLeftMonitor)) << "t=" << t;
            EXPECT_LT(s.left, s.right);
            EXPECT_LT(s.top, s.bottom);
        }
    }
}

TEST(TestScene, RectangleThenCircle) {
    const auto shapes = vp::buildTestScene(kLeftMonitor, 0.0);
    EXPECT_EQ(shapes[0].shape, vp::kShapeRectangle);
    EXPECT_EQ(shapes[1].shape, vp::kShapeCircle);
}

TEST(TestScene, ColorsArePremultipliedAndTranslucent) {
    for (const auto& s : vp::buildTestScene(kLeftMonitor, 0.0)) {
        EXPECT_GT(s.a, 0.0f);
        EXPECT_LT(s.a, 1.0f);
        EXPECT_LE(s.r, s.a);
        EXPECT_LE(s.g, s.a);
        EXPECT_LE(s.b, s.a);
    }
}

TEST(TestScene, CircleMovesAndIsPeriodic) {
    const auto start = vp::buildTestScene(kLeftMonitor, 0.0)[1];
    const auto quarter = vp::buildTestScene(kLeftMonitor, vp::kTestSceneCirclePeriodSeconds / 4)[1];
    const auto full = vp::buildTestScene(kLeftMonitor, vp::kTestSceneCirclePeriodSeconds)[1];
    EXPECT_GT(quarter.left, start.left + 100.0f);
    EXPECT_NEAR(full.left, start.left, 0.01f);
    EXPECT_FLOAT_EQ(quarter.top, start.top);
}

TEST(TestScene, CircleIsSquare) {
    const auto circle = vp::buildTestScene(kLeftMonitor, 1.0)[1];
    EXPECT_FLOAT_EQ(circle.right - circle.left, circle.bottom - circle.top);
}
