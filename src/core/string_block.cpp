#include "core/string_block.h"

namespace vp {

std::optional<std::wstring_view> stringFromBlock(std::wstring_view block, unsigned index) noexcept {
    if (index >= 16) {
        return std::nullopt;
    }
    size_t pos = 0;
    for (unsigned i = 0;; ++i) {
        if (pos >= block.size()) {
            return std::nullopt;
        }
        const size_t length = static_cast<unsigned short>(block[pos]);
        ++pos;
        if (length > block.size() - pos) {
            return std::nullopt;
        }
        if (i == index) {
            return block.substr(pos, length);
        }
        pos += length;
    }
}

} // namespace vp
