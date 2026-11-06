#include "core/notices_text.h"

#include <gtest/gtest.h>

TEST(NoticesText, RemovesHeadingMarkers) {
    EXPECT_EQ(vp::noticesToPlainText("# Third-Party Notices\n\n## GoogleTest\n### Detail"),
              "Third-Party Notices\n\nGoogleTest\nDetail");
}

TEST(NoticesText, KeepsLicenseTextInsideFencesVerbatim) {
    const char* markdown = "Intro\n```\nCopyright (c) **X**\n  indented `line`\n```\nAfter";
    EXPECT_EQ(vp::noticesToPlainText(markdown), "Intro\nCopyright (c) **X**\n  indented `line`\nAfter");
}

TEST(NoticesText, RemovesBoldAndInlineCode) {
    EXPECT_EQ(vp::noticesToPlainText("- **License:** `MIT`"), "- License: MIT");
}

TEST(NoticesText, WritesAbsoluteLinksWithUrl) {
    EXPECT_EQ(vp::noticesToPlainText("- Source: [cppwinrt](https://github.com/microsoft/cppwinrt)"),
              "- Source: cppwinrt (https://github.com/microsoft/cppwinrt)");
}

TEST(NoticesText, WritesRelativeLinksAsText) {
    EXPECT_EQ(vp::noticesToPlainText("see [LICENSE](LICENSE)."), "see LICENSE.");
}

TEST(NoticesText, KeepsUnclosedBrackets) {
    EXPECT_EQ(vp::noticesToPlainText("[not a link and (x)"), "[not a link and (x)");
}

TEST(NoticesText, NormalizesLineEndingsAndTrailingLines) {
    EXPECT_EQ(vp::noticesToPlainText("a\r\nb\r\n\r\n"), "a\nb");
}

TEST(NoticesText, KeepsHashWithoutSpace) {
    EXPECT_EQ(vp::noticesToPlainText("#define X"), "#define X");
}
