
#include <gtest/gtest.h>

#include <derive-cpp/fmt/remove_ptrs.hpp>

#include <derive-c/utils/for.h>
#include <derive-c/utils/debug.h>

#define CAPACITY 256
#define KEY size_t
#define VALUE size_t
#define NAME int_map
#include <derive-c/container/map/staticlinear/template.h>

TEST(StaticLinearMap, InsertAndRead) {
    DC_SCOPED(int_map) map = int_map_new();

    for (size_t i = 0; i < int_map_capacity(); i++) {
        size_t* placed = int_map_insert(&map, i, i * 10);
        ASSERT_NE(placed, nullptr);
        ASSERT_EQ(*placed, i * 10);
    }

    FOR(int_map, &map, iter, item) {
        size_t key = *item.key;
        size_t value = *item.value;
        ASSERT_EQ(value, key * 10);
    }
}

#define CAPACITY 256
#define KEY size_t
#define VALUE const char*
#define NAME test_map
#include <derive-c/container/map/staticlinear/template.h>

TEST(StaticLinearMap, Debug) {
    DC_SCOPED(test_map) map = test_map_new();

    {
        DC_SCOPED(dc_debug_string_builder) sb = dc_debug_string_builder_new(stdalloc_get());
        test_map_debug(&map, dc_debug_fmt_new(), dc_debug_string_builder_stream(&sb));

        EXPECT_EQ(
            // clang-format off
            "test_map@" DC_PTR_REPLACE " {\n"
            "  capacity: 256,\n"
            "  entries: [\n"
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
        DC_SCOPED(dc_debug_string_builder) sb = dc_debug_string_builder_new(stdalloc_get());
        test_map_debug(&map, dc_debug_fmt_new(), dc_debug_string_builder_stream(&sb));

        EXPECT_EQ(
            // clang-format off
            "test_map@" DC_PTR_REPLACE " {\n"
            "  capacity: 256,\n"
            "  entries: [\n"
            "    {index: 0, key: 3, value: char*@" DC_PTR_REPLACE " \"foo\"},\n"
            "    {index: 1, key: 4, value: char*@" DC_PTR_REPLACE " \"bar\"},\n"
            "    {index: 2, key: 5, value: char*@" DC_PTR_REPLACE " \"bing\"},\n"
            "    {index: 3, key: 6, value: char*@" DC_PTR_REPLACE " \"zing\"},\n"
            "  ],\n"
            "}"
            // clang-format on
            ,
            derivecpp::fmt::pointer_replace(dc_debug_string_builder_string(&sb)));
    }
}
