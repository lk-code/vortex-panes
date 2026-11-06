#pragma once

#include <string>
#include <string_view>

namespace vp {

// Converts THIRD_PARTY_NOTICES.md (UTF-8 Markdown) into plain text for display in the settings
// window: code fence lines are dropped, heading markers, bold and inline code markers removed,
// and links written as "text (url)" (or only "text" for relative links). License texts inside
// code fences are kept verbatim. Line endings are normalized to '\n'.
std::string noticesToPlainText(std::string_view markdown);

} // namespace vp
