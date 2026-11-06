#include "core/tray_theme.h"

#include <gtest/gtest.h>

TEST(TrayTheme, LightTaskbarUsesLightVariant) {
    EXPECT_EQ(vp::trayIconVariant(1u), vp::TrayIconVariant::LightTaskbar);
}

TEST(TrayTheme, DarkTaskbarUsesDarkVariant) {
    EXPECT_EQ(vp::trayIconVariant(0u), vp::TrayIconVariant::DarkTaskbar);
}

TEST(TrayTheme, MissingValueMeansDarkTaskbar) {
    EXPECT_EQ(vp::trayIconVariant(std::nullopt), vp::TrayIconVariant::DarkTaskbar);
}

TEST(TrayTheme, AnyNonZeroValueMeansLight) {
    EXPECT_EQ(vp::trayIconVariant(2u), vp::TrayIconVariant::LightTaskbar);
}
