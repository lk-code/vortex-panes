#pragma once

#include <cstdint>
#include <vector>

namespace vp {

enum class TrayCommand : uint8_t {
    Settings,
    Exit,
};

enum class TrayMenuItemKind : uint8_t {
    Command,
    Separator,
};

struct TrayMenuItem {
    TrayMenuItemKind kind = TrayMenuItemKind::Command;
    TrayCommand command = TrayCommand::Exit; // only meaningful for TrayMenuItemKind::Command
    bool enabled = true;
    bool isDefault = false; // shown in bold, also triggered by a double click on the icon
};

struct TrayMenuState {
    bool settingsAvailable = false; // the settings window arrives with M2
};

// Layout of the notification area context menu, top to bottom.
std::vector<TrayMenuItem> buildTrayMenu(const TrayMenuState& state);

} // namespace vp
