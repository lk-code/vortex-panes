#pragma once

#include <cstdint>
#include <span>

namespace vp {

// Shader bytecode compiled at build time by fxc (shaders/*.hlsl).
std::span<const uint8_t> flakeVertexShaderBytecode() noexcept;
std::span<const uint8_t> flakePixelShaderBytecode() noexcept;

} // namespace vp
