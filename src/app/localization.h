#pragma once

#include <windows.h>

#include <string>
#include <string_view>

namespace vp {

// Chooses the UI language (Windows display language preferences, or an explicit override from
// --lang in Debug builds), applies it to the calling thread and remembers it for loadString().
// Returns the chosen language tag, e.g. L"de-DE".
std::wstring_view initializeUiLanguage(std::wstring_view overrideTag);

// Loads a string from the STRINGTABLE of the chosen language, falling back to en-US.
std::wstring loadString(UINT id);

} // namespace vp
