#include "tray_icon.h"

#include "debug_log.h"
#include "localization.h"
#include "resource.h"

#include <commctrl.h>
#include <shellapi.h>

namespace vp {

namespace {

constexpr UINT kIconId = 1;

struct CommandResources {
    TrayCommand command;
    UINT menuId;
    UINT stringId;
};

constexpr CommandResources kCommandResources[] = {
    {TrayCommand::Settings, IDM_SETTINGS, IDS_MENU_SETTINGS},
    {TrayCommand::Exit, IDM_EXIT, IDS_MENU_EXIT},
};

const CommandResources* resourcesFor(TrayCommand command) {
    for (const auto& entry : kCommandResources) {
        if (entry.command == command) {
            return &entry;
        }
    }
    return nullptr;
}

std::optional<uint32_t> readSystemUsesLightTheme() {
    DWORD value = 0;
    DWORD size = sizeof(value);
    const LSTATUS status =
        ::RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                       L"SystemUsesLightTheme", RRF_RT_REG_DWORD, nullptr, &value, &size);
    if (status != ERROR_SUCCESS) {
        return std::nullopt;
    }
    return static_cast<uint32_t>(value);
}

NOTIFYICONDATAW makeIconData(HWND owner) {
    NOTIFYICONDATAW data{};
    data.cbSize = sizeof(data);
    data.hWnd = owner;
    data.uID = kIconId; // no NIF_GUID: a GUID is bound to the executable path
    return data;
}

} // namespace

TrayIcon::TrayIcon(HINSTANCE instance) : m_instance(instance) {}

TrayIcon::~TrayIcon() {
    remove();
}

void TrayIcon::loadIcon() {
    m_variant = trayIconVariant(readSystemUsesLightTheme());
    const UINT iconId = m_variant == TrayIconVariant::LightTaskbar ? IDI_TRAY_LIGHT : IDI_TRAY_DARK;
    HICON icon = nullptr;
    if (SUCCEEDED(::LoadIconMetric(m_instance, MAKEINTRESOURCEW(iconId), LIM_SMALL, &icon))) {
        m_icon.reset(icon);
    }
}

bool TrayIcon::add(HWND owner) {
    m_owner = owner;
    loadIcon();

    // Remove a stale entry first (e.g. a previous attempt that timed out but succeeded).
    NOTIFYICONDATAW stale = makeIconData(owner);
    ::Shell_NotifyIconW(NIM_DELETE, &stale);
    m_added = false;

    NOTIFYICONDATAW data = makeIconData(owner);
    data.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_SHOWTIP;
    data.uCallbackMessage = kCallbackMessage;
    data.hIcon = m_icon.get();
    const std::wstring tooltip = loadString(IDS_TRAY_TOOLTIP);
    wcsncpy_s(data.szTip, tooltip.c_str(), _TRUNCATE);
    if (!::Shell_NotifyIconW(NIM_ADD, &data)) {
        VP_DEBUG_LOG(L"Shell_NotifyIcon(NIM_ADD) failed");
        return false;
    }

    data.uVersion = NOTIFYICON_VERSION_4;
    ::Shell_NotifyIconW(NIM_SETVERSION, &data);
    m_added = true;
    VP_DEBUG_LOG(L"tray icon added (%s taskbar)", m_variant == TrayIconVariant::LightTaskbar ? L"light" : L"dark");
    return true;
}

void TrayIcon::remove() noexcept {
    if (!m_added) {
        return;
    }
    NOTIFYICONDATAW data = makeIconData(m_owner);
    ::Shell_NotifyIconW(NIM_DELETE, &data);
    m_added = false;
}

void TrayIcon::refreshTheme() {
    const TrayIconVariant previous = m_variant;
    loadIcon();
    if (!m_added || m_variant == previous) {
        return;
    }
    NOTIFYICONDATAW data = makeIconData(m_owner);
    data.uFlags = NIF_ICON;
    data.hIcon = m_icon.get();
    ::Shell_NotifyIconW(NIM_MODIFY, &data);
    VP_DEBUG_LOG(L"tray icon switched to %s taskbar variant",
                 m_variant == TrayIconVariant::LightTaskbar ? L"light" : L"dark");
}

std::optional<TrayCommand> TrayIcon::showContextMenu(HWND owner, POINT anchor, const std::vector<TrayMenuItem>& items) {
    UniqueHmenu menu(::CreatePopupMenu());
    if (!menu) {
        return std::nullopt;
    }
    for (const TrayMenuItem& item : items) {
        if (item.kind == TrayMenuItemKind::Separator) {
            ::AppendMenuW(menu.get(), MF_SEPARATOR, 0, nullptr);
            continue;
        }
        const CommandResources* res = resourcesFor(item.command);
        if (!res) {
            continue;
        }
        const std::wstring text = loadString(res->stringId);
        ::AppendMenuW(menu.get(), MF_STRING | (item.enabled ? MF_ENABLED : MF_GRAYED), res->menuId, text.c_str());
        if (item.isDefault) {
            ::SetMenuDefaultItem(menu.get(), res->menuId, FALSE);
        }
    }

    // Required so that the menu closes when the user clicks elsewhere.
    ::SetForegroundWindow(owner);
    const UINT alignment = ::GetSystemMetrics(SM_MENUDROPALIGNMENT) ? TPM_RIGHTALIGN : TPM_LEFTALIGN;
    const UINT chosen = static_cast<UINT>(
        ::TrackPopupMenuEx(menu.get(), TPM_RIGHTBUTTON | TPM_BOTTOMALIGN | TPM_RETURNCMD | TPM_NONOTIFY | alignment,
                           anchor.x, anchor.y, owner, nullptr));
    ::PostMessageW(owner, WM_NULL, 0, 0);

    for (const auto& entry : kCommandResources) {
        if (entry.menuId == chosen) {
            return entry.command;
        }
    }
    return std::nullopt;
}

} // namespace vp
