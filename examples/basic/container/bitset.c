/// @file
/// @example container/bitset.c
/// @brief Examples for using bitset containers.

#include <derive-c/prelude.h>

#define EXCLUSIVE_END_INDEX 128
#define NAME bitset
#include <derive-c/container/bitset/static/template.h>

static void example_basic(DC_LOGGER* parent) {
    DC_SCOPED(DC_LOGGER) log = DC_LOGGER_NEW(parent, "%s", __func__);
    DC_SCOPED(bitset) bs = bitset_new();

    DC_LOG(log, DC_INFO, "setting bits at index 0 and 5");
    bitset_set(&bs, 0, true);
    bitset_set(&bs, 5, true);

    DC_LOG(log, DC_INFO, "bitset: %s", DC_DEBUG(bitset_debug, &bs));

    for (bitset_index_t i = 0; i < 128; i++) {
        if (bitset_get(&bs, i)) {
            DC_LOG(log, DC_INFO, "bit at index %u is set", i);
        }
    }

    DC_FOR_CONST(bitset, &bs, iter, index) { DC_LOG(log, DC_INFO, "iterated index: %u", index); }
}

int main() {
    DC_SCOPED(DC_LOGGER)
    root = NS(DC_LOGGER,
              new_global)((NS(DC_LOGGER, global_config)){.stream = stdout, .ansi_colours = true},
                          (dc_log_id){"bitset"});

    example_basic(&root);
    return 0;
}
