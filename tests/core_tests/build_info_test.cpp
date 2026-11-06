#include "core/build_info.h"

#include <gtest/gtest.h>

TEST(BuildInfo, ArchitectureNameMatchesPointerSize) {
    const auto name = vp::architectureName();
    if constexpr (sizeof(void*) == 4) {
        EXPECT_EQ(name, L"x86");
    } else {
        EXPECT_TRUE(name == L"x64" || name == L"arm64");
    }
}

TEST(BuildInfo, ArchitectureNameMatchesCompilerTarget) {
#if defined(_M_ARM64)
    EXPECT_EQ(vp::architectureName(), L"arm64");
#elif defined(_M_X64)
    EXPECT_EQ(vp::architectureName(), L"x64");
#else
    EXPECT_EQ(vp::architectureName(), L"x86");
#endif
}
