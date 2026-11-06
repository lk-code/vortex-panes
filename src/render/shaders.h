#pragma once

#include <cstdint>
#include <span>

namespace vp {

// Shader bytecode compiled at build time by fxc (shaders/*.hlsl).
std::span<const uint8_t> shapeVertexShaderBytecode() noexcept;
std::span<const uint8_t> shapePixelShaderBytecode() noexcept;

} // namespace vp
