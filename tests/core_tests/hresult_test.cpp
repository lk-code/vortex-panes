#include "core/hresult.h"

#include <gtest/gtest.h>

#include <string>

namespace {

constexpr int32_t kOk = 0;
constexpr int32_t kFalse = 1;
constexpr int32_t kFail = static_cast<int32_t>(0x80004005u);

} // namespace

TEST(HResult, FailedOnlyForNegativeValues) {
    EXPECT_FALSE(vp::hresultFailed(kOk));
    EXPECT_FALSE(vp::hresultFailed(kFalse));
    EXPECT_TRUE(vp::hresultFailed(kFail));
}

TEST(HResult, ThrowIfFailedDoesNotThrowOnSuccess) {
    EXPECT_NO_THROW(THROW_IF_FAILED(kOk));
    EXPECT_NO_THROW(THROW_IF_FAILED(kFalse));
}

TEST(HResult, ThrowIfFailedCarriesCodeAndExpression) {
    try {
        THROW_IF_FAILED(kFail);
        FAIL() << "expected HResultError";
    } catch (const vp::HResultError& e) {
        EXPECT_EQ(e.code(), kFail);
        const std::string what = e.what();
        EXPECT_NE(what.find("kFail"), std::string::npos);
        EXPECT_NE(what.find("0x80004005"), std::string::npos);
    }
}
