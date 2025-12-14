#pragma once
#include <derive-c/core/math.h>

#define DC_ARENA_GEO_INDEX_TO_BLOCK(IDX, INITIAL_BLOCK_INDEX_BITS)                                              \
    (uint8_t)(DC_MATH_MSB_INDEX(IDX) < (INITIAL_BLOCK_INDEX_BITS)                                          \
                  ? 0                                                                              \
                  : ((1 + DC_MATH_MSB_INDEX(IDX)) - (INITIAL_BLOCK_INDEX_BITS)))

#define DC_ARENA_GEO_INDEX_TO_OFFSET(IDX, BLOCK, INITIAL_BLOCK_INDEX_BITS)                                      \
    (size_t)(IDX - (BLOCK == 0 ? 0 : (1ULL << ((BLOCK - 1) + (INITIAL_BLOCK_INDEX_BITS)))))

#define DC_ARENA_GEO_BLOCK_TO_SIZE(BLOCK, INITIAL_BLOCK_INDEX_BITS)                                             \
    (size_t)(BLOCK == 0 ? (1ULL << INITIAL_BLOCK_INDEX_BITS)                                       \
                        : (1ULL << ((BLOCK - 1) + (INITIAL_BLOCK_INDEX_BITS))))

#define DC_ARENA_GEO_BLOCK_OFFSET_TO_INDEX(BLOCK, OFFSET, INITIAL_BLOCK_INDEX_BITS)                             \
    ((BLOCK == 0 ? 0 : (1ULL << ((BLOCK - 1) + (INITIAL_BLOCK_INDEX_BITS)))) + OFFSET)

#define DC_ARENA_GEO_MAX_NUM_BLOCKS(INDEX_BITS, INITIAL_BLOCK_INDEX_BITS)                                       \
    ((INDEX_BITS - INITIAL_BLOCK_INDEX_BITS) + 1)
