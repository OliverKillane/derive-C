#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <derive-c/utils/debug.h>

#include <derive-cpp/test/trampoline.hpp>
#include <derive-cpp/test/gtest_mock.hpp>
#include <derive-cpp/fmt/remove_ptrs.hpp>

#include <derive-c/test/mock.h>
namespace {
DC_MOCKABLE(void, free_int, (int const* ptr)) { (void)ptr; }
DC_MOCKABLE(int, clone_int, (int const* self)) { return *self; }
} // namespace

#define NAME optional_int
#define ITEM int
#define ITEM_CLONE clone_int
#define ITEM_DELETE free_int
#include <derive-c/utils/option/template.h>

using namespace testing;

struct OptionTests : Test {
    FIXTURE_MOCK(OptionTests, void, free_int, (int const* ptr), ());
    FIXTURE_MOCK(OptionTests, int, clone_int, (int const* self), ());
};

TEST_F(OptionTests, EmptyNotPresent) {
    optional_int opt = optional_int_empty();
    ASSERT_FALSE(optional_int_is_present(&opt));
    ASSERT_EQ(optional_int_get(&opt), nullptr);
    ASSERT_EQ(optional_int_get_const(&opt), nullptr);

    optional_int_delete(&opt);
}

TEST_F(OptionTests, Present) {
    optional_int opt = optional_int_from(42);
    ASSERT_TRUE(optional_int_is_present(&opt));
    ASSERT_EQ(*optional_int_get(&opt), 42);
    ASSERT_EQ(*optional_int_get_const(&opt), 42);

    EXPECT_CALL(*this, free_int_mock(_));
    optional_int_delete(&opt);
}

TEST_F(OptionTests, From) {
    optional_int opt = optional_int_from(100);
    ASSERT_TRUE(optional_int_is_present(&opt));
    ASSERT_EQ(*optional_int_get(&opt), 100);
    ASSERT_EQ(*optional_int_get_const(&opt), 100);

    EXPECT_CALL(*this, free_int_mock(_));
    optional_int_delete(&opt);
}

TEST_F(OptionTests, Replace) {
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

TEST_F(OptionTests, ReplaceEmpty) {
    optional_int opt = optional_int_empty();
    ASSERT_FALSE(optional_int_is_present(&opt));

    optional_int_replace(&opt, 100);
    ASSERT_TRUE(optional_int_is_present(&opt));
    ASSERT_EQ(*optional_int_get(&opt), 100);

    EXPECT_CALL(*this, free_int_mock(_));
    optional_int_delete(&opt);
}

TEST_F(OptionTests, Clone) {
    optional_int opt_1 = optional_int_from(10);

    EXPECT_CALL(*this, clone_int_mock(_)).WillOnce(Return(10));
    optional_int opt_2 = optional_int_clone(&opt_1);

    EXPECT_CALL(*this, free_int_mock(_));
    optional_int_delete(&opt_1);

    EXPECT_CALL(*this, free_int_mock(_));
    optional_int_delete(&opt_2);
}

TEST_F(OptionTests, GetOr) {
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

TEST_F(OptionTests, DebugSome) {
    DC_SCOPED(optional_int) opt_some = optional_int_from(10);
    DC_SCOPED(dc_debug_string_builder) sb = dc_debug_string_builder_new(stdalloc_get_ref());
    optional_int_debug(&opt_some, dc_debug_fmt_new(), dc_debug_string_builder_stream(&sb));

    EXPECT_EQ("optional_int@" DC_PTR_REPLACE " { 10 }",
              derivecpp::fmt::pointer_replace(dc_debug_string_builder_string(&sb)));
    EXPECT_CALL(*this, free_int_mock(_));
}

TEST_F(OptionTests, DebugNone) {
    DC_SCOPED(optional_int) opt_none = optional_int_empty();
    DC_SCOPED(dc_debug_string_builder) sb = dc_debug_string_builder_new(stdalloc_get_ref());
    optional_int_debug(&opt_none, dc_debug_fmt_new(), dc_debug_string_builder_stream(&sb));
    EXPECT_EQ("optional_int@" DC_PTR_REPLACE " { NONE }",
              derivecpp::fmt::pointer_replace(dc_debug_string_builder_string(&sb)));
}
