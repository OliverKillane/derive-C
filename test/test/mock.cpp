#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <derive-c/test/trampoline.hpp>

extern "C" {
#include <derive-c/test/mock.h>

MOCKABLE(int, foo, int a) { return a + 1; }
}

struct MockTests : testing::Test {
    MOCK_METHOD(int, foo_mock, (int a), ());

    Trampoline<&foo, &MockTests::foo_mock> t{this};
};

TEST_F(MockTests, foo_test) {
    EXPECT_CALL(*this, foo_mock(1)).WillOnce(testing::Return(42));

    int result = foo(1);
    EXPECT_EQ(result, 42);
}
