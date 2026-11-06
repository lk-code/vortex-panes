#include "core/test_scene.h"

#include <cmath>
#include <numbers>

namespace vp {

std::array<ShapeInstance, kTestSceneShapeCount> buildTestScene(const RectI& monitor, double timeSeconds) noexcept {
    const auto left = static_cast<float>(monitor.left);
    const auto top = static_cast<float>(monitor.top);
    const auto width = static_cast<float>(monitor.width());
    const auto height = static_cast<float>(monitor.height());

    // Rectangle: blue, 35 % opacity, premultiplied.
    constexpr float kRectAlpha = 0.35f;
    ShapeInstance rect{};
    rect.left = left + width * 0.10f;
    rect.top = top + height * 0.15f;
    rect.right = left + width * 0.35f;
    rect.bottom = top + height * 0.35f;
    rect.r = 0.20f * kRectAlpha;
    rect.g = 0.50f * kRectAlpha;
    rect.b = 0.90f * kRectAlpha;
    rect.a = kRectAlpha;
    rect.shape = kShapeRectangle;

    // Circle: white, 80 % opacity, soft edge; swings around the horizontal center.
    constexpr float kCircleAlpha = 0.80f;
    const float radius = height * 0.075f;
    const double phase = std::fmod(timeSeconds, kTestSceneCirclePeriodSeconds) / kTestSceneCirclePeriodSeconds;
    const float swing = static_cast<float>(std::sin(phase * 2.0 * std::numbers::pi));
    const float centerX = left + width * (0.5f + 0.3f * swing);
    const float centerY = top + height * 0.60f;
    ShapeInstance circle{};
    circle.left = centerX - radius;
    circle.top = centerY - radius;
    circle.right = centerX + radius;
    circle.bottom = centerY + radius;
    circle.r = kCircleAlpha;
    circle.g = kCircleAlpha;
    circle.b = kCircleAlpha;
    circle.a = kCircleAlpha;
    circle.shape = kShapeCircle;

    return {rect, circle};
}

} // namespace vp
