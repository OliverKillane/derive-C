#include <derive-cpp/test/gtest_panic.hpp>
#include <gtest/gtest.h>

extern "C" {
#include <derive-c/container/arena/geometric/utils.h>
}

TEST(ArenaGeometricUtils, MostSignificantBit) {
    EXPECT_EQ(0, MSB_INDEX((uint8_t)0));
    EXPECT_EQ(0, MSB_INDEX((uint8_t)1));
    EXPECT_EQ(1, MSB_INDEX((uint8_t)2));
    EXPECT_EQ(2, MSB_INDEX((uint8_t)4));
    EXPECT_EQ(3, MSB_INDEX((uint8_t)8));
    EXPECT_EQ(7, MSB_INDEX((uint8_t)128));
}

TEST(ArenaGeometricUtils, IndexToBlock) {
    EXPECT_EQ(0, INDEX_TO_BLOCK((uint8_t)0, 3));
    EXPECT_EQ(0, INDEX_TO_BLOCK((uint8_t)7, 3));
    EXPECT_EQ(1, INDEX_TO_BLOCK((uint8_t)8, 3));
    EXPECT_EQ(1, INDEX_TO_BLOCK((uint8_t)15, 3));
    EXPECT_EQ(2, INDEX_TO_BLOCK((uint8_t)16, 3));
    EXPECT_EQ(2, INDEX_TO_BLOCK((uint8_t)31, 3));
    EXPECT_EQ(3, INDEX_TO_BLOCK((uint8_t)32, 3));
    EXPECT_EQ(3, INDEX_TO_BLOCK((uint8_t)63, 3));
    EXPECT_EQ(4, INDEX_TO_BLOCK((uint8_t)64, 3));
    EXPECT_EQ(4, INDEX_TO_BLOCK((uint8_t)127, 3));
    EXPECT_EQ(5, INDEX_TO_BLOCK((uint8_t)128, 3));
    EXPECT_EQ(5, INDEX_TO_BLOCK((uint8_t)255, 3));
}

TEST(ArenaGeometricUtils, BlockToSize) {
    EXPECT_EQ(8ULL, BLOCK_TO_SIZE(0, 3));
    EXPECT_EQ(8ULL, BLOCK_TO_SIZE(1, 3));
    EXPECT_EQ(16ULL, BLOCK_TO_SIZE(2, 3));
    EXPECT_EQ(32ULL, BLOCK_TO_SIZE(3, 3));
    EXPECT_EQ(64ULL, BLOCK_TO_SIZE(4, 3));
    EXPECT_EQ(128ULL, BLOCK_TO_SIZE(5, 3));
}

TEST(ArenaGeometricUtils, MaxBlocks) {
    EXPECT_EQ(6, MAX_NUM_BLOCKS(8, 3));
    EXPECT_EQ(14, MAX_NUM_BLOCKS(16, 3));
    EXPECT_EQ(30, MAX_NUM_BLOCKS(32, 3));
    EXPECT_EQ(62, MAX_NUM_BLOCKS(64, 3));
}

TEST(ArenaGeometricUtils, BlockOffsetToIndex) {
    EXPECT_EQ(0, BLOCK_OFFSET_TO_INDEX(0, 0, 3));
    EXPECT_EQ(7, BLOCK_OFFSET_TO_INDEX(0, 7, 3));

    EXPECT_EQ(8, BLOCK_OFFSET_TO_INDEX(1, 0, 3));
    EXPECT_EQ(15, BLOCK_OFFSET_TO_INDEX(1, 7, 3));

    EXPECT_EQ(16, BLOCK_OFFSET_TO_INDEX(2, 0, 3));
    EXPECT_EQ(31, BLOCK_OFFSET_TO_INDEX(2, 15, 3));

    EXPECT_EQ(32, BLOCK_OFFSET_TO_INDEX(3, 0, 3));
    EXPECT_EQ(63, BLOCK_OFFSET_TO_INDEX(3, 31, 3));

    EXPECT_EQ(64, BLOCK_OFFSET_TO_INDEX(4, 0, 3));
    EXPECT_EQ(127, BLOCK_OFFSET_TO_INDEX(4, 63, 3));

    EXPECT_EQ(128, BLOCK_OFFSET_TO_INDEX(5, 0, 3));
    EXPECT_EQ(255, BLOCK_OFFSET_TO_INDEX(5, 127, 3));
}

TEST(ArenaGeometricUtils, IndexToOffset) {
    EXPECT_EQ(0, INDEX_TO_OFFSET(0, 0, 3));
    EXPECT_EQ(1, INDEX_TO_OFFSET(1, 0, 3));
    EXPECT_EQ(7, INDEX_TO_OFFSET(7, 0, 3));

    EXPECT_EQ(0, INDEX_TO_OFFSET(8, 1, 3));
    EXPECT_EQ(1, INDEX_TO_OFFSET(9, 1, 3));
    EXPECT_EQ(7, INDEX_TO_OFFSET(15, 1, 3));

    EXPECT_EQ(0, INDEX_TO_OFFSET(16, 2, 3));
    EXPECT_EQ(1, INDEX_TO_OFFSET(17, 2, 3));
    EXPECT_EQ(15, INDEX_TO_OFFSET(31, 2, 3));
}
