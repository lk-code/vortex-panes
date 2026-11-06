#pragma once

#include "win32_handles.h"

namespace vp {

// Window class of the hidden shell window; used to find the running instance.
inline constexpr wchar_t kShellWindowClass[] = L"VortexPanes.Shell";

// Registered message that asks the running instance to open its settings window.
UINT showSettingsMessage();

class SingleInstance {
public:
    // Returns true if this process is the first instance in the current session.
    bool acquire();

    // Asks the running instance to show its settings window.
    static void notifyRunningInstance();

private:
    UniqueHandle m_mutex;
};

} // namespace vp
