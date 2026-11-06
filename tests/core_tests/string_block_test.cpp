#include "core/string_block.h"

#include <gtest/gtest.h>

#include <string>
#include <string_view>

using namespace std::literals;

namespace {

// Builds a block with 16 entries; entry i gets texts[i] if present, otherwise empty.
std::wstring makeBlock(std::initializer_list<std::wstring_view> texts) {
    std::wstring block;
    size_t count = 0;
    for (const auto text : texts) {
        block.push_back(static_cast<wchar_t>(text.size()));
        block.append(text);
        ++count;
    }
    for (; count < 16; ++count) {
        block.push_back(L'\0');
    }
    return block;
}

} // namespace

TEST(StringBlock, IdMapping) {
    EXPECT_EQ(vp::stringBlockId(100), 7u);
    EXPECT_EQ(vp::stringBlockIndex(100), 4u);
    EXPECT_EQ(vp::stringBlockId(15), 1u);
    EXPECT_EQ(vp::stringBlockId(16), 2u);
    EXPECT_EQ(vp::stringBlockIndex(16), 0u);
}

TEST(StringBlock, ExtractsEntries) {
    const std::wstring block = makeBlock({L"", L"Einstellungen", L"", L"設定"});
    EXPECT_EQ(vp::stringFromBlock(block, 1), L"Einstellungen"sv);
    EXPECT_EQ(vp::stringFromBlock(block, 3), L"設定"sv);
    EXPECT_EQ(vp::stringFromBlock(block, 0), L""sv);
    EXPECT_EQ(vp::stringFromBlock(block, 15), L""sv);
}

TEST(StringBlock, RejectsOutOfRangeIndex) {
    const std::wstring block = makeBlock({L"a"});
    EXPECT_FALSE(vp::stringFromBlock(block, 16).has_value());
}

TEST(StringBlock, RejectsTruncatedBlocks) {
    std::wstring block;
    block.push_back(static_cast<wchar_t>(10)); // claims 10 characters
    block.append(L"abc");
    EXPECT_FALSE(vp::stringFromBlock(block, 0).has_value());
    EXPECT_FALSE(vp::stringFromBlock(L""sv, 0).has_value());
    EXPECT_FALSE(vp::stringFromBlock(makeBlock({L"a"}).substr(0, 3), 5).has_value());
}
