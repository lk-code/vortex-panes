#pragma once

#include <span>
#include <string>
#include <string_view>

namespace vp {

// Developer switches. The application only honors them in Debug builds.
struct CommandLineOptions {
    std::wstring language;  // --lang <tag> or --lang=<tag>; empty means automatic
    bool forceWarp = false; // --warp: use the WARP software rasterizer
};

// Parses the arguments without the program name. Unknown arguments are ignored.
CommandLineOptions parseCommandLine(std::span<const std::wstring_view> args);

} // namespace vp
