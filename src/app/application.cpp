#include "application.h"

#include "debug_log.h"
#include "localization.h"
#include "resource.h"

#include "core/app_info.h"
#include "platform/monitors.h"
#include "render/renderer.h"

#include <shellapi.h>
#include <windowsx.h>

#include <stdexcept>
#include <utility>
#include <vector>

namespace vp {

namespace {

constexpr UINT_PTR kTrayRetryTimer = 1;
constexpr UINT kTrayRetryIntervalMs = 1000;
constexpr UINT kTrayMaxAttempts = 30;

constexpr double kTargetFps = 60.0;
constexpr int64_t kTopmostRefreshSeconds = 2;

int64_t qpcFrequency() noexcept {
    LARGE_INTEGER frequency{};
    ::QueryPerformanceFrequency(&frequency);
    return frequency.QuadPart;
}

int64_t qpcNow() noexcept {
    LARGE_INTEGER now{};
    ::QueryPerformanceCounter(&now);
    return now.QuadPart;
}

} // namespace

Application::Application(HINSTANCE instance, CommandLineOptions options)
    : m_instance(instance), m_options(std::move(options)), m_tray(instance), m_ticksPerSecond(qpcFrequency()),
      m_pacer(m_ticksPerSecond, kTargetFps), m_fps(m_ticksPerSecond) {}

Application::~Application() = default;

int Application::run() {
    if (!m_singleInstance.acquire()) {
        VP_DEBUG_LOG(L"another instance is running; asking it to show the settings");
        SingleInstance::notifyRunningInstance();
        return 0;
    }

    createShellWindow();
    addTrayIcon();
    createOverlays();
    return mainLoop();
}

void Application::createOverlays() {
    const std::vector<MonitorInfo> monitors = enumerateMonitors();
    std::vector<RectI> bounds;
    bounds.reserve(monitors.size());
    for (const MonitorInfo& monitor : monitors) {
        bounds.push_back(monitor.bounds);
        VP_DEBUG_LOG(L"monitor (%d,%d)-(%d,%d)%s", monitor.bounds.left, monitor.bounds.top, monitor.bounds.right,
                     monitor.bounds.bottom, monitor.primary ? L" primary" : L"");
    }

    GraphicsDeviceOptions options;
    options.forceWarp = m_options.forceWarp;
#ifdef VP_DEBUG
    options.debugLayer = true;
#endif

    try {
        m_renderer = std::make_unique<Renderer>(m_instance, bounds, options);
    } catch (const std::exception& e) {
        VP_DEBUG_LOG(L"graphics initialization failed: %hs", e.what());
        (void)e;
        m_renderer.reset();
        const std::wstring message = loadString(IDS_ERROR_GRAPHICS);
        ::MessageBoxW(nullptr, message.c_str(), kAppName, MB_OK | MB_ICONERROR);
        requestExit(1);
        return;
    }

    VP_DEBUG_LOG(L"device: %s, feature level 0x%X, %zu overlay(s)",
                 m_renderer->graphics().adapterKind() == AdapterKind::Warp ? L"WARP" : L"hardware",
                 static_cast<unsigned>(m_renderer->graphics().featureLevel()), m_renderer->overlayCount());

    // Present the first frame before showing the windows, so they never show stale content.
    m_startTicks = qpcNow();
    m_lastTopmostRefresh = m_startTicks;
    m_rendering = m_renderer->renderFrame(0.0) == FrameResult::Ok;
    m_renderer->showOverlays();
}

void Application::createShellWindow() {
    m_taskbarCreatedMessage = ::RegisterWindowMessageW(L"TaskbarCreated");

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = &Application::windowProc;
    wc.hInstance = m_instance;
    wc.lpszClassName = kShellWindowClass;
    if (!::RegisterClassExW(&wc)) {
        throw std::runtime_error("RegisterClassExW failed for the shell window");
    }

    // A hidden top-level window (never shown) instead of a message-only window: message-only
    // windows do not receive broadcasts such as TaskbarCreated and WM_SETTINGCHANGE.
    // WS_EX_TOOLWINDOW keeps it out of the taskbar and Alt+Tab even if it were ever shown.
    const HWND window = ::CreateWindowExW(WS_EX_TOOLWINDOW, kShellWindowClass, kAppName, WS_POPUP, 0, 0, 0, 0, nullptr,
                                          nullptr, m_instance, this);
    if (!window) {
        throw std::runtime_error("CreateWindowExW failed for the shell window");
    }
    m_window.reset(window);
}

void Application::addTrayIcon() {
    if (m_tray.add(m_window.get())) {
        ::KillTimer(m_window.get(), kTrayRetryTimer);
        m_trayAttempts = 0;
        return;
    }
    // Explorer may not be ready yet (e.g. right after logon). Retry for a while; TaskbarCreated
    // also triggers a new attempt as soon as the taskbar exists.
    if (++m_trayAttempts >= kTrayMaxAttempts) {
        ::KillTimer(m_window.get(), kTrayRetryTimer);
        const std::wstring message = loadString(IDS_ERROR_TRAY_ICON);
        ::MessageBoxW(nullptr, message.c_str(), kAppName, MB_OK | MB_ICONERROR);
        requestExit(1);
        return;
    }
    ::SetTimer(m_window.get(), kTrayRetryTimer, kTrayRetryIntervalMs, nullptr);
}

LRESULT CALLBACK Application::windowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lParam);
        ::SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
    }
    auto* self = reinterpret_cast<Application*>(::GetWindowLongPtrW(window, GWLP_USERDATA));
    if (!self) {
        return ::DefWindowProcW(window, message, wParam, lParam);
    }
    if (message == WM_NCDESTROY) {
        ::SetWindowLongPtrW(window, GWLP_USERDATA, 0);
        self->m_window.release(); // the window is already being destroyed
        return ::DefWindowProcW(window, message, wParam, lParam);
    }
    if (!self->m_window) {
        // Messages sent during CreateWindowExW, before m_window is set.
        return ::DefWindowProcW(window, message, wParam, lParam);
    }
    return self->handleMessage(message, wParam, lParam);
}

LRESULT Application::handleMessage(UINT message, WPARAM wParam, LPARAM lParam) {
    const HWND window = m_window.get();

    if (message == TrayIcon::kCallbackMessage) {
        onTrayEvent(wParam, lParam);
        return 0;
    }
    if (message == m_taskbarCreatedMessage && m_taskbarCreatedMessage != 0) {
        VP_DEBUG_LOG(L"TaskbarCreated: re-adding the tray icon");
        m_trayAttempts = 0;
        addTrayIcon();
        return 0;
    }
    if (message == showSettingsMessage() && showSettingsMessage() != 0) {
        showSettings();
        return 0;
    }

    switch (message) {
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDM_SETTINGS:
            execute(TrayCommand::Settings);
            break;
        case IDM_EXIT:
            execute(TrayCommand::Exit);
            break;
        default:
            break;
        }
        return 0;
    case WM_SETTINGCHANGE:
        if (lParam != 0 && ::CompareStringOrdinal(reinterpret_cast<LPCWSTR>(lParam), -1, L"ImmersiveColorSet", -1,
                                                  TRUE) == CSTR_EQUAL) {
            m_tray.refreshTheme();
        }
        return 0;
    case WM_TIMER:
        if (wParam == kTrayRetryTimer) {
            addTrayIcon();
        }
        return 0;
    case WM_ENDSESSION:
        if (wParam) {
            m_tray.remove();
        }
        return 0;
    case WM_DESTROY:
        ::PostQuitMessage(m_exitCode);
        return 0;
    default:
        return ::DefWindowProcW(window, message, wParam, lParam);
    }
}

void Application::onTrayEvent(WPARAM wParam, LPARAM lParam) {
    // NOTIFYICON_VERSION_4: LOWORD(lParam) is the event, wParam holds the anchor point.
    switch (LOWORD(lParam)) {
    case WM_CONTEXTMENU: {
        const POINT anchor{GET_X_LPARAM(wParam), GET_Y_LPARAM(wParam)};
        const auto menu = buildTrayMenu({.settingsAvailable = SettingsWindow::isAvailable()});
        if (const auto command = TrayIcon::showContextMenu(m_window.get(), anchor, menu)) {
            execute(*command);
        }
        break;
    }
    case NIN_SELECT:
    case NIN_KEYSELECT:
    case WM_LBUTTONDBLCLK:
        showSettings();
        break;
    default:
        break;
    }
}

void Application::execute(TrayCommand command) {
    switch (command) {
    case TrayCommand::Settings:
        showSettings();
        break;
    case TrayCommand::Exit:
        requestExit(0);
        break;
    }
}

void Application::showSettings() {
    if (SettingsWindow::isAvailable()) {
        m_settings.show();
    }
}

void Application::requestExit(int exitCode) {
    VP_DEBUG_LOG(L"exit requested (code %d)", exitCode);
    m_exitCode = exitCode;
    stopRendering();
    m_renderer.reset(); // destroys the overlay windows
    m_tray.remove();
    if (m_window) {
        ::KillTimer(m_window.get(), kTrayRetryTimer);
        ::DestroyWindow(m_window.get()); // WM_DESTROY posts WM_QUIT
    } else {
        ::PostQuitMessage(exitCode);
    }
}

int Application::mainLoop() {
    m_frameTimer.reset(
        ::CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS));
    if (!m_frameTimer) {
        // High-resolution timers need Windows 10 1803; fall back to a regular waitable timer.
        m_frameTimer.reset(::CreateWaitableTimerExW(nullptr, nullptr, 0, TIMER_ALL_ACCESS));
    }
    if (!m_frameTimer) {
        throw std::runtime_error("CreateWaitableTimerExW failed");
    }
    if (m_rendering) {
        const int64_t now = qpcNow();
        m_pacer.start(now);
        armFrameTimer(now);
    }

    // Waits for the frame timer or window messages. When not rendering, only messages wake the
    // loop, so an idle or paused application uses no CPU.
    MSG msg{};
    for (;;) {
        const HANDLE timer = m_frameTimer.get();
        const DWORD handleCount = m_rendering ? 1 : 0;
        const DWORD result =
            ::MsgWaitForMultipleObjectsEx(handleCount, &timer, INFINITE, QS_ALLINPUT, MWMO_INPUTAVAILABLE);

        while (::PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                return static_cast<int>(msg.wParam);
            }
            ::TranslateMessage(&msg);
            ::DispatchMessageW(&msg);
        }

        if (m_rendering && result == WAIT_OBJECT_0) {
            renderFrame();
        }
    }
}

void Application::renderFrame() {
    const int64_t frameStart = qpcNow();
    const double timeSeconds = static_cast<double>(frameStart - m_startTicks) / static_cast<double>(m_ticksPerSecond);

    if (m_renderer->renderFrame(timeSeconds) == FrameResult::DeviceLost) {
        // Recreating the device follows in M9; until then rendering stops.
        VP_DEBUG_LOG(L"graphics device lost; rendering stopped");
        stopRendering();
        return;
    }
    if (frameStart - m_lastTopmostRefresh >= kTopmostRefreshSeconds * m_ticksPerSecond) {
        m_renderer->refreshTopmost();
        m_lastTopmostRefresh = frameStart;
    }

    const int64_t frameEnd = qpcNow();
#ifdef VP_DEBUG
    if (m_fps.addFrame(frameEnd, frameEnd - frameStart)) {
        const FpsCounter::Report& report = m_fps.report();
        VP_DEBUG_LOG(L"%.1f fps, CPU frame time avg %.3f ms, max %.3f ms", report.fps, report.averageCpuMs,
                     report.maxCpuMs);
    }
#endif
    m_pacer.advance(frameEnd);
    armFrameTimer(frameEnd);
}

void Application::armFrameTimer(int64_t now) {
    LARGE_INTEGER due{};
    due.QuadPart = -FramePacer::waitIn100ns(m_pacer.nextDeadline(), now, m_ticksPerSecond); // negative = relative
    ::SetWaitableTimerEx(m_frameTimer.get(), &due, 0, nullptr, nullptr, nullptr, 0);
}

void Application::stopRendering() noexcept {
    m_rendering = false;
    if (m_frameTimer) {
        ::CancelWaitableTimer(m_frameTimer.get());
    }
}

} // namespace vp
