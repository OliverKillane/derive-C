#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <stdexcept>

#include <derive-c/utils/debug.h>

#include <derive-cpp/test/trampoline.hpp>
#include <derive-cpp/test/gtest_mock.hpp>
#include <derive-cpp/fmt/remove_ptrs.hpp>

#include <derive-c/test/mock.h>
namespace {
DC_MOCKABLE(void, free_char, (char const* ptr)) { (void)ptr; }
DC_MOCKABLE(void, free_i32, (int32_t const* ptr)) { (void)ptr; }
DC_MOCKABLE(void, debug_char, (char const* self, dc_debug_fmt fmt, FILE* stream)) {
    (void)fmt;
    fprintf(stream, "%c", *self);
}
DC_MOCKABLE(void, debug_i32, (int32_t const* self, dc_debug_fmt fmt, FILE* stream)) {
    (void)fmt;
    fprintf(stream, "%" PRId32, *self);
}
DC_MOCKABLE(void, throw_i32, (int32_t const* error)) { (void)error; }
} // namespace

#define NAME result_char_i32
#define OK char
#define OK_DELETE free_char
#define OK_DEBUG debug_char
#define ERROR int32_t
#define ERROR_DELETE free_i32
#define ERROR_DEBUG debug_i32
#define ERROR_THROW throw_i32
#include <derive-c/utils/result/template.h>

using namespace testing;

struct ResultTests : Test {
    FIXTURE_MOCK(ResultTests, void, free_char, (char const* ptr), ());
    FIXTURE_MOCK(ResultTests, void, free_i32, (int32_t const* ptr), ());
    FIXTURE_MOCK(ResultTests, void, debug_char, (char const* self, dc_debug_fmt fmt, FILE* stream),
                 ());
    FIXTURE_MOCK(ResultTests, void, debug_i32,
                 (int32_t const* self, dc_debug_fmt fmt, FILE* stream), ());
    FIXTURE_MOCK(ResultTests, void, throw_i32, (int32_t const* error), ());
};

TEST_F(ResultTests, FromOk) {
    result_char_i32 res = result_char_i32_from_ok('a');
    ASSERT_FALSE(result_char_i32_is_error(&res));
    ASSERT_EQ(*result_char_i32_get_okay(&res), 'a');
    ASSERT_EQ(result_char_i32_get_error(&res), nullptr);
    ASSERT_EQ(*result_char_i32_strict_get_const(&res), 'a');

    EXPECT_CALL(*this, free_char_mock(_));
    result_char_i32_delete(&res);
}

TEST_F(ResultTests, FromError) {
    result_char_i32 res = result_char_i32_from_error(42);
    ASSERT_TRUE(result_char_i32_is_error(&res));
    ASSERT_EQ(result_char_i32_get_okay(&res), nullptr);
    ASSERT_EQ(*result_char_i32_get_error(&res), 42);

    EXPECT_CALL(*this, throw_i32_mock(_));
    EXPECT_THROW((void)result_char_i32_strict_get_const(&res), std::runtime_error);

    EXPECT_CALL(*this, free_i32_mock(_));
    result_char_i32_delete(&res);
}

TEST_F(ResultTests, DebugOk) {
    DC_SCOPED(result_char_i32) res = result_char_i32_from_ok('b');
    DC_SCOPED(dc_debug_string_builder) sb = dc_debug_string_builder_new(stdalloc_get_ref());

    EXPECT_CALL(*this, debug_char_mock(_, _, _)).WillOnce(Invoke(DC_MOCKABLE_REAL(debug_char)));
    result_char_i32_debug(&res, dc_debug_fmt_new(), dc_debug_string_builder_stream(&sb));

    EXPECT_EQ("result_char_i32@" DC_PTR_REPLACE " {\n  ok: b,\n}",
              derivecpp::fmt::pointer_replace(dc_debug_string_builder_string(&sb)));
    EXPECT_CALL(*this, free_char_mock(_));
}

TEST_F(ResultTests, DebugError) {
    DC_SCOPED(result_char_i32) res = result_char_i32_from_error(-7);
    DC_SCOPED(dc_debug_string_builder) sb = dc_debug_string_builder_new(stdalloc_get_ref());

    EXPECT_CALL(*this, debug_i32_mock(_, _, _)).WillOnce(Invoke(DC_MOCKABLE_REAL(debug_i32)));
    result_char_i32_debug(&res, dc_debug_fmt_new(), dc_debug_string_builder_stream(&sb));

    EXPECT_EQ("result_char_i32@" DC_PTR_REPLACE " {\n  error: -7,\n}",
              derivecpp::fmt::pointer_replace(dc_debug_string_builder_string(&sb)));
    EXPECT_CALL(*this, free_i32_mock(_));
}
