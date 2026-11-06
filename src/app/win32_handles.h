#pragma once

#include <windows.h>

#include "core/unique_resource.h"

namespace vp {

struct HandleTraits {
    using value_type = HANDLE;
    static HANDLE invalid() noexcept { return nullptr; }
    static bool isValid(HANDLE h) noexcept { return h != nullptr && h != INVALID_HANDLE_VALUE; }
    static void close(HANDLE h) noexcept { ::CloseHandle(h); }
};

struct HwndTraits {
    using value_type = HWND;
    static HWND invalid() noexcept { return nullptr; }
    static bool isValid(HWND h) noexcept { return h != nullptr; }
    static void close(HWND h) noexcept { ::DestroyWindow(h); }
};

struct HiconTraits {
    using value_type = HICON;
    static HICON invalid() noexcept { return nullptr; }
    static bool isValid(HICON h) noexcept { return h != nullptr; }
    static void close(HICON h) noexcept { ::DestroyIcon(h); }
};

struct HmenuTraits {
    using value_type = HMENU;
    static HMENU invalid() noexcept { return nullptr; }
    static bool isValid(HMENU h) noexcept { return h != nullptr; }
    static void close(HMENU h) noexcept { ::DestroyMenu(h); }
};

struct LocalMemoryTraits {
    using value_type = HLOCAL;
    static HLOCAL invalid() noexcept { return nullptr; }
    static bool isValid(HLOCAL h) noexcept { return h != nullptr; }
    static void close(HLOCAL h) noexcept { ::LocalFree(h); }
};

using UniqueHandle = UniqueResource<HandleTraits>;
using UniqueHwnd = UniqueResource<HwndTraits>;
using UniqueHicon = UniqueResource<HiconTraits>;
using UniqueHmenu = UniqueResource<HmenuTraits>;
using UniqueLocalMemory = UniqueResource<LocalMemoryTraits>;

} // namespace vp
