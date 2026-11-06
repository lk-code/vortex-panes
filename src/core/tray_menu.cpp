#include "core/tray_menu.h"

namespace vp {

std::vector<TrayMenuItem> buildTrayMenu(const TrayMenuState& state) {
    return {
        {TrayMenuItemKind::Command, TrayCommand::Settings, state.settingsAvailable, true},
        {TrayMenuItemKind::Separator, TrayCommand::Exit, false, false},
        {TrayMenuItemKind::Command, TrayCommand::Exit, true, false},
    };
}

} // namespace vp
