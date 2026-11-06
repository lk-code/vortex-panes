#pragma once

#include "single_instance.h"
#include "tray_icon.h"
#include "win32_handles.h"

#include "core/command_line.h"
#include "core/fixed_timestep.h"
#include "core/fps_counter.h"
#include "core/frame_pacer.h"
#include "core/snowfall.h"
#include "settings/settings_window.h"

#include <memory>

namespace vp {

class Renderer;

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
    void createOverlays();
    void onTrayEvent(WPARAM wParam, LPARAM lParam);
    void execute(TrayCommand command);
    void showSettings();
    SettingsContent settingsContent() const;
    void requestExit(int exitCode);

    int mainLoop();
    void renderFrame();
    void armFrameTimer(int64_t now);
    void stopRendering() noexcept;

    HINSTANCE m_instance;
    CommandLineOptions m_options;
    SingleInstance m_singleInstance;
    UniqueHwnd m_window;
    TrayIcon m_tray;
    SettingsWindow m_settings;
    UINT m_taskbarCreatedMessage = 0;
    UINT m_trayAttempts = 0;
    int m_exitCode = 0;

    // Rendering and frame pacing
    std::unique_ptr<Renderer> m_renderer;
    UniqueHandle m_frameTimer;
    int64_t m_ticksPerSecond;
    int64_t m_lastTopmostRefresh = 0;
    FramePacer m_pacer;
    FpsCounter m_fps;
    bool m_rendering = false;

    // Simulation at the fixed rate
    Snowfall m_snowfall;
    FixedTimestep m_timestep;
    uint64_t m_fadeStartTick = 0;
};

} // namespace vp
