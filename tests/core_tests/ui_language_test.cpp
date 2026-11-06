#include "core/ui_language.h"

#include <gtest/gtest.h>

#include <array>
#include <optional>
#include <string_view>

using namespace std::literals;

namespace {

std::wstring_view matchOrNone(std::wstring_view name) {
    return vp::matchUiLanguage(name).value_or(L"<none>"sv);
}

} // namespace

TEST(UiLanguage, FallbackIsEnglish) {
    EXPECT_EQ(vp::kFallbackUiLanguage, L"en-US"sv);
}

TEST(UiLanguage, ExactMatchesAreCaseInsensitive) {
    EXPECT_EQ(matchOrNone(L"de-DE"), L"de-DE"sv);
    EXPECT_EQ(matchOrNone(L"DE-de"), L"de-DE"sv);
    EXPECT_EQ(matchOrNone(L"ko-KR"), L"ko-KR"sv);
    EXPECT_EQ(matchOrNone(L"ja_JP"), L"ja-JP"sv);
}

TEST(UiLanguage, PrimaryLanguageFallsBackToSupportedRegion) {
    EXPECT_EQ(matchOrNone(L"de-AT"), L"de-DE"sv);
    EXPECT_EQ(matchOrNone(L"de-CH"), L"de-DE"sv);
    EXPECT_EQ(matchOrNone(L"de"), L"de-DE"sv);
    EXPECT_EQ(matchOrNone(L"en-GB"), L"en-US"sv);
    EXPECT_EQ(matchOrNone(L"es-MX"), L"es-ES"sv);
    EXPECT_EQ(matchOrNone(L"fr-CA"), L"fr-FR"sv);
    EXPECT_EQ(matchOrNone(L"ja"), L"ja-JP"sv);
}

TEST(UiLanguage, UnsupportedLanguagesDoNotMatch) {
    EXPECT_EQ(matchOrNone(L"pt-BR"), L"<none>"sv);
    EXPECT_EQ(matchOrNone(L"nl-NL"), L"<none>"sv);
    EXPECT_EQ(matchOrNone(L""), L"<none>"sv);
    EXPECT_EQ(matchOrNone(L"-DE"), L"<none>"sv);
}

TEST(UiLanguage, SimplifiedChineseMatchesZhCn) {
    EXPECT_EQ(matchOrNone(L"zh-CN"), L"zh-CN"sv);
    EXPECT_EQ(matchOrNone(L"zh-SG"), L"zh-CN"sv);
    EXPECT_EQ(matchOrNone(L"zh-Hans"), L"zh-CN"sv);
    EXPECT_EQ(matchOrNone(L"zh-Hans-HK"), L"zh-CN"sv);
    EXPECT_EQ(matchOrNone(L"zh"), L"zh-CN"sv);
}

TEST(UiLanguage, TraditionalChineseDoesNotMatchSimplified) {
    EXPECT_EQ(matchOrNone(L"zh-TW"), L"<none>"sv);
    EXPECT_EQ(matchOrNone(L"zh-HK"), L"<none>"sv);
    EXPECT_EQ(matchOrNone(L"zh-MO"), L"<none>"sv);
    EXPECT_EQ(matchOrNone(L"zh-Hant-TW"), L"<none>"sv);
}

TEST(UiLanguage, ResolvePicksFirstSupportedPreference) {
    const std::array preferred{L"pt-BR"sv, L"de-AT"sv, L"fr-FR"sv};
    EXPECT_EQ(vp::resolveUiLanguage(preferred), L"de-DE"sv);
}

TEST(UiLanguage, ResolveFallsBackToEnglish) {
    const std::array unsupported{L"pt-BR"sv, L"zh-TW"sv};
    EXPECT_EQ(vp::resolveUiLanguage(unsupported), L"en-US"sv);
    EXPECT_EQ(vp::resolveUiLanguage({}), L"en-US"sv);
}
