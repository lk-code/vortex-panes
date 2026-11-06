#include <windows.h>
#include <shellapi.h>

#include "application.h"
#include "debug_log.h"
#include "localization.h"
#include "resource.h"
#include "win32_handles.h"

#include "core/app_info.h"
#include "core/command_line.h"

#include <exception>
#include <string_view>
#include <utility>
#include <vector>

namespace {

#ifdef VP_DEBUG
// Developer switches (--lang, --warp) are only honored in Debug builds.
vp::CommandLineOptions readCommandLine() {
    int argc = 0;
    vp::UniqueLocalMemory argvMemory(::CommandLineToArgvW(::GetCommandLineW(), &argc));
    if (!argvMemory || argc <= 1) {
        return {};
    }
    const auto* argv = static_cast<const LPWSTR*>(argvMemory.get());
    std::vector<std::wstring_view> args(argv + 1, argv + argc);
    return vp::parseCommandLine(args);
}
#endif

} // namespace

int WINAPI wWinMain(_In_ HINSTANCE instance, _In_opt_ HINSTANCE /*prevInstance*/, _In_ PWSTR /*cmdLine*/,
                    _In_ int /*showCmd*/) {
    vp::CommandLineOptions options;
#ifdef VP_DEBUG
    options = readCommandLine();
#endif
    vp::initializeUiLanguage(options.language);

    try {
        vp::Application app(instance, std::move(options));
        return app.run();
    } catch (const std::exception& e) {
        VP_DEBUG_LOG(L"fatal error: %hs", e.what());
        (void)e;
        const std::wstring message = vp::loadString(IDS_ERROR_STARTUP);
        ::MessageBoxW(nullptr, message.c_str(), vp::kAppName, MB_OK | MB_ICONERROR);
        return 1;
    }
}
