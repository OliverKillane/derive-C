#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <derive-cpp/test/trampoline.hpp>
#include <derive-cpp/test/gtest_mock.hpp>

#include <derive-c/test/mock.h>

using namespace testing;

namespace {
size_t none_count = 0;
DC_MOCKABLE(void, none, (void)) { none_count++; }

DC_MOCKABLE(int, foo, (int)) { return 2; }
DC_MOCKABLE(void, array, (int a[6])) {}
DC_MOCKABLE(void, fn_ptr, (void (*fn)(int))) {}
} // namespace

struct MockTests : Test {
    FIXTURE_MOCK(MockTests, void, none, (), ());
    FIXTURE_MOCK(MockTests, int, foo, (int a), ());
    FIXTURE_MOCK(MockTests, void, array, (int a[6]), ());
    FIXTURE_MOCK(MockTests, void, fn_ptr, (void (*fn)(int)), ());
};

TEST_F(MockTests, ArgTypes) {
    {
        EXPECT_CALL(*this, none_mock()).Times(1);
        none();
    }

    {
        EXPECT_CALL(*this, foo_mock(1)).WillOnce(Return(42));
        int result = foo(1);
        EXPECT_EQ(result, 42);
    }

    {
        int arr[6] = {0, 1, 2, 3, 4, 5};
        EXPECT_CALL(*this, array_mock(_)).Times(1);
        array(arr);
    }

    {
        EXPECT_CALL(*this, fn_ptr_mock(_)).Times(1).WillOnce([](void (*fn)(int)) {
            EXPECT_EQ(fn, nullptr);
        });
        fn_ptr(nullptr);
    }
}

TEST_F(MockTests, EnableDisable) {
    EXPECT_CALL(*this, none_mock()).Times(1);
    none();

    const size_t none_count_before = none_count;
    none_tramp.Disable();
    none();
    EXPECT_EQ(none_count, none_count_before + 1);

    none_tramp.Enable();
    EXPECT_CALL(*this, none_mock()).Times(1);
    none();
}
