#include "core/ui_language.h"

namespace vp {

namespace {

constexpr wchar_t toLowerAscii(wchar_t c) noexcept {
    return (c >= L'A' && c <= L'Z') ? static_cast<wchar_t>(c - L'A' + L'a') : c;
}

constexpr bool equalsIgnoreCase(std::wstring_view a, std::wstring_view b) noexcept {
    if (a.size() != b.size()) {
        return false;
    }
    for (size_t i = 0; i < a.size(); ++i) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) {
            return false;
        }
    }
    return true;
}

constexpr bool isSeparator(wchar_t c) noexcept {
    return c == L'-' || c == L'_';
}

// Returns the subtag starting at pos and advances pos past the following separator.
constexpr std::wstring_view nextSubtag(std::wstring_view name, size_t& pos) noexcept {
    const size_t start = pos;
    while (pos < name.size() && !isSeparator(name[pos])) {
        ++pos;
    }
    const std::wstring_view subtag = name.substr(start, pos - start);
    if (pos < name.size()) {
        ++pos;
    }
    return subtag;
}

constexpr std::wstring_view primarySubtag(std::wstring_view name) noexcept {
    size_t pos = 0;
    return nextSubtag(name, pos);
}

// Decides whether a "zh..." locale uses the simplified script.
bool isSimplifiedChinese(std::wstring_view name) noexcept {
    size_t pos = 0;
    nextSubtag(name, pos); // "zh"
    bool traditional = false;
    while (pos < name.size()) {
        const std::wstring_view subtag = nextSubtag(name, pos);
        if (equalsIgnoreCase(subtag, L"hans")) {
            return true;
        }
        if (equalsIgnoreCase(subtag, L"hant") || equalsIgnoreCase(subtag, L"tw") || equalsIgnoreCase(subtag, L"hk") ||
            equalsIgnoreCase(subtag, L"mo")) {
            traditional = true;
        }
    }
    return !traditional;
}

} // namespace

std::optional<std::wstring_view> matchUiLanguage(std::wstring_view localeName) noexcept {
    const std::wstring_view primary = primarySubtag(localeName);
    if (primary.empty()) {
        return std::nullopt;
    }
    for (const std::wstring_view language : kUiLanguages) {
        if (equalsIgnoreCase(language, localeName)) {
            return language;
        }
    }
    if (equalsIgnoreCase(primary, L"zh") && !isSimplifiedChinese(localeName)) {
        return std::nullopt;
    }
    for (const std::wstring_view language : kUiLanguages) {
        if (equalsIgnoreCase(primarySubtag(language), primary)) {
            return language;
        }
    }
    return std::nullopt;
}

std::wstring_view resolveUiLanguage(std::span<const std::wstring_view> preferred) noexcept {
    for (const std::wstring_view name : preferred) {
        if (const auto match = matchUiLanguage(name)) {
            return *match;
        }
    }
    return kFallbackUiLanguage;
}

} // namespace vp
