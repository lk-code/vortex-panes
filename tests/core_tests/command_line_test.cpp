#include "core/command_line.h"

#include <gtest/gtest.h>

#include <array>
#include <string_view>

using namespace std::literals;

TEST(CommandLine, EmptyGivesDefaults) {
    const auto options = vp::parseCommandLine({});
    EXPECT_TRUE(options.language.empty());
    EXPECT_FALSE(options.forceWarp);
}

TEST(CommandLine, LanguageAsSeparateArgument) {
    const std::array args{L"--lang"sv, L"ja-JP"sv};
    EXPECT_EQ(vp::parseCommandLine(args).language, L"ja-JP");
}

TEST(CommandLine, LanguageWithEquals) {
    const std::array args{L"--lang=ko-KR"sv};
    EXPECT_EQ(vp::parseCommandLine(args).language, L"ko-KR");
}

TEST(CommandLine, LanguageWithoutValueIsIgnored) {
    const std::array trailing{L"--lang"sv};
    EXPECT_TRUE(vp::parseCommandLine(trailing).language.empty());

    const std::array followedBySwitch{L"--lang"sv, L"--warp"sv};
    const auto options = vp::parseCommandLine(followedBySwitch);
    EXPECT_TRUE(options.language.empty());
    EXPECT_TRUE(options.forceWarp);
}

TEST(CommandLine, WarpAndUnknownArguments) {
    const std::array args{L"--unknown"sv, L"--warp"sv, L"value"sv};
    const auto options = vp::parseCommandLine(args);
    EXPECT_TRUE(options.forceWarp);
    EXPECT_TRUE(options.language.empty());
}
