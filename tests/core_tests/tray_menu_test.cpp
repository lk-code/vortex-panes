#include "core/tray_menu.h"

#include <gtest/gtest.h>

using vp::TrayCommand;
using vp::TrayMenuItemKind;

TEST(TrayMenu, LayoutIsSettingsSeparatorExit) {
    const auto menu = vp::buildTrayMenu({});
    ASSERT_EQ(menu.size(), 3u);
    EXPECT_EQ(menu[0].kind, TrayMenuItemKind::Command);
    EXPECT_EQ(menu[0].command, TrayCommand::Settings);
    EXPECT_EQ(menu[1].kind, TrayMenuItemKind::Separator);
    EXPECT_EQ(menu[2].kind, TrayMenuItemKind::Command);
    EXPECT_EQ(menu[2].command, TrayCommand::Exit);
}

TEST(TrayMenu, SettingsIsDefaultItem) {
    const auto menu = vp::buildTrayMenu({});
    EXPECT_TRUE(menu[0].isDefault);
    EXPECT_FALSE(menu[2].isDefault);
}

TEST(TrayMenu, SettingsDisabledWhileUnavailable) {
    const auto menu = vp::buildTrayMenu({.settingsAvailable = false});
    EXPECT_FALSE(menu[0].enabled);
    EXPECT_TRUE(menu[2].enabled);
}

TEST(TrayMenu, SettingsEnabledWhenAvailable) {
    const auto menu = vp::buildTrayMenu({.settingsAvailable = true});
    EXPECT_TRUE(menu[0].enabled);
}
