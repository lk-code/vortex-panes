#pragma once

#include <array>
#include <optional>
#include <span>
#include <string_view>

namespace vp {

// UI languages with a STRINGTABLE in resources/strings. The first entry is the fallback.
inline constexpr std::array<std::wstring_view, 7> kUiLanguages{L"en-US", L"de-DE", L"es-ES", L"fr-FR",
                                                               L"zh-CN", L"ja-JP", L"ko-KR"};
inline constexpr std::wstring_view kFallbackUiLanguage = kUiLanguages[0];

// Maps a locale name (e.g. "de-AT", "de_DE", "ja") to a supported UI language.
// Exact matches win, otherwise the primary language decides ("de-AT" -> "de-DE").
// Traditional Chinese (zh-TW, zh-HK, zh-MO, zh-Hant-*) has no translation and does not
// match zh-CN. Returns nullopt if no supported language fits.
std::optional<std::wstring_view> matchUiLanguage(std::wstring_view localeName) noexcept;

// Picks the first supported language from an ordered list of preferences, else en-US.
std::wstring_view resolveUiLanguage(std::span<const std::wstring_view> preferred) noexcept;

} // namespace vp
