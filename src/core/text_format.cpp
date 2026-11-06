#include "core/text_format.h"

namespace vp {

std::wstring formatText(std::wstring_view pattern, std::initializer_list<std::wstring_view> args) {
    std::wstring result;
    result.reserve(pattern.size() + 32);
    for (size_t i = 0; i < pattern.size(); ++i) {
        const bool placeholder = pattern[i] == L'{' && i + 2 < pattern.size() && pattern[i + 1] >= L'0' &&
                                 pattern[i + 1] <= L'9' && pattern[i + 2] == L'}';
        if (placeholder) {
            const auto index = static_cast<size_t>(pattern[i + 1] - L'0');
            if (index < args.size()) {
                result.append(*(args.begin() + index));
                i += 2;
                continue;
            }
        }
        result.push_back(pattern[i]);
    }
    return result;
}

} // namespace vp
