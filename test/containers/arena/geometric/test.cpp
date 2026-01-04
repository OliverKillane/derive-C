
#include <gtest/gtest.h>

#include <derive-cpp/fmt/remove_ptrs.hpp>

#include <derive-c/container/arena/geometric/utils.h>
#include <derive-c/utils/debug.h>

TEST(ArenaGeometricUtils, MostSignificantBit) {
    EXPECT_EQ(0, DC_MATH_MSB_INDEX((uint8_t)0));
    EXPECT_EQ(0, DC_MATH_MSB_INDEX((uint8_t)1));
    EXPECT_EQ(1, DC_MATH_MSB_INDEX((uint8_t)2));
    EXPECT_EQ(2, DC_MATH_MSB_INDEX((uint8_t)4));
    EXPECT_EQ(3, DC_MATH_MSB_INDEX((uint8_t)8));
    EXPECT_EQ(7, DC_MATH_MSB_INDEX((uint8_t)128));
}

TEST(ArenaGeometricUtils, IndexToBlock) {
    EXPECT_EQ(0, DC_ARENA_GEO_INDEX_TO_BLOCK((uint8_t)0, 3));
    EXPECT_EQ(0, DC_ARENA_GEO_INDEX_TO_BLOCK((uint8_t)7, 3));
    EXPECT_EQ(1, DC_ARENA_GEO_INDEX_TO_BLOCK((uint8_t)8, 3));
    EXPECT_EQ(1, DC_ARENA_GEO_INDEX_TO_BLOCK((uint8_t)15, 3));
    EXPECT_EQ(2, DC_ARENA_GEO_INDEX_TO_BLOCK((uint8_t)16, 3));
    EXPECT_EQ(2, DC_ARENA_GEO_INDEX_TO_BLOCK((uint8_t)31, 3));
    EXPECT_EQ(3, DC_ARENA_GEO_INDEX_TO_BLOCK((uint8_t)32, 3));
    EXPECT_EQ(3, DC_ARENA_GEO_INDEX_TO_BLOCK((uint8_t)63, 3));
    EXPECT_EQ(4, DC_ARENA_GEO_INDEX_TO_BLOCK((uint8_t)64, 3));
    EXPECT_EQ(4, DC_ARENA_GEO_INDEX_TO_BLOCK((uint8_t)127, 3));
    EXPECT_EQ(5, DC_ARENA_GEO_INDEX_TO_BLOCK((uint8_t)128, 3));
    EXPECT_EQ(5, DC_ARENA_GEO_INDEX_TO_BLOCK((uint8_t)255, 3));
}

TEST(ArenaGeometricUtils, BlockToSize) {
    EXPECT_EQ(8ULL, DC_ARENA_GEO_BLOCK_TO_SIZE(0, 3));
    EXPECT_EQ(8ULL, DC_ARENA_GEO_BLOCK_TO_SIZE(1, 3));
    EXPECT_EQ(16ULL, DC_ARENA_GEO_BLOCK_TO_SIZE(2, 3));
    EXPECT_EQ(32ULL, DC_ARENA_GEO_BLOCK_TO_SIZE(3, 3));
    EXPECT_EQ(64ULL, DC_ARENA_GEO_BLOCK_TO_SIZE(4, 3));
    EXPECT_EQ(128ULL, DC_ARENA_GEO_BLOCK_TO_SIZE(5, 3));
}

TEST(ArenaGeometricUtils, MaxBlocks) {
    EXPECT_EQ(6, DC_ARENA_GEO_MAX_NUM_BLOCKS(8, 3));
    EXPECT_EQ(14, DC_ARENA_GEO_MAX_NUM_BLOCKS(16, 3));
    EXPECT_EQ(30, DC_ARENA_GEO_MAX_NUM_BLOCKS(32, 3));
    EXPECT_EQ(62, DC_ARENA_GEO_MAX_NUM_BLOCKS(64, 3));
}

TEST(ArenaGeometricUtils, BlockOffsetToIndex) {
    EXPECT_EQ(0, DC_ARENA_GEO_BLOCK_OFFSET_TO_INDEX(0, 0, 3));
    EXPECT_EQ(7, DC_ARENA_GEO_BLOCK_OFFSET_TO_INDEX(0, 7, 3));

    EXPECT_EQ(8, DC_ARENA_GEO_BLOCK_OFFSET_TO_INDEX(1, 0, 3));
    EXPECT_EQ(15, DC_ARENA_GEO_BLOCK_OFFSET_TO_INDEX(1, 7, 3));

    EXPECT_EQ(16, DC_ARENA_GEO_BLOCK_OFFSET_TO_INDEX(2, 0, 3));
    EXPECT_EQ(31, DC_ARENA_GEO_BLOCK_OFFSET_TO_INDEX(2, 15, 3));

    EXPECT_EQ(32, DC_ARENA_GEO_BLOCK_OFFSET_TO_INDEX(3, 0, 3));
    EXPECT_EQ(63, DC_ARENA_GEO_BLOCK_OFFSET_TO_INDEX(3, 31, 3));

    EXPECT_EQ(64, DC_ARENA_GEO_BLOCK_OFFSET_TO_INDEX(4, 0, 3));
    EXPECT_EQ(127, DC_ARENA_GEO_BLOCK_OFFSET_TO_INDEX(4, 63, 3));

    EXPECT_EQ(128, DC_ARENA_GEO_BLOCK_OFFSET_TO_INDEX(5, 0, 3));
    EXPECT_EQ(255, DC_ARENA_GEO_BLOCK_OFFSET_TO_INDEX(5, 127, 3));
}

TEST(ArenaGeometricUtils, IndexToOffset) {
    EXPECT_EQ(0, DC_ARENA_GEO_INDEX_TO_OFFSET(0, 0, 3));
    EXPECT_EQ(1, DC_ARENA_GEO_INDEX_TO_OFFSET(1, 0, 3));
    EXPECT_EQ(7, DC_ARENA_GEO_INDEX_TO_OFFSET(7, 0, 3));

    EXPECT_EQ(0, DC_ARENA_GEO_INDEX_TO_OFFSET(8, 1, 3));
    EXPECT_EQ(1, DC_ARENA_GEO_INDEX_TO_OFFSET(9, 1, 3));
    EXPECT_EQ(7, DC_ARENA_GEO_INDEX_TO_OFFSET(15, 1, 3));

    EXPECT_EQ(0, DC_ARENA_GEO_INDEX_TO_OFFSET(16, 2, 3));
    EXPECT_EQ(1, DC_ARENA_GEO_INDEX_TO_OFFSET(17, 2, 3));
    EXPECT_EQ(15, DC_ARENA_GEO_INDEX_TO_OFFSET(31, 2, 3));
}

#define VALUE int32_t
#define INITIAL_BLOCK_INDEX_BITS 3
#define INDEX_BITS 8
#define NAME int_arena
#include <derive-c/container/arena/geometric/template.h>

TEST(GeometricArena, Debug) {
    DC_SCOPED(int_arena) arena = int_arena_new(stdalloc_get_ref());

    {
        DC_SCOPED(dc_debug_string_builder) sb = dc_debug_string_builder_new(stdalloc_get_ref());
        int_arena_debug(&arena, dc_debug_fmt_new(), dc_debug_string_builder_stream(&sb));
        EXPECT_EQ(
            // clang-format off
            "int_arena@" DC_PTR_REPLACE " {\n"
            "  count: 0,\n"
            "  free_list: INDEX_NONE,\n"
            "  alloc: stdalloc@" DC_PTR_REPLACE " { },\n"
            "  blocks: [\n"
            "    {\n"
            "      block_index: 0,\n"
            "      block_ptr: " DC_PTR_REPLACE ",\n"
            "      capacity: 8,\n"
            "      size: 0,\n"
            "      slots: [\n"
            "      ],\n"
            "    },\n"
            "  ],\n"
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
            "  count: 3,\n"
            "  free_list: INDEX_NONE,\n"
            "  alloc: stdalloc@" DC_PTR_REPLACE " { },\n"
            "  blocks: [\n"
            "    {\n"
            "      block_index: 0,\n"
            "      block_ptr: " DC_PTR_REPLACE ",\n"
            "      capacity: 8,\n"
            "      size: 3,\n"
            "      slots: [\n"
            "        {\n"
            "          present: true,\n"
            "          value: 1,\n"
            "        },\n"
            "        {\n"
            "          present: true,\n"
            "          value: 2,\n"
            "        },\n"
            "        {\n"
            "          present: true,\n"
            "          value: 3,\n"
            "        },\n"
            "      ],\n"
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
            ""
            // clang-format on
            ,
            derivecpp::fmt::pointer_replace(dc_debug_string_builder_string(&sb)));
    }
}