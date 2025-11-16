#pragma once

#define MSB_INDEX(x)                                                                               \
    (x == 0 ? 0                                                                                    \
            : _Generic((x),                                                                        \
             uint8_t: (7u - __builtin_clz((uint32_t)((x)) << 24)),                                 \
             uint16_t: (15u - __builtin_clz((uint32_t)((x)) << 16)),                               \
             uint32_t: (31u - __builtin_clz((uint32_t)(x))),                                       \
             uint64_t: (63u - __builtin_clzll((uint64_t)(x)))))

#define INDEX_TO_BLOCK(IDX, INITIAL_BLOCK_INDEX_BITS)                                              \
    (uint8_t)(MSB_INDEX(IDX) < (INITIAL_BLOCK_INDEX_BITS)                                          \
                  ? 0                                                                              \
                  : ((1 + MSB_INDEX(IDX)) - (INITIAL_BLOCK_INDEX_BITS)))

#define INDEX_TO_OFFSET(IDX, BLOCK, INITIAL_BLOCK_INDEX_BITS)                                      \
    (size_t)(IDX - (BLOCK == 0 ? 0 : (1ULL << ((BLOCK - 1) + (INITIAL_BLOCK_INDEX_BITS)))))

#define BLOCK_TO_SIZE(BLOCK, INITIAL_BLOCK_INDEX_BITS)                                             \
    (size_t)(BLOCK == 0 ? (1ULL << INITIAL_BLOCK_INDEX_BITS)                                       \
                        : (1ULL << ((BLOCK - 1) + (INITIAL_BLOCK_INDEX_BITS))))

#define BLOCK_OFFSET_TO_INDEX(BLOCK, OFFSET, INITIAL_BLOCK_INDEX_BITS)                             \
    ((BLOCK == 0 ? 0 : (1ULL << ((BLOCK - 1) + (INITIAL_BLOCK_INDEX_BITS)))) + OFFSET)

#define MAX_NUM_BLOCKS(INDEX_BITS, INITIAL_BLOCK_INDEX_BITS)                                       \
    ((INDEX_BITS - INITIAL_BLOCK_INDEX_BITS) + 1)
