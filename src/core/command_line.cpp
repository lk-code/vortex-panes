#include "core/command_line.h"

namespace vp {

CommandLineOptions parseCommandLine(std::span<const std::wstring_view> args) {
    constexpr std::wstring_view kLang = L"--lang";
    constexpr std::wstring_view kLangEquals = L"--lang=";
    constexpr std::wstring_view kWarp = L"--warp";

    CommandLineOptions options;
    for (size_t i = 0; i < args.size(); ++i) {
        const std::wstring_view arg = args[i];
        if (arg == kWarp) {
            options.forceWarp = true;
        } else if (arg == kLang) {
            if (i + 1 < args.size() && !args[i + 1].starts_with(L"--")) {
                options.language = args[++i];
            }
        } else if (arg.starts_with(kLangEquals)) {
            options.language = arg.substr(kLangEquals.size());
        }
    }
    return options;
}

} // namespace vp
