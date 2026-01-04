
#include <gtest/gtest.h>

#include <derive-c/utils/debug.h>

#include <derive-cpp/fmt/remove_ptrs.hpp>

#define NAME test_vec
#define INPLACE_CAPACITY 16
#define ITEM char const*
#include <derive-c/container/vector/static/template.h>

TEST(VectorTests, Debug) {
    DC_SCOPED(test_vec) v = test_vec_new();

    {
        DC_SCOPED(dc_debug_string_builder) sb = dc_debug_string_builder_new(stdalloc_get_ref());
        test_vec_debug(&v, dc_debug_fmt_new(), dc_debug_string_builder_stream(&sb));

        EXPECT_EQ(
            // clang-format off
            "test_vec@" DC_PTR_REPLACE " {\n"
            "  capacity: 16,\n"
            "  size: 0,\n"
            "  items: @" DC_PTR_REPLACE " [\n"
            "  ],\n"
            "}"
            // clang-format on
            ,
            derivecpp::fmt::pointer_replace(dc_debug_string_builder_string(&sb)));
    }

    test_vec_push(&v, "foo");
    test_vec_push(&v, "bar");
    test_vec_push(&v, "bing");

    {
        DC_SCOPED(dc_debug_string_builder) sb = dc_debug_string_builder_new(stdalloc_get_ref());
        test_vec_debug(&v, dc_debug_fmt_new(), dc_debug_string_builder_stream(&sb));

        EXPECT_EQ(
            // clang-format off
            "test_vec@" DC_PTR_REPLACE " {\n"
            "  capacity: 16,\n"
            "  size: 3,\n"
            "  items: @" DC_PTR_REPLACE " [\n"
            "    char*@" DC_PTR_REPLACE " \"foo\",\n"
            "    char*@" DC_PTR_REPLACE " \"bar\",\n"
            "    char*@" DC_PTR_REPLACE " \"bing\",\n"
            "  ],\n"
            "}"
            // clang-format on
            ,
            derivecpp::fmt::pointer_replace(dc_debug_string_builder_string(&sb)));
    }
}
