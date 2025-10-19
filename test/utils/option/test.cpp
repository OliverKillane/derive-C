#include <derive-cpp/test/gtest_panic.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <derive-cpp/test/trampoline.hpp>

extern "C" {
#include <derive-c/test/mock.h>
MOCKABLE(void, free_int, (int* ptr)) { (void)ptr; }
MOCKABLE(int, clone_int, (int const* self)) { return *self; }

#define NAME optional_int
#define ITEM int
#define ITEM_CLONE clone_int
#define ITEM_DELETE free_int
#include <derive-c/utils/option/template.h>
}

using namespace testing;

struct Option : Test {
    MOCK_METHOD(void, free_int_mock, (int* ptr), ());
    derivecpp::Trampoline<&free_int, &Option::free_int_mock> free_int_tramp{this};

    MOCK_METHOD(int, clone_int_mock, (int const* self), ());
    derivecpp::Trampoline<&clone_int, &Option::clone_int_mock> clone_int_tramp{this};
};

TEST_F(Option, EmptyNotPresent) {
    optional_int opt = optional_int_empty();
    ASSERT_FALSE(optional_int_is_present(&opt));
    ASSERT_EQ(optional_int_get(&opt), nullptr);
    ASSERT_EQ(optional_int_get_const(&opt), nullptr);

    optional_int_delete(&opt);
}

TEST_F(Option, Present) {
    optional_int opt = optional_int_from(42);
    ASSERT_TRUE(optional_int_is_present(&opt));
    ASSERT_EQ(*optional_int_get(&opt), 42);
    ASSERT_EQ(*optional_int_get_const(&opt), 42);

    EXPECT_CALL(*this, free_int_mock(_));
    optional_int_delete(&opt);
}

TEST_F(Option, From) {
    optional_int opt = optional_int_from(100);
    ASSERT_TRUE(optional_int_is_present(&opt));
    ASSERT_EQ(*optional_int_get(&opt), 100);
    ASSERT_EQ(*optional_int_get_const(&opt), 100);

    EXPECT_CALL(*this, free_int_mock(_));
    optional_int_delete(&opt);
}

TEST_F(Option, Replace) {
    optional_int opt = optional_int_from(10);
    ASSERT_TRUE(optional_int_is_present(&opt));
    ASSERT_EQ(*optional_int_get(&opt), 10);

    EXPECT_CALL(*this, free_int_mock(_));
    optional_int_replace(&opt, 100);

    ASSERT_TRUE(optional_int_is_present(&opt));
    ASSERT_EQ(*optional_int_get(&opt), 100);

    EXPECT_CALL(*this, free_int_mock(_));
    optional_int_delete(&opt);
}

TEST_F(Option, ReplaceEmpty) {
    optional_int opt = optional_int_empty();
    ASSERT_FALSE(optional_int_is_present(&opt));

    optional_int_replace(&opt, 100);
    ASSERT_TRUE(optional_int_is_present(&opt));
    ASSERT_EQ(*optional_int_get(&opt), 100);

    EXPECT_CALL(*this, free_int_mock(_));
    optional_int_delete(&opt);
}

TEST_F(Option, Clone) {
    optional_int opt_1 = optional_int_from(10);

    EXPECT_CALL(*this, clone_int_mock(_)).WillOnce(Return(10));
    optional_int opt_2 = optional_int_clone(&opt_1);

    EXPECT_CALL(*this, free_int_mock(_));
    optional_int_delete(&opt_1);

    EXPECT_CALL(*this, free_int_mock(_));
    optional_int_delete(&opt_2);
}

TEST_F(Option, GetOr) {
    optional_int opt_some = optional_int_from(10);
    optional_int opt_none = optional_int_empty();

    int def = 20;

    ASSERT_EQ(*optional_int_get_const_or(&opt_some, &def), 10);
    ASSERT_EQ(*optional_int_get_const_or(&opt_none, &def), 20);

    ASSERT_EQ(optional_int_get_value_or(&opt_some, 20), 10);
    ASSERT_EQ(optional_int_get_value_or(&opt_none, 20), 20);

    optional_int_delete(&opt_none);

    EXPECT_CALL(*this, free_int_mock(_));
    optional_int_delete(&opt_some);
}
