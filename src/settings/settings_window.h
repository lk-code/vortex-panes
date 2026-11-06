#pragma once

#include <windows.h>

#include <memory>
#include <string>
#include <string_view>

namespace vp {

// Texts and data of the settings window. The application supplies them (localized strings and
// embedded resources), so this module does not depend on the application's resource IDs.
struct SettingsContent {
    std::wstring windowTitle;
    std::wstring appName;
    std::wstring versionLine; // e.g. "Version 0.1.0 (x64)"
    std::wstring developerLabel;
    std::wstring developerName;
    std::wstring websiteLabel;
    std::wstring websiteUrl;
    std::wstring licensesLabel;    // card label and dialog title
    std::wstring licensesLinkText; // e.g. "Show"
    std::wstring licensesLinkName; // accessible name of the link, e.g. "Show third-party licenses"
    std::wstring closeButtonText;
    std::wstring appIconName;         // accessible name of the application icon
    std::string_view pageXaml;        // UTF-8 XAML of the page; contains no texts
    std::string_view noticesMarkdown; // UTF-8 THIRD_PARTY_NOTICES.md
};

// Settings window: WinUI 3 content hosted in a Win32 window via XAML Islands. WinUI is
// initialized on the first call of show() and stays initialized for the rest of the process;
// before that no Windows App SDK library is loaded. At most one window exists at a time.
// All methods must be called on the UI thread.
class SettingsWindow {
public:
    // iconResourceId: icon resource in `instance` used for the window and the page header.
    SettingsWindow(HINSTANCE instance, UINT iconResourceId);
    ~SettingsWindow();

    SettingsWindow(const SettingsWindow&) = delete;
    SettingsWindow& operator=(const SettingsWindow&) = delete;

    // False after WinUI failed to initialize; the window cannot be opened then.
    bool isAvailable() const noexcept;

    // Opens the window, or brings it to the foreground if it is already open. Returns false if
    // the window could not be created.
    bool show(const SettingsContent& content);

    // Lets WinUI handle keyboard messages (Tab, arrow keys, access keys) before TranslateMessage.
    // Returns true if the message was consumed. Cheap no-op until WinUI is initialized.
    bool preTranslateMessage(const MSG& msg) noexcept;

    // Closes the window and shuts WinUI down on this thread. Call before the message loop ends.
    void shutdown() noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace vp
