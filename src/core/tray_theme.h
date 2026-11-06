#pragma once

#include <cstdint>
#include <optional>

namespace vp {

// Which tray icon to show: tray_light.ico (dark glyph) on a light taskbar,
// tray_dark.ico (light glyph) on a dark taskbar.
enum class TrayIconVariant : uint8_t {
    LightTaskbar,
    DarkTaskbar,
};

// Evaluates HKCU\...\Themes\Personalize\SystemUsesLightTheme. 1 means a light taskbar.
// A missing value (older Windows 10 builds) means the classic dark taskbar.
constexpr TrayIconVariant trayIconVariant(std::optional<uint32_t> systemUsesLightTheme) noexcept {
    return systemUsesLightTheme.value_or(0) != 0 ? TrayIconVariant::LightTaskbar : TrayIconVariant::DarkTaskbar;
}

} // namespace vp
