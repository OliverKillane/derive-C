
#include <gtest/gtest.h>

#include <derive-cpp/fmt/remove_ptrs.hpp>

#include <derive-c/alloc/std.h>
#include <derive-c/core/debug/memory_tracker.h>
#include <derive-c/utils/debug.h>

#define NAME int_arena
#define VALUE size_t
#define INDEX_BITS 8
#include <derive-c/container/arena/contiguous/template.h>

TEST(ArenaTests, Full) {
    int_arena sut = int_arena_new_with_capacity_for(1, stdalloc_get_ref());
    ASSERT_FALSE(int_arena_full(&sut));
    ASSERT_EQ(int_arena_max_entries, 254);

    for (size_t i = 0; i < int_arena_max_entries; ++i) {
        int_arena_insert(&sut, i);
    }
    ASSERT_TRUE(int_arena_full(&sut));

    int_arena_delete(&sut);
}

TEST(ArenaTests, Empty) {
    int_arena sut = int_arena_new_with_capacity_for(1, stdalloc_get_ref());
    int_arena_delete(&sut);
}

TEST(ArenaTests, Single) {
    int_arena sut = int_arena_new_with_capacity_for(1, stdalloc_get_ref());
    int_arena_insert(&sut, 1);
    int_arena_delete(&sut);
}

TEST(ArenaTests, Debug) {
    DC_SCOPED(int_arena) arena = int_arena_new_with_capacity_for(10, stdalloc_get_ref());

    {
        DC_SCOPED(dc_debug_string_builder) sb = dc_debug_string_builder_new(stdalloc_get_ref());
        int_arena_debug(&arena, dc_debug_fmt_new(), dc_debug_string_builder_stream(&sb));
        EXPECT_EQ(
            // clang-format off
            "int_arena@" DC_PTR_REPLACE " {\n"
            "  capacity: 16,\n"
            "  count: 0,\n"
            "  slots: " DC_PTR_REPLACE ",\n"
            "  alloc: stdalloc@" DC_PTR_REPLACE " { },\n"
            "  items: [  ],\n"
            "}"
            // clang-format on
            ,
            derivecpp::fmt::pointer_replace(dc_debug_string_builder_string(&sb)));
    }

    int_arena_index_t index1 = int_arena_insert(&arena, 1);
    int_arena_index_t index2 = int_arena_insert(&arena, 2);
    int_arena_index_t index3 = int_arena_insert(&arena, 3);

    {
        DC_SCOPED(dc_debug_string_builder) sb = dc_debug_string_builder_new(stdalloc_get_ref());
        int_arena_debug(&arena, dc_debug_fmt_new(), dc_debug_string_builder_stream(&sb));
        EXPECT_EQ(
            // clang-format off
            "int_arena@" DC_PTR_REPLACE " {\n"
            "  capacity: 16,\n"
            "  count: 3,\n"
            "  slots: " DC_PTR_REPLACE ",\n"
            "  alloc: stdalloc@" DC_PTR_REPLACE " { },\n"
            "  items: [    {\n"
            "      key: int_arena_index_t { 0 },\n"
            "      value: 1,\n"
            "    },\n"
            "    {\n"
            "      key: int_arena_index_t { 1 },\n"
            "      value: 2,\n"
            "    },\n"
            "    {\n"
            "      key: int_arena_index_t { 2 },\n"
            "      value: 3,\n"
            "    },\n"
            "  ],\n"
            "}"
            // clang-format on
            ,
            derivecpp::fmt::pointer_replace(dc_debug_string_builder_string(&sb)));
    }

    int_arena_remove(&arena, index1);
    int_arena_remove(&arena, index2);
    int_arena_remove(&arena, index3);

    {
        DC_SCOPED(dc_debug_string_builder) sb = dc_debug_string_builder_new(stdalloc_get_ref());
        int_arena_debug(&arena, dc_debug_fmt_new(), dc_debug_string_builder_stream(&sb));
        EXPECT_EQ(
            // clang-format off
            "int_arena@" DC_PTR_REPLACE " {\n"
            "  capacity: 16,\n"
            "  count: 0,\n"
            "  slots: " DC_PTR_REPLACE ",\n"
            "  alloc: stdalloc@" DC_PTR_REPLACE " { },\n"
            "  items: [  ],\n"
            "}"
            // clang-format on
            ,
            derivecpp::fmt::pointer_replace(dc_debug_string_builder_string(&sb)));
    }
}
