#pragma once

#include <optional>
#include <string_view>

namespace vp {

// An RT_STRING resource block holds 16 consecutive strings; string ID n lives in block n / 16 + 1
// at index n % 16. Each entry is a UTF-16 length followed by that many characters (not terminated).
constexpr unsigned stringBlockId(unsigned stringId) noexcept {
    return stringId / 16 + 1;
}

constexpr unsigned stringBlockIndex(unsigned stringId) noexcept {
    return stringId % 16;
}

// Extracts entry `index` (0..15) from a string block. Returns nullopt if the block is malformed
// or the index is out of range; an empty view means the entry exists but is empty.
std::optional<std::wstring_view> stringFromBlock(std::wstring_view block, unsigned index) noexcept;

} // namespace vp
