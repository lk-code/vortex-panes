#pragma once

#include "single_instance.h"
#include "tray_icon.h"
#include "win32_handles.h"

#include "core/command_line.h"
#include "settings/settings_window.h"

namespace vp {

// Owns the hidden shell window, the notification area icon and the overlays, and runs the loop.
class Application {
public:
    Application(HINSTANCE instance, CommandLineOptions options);
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    // Returns the process exit code.
    int run();

private:
    static LRESULT CALLBACK windowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam);

    void createShellWindow();
    void addTrayIcon();
    void onTrayEvent(WPARAM wParam, LPARAM lParam);
    void execute(TrayCommand command);
    void showSettings();
    void requestExit(int exitCode);
    int messageLoop();

    HINSTANCE m_instance;
    CommandLineOptions m_options;
    SingleInstance m_singleInstance;
    UniqueHwnd m_window;
    TrayIcon m_tray;
    SettingsWindow m_settings;
    UINT m_taskbarCreatedMessage = 0;
    UINT m_trayAttempts = 0;
    int m_exitCode = 0;
};

} // namespace vp
