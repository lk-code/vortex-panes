#pragma once

#include "core/flake.h"
#include "core/geometry.h"

#include <cstddef>
#include <span>

namespace vp {

// GPU instance data of one flake (shaders/flake_vs.hlsl). The layout must match the input
// layout in render/renderer.cpp.
struct FlakeInstance {
    float x;      // center in virtual-screen pixels
    float y;      // center in virtual-screen pixels
    float radius; // px
    float alpha;  // opacity
};
static_assert(sizeof(FlakeInstance) == 4 * sizeof(float));

// Writes the flakes whose drawn quad (see flakeExtent) intersects `bounds` to `out` and returns
// their number. Stops when `out` is full. Writes sequentially and never reads `out`, so it can
// fill a mapped GPU buffer directly.
size_t collectVisibleFlakes(std::span<const Flake> flakes, const RectI& bounds, std::span<FlakeInstance> out) noexcept;

} // namespace vp
