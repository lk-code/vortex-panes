#include "core/unique_resource.h"

#include <gtest/gtest.h>

#include <utility>
#include <vector>

namespace {

std::vector<int>& closedValues() {
    static std::vector<int> values;
    return values;
}

struct TestTraits {
    using value_type = int;
    static int invalid() noexcept { return -1; }
    static bool isValid(int v) noexcept { return v >= 0; }
    static void close(int v) noexcept { closedValues().push_back(v); }
};

using TestResource = vp::UniqueResource<TestTraits>;

class UniqueResourceTest : public ::testing::Test {
protected:
    void SetUp() override { closedValues().clear(); }
};

} // namespace

TEST_F(UniqueResourceTest, DefaultIsInvalidAndClosesNothing) {
    {
        TestResource r;
        EXPECT_FALSE(r);
        EXPECT_EQ(r.get(), -1);
    }
    EXPECT_TRUE(closedValues().empty());
}

TEST_F(UniqueResourceTest, ClosesOnDestruction) {
    {
        TestResource r(7);
    }
    EXPECT_EQ(closedValues(), std::vector<int>{7});
}

TEST_F(UniqueResourceTest, ResetClosesOldValue) {
    TestResource r(1);
    r.reset(2);
    EXPECT_EQ(closedValues(), std::vector<int>{1});
    EXPECT_EQ(r.get(), 2);
    r.reset();
    EXPECT_EQ(closedValues(), (std::vector<int>{1, 2}));
    EXPECT_FALSE(r);
}

TEST_F(UniqueResourceTest, ReleaseGivesUpOwnership) {
    TestResource r(3);
    EXPECT_EQ(r.release(), 3);
    EXPECT_FALSE(r);
    EXPECT_TRUE(closedValues().empty());
}

TEST_F(UniqueResourceTest, MoveTransfersOwnership) {
    TestResource a(4);
    TestResource b(std::move(a));
    EXPECT_FALSE(a);
    EXPECT_EQ(b.get(), 4);

    TestResource c(5);
    c = std::move(b);
    EXPECT_EQ(closedValues(), std::vector<int>{5});
    EXPECT_EQ(c.get(), 4);
}
