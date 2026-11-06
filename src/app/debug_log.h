#pragma once

// Debug-only diagnostics via OutputDebugString. Compiled out of Release builds.
#ifdef VP_DEBUG

namespace vp {
void debugLog(const wchar_t* format, ...) noexcept;
} // namespace vp

#define VP_DEBUG_LOG(...) ::vp::debugLog(__VA_ARGS__)

#else

#define VP_DEBUG_LOG(...) ((void)0)

#endif
