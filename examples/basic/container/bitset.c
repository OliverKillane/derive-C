/// @file
/// @example container/bitset.c
/// @brief Examples for using bitset containers.

#include <derive-c/prelude.h>
#include <stdio.h>

#define EXCLUSIVE_END_INDEX 128
#define NAME bitset
#include <derive-c/container/bitset/static/template.h>

static void example_basic() {
    DC_DEBUG_TRACE;
    DC_SCOPED(bitset) bs = bitset_new();

    bitset_set(&bs, 0, true);
    bitset_set(&bs, 5, true);

    bitset_debug(&bs, dc_debug_fmt_new(), stdout);

    for (bitset_index_t i = 0; i < 128; i++) {
        if (bitset_get(&bs, i)) {
            printf("Bit at index %u is set\n", i);
        }
    }

    DC_FOR_CONST(bitset, &bs, iter, index) { printf("Iterated index: %u\n", index); }
}

int main() {
    example_basic();
    return 0;
}
