
#include <gtest/gtest.h>

extern "C" {
#include <derive-c/container/bitset/static/utils.h>
#include <derive-c/utils/for.h>
}

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

extern "C" {
#define EXCLUSIVE_END_INDEX 16
#define NAME Sut
#include <derive-c/container/bitset/static/template.h>
}

TEST(BitsetStaticUtils, SutUsage) {
    Sut bitset = Sut_new();

    for (Sut_index_t i = Sut_min_index(); i <= Sut_max_index(); i++) {
        EXPECT_FALSE(Sut_get(&bitset, i));
    }

    for (Sut_index_t i = Sut_min_index(); i <= Sut_max_index(); i++) {
        Sut_set(&bitset, i, true);
    }

    for (Sut_index_t i = Sut_min_index(); i <= Sut_max_index(); i++) {
        EXPECT_TRUE(Sut_get(&bitset, i));
    }

    FOR(Sut, &bitset, iter, item) { Sut_get(&bitset, item); }

    Sut_delete(&bitset);
}

TEST(BitsetStaticUtils, IterEmpty) {
    Sut bitset = Sut_new();

    Sut_set(&bitset, 0, true);

    Sut_debug(&bitset, dc_debug_fmt_new(), stdout);

    FOR(Sut, &bitset, iter, item) {
        // std::cout << static_cast<size_t>(item) << "\n";
        Sut_get(&bitset, item);
    }

    Sut_delete(&bitset);
}