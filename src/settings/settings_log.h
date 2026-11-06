#pragma once

// Debug-only diagnostics of the settings module via OutputDebugString (same prefix as the
// application log). Compiled out of Release builds.
#ifdef VP_DEBUG

#include <windows.h>

#include <cstdarg>
#include <cstdio>
#include <cwchar>

namespace vp::settings_detail {

inline void log(const wchar_t* format, ...) noexcept {
    wchar_t buffer[512] = L"VortexPanes: settings: ";
    const size_t prefix = wcslen(buffer);
    va_list args;
    va_start(args, format);
    _vsnwprintf_s(buffer + prefix, sizeof(buffer) / sizeof(buffer[0]) - prefix - 1, _TRUNCATE, format, args);
    va_end(args);
    wcscat_s(buffer, L"\n");
    ::OutputDebugStringW(buffer);
}

} // namespace vp::settings_detail

#define VP_SETTINGS_LOG(...) ::vp::settings_detail::log(__VA_ARGS__)

#else

#define VP_SETTINGS_LOG(...) ((void)0)

#endif
