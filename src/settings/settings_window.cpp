#include "settings/settings_window.h"

#include "settings/settings_log.h"
#include "settings/xaml_runtime.h"

#include "core/notices_text.h"

#include <commctrl.h>
#include <dwmapi.h>
#include <shellapi.h>
#include <shellscalingapi.h>

#include <algorithm>
#include <cstring>
#include <utility>

namespace mux = winrt::Microsoft::UI::Xaml;
namespace muxc = winrt::Microsoft::UI::Xaml::Controls;
namespace muxh = winrt::Microsoft::UI::Xaml::Hosting;

namespace vp {

namespace {

constexpr wchar_t kWindowClass[] = L"VortexPanes.Settings";
constexpr DWORD kWindowStyle = WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX;
constexpr int kDefaultWidthDip = 720;
constexpr int kDefaultHeightDip = 560;
constexpr int kMinWidthDip = 500;
constexpr int kMinHeightDip = 420;
constexpr int kIconBitmapSize = 128; // header icon is 64 DIP; sharp up to 200 % scaling
constexpr double kDialogMaxWidth = 760.0;

int dipToPixels(int dip, UINT dpi) {
    return ::MulDiv(dip, static_cast<int>(dpi), USER_DEFAULT_SCREEN_DPI);
}

// Windows app mode (Settings > Personalization > Colors), not the taskbar mode.
bool appsUseDarkTheme() {
    DWORD value = 1;
    DWORD size = sizeof(value);
    ::RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                   L"AppsUseLightTheme", RRF_RT_REG_DWORD, nullptr, &value, &size);
    return value == 0;
}

std::wstring utf8ToWide(std::string_view text) {
    if (text.size() >= 3 && text.substr(0, 3) == "\xEF\xBB\xBF") {
        text.remove_prefix(3);
    }
    if (text.empty()) {
        return {};
    }
    const int length = ::MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    std::wstring result(static_cast<size_t>(length), L'\0');
    ::MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), result.data(), length);
    return result;
}

// Renders an icon resource into a premultiplied BGRA bitmap for the XAML Image.
winrt::Windows::Graphics::Imaging::SoftwareBitmap iconBitmap(HINSTANCE instance, UINT iconId, int size) {
    using namespace winrt::Windows::Graphics::Imaging;
    HICON icon = nullptr;
    winrt::check_hresult(::LoadIconWithScaleDown(instance, MAKEINTRESOURCEW(iconId), size, size, &icon));

    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(info.bmiHeader);
    info.bmiHeader.biWidth = size;
    info.bmiHeader.biHeight = -size; // top-down
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    void* bits = nullptr;
    const HDC dc = ::CreateCompatibleDC(nullptr);
    const HBITMAP dib = ::CreateDIBSection(dc, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
    SoftwareBitmap bitmap{nullptr};
    if (dib && bits) {
        const HGDIOBJ previous = ::SelectObject(dc, dib);
        // Alpha-blending onto the zeroed DIB yields premultiplied BGRA.
        ::DrawIconEx(dc, 0, 0, icon, size, size, 0, nullptr, DI_NORMAL);
        ::GdiFlush();
        const uint32_t bytes = static_cast<uint32_t>(size) * static_cast<uint32_t>(size) * 4u;
        winrt::Windows::Storage::Streams::Buffer buffer(bytes);
        std::memcpy(buffer.data(), bits, bytes);
        buffer.Length(bytes);
        bitmap = SoftwareBitmap(BitmapPixelFormat::Bgra8, size, size, BitmapAlphaMode::Premultiplied);
        bitmap.CopyFromBuffer(buffer);
        ::SelectObject(dc, previous);
    }
    if (dib) {
        ::DeleteObject(dib);
    }
    ::DeleteDC(dc);
    ::DestroyIcon(icon);
    winrt::check_bool(static_cast<bool>(bitmap));
    return bitmap;
}

} // namespace

struct SettingsWindow::Impl {
    Impl(HINSTANCE instanceHandle, UINT iconId) : instance(instanceHandle), iconResourceId(iconId) {}

    HINSTANCE instance;
    UINT iconResourceId;
    XamlRuntime runtime;
    bool classRegistered = false;

    HWND hwnd = nullptr;
    HICON largeIcon = nullptr;
    HICON smallIcon = nullptr;
    SettingsContent content;

    muxh::DesktopWindowXamlSource source{nullptr};
    mux::FrameworkElement root{nullptr};
    muxc::ContentDialog dialog{nullptr};
    winrt::Windows::Foundation::IAsyncOperation<muxc::ContentDialogResult> dialogOperation{nullptr};
    winrt::Windows::Foundation::IAsyncAction iconOperation{nullptr};
    bool focusedOnce = false;
    bool navigatingFocus = false;

    bool create();
    void buildContent();
    void applyTheme();
    void applyBackdrop();
    void resizeIsland();
    void focusIsland();
    bool navigateFocus(muxh::XamlSourceFocusNavigationReason reason);
    void showLicenses();
    void releaseContent() noexcept;
    LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam);

    template <typename T>
    T find(const wchar_t* name) const {
        return root.FindName(name).as<T>();
    }

    static LRESULT CALLBACK windowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
};

bool SettingsWindow::Impl::create() {
    if (!classRegistered) {
        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = &Impl::windowProc;
        wc.hInstance = instance;
        wc.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
        wc.lpszClassName = kWindowClass;
        if (!::RegisterClassExW(&wc)) {
            return false;
        }
        classRegistered = true;
    }

    // Centered on the work area of the monitor under the mouse pointer (where the tray icon is).
    POINT cursor{};
    ::GetCursorPos(&cursor);
    const HMONITOR monitor = ::MonitorFromPoint(cursor, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monitorInfo{};
    monitorInfo.cbSize = sizeof(monitorInfo);
    ::GetMonitorInfoW(monitor, &monitorInfo);
    UINT dpiX = USER_DEFAULT_SCREEN_DPI;
    UINT dpiY = USER_DEFAULT_SCREEN_DPI;
    ::GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);
    RECT frame{0, 0, dipToPixels(kDefaultWidthDip, dpiX), dipToPixels(kDefaultHeightDip, dpiX)};
    ::AdjustWindowRectExForDpi(&frame, kWindowStyle, FALSE, 0, dpiX);
    const RECT& work = monitorInfo.rcWork;
    const int width = std::min<int>(frame.right - frame.left, work.right - work.left);
    const int height = std::min<int>(frame.bottom - frame.top, work.bottom - work.top);
    const int x = work.left + (work.right - work.left - width) / 2;
    const int y = work.top + (work.bottom - work.top - height) / 2;

    hwnd = ::CreateWindowExW(0, kWindowClass, content.windowTitle.c_str(), kWindowStyle, x, y, width, height, nullptr,
                             nullptr, instance, this);
    if (!hwnd) {
        return false;
    }
    if (SUCCEEDED(::LoadIconMetric(instance, MAKEINTRESOURCEW(iconResourceId), LIM_LARGE, &largeIcon))) {
        ::SendMessageW(hwnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(largeIcon));
    }
    if (SUCCEEDED(::LoadIconMetric(instance, MAKEINTRESOURCEW(iconResourceId), LIM_SMALL, &smallIcon))) {
        ::SendMessageW(hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(smallIcon));
    }

    source = muxh::DesktopWindowXamlSource();
    source.Initialize(winrt::Microsoft::UI::GetWindowIdFromWindow(hwnd));
    buildContent();
    source.Content(root);
    applyTheme();
    applyBackdrop(); // only after Content is set; earlier it crashes in Microsoft.UI.Xaml.dll
    source.TakeFocusRequested([this](auto&&, muxh::DesktopWindowXamlSourceTakeFocusRequestedEventArgs const& args) {
        // The island is the only content of the window: keyboard navigation wraps around instead
        // of leaving it. Requests raised by our own NavigateFocus call are ignored, otherwise
        // they would recurse.
        if (!navigatingFocus) {
            navigateFocus(args.Request().Reason());
        }
    });
    resizeIsland();
    return true;
}

void SettingsWindow::Impl::buildContent() {
    root = mux::Markup::XamlReader::Load(utf8ToWide(content.pageXaml)).as<mux::FrameworkElement>();

    find<muxc::TextBlock>(L"AppName").Text(content.appName);
    find<muxc::TextBlock>(L"VersionText").Text(content.versionLine);
    find<muxc::TextBlock>(L"DeveloperLabel").Text(content.developerLabel);
    find<muxc::TextBlock>(L"DeveloperValue").Text(content.developerName);
    find<muxc::TextBlock>(L"WebsiteLabel").Text(content.websiteLabel);
    find<muxc::TextBlock>(L"LicensesLabel").Text(content.licensesLabel);

    auto website = find<muxc::HyperlinkButton>(L"WebsiteLink");
    website.Content(winrt::box_value(content.websiteUrl));
    website.Click([this](auto&&, auto&&) {
        ::ShellExecuteW(hwnd, L"open", content.websiteUrl.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    });

    auto licenses = find<muxc::HyperlinkButton>(L"LicensesLink");
    licenses.Content(winrt::box_value(content.licensesLinkText));
    mux::Automation::AutomationProperties::SetName(licenses, content.licensesLinkName);
    licenses.Click([this](auto&&, auto&&) { showLicenses(); });

    auto icon = find<muxc::Image>(L"AppIcon");
    mux::Automation::AutomationProperties::SetName(icon, content.appIconName);
    mux::Media::Imaging::SoftwareBitmapSource iconSource;
    iconOperation = iconSource.SetBitmapAsync(iconBitmap(instance, iconResourceId, kIconBitmapSize));
    icon.Source(iconSource);

    root.KeyDown([this](auto&&, mux::Input::KeyRoutedEventArgs const& args) {
        // Esc closes the window. An open dialog handles Esc itself (it is not part of this tree).
        if (args.Key() == winrt::Windows::System::VirtualKey::Escape) {
            args.Handled(true);
            ::PostMessageW(hwnd, WM_CLOSE, 0, 0);
        }
    });
    // Keyboard focus can only be placed once the content is loaded.
    root.Loaded([this](auto&&, auto&&) { focusIsland(); });
}

void SettingsWindow::Impl::applyTheme() {
    const bool dark = appsUseDarkTheme();
    const BOOL darkTitleBar = dark ? TRUE : FALSE;
    ::DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkTitleBar, sizeof(darkTitleBar));
    if (root) {
        root.RequestedTheme(dark ? mux::ElementTheme::Dark : mux::ElementTheme::Light);
    }
    if (dialog) {
        dialog.RequestedTheme(dark ? mux::ElementTheme::Dark : mux::ElementTheme::Light);
    }
}

void SettingsWindow::Impl::applyBackdrop() {
    if (winrt::Microsoft::UI::Composition::SystemBackdrops::MicaController::IsSupported()) {
        source.SystemBackdrop(mux::Media::MicaBackdrop());
        return;
    }
    // No Mica (e.g. transparency not supported by the system): solid window background.
    const auto brush =
        mux::Application::Current().Resources().Lookup(winrt::box_value(L"SolidBackgroundFillColorBaseBrush"));
    root.as<muxc::Panel>().Background(brush.as<mux::Media::Brush>());
}

void SettingsWindow::Impl::resizeIsland() {
    if (source && hwnd) {
        RECT client{};
        ::GetClientRect(hwnd, &client);
        source.SiteBridge().MoveAndResize({0, 0, client.right - client.left, client.bottom - client.top});
    }
}

bool SettingsWindow::Impl::navigateFocus(muxh::XamlSourceFocusNavigationReason reason) {
    if (navigatingFocus || !source) {
        return false;
    }
    navigatingFocus = true;
    bool moved = false;
    try {
        moved = source.NavigateFocus(muxh::XamlSourceFocusNavigationRequest(reason)).WasFocusMoved();
    } catch (const winrt::hresult_error&) {
    }
    navigatingFocus = false;
    return moved;
}

void SettingsWindow::Impl::focusIsland() {
    if (!source || !::IsWindowVisible(hwnd)) {
        return;
    }
    // Win32 keyboard focus must be on the island's child window, otherwise key messages go to
    // the host window and never reach XAML.
    const HWND island = winrt::Microsoft::UI::GetWindowFromWindowId(source.SiteBridge().WindowId());
    if (island && ::GetFocus() != island) {
        ::SetFocus(island);
    }
    // First time: first focusable element; afterwards the element that had focus before.
    const bool moved = navigateFocus(focusedOnce ? muxh::XamlSourceFocusNavigationReason::Restore
                                                 : muxh::XamlSourceFocusNavigationReason::First);
    focusedOnce = focusedOnce || moved;
}

void SettingsWindow::Impl::showLicenses() {
    if (dialog) {
        return;
    }
    muxc::TextBlock text;
    text.Text(utf8ToWide(noticesToPlainText(content.noticesMarkdown)));
    text.TextWrapping(mux::TextWrapping::Wrap);
    text.IsTextSelectionEnabled(true);
    muxc::ScrollViewer scroll;
    scroll.Content(text);
    scroll.Padding({0, 0, 16, 0});

    muxc::ContentDialog licenseDialog;
    static_cast<void>(licenseDialog.Resources().Insert(winrt::box_value(L"ContentDialogMaxWidth"),
                                                       winrt::box_value(kDialogMaxWidth)));
    licenseDialog.XamlRoot(root.XamlRoot());
    licenseDialog.Title(winrt::box_value(content.licensesLabel));
    licenseDialog.Content(scroll);
    licenseDialog.CloseButtonText(content.closeButtonText);
    licenseDialog.DefaultButton(muxc::ContentDialogButton::Close);
    licenseDialog.Closed([this](auto&&, auto&&) {
        dialog = nullptr;
        dialogOperation = nullptr;
    });
    dialog = licenseDialog;
    applyTheme();
    dialogOperation = licenseDialog.ShowAsync();
}

void SettingsWindow::Impl::releaseContent() noexcept {
    try {
        if (dialog) {
            dialog.Hide();
        }
        if (source) {
            source.Close();
        }
    } catch (...) {
        VP_SETTINGS_LOG(L"closing the island failed");
    }
    dialog = nullptr;
    dialogOperation = nullptr;
    iconOperation = nullptr;
    root = nullptr;
    source = nullptr;
    focusedOnce = false;
}

LRESULT SettingsWindow::Impl::handleMessage(UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_SIZE:
        resizeIsland();
        return 0;
    case WM_DPICHANGED: {
        const auto* suggested = reinterpret_cast<const RECT*>(lParam);
        ::SetWindowPos(hwnd, nullptr, suggested->left, suggested->top, suggested->right - suggested->left,
                       suggested->bottom - suggested->top, SWP_NOZORDER | SWP_NOACTIVATE);
        return 0;
    }
    case WM_GETMINMAXINFO: {
        auto* info = reinterpret_cast<MINMAXINFO*>(lParam);
        const UINT dpi = ::GetDpiForWindow(hwnd);
        RECT frame{0, 0, dipToPixels(kMinWidthDip, dpi), dipToPixels(kMinHeightDip, dpi)};
        ::AdjustWindowRectExForDpi(&frame, kWindowStyle, FALSE, 0, dpi);
        info->ptMinTrackSize = {frame.right - frame.left, frame.bottom - frame.top};
        return 0;
    }
    case WM_ACTIVATE:
        if (LOWORD(wParam) != WA_INACTIVE) {
            focusIsland();
        }
        break;
    case WM_SETFOCUS:
        focusIsland();
        return 0;
    case WM_ERASEBKGND:
        return 1; // the island covers the client area
    case WM_SETTINGCHANGE:
        if (lParam != 0 && ::CompareStringOrdinal(reinterpret_cast<LPCWSTR>(lParam), -1, L"ImmersiveColorSet", -1,
                                                  TRUE) == CSTR_EQUAL) {
            applyTheme();
        }
        break;
    case WM_CLOSE:
        ::DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        releaseContent();
        return 0;
    default:
        break;
    }
    return ::DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT CALLBACK SettingsWindow::Impl::windowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lParam);
        ::SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
    }
    auto* self = reinterpret_cast<Impl*>(::GetWindowLongPtrW(window, GWLP_USERDATA));
    if (!self || !self->hwnd) {
        // Messages during CreateWindowExW, before hwnd is set.
        return ::DefWindowProcW(window, message, wParam, lParam);
    }
    if (message == WM_NCDESTROY) {
        ::SetWindowLongPtrW(window, GWLP_USERDATA, 0);
        self->hwnd = nullptr;
        if (self->largeIcon) {
            ::DestroyIcon(self->largeIcon);
            self->largeIcon = nullptr;
        }
        if (self->smallIcon) {
            ::DestroyIcon(self->smallIcon);
            self->smallIcon = nullptr;
        }
        return ::DefWindowProcW(window, message, wParam, lParam);
    }
    // No exception may leave a window procedure.
    try {
        return self->handleMessage(message, wParam, lParam);
    } catch (const winrt::hresult_error& e) {
        VP_SETTINGS_LOG(L"message 0x%04X failed: 0x%08X %s", message, static_cast<unsigned>(e.code().value),
                        e.message().c_str());
        (void)e;
    } catch (...) {
        VP_SETTINGS_LOG(L"message 0x%04X failed", message);
    }
    return ::DefWindowProcW(window, message, wParam, lParam);
}

SettingsWindow::SettingsWindow(HINSTANCE instance, UINT iconResourceId)
    : m_impl(std::make_unique<Impl>(instance, iconResourceId)) {}

SettingsWindow::~SettingsWindow() {
    shutdown();
}

bool SettingsWindow::isAvailable() const noexcept {
    return !m_impl->runtime.hasFailed();
}

bool SettingsWindow::show(const SettingsContent& content) {
    Impl& impl = *m_impl;
    if (impl.hwnd) {
        if (::IsIconic(impl.hwnd)) {
            ::ShowWindow(impl.hwnd, SW_RESTORE);
        }
        ::SetForegroundWindow(impl.hwnd);
        return true;
    }
    if (!impl.runtime.ensureInitialized()) {
        return false;
    }
    impl.content = content;
    try {
        if (!impl.create()) {
            VP_SETTINGS_LOG(L"creating the window failed (error %lu)", ::GetLastError());
            if (impl.hwnd) {
                ::DestroyWindow(impl.hwnd);
            }
            return false;
        }
    } catch (const winrt::hresult_error& e) {
        VP_SETTINGS_LOG(L"creating the content failed: 0x%08X %s", static_cast<unsigned>(e.code().value),
                        e.message().c_str());
        (void)e;
        if (impl.hwnd) {
            ::DestroyWindow(impl.hwnd);
        }
        return false;
    }
    ::ShowWindow(impl.hwnd, SW_SHOWNORMAL);
    ::SetForegroundWindow(impl.hwnd);
    VP_SETTINGS_LOG(L"window opened");
    return true;
}

bool SettingsWindow::preTranslateMessage(const MSG& msg) noexcept {
    return m_impl->runtime.preTranslateMessage(msg);
}

void SettingsWindow::shutdown() noexcept {
    if (m_impl->hwnd) {
        ::DestroyWindow(m_impl->hwnd);
    }
    m_impl->runtime.shutdown();
}

} // namespace vp
