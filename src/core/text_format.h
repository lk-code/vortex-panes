#pragma once

#include <initializer_list>
#include <string>
#include <string_view>

namespace vp {

// Replaces the placeholders {0} to {9} in a localized pattern with the given arguments, e.g.
// formatText(L"Version {0} ({1})", {L"0.1.0", L"x64"}) -> L"Version 0.1.0 (x64)".
// Placeholders without a matching argument and all other braces are kept unchanged.
std::wstring formatText(std::wstring_view pattern, std::initializer_list<std::wstring_view> args);

} // namespace vp
