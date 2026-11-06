#pragma once

#include <windows.h>

#include <memory>

namespace vp {

// The WinUI 3 environment of the UI thread for XAML Islands: single-threaded apartment,
// dispatcher queue, XAML application object (control metadata and Fluent theme resources) and
// WindowsXamlManager. Created on first use; loading the Windows App SDK happens here.
class XamlRuntime {
public:
    XamlRuntime();
    ~XamlRuntime();

    XamlRuntime(const XamlRuntime&) = delete;
    XamlRuntime& operator=(const XamlRuntime&) = delete;

    // Initializes WinUI on the calling thread if needed. Returns false if that failed; the
    // failure is permanent for this process.
    bool ensureInitialized() noexcept;
    bool hasFailed() const noexcept { return m_failed; }

    // Forwards a message to ContentPreTranslateMessage (keyboard handling of islands).
    bool preTranslateMessage(const MSG& msg) const noexcept;

    // Releases the XAML application, WindowsXamlManager and the dispatcher queue. All islands must
    // be closed before.
    void shutdown() noexcept;

private:
    struct State;
    using PreTranslateFn = BOOL(WINAPI*)(const MSG*);

    std::unique_ptr<State> m_state;
    PreTranslateFn m_preTranslate = nullptr;
    bool m_failed = false;
    bool m_comInitialized = false;
};

} // namespace vp
