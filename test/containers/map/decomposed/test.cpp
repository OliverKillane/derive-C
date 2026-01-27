#include <gtest/gtest.h>

#include <derive-cpp/fmt/remove_ptrs.hpp>

#include <derive-c/algorithm/hash/default.h>
#include <derive-c/utils/debug/string.h>

#define KEY int32_t
#define KEY_HASH DC_DEFAULT_HASH
#define VALUE const char*
#define NAME test_map
#include <derive-c/container/map/decomposed/template.h>

TEST(DecomposedTest, Debug) {
    DC_SCOPED(test_map) map = test_map_new(stdalloc_get_ref());

    {
        DC_SCOPED(dc_debug_string_builder) sb = dc_debug_string_builder_new(stdalloc_get_ref());
        test_map_debug(&map, dc_debug_fmt_new(), dc_debug_string_builder_stream(&sb));

        EXPECT_EQ(
            // clang-format off
            "test_map@" DC_PTR_REPLACE " {\n"
            "  capacity: 64,\n"
            "  size: 0,\n"
            "  keys: @" DC_PTR_REPLACE ",\n"
            "  values: @" DC_PTR_REPLACE ",\n"
            "  alloc: stdalloc@" DC_PTR_REPLACE " { },\n"
            "  items: [\n"
            "  ],\n"
            "}"
            // clang-format on
            ,
            derivecpp::fmt::pointer_replace(dc_debug_string_builder_string(&sb)));
    }

    test_map_insert(&map, 3, "foo");
    test_map_insert(&map, 4, "bar");
    test_map_insert(&map, 5, "bing");
    test_map_insert(&map, 6, "zing");

    {
        DC_SCOPED(dc_debug_string_builder) sb = dc_debug_string_builder_new(stdalloc_get_ref());
        test_map_debug(&map, dc_debug_fmt_new(), dc_debug_string_builder_stream(&sb));

        EXPECT_EQ(
            // clang-format off
            "test_map@" DC_PTR_REPLACE " {\n"
            "  capacity: 64,\n"
            "  size: 4,\n"
            "  keys: @" DC_PTR_REPLACE ",\n"
            "  values: @" DC_PTR_REPLACE ",\n"
            "  alloc: stdalloc@" DC_PTR_REPLACE " { },\n"
            "  items: [\n"
            "    {\n"
            "      key: 3,\n"
            "      value: char*@" DC_PTR_REPLACE " \"foo\",\n"
            "    },\n"
            "    {\n"
            "      key: 4,\n"
            "      value: char*@" DC_PTR_REPLACE " \"bar\",\n"
            "    },\n"
            "    {\n"
            "      key: 5,\n"
            "      value: char*@" DC_PTR_REPLACE " \"bing\",\n"
            "    },\n"
            "    {\n"
            "      key: 6,\n"
            "      value: char*@" DC_PTR_REPLACE " \"zing\",\n"
            "    },\n"
            "  ],\n"
            "}"
            // clang-format on
            ,
            derivecpp::fmt::pointer_replace(dc_debug_string_builder_string(&sb)));
    }
}
