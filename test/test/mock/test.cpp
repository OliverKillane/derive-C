#include <derive-cpp/test/gtest_panic.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <derive-cpp/test/trampoline.hpp>

extern "C" {
#include <derive-c/test/mock.h>

static size_t none_count = 0;
MOCKABLE(void, none, (void)) { none_count++; }
MOCKABLE(int, foo, (int a)) { return 2; }
MOCKABLE(void, array, (int a[6])) {}
MOCKABLE(void, fn_ptr, (void (*fn)(int))) {}
}

using namespace testing;

struct MockTests : Test {
    MOCK_METHOD(void, none_mock, (), ());
    derivecpp::Trampoline<&none, &MockTests::none_mock> none_tramp{this};

    MOCK_METHOD(int, foo_mock, (int a), ());
    derivecpp::Trampoline<&foo, &MockTests::foo_mock> foo_tramp{this};

    MOCK_METHOD(void, array_mock, (int a[6]), ());
    derivecpp::Trampoline<&array, &MockTests::array_mock> array_tramp{this};

    MOCK_METHOD(void, fn_ptr_mock, (void (*fn)(int)), ());
    derivecpp::Trampoline<&fn_ptr, &MockTests::fn_ptr_mock> fn_ptr_tramp{this};
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
