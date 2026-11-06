#include "render/shaders.h"

#include <windows.h> // BYTE, used by the fxc-generated headers

#include "shape_ps.h"
#include "shape_vs.h"

namespace vp {

std::span<const uint8_t> shapeVertexShaderBytecode() noexcept {
    return {g_shapeVS, sizeof(g_shapeVS)};
}

std::span<const uint8_t> shapePixelShaderBytecode() noexcept {
    return {g_shapePS, sizeof(g_shapePS)};
}

} // namespace vp
