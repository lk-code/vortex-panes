#pragma once

#include "core/geometry.h"

#include <array>

namespace vp {

// GPU instance data for the shape shaders (shaders/shape_vs.hlsl). Layout must match the
// input layout in render/renderer.cpp.
struct ShapeInstance {
    float left, top, right, bottom; // virtual-screen pixels
    float r, g, b, a;               // premultiplied color
    float shape;                    // 0 = rectangle, 1 = soft circle
};
static_assert(sizeof(ShapeInstance) == 9 * sizeof(float));

inline constexpr float kShapeRectangle = 0.0f;
inline constexpr float kShapeCircle = 1.0f;

// M1 test content per monitor: a semi-transparent rectangle (static) and a soft circle that
// moves slowly from side to side, so that running frames are visible.
inline constexpr size_t kTestSceneShapeCount = 2;
inline constexpr double kTestSceneCirclePeriodSeconds = 8.0;

std::array<ShapeInstance, kTestSceneShapeCount> buildTestScene(const RectI& monitor, double timeSeconds) noexcept;

} // namespace vp
