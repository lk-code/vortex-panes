#include "single_instance.h"

namespace vp {

UINT showSettingsMessage() {
    static const UINT message = ::RegisterWindowMessageW(L"VortexPanes.ShowSettings");
    return message;
}

bool SingleInstance::acquire() {
    m_mutex.reset(::CreateMutexW(nullptr, TRUE, L"Local\\VortexPanes.SingleInstance"));
    if (!m_mutex) {
        // Without a mutex we cannot tell; run rather than refuse to start.
        return true;
    }
    return ::GetLastError() != ERROR_ALREADY_EXISTS;
}

void SingleInstance::notifyRunningInstance() {
    // The shell window is a hidden top-level window, so FindWindow can see it.
    const HWND running = ::FindWindowW(kShellWindowClass, nullptr);
    if (!running) {
        return;
    }
    DWORD processId = 0;
    ::GetWindowThreadProcessId(running, &processId);
    if (processId != 0) {
        // Let the running instance bring its settings window to the foreground.
        ::AllowSetForegroundWindow(processId);
    }
    ::PostMessageW(running, showSettingsMessage(), 0, 0);
}

} // namespace vp
