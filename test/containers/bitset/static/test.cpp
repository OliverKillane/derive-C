
#include <gtest/gtest.h>

#include <derive-cpp/fmt/remove_ptrs.hpp>

#include <derive-c/container/bitset/static/utils.h>
#include <derive-c/utils/for.h>
#include <derive-c/utils/debug.h>

TEST(BitsetStaticUtils, OffsetToMask) {
    EXPECT_EQ(0x01, DC_BITSET_STATIC_OFFSET_TO_MASK((uint8_t)0));
    EXPECT_EQ(0x02, DC_BITSET_STATIC_OFFSET_TO_MASK((uint8_t)1));
    EXPECT_EQ(0x04, DC_BITSET_STATIC_OFFSET_TO_MASK((uint8_t)2));
    EXPECT_EQ(0x08, DC_BITSET_STATIC_OFFSET_TO_MASK((uint8_t)3));
    EXPECT_EQ(0x10, DC_BITSET_STATIC_OFFSET_TO_MASK((uint8_t)4));
    EXPECT_EQ(0x20, DC_BITSET_STATIC_OFFSET_TO_MASK((uint8_t)5));
    EXPECT_EQ(0x40, DC_BITSET_STATIC_OFFSET_TO_MASK((uint8_t)6));
    EXPECT_EQ(0x80, DC_BITSET_STATIC_OFFSET_TO_MASK((uint8_t)7));
}

TEST(BitsetStaticUtils, CapacityToBytes) {
    EXPECT_EQ(0, DC_BITSET_STATIC_CAPACITY_TO_BYTES((uint8_t)0));
    EXPECT_EQ(1, DC_BITSET_STATIC_CAPACITY_TO_BYTES((uint8_t)1));
    EXPECT_EQ(1, DC_BITSET_STATIC_CAPACITY_TO_BYTES((uint8_t)7));
    EXPECT_EQ(1, DC_BITSET_STATIC_CAPACITY_TO_BYTES((uint8_t)8));
    EXPECT_EQ(2, DC_BITSET_STATIC_CAPACITY_TO_BYTES((uint8_t)9));
    EXPECT_EQ(2, DC_BITSET_STATIC_CAPACITY_TO_BYTES((uint8_t)15));
    EXPECT_EQ(2, DC_BITSET_STATIC_CAPACITY_TO_BYTES((uint8_t)16));
    EXPECT_EQ(3, DC_BITSET_STATIC_CAPACITY_TO_BYTES((uint8_t)17));
}

TEST(BitsetStaticUtils, IndexToBytesAndOffset) {
    EXPECT_EQ(0, DC_BITSET_STATIC_INDEX_TO_BYTES((uint8_t)0));
    EXPECT_EQ(0, DC_BITSET_STATIC_INDEX_TO_OFFSET((uint8_t)0));

    EXPECT_EQ(0, DC_BITSET_STATIC_INDEX_TO_BYTES((uint8_t)7));
    EXPECT_EQ(7, DC_BITSET_STATIC_INDEX_TO_OFFSET((uint8_t)7));

    EXPECT_EQ(1, DC_BITSET_STATIC_INDEX_TO_BYTES((uint8_t)8));
    EXPECT_EQ(0, DC_BITSET_STATIC_INDEX_TO_OFFSET((uint8_t)8));

    EXPECT_EQ(1, DC_BITSET_STATIC_INDEX_TO_BYTES((uint8_t)15));
    EXPECT_EQ(7, DC_BITSET_STATIC_INDEX_TO_OFFSET((uint8_t)15));

    EXPECT_EQ(2, DC_BITSET_STATIC_INDEX_TO_BYTES((uint8_t)16));
    EXPECT_EQ(0, DC_BITSET_STATIC_INDEX_TO_OFFSET((uint8_t)16));

    EXPECT_EQ(2, DC_BITSET_STATIC_INDEX_TO_BYTES((uint8_t)23));
    EXPECT_EQ(7, DC_BITSET_STATIC_INDEX_TO_OFFSET((uint8_t)23));
}

#define EXCLUSIVE_END_INDEX 16
#define NAME sut
#include <derive-c/container/bitset/static/template.h>

TEST(BitsetStatic, SutUsage) {
    DC_SCOPED(sut) bitset = sut_new();

    for (sut_index_t i = sut_min_index(); i <= sut_max_index(); i++) {
        EXPECT_FALSE(sut_get(&bitset, i));
    }

    for (sut_index_t i = sut_min_index(); i <= sut_max_index(); i++) {
        sut_set(&bitset, i, true);
    }

    for (sut_index_t i = sut_min_index(); i <= sut_max_index(); i++) {
        EXPECT_TRUE(sut_get(&bitset, i));
    }

    FOR(sut, &bitset, iter, item) { sut_get(&bitset, item); }
}

TEST(BitsetStatic, Debug) {
    DC_SCOPED(sut) bitset = sut_new();

    {
        DC_SCOPED(dc_debug_string_builder) sb = dc_debug_string_builder_new(stdalloc_get());
        sut_debug(&bitset, dc_debug_fmt_new(), dc_debug_string_builder_stream(&sb));
        EXPECT_EQ(
            // clang-format off
            "sut@" DC_PTR_REPLACE " {\n"
            "  blocks: [\n"
            "  ],\n"
            "}"
            // clang-format on
            ,
            derivecpp::fmt::pointer_replace(dc_debug_string_builder_string(&sb)));
    }

    sut_set(&bitset, 0, true);
    sut_set(&bitset, 3, true);
    sut_set(&bitset, 4, true);

    {
        DC_SCOPED(dc_debug_string_builder) sb = dc_debug_string_builder_new(stdalloc_get());
        sut_debug(&bitset, dc_debug_fmt_new(), dc_debug_string_builder_stream(&sb));
        EXPECT_EQ(
            // clang-format off
            "sut@" DC_PTR_REPLACE " {\n"
            "  blocks: [\n"
            "    { byte: 0, offset: 0, index: 0},\n"
            "    { byte: 0, offset: 3, index: 3},\n"
            "    { byte: 0, offset: 4, index: 4},\n"
            "  ],\n"
            "}"
            // clang-format on
            ,
            derivecpp::fmt::pointer_replace(dc_debug_string_builder_string(&sb)));
    }
}
