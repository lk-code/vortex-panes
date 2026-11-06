#include "core/text_format.h"

#include <gtest/gtest.h>

TEST(TextFormat, ReplacesPlaceholdersInOrder) {
    EXPECT_EQ(vp::formatText(L"Version {0} ({1})", {L"0.1.0", L"x64"}), L"Version 0.1.0 (x64)");
}

TEST(TextFormat, PlaceholdersMayBeReordered) {
    EXPECT_EQ(vp::formatText(L"{1} / {0}", {L"a", L"b"}), L"b / a");
}

TEST(TextFormat, PlaceholderMayRepeat) {
    EXPECT_EQ(vp::formatText(L"{0}{0}", {L"ab"}), L"abab");
}

TEST(TextFormat, KeepsPlaceholdersWithoutArgument) {
    EXPECT_EQ(vp::formatText(L"{0} {1}", {L"x"}), L"x {1}");
}

TEST(TextFormat, KeepsOtherBraces) {
    EXPECT_EQ(vp::formatText(L"{a} {} {12", {L"x"}), L"{a} {} {12");
}

TEST(TextFormat, HandlesNonAsciiText) {
    EXPECT_EQ(vp::formatText(L"バージョン {0} ({1})", {L"1.2.3", L"arm64"}), L"バージョン 1.2.3 (arm64)");
}

TEST(TextFormat, EmptyPattern) {
    EXPECT_EQ(vp::formatText(L"", {L"x"}), L"");
}
