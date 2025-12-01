#include <derive-cpp/test/gtest_panic.hpp>
#include <gtest/gtest.h>

extern "C" {
#include <derive-c/container/bitset/static/utils.h>
}

TEST(BitsetStaticUtils, OffsetToMask) {
    EXPECT_EQ(0x01, OFFSET_TO_MASK((uint8_t)0));
    EXPECT_EQ(0x02, OFFSET_TO_MASK((uint8_t)1));
    EXPECT_EQ(0x04, OFFSET_TO_MASK((uint8_t)2));
    EXPECT_EQ(0x08, OFFSET_TO_MASK((uint8_t)3));
    EXPECT_EQ(0x10, OFFSET_TO_MASK((uint8_t)4));
    EXPECT_EQ(0x20, OFFSET_TO_MASK((uint8_t)5));
    EXPECT_EQ(0x40, OFFSET_TO_MASK((uint8_t)6));
    EXPECT_EQ(0x80, OFFSET_TO_MASK((uint8_t)7));
}

TEST(BitsetStaticUtils, CapacityToBytes) {
    EXPECT_EQ(1, CAPACITY_TO_BYTES((uint8_t)0));
    EXPECT_EQ(1, CAPACITY_TO_BYTES((uint8_t)1));
    EXPECT_EQ(1, CAPACITY_TO_BYTES((uint8_t)7));
    EXPECT_EQ(1, CAPACITY_TO_BYTES((uint8_t)8));
    EXPECT_EQ(2, CAPACITY_TO_BYTES((uint8_t)9));
    EXPECT_EQ(2, CAPACITY_TO_BYTES((uint8_t)15));
    EXPECT_EQ(2, CAPACITY_TO_BYTES((uint8_t)16));
    EXPECT_EQ(3, CAPACITY_TO_BYTES((uint8_t)17));
}

TEST(BitsetStaticUtils, IndexToBytesAndOffset) {
    EXPECT_EQ(0, INDEX_TO_BYTES((uint8_t)0));
    EXPECT_EQ(0, INDEX_TO_OFFSET((uint8_t)0));

    EXPECT_EQ(0, INDEX_TO_BYTES((uint8_t)7));
    EXPECT_EQ(7, INDEX_TO_OFFSET((uint8_t)7));

    EXPECT_EQ(1, INDEX_TO_BYTES((uint8_t)8));
    EXPECT_EQ(0, INDEX_TO_OFFSET((uint8_t)8));

    EXPECT_EQ(1, INDEX_TO_BYTES((uint8_t)15));
    EXPECT_EQ(7, INDEX_TO_OFFSET((uint8_t)15));

    EXPECT_EQ(2, INDEX_TO_BYTES((uint8_t)16));
    EXPECT_EQ(0, INDEX_TO_OFFSET((uint8_t)16));

    EXPECT_EQ(3, INDEX_TO_BYTES((uint8_t)23));
    EXPECT_EQ(7, INDEX_TO_OFFSET((uint8_t)23));
}
