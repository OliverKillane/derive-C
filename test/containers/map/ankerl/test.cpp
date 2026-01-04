#include <gtest/gtest.h>

#include <derive-cpp/fmt/remove_ptrs.hpp>

#include <derive-c/algorithm/hash/default.h>
#include <derive-c/utils/debug.h>

#define KEY int32_t
#define KEY_HASH DC_DEFAULT_HASH
#define VALUE const char*
#define NAME test_map
#include <derive-c/container/map/ankerl/template.h>

TEST(AnkerlTest, Debug) {
    char const* a = "lol";
    {

        DC_SCOPED(dc_debug_string_builder) sb = dc_debug_string_builder_new(stdalloc_get());
        DC_DEFAULT_DEBUG(&a, dc_debug_fmt_new(), dc_debug_string_builder_stream(&sb));
        std::cout << dc_debug_string_builder_string(&sb) << std::endl;
    }

    DC_SCOPED(test_map) map = test_map_new(stdalloc_get());

    {
        DC_SCOPED(dc_debug_string_builder) sb = dc_debug_string_builder_new(stdalloc_get());
        test_map_debug(&map, dc_debug_fmt_new(), dc_debug_string_builder_stream(&sb));

        EXPECT_EQ(
            // clang-format off
            "test_map@" DC_PTR_REPLACE " {\n"
            "  bucket capacity: 256,\n"
            "  alloc: stdalloc@" DC_PTR_REPLACE " { },\n"
            "  slots: test_map_item_vectors@" DC_PTR_REPLACE " {\n"
            "    size: 0,\n"
            "    capacity: 256,\n"
            "    alloc: stdalloc@" DC_PTR_REPLACE " { },\n"
            "    items: @" DC_PTR_REPLACE " [\n"
            "    ],\n"
            "  },\n"
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
            "  bucket capacity: 256,\n"
            "  alloc: stdalloc@" DC_PTR_REPLACE " { },\n"
            "  slots: test_map_item_vectors@" DC_PTR_REPLACE " {\n"
            "    size: 4,\n"
            "    capacity: 256,\n"
            "    alloc: stdalloc@" DC_PTR_REPLACE " { },\n"
            "    items: @" DC_PTR_REPLACE " [\n"
            "      test_map_slot_t@" DC_PTR_REPLACE " {\n"
            "        key: 3,\n"
            "        value: char*@" DC_PTR_REPLACE " \"foo\",\n"
            "      },\n"
            "      test_map_slot_t@" DC_PTR_REPLACE " {\n"
            "        key: 4,\n"
            "        value: char*@" DC_PTR_REPLACE " \"bar\",\n"
            "      },\n"
            "      test_map_slot_t@" DC_PTR_REPLACE " {\n"
            "        key: 5,\n"
            "        value: char*@" DC_PTR_REPLACE " \"bing\",\n"
            "      },\n"
            "      test_map_slot_t@" DC_PTR_REPLACE " {\n"
            "        key: 6,\n"
            "        value: char*@" DC_PTR_REPLACE " \"zing\",\n"
            "      },\n"
            "    ],\n"
            "  },\n"
            "}"
            // clang-format on
            ,
            derivecpp::fmt::pointer_replace(dc_debug_string_builder_string(&sb)));
    }
}