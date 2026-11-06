#pragma once

#include <string_view>

namespace vp {

// Target architecture of this build, as used in release file names.
constexpr std::wstring_view architectureName() noexcept {
#if defined(_M_ARM64)
    return L"arm64";
#elif defined(_M_X64)
    return L"x64";
#elif defined(_M_IX86)
    return L"x86";
#else
#error "Unsupported target architecture"
#endif
}

} // namespace vp
