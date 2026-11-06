#pragma once

#include "win32_handles.h"

#include "core/tray_menu.h"
#include "core/tray_theme.h"

#include <optional>
#include <vector>

namespace vp {

// Icon in the notification area, owned by the hidden shell window.
class TrayIcon {
public:
    static constexpr UINT kCallbackMessage = WM_APP + 1;

    explicit TrayIcon(HINSTANCE instance);
    ~TrayIcon();

    TrayIcon(const TrayIcon&) = delete;
    TrayIcon& operator=(const TrayIcon&) = delete;

    // Adds the icon (NIM_ADD + NOTIFYICON_VERSION_4). Also used after Explorer restarts.
    bool add(HWND owner);
    void remove() noexcept;
    bool isAdded() const noexcept { return m_added; }

    // Reloads the icon for the current taskbar theme and updates it if it changed.
    void refreshTheme();

    // Shows the context menu at the anchor point and returns the chosen command.
    static std::optional<TrayCommand> showContextMenu(HWND owner, POINT anchor, const std::vector<TrayMenuItem>& items);

private:
    void loadIcon();

    HINSTANCE m_instance;
    HWND m_owner = nullptr;
    bool m_added = false;
    TrayIconVariant m_variant = TrayIconVariant::DarkTaskbar;
    UniqueHicon m_icon;
};

} // namespace vp
