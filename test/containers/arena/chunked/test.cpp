#include <gtest/gtest.h>

#include <derive-cpp/fmt/remove_ptrs.hpp>

#include <derive-c/container/arena/chunked/utils.h>
#include <derive-c/utils/debug.h>

TEST(ArenaChunkedUtils, IndexToBlock) {
    EXPECT_EQ(DC_ARENA_CHUNKED_INDEX_TO_BLOCK(0, 1), 0);
    EXPECT_EQ(DC_ARENA_CHUNKED_INDEX_TO_BLOCK(1, 1), 0);
    EXPECT_EQ(DC_ARENA_CHUNKED_INDEX_TO_BLOCK(2, 1), 1);
    EXPECT_EQ(DC_ARENA_CHUNKED_INDEX_TO_BLOCK(3, 1), 1);

    EXPECT_EQ(DC_ARENA_CHUNKED_INDEX_TO_BLOCK(0, 3), 0);
    EXPECT_EQ(DC_ARENA_CHUNKED_INDEX_TO_BLOCK(7, 3), 0);
    EXPECT_EQ(DC_ARENA_CHUNKED_INDEX_TO_BLOCK(8, 3), 1);
}

TEST(ArenaChunkedUtils, IndexToOffset) {
    EXPECT_EQ(DC_ARENA_CHUNKED_INDEX_TO_OFFSET(0, 1), 0);
    EXPECT_EQ(DC_ARENA_CHUNKED_INDEX_TO_OFFSET(1, 1), 1);
    EXPECT_EQ(DC_ARENA_CHUNKED_INDEX_TO_OFFSET(2, 1), 0);
    EXPECT_EQ(DC_ARENA_CHUNKED_INDEX_TO_OFFSET(3, 1), 1);

    EXPECT_EQ(DC_ARENA_CHUNKED_INDEX_TO_OFFSET(0, 3), 0);
    EXPECT_EQ(DC_ARENA_CHUNKED_INDEX_TO_OFFSET(7, 3), 7);
    EXPECT_EQ(DC_ARENA_CHUNKED_INDEX_TO_OFFSET(8, 3), 0);
}

TEST(ArenaChunkedUtils, BlockSize) {
    EXPECT_EQ(DC_ARENA_CHUNKED_BLOCK_SIZE(0), 1);
    EXPECT_EQ(DC_ARENA_CHUNKED_BLOCK_SIZE(1), 2);
    EXPECT_EQ(DC_ARENA_CHUNKED_BLOCK_SIZE(2), 4);
}

TEST(ArenaChunkedUtils, BlockOffsetToIndex) {
    EXPECT_EQ(DC_ARENA_CHUNKED_BLOCK_OFFSET_TO_INDEX(0, 0, 0), 0);
    EXPECT_EQ(DC_ARENA_CHUNKED_BLOCK_OFFSET_TO_INDEX(0, 0, 31), 0);

    EXPECT_EQ(DC_ARENA_CHUNKED_BLOCK_OFFSET_TO_INDEX(0, 7, 3), 7);
    EXPECT_EQ(DC_ARENA_CHUNKED_BLOCK_OFFSET_TO_INDEX(1, 0, 3), 8);
    EXPECT_EQ(DC_ARENA_CHUNKED_BLOCK_OFFSET_TO_INDEX(1, 1, 3), 9);
}

#define NAME int_arena
#define VALUE int32_t
#define INDEX_BITS 8
#define BLOCK_INDEX_BITS 2
#include <derive-c/container/arena/chunked/template.h>

TEST(ArenaChunked, Debug) {
    DC_SCOPED(int_arena) arena = int_arena_new(stdalloc_get());

    {
        DC_SCOPED(dc_debug_string_builder) sb = dc_debug_string_builder_new(stdalloc_get());
        int_arena_debug(&arena, dc_debug_fmt_new(), dc_debug_string_builder_stream(&sb));
        EXPECT_EQ(
            // clang-format off
            "int_arena@" DC_PTR_REPLACE " {\n"
            "  count: 0,\n"
            "  free_list: 255,\n"
            "  alloc: stdalloc@" DC_PTR_REPLACE " { },\n"
            "  current_block: 0\n"
            "  block_current_exclusive_end: 0\n"
            "  blocks: [\n"
            "    block[0]: @" DC_PTR_REPLACE " [\n"
            "    ],\n"
            "  ],\n"
            "}"
            // clang-format on
            ,
            derivecpp::fmt::pointer_replace(dc_debug_string_builder_string(&sb)));
    }

    int_arena_insert(&arena, 1);
    int_arena_insert(&arena, 2);
    int_arena_insert(&arena, 3);

    {
        DC_SCOPED(dc_debug_string_builder) sb = dc_debug_string_builder_new(stdalloc_get());
        int_arena_debug(&arena, dc_debug_fmt_new(), dc_debug_string_builder_stream(&sb));
        EXPECT_EQ(
            // clang-format off
            "int_arena@" DC_PTR_REPLACE " {\n"
            "  count: 3,\n"
            "  free_list: 255,\n"
            "  alloc: stdalloc@" DC_PTR_REPLACE " { },\n"
            "  current_block: 0\n"
            "  block_current_exclusive_end: 3\n"
            "  blocks: [\n"
            "    block[0]: @" DC_PTR_REPLACE " [\n"
            "      [index=0] 1,\n"
            "      [index=1] 2,\n"
            "      [index=2] 3,\n"
            "    ],\n"
            "  ],\n"
            "}"
            // clang-format on
            ,
            derivecpp::fmt::pointer_replace(dc_debug_string_builder_string(&sb)));
    }
}
