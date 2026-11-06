#pragma once

#include <windows.h>

#include <string_view>

namespace vp {

// Returns the bytes of an RCDATA resource of the executable, or an empty view if it is missing.
// The data stays valid for the lifetime of the process.
std::string_view embeddedData(UINT id) noexcept;

} // namespace vp
