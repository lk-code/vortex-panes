#pragma once

namespace vp {

// Settings window (information page). The WinUI 3 implementation follows in M2; until then the
// window is not available and show() does nothing. At most one instance exists at a time.
class SettingsWindow {
public:
    static constexpr bool isAvailable() noexcept { return false; }

    // Opens the window or brings it to the foreground if it is already open.
    void show() noexcept;
};

} // namespace vp
