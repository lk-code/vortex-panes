#include "debug_log.h"

#ifdef VP_DEBUG

#include <windows.h>

#include <cstdarg>
#include <cstdio>
#include <cwchar>

namespace vp {

void debugLog(const wchar_t* format, ...) noexcept {
    constexpr size_t kCapacity = 512;
    constexpr wchar_t kPrefix[] = L"VortexPanes: ";
    constexpr size_t kPrefixLength = sizeof(kPrefix) / sizeof(kPrefix[0]) - 1;

    wchar_t buffer[kCapacity];
    wcscpy_s(buffer, kPrefix);

    // Leave room for the trailing newline.
    const size_t available = kCapacity - kPrefixLength - 1;
    va_list args;
    va_start(args, format);
    _vsnwprintf_s(buffer + kPrefixLength, available, _TRUNCATE, format, args);
    va_end(args);

    const size_t length = wcslen(buffer);
    buffer[length] = L'\n';
    buffer[length + 1] = L'\0';
    ::OutputDebugStringW(buffer);
}

} // namespace vp

#endif
