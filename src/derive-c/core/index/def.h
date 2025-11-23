#if !defined INDEX_BITS
    #error "INDEX_BITS must be defined"
#endif

#if INDEX_BITS == 8
    #define INDEX_TYPE uint8_t
    #define CAPACITY_EXCLUSIVE_UPPER (UINT8_MAX + 1ULL)
    #define MAX_INDEX (UINT8_MAX - 1ULL)
    #define INDEX_NONE UINT8_MAX
#elif INDEX_BITS == 16
    #define INDEX_TYPE uint16_t
    #define CAPACITY_EXCLUSIVE_UPPER (UINT16_MAX + 1ULL)
    #define MAX_INDEX (UINT16_MAX - 1ULL)
    #define INDEX_NONE UINT16_MAX
#elif INDEX_BITS == 32
    #define INDEX_TYPE uint32_t
    #define CAPACITY_EXCLUSIVE_UPPER (UINT32_MAX + 1ULL)
    #define MAX_INDEX (UINT32_MAX - 1ULL)
    #define INDEX_NONE UINT32_MAX
#elif INDEX_BITS == 64
    #define INDEX_TYPE uint64_t
    // JUSTIFY: Special case, we cannot store the max capacity as a size_t integer
    #define CAPACITY_EXCLUSIVE_UPPER UINT64_MAX
    #define MAX_INDEX (UINT64_MAX - 1ULL)
    #define INDEX_NONE UINT64_MAX
#endif

#if defined INDEX_TYPE
    #define INDEX NS(SELF, index_t)

// INVARIANT: < CAPACITY_EXCLUSIVE_UPPER
typedef struct {
    INDEX_TYPE index;
} INDEX;

static bool NS(INDEX, eq)(INDEX const* idx_1, INDEX const* idx_2) {
    return idx_1->index == idx_2->index;
}

static void NS(INDEX, debug)(INDEX const* idx, debug_fmt fmt, FILE* stream) {
    (void)fmt;
    fprintf(stream, EXPAND_STRING(INDEX) " { %lu }", (size_t)idx->index);
}
#endif