#include "render/shaders.h"

#include <windows.h> // BYTE, used by the fxc-generated headers

#include "flake_ps.h"
#include "flake_vs.h"

namespace vp {

std::span<const uint8_t> flakeVertexShaderBytecode() noexcept {
    return {g_flakeVS, sizeof(g_flakeVS)};
}

std::span<const uint8_t> flakePixelShaderBytecode() noexcept {
    return {g_flakePS, sizeof(g_flakePS)};
}

} // namespace vp
