#include "application.h"

#include "debug_log.h"
#include "localization.h"
#include "resource.h"

#include "core/app_info.h"

#include <shellapi.h>
#include <windowsx.h>

#include <stdexcept>
#include <utility>

namespace vp {

namespace {

constexpr UINT_PTR kTrayRetryTimer = 1;
constexpr UINT kTrayRetryIntervalMs = 1000;
constexpr UINT kTrayMaxAttempts = 30;

} // namespace

Application::Application(HINSTANCE instance, CommandLineOptions options)
    : m_instance(instance), m_options(std::move(options)), m_tray(instance) {}

Application::~Application() = default;

int Application::run() {
    if (!m_singleInstance.acquire()) {
        VP_DEBUG_LOG(L"another instance is running; asking it to show the settings");
        SingleInstance::notifyRunningInstance();
        return 0;
    }

    createShellWindow();
    addTrayIcon();
    return messageLoop();
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
    m_tray.remove();
    if (m_window) {
        ::KillTimer(m_window.get(), kTrayRetryTimer);
        ::DestroyWindow(m_window.get()); // WM_DESTROY posts WM_QUIT
    } else {
        ::PostQuitMessage(exitCode);
    }
}

int Application::messageLoop() {
    MSG msg{};
    while (::GetMessageW(&msg, nullptr, 0, 0) > 0) {
        ::TranslateMessage(&msg);
        ::DispatchMessageW(&msg);
    }
    return static_cast<int>(msg.wParam);
}

} // namespace vp
