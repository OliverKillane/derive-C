#if defined INDEX_TYPE
    #define INDEX NS(SELF, index_t)

// INVARIANT: < CAPACITY_EXCLUSIVE_UPPER
typedef struct {
    INDEX_TYPE index;
} INDEX;

PUBLIC static bool NS(INDEX, eq)(INDEX const* idx_1, INDEX const* idx_2) {
    return idx_1->index == idx_2->index;
}

PUBLIC static void NS(INDEX, debug)(INDEX const* idx, dc_debug_fmt fmt, FILE* stream) {
    (void)fmt;
    fprintf(stream, DC_EXPAND_STRING(INDEX) " { %lu }", (size_t)idx->index);
}
#endif
