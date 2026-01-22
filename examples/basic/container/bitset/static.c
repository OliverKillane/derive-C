/// @file
/// @example container/bitset/static.c
/// @brief Examples for using statically-sized bitsets.

#include <stdio.h>

#include <derive-c/prelude.h>

#define EXCLUSIVE_END_INDEX 128
#define NAME bitset
#include <derive-c/container/bitset/static/template.h>

static void basic_operations() {
    printf("=== Basic Bitset Operations ===\n");
    DC_SCOPED(bitset) bs = bitset_new();

    bitset_set(&bs, 0, true);
    bitset_set(&bs, 5, true);
    bitset_set(&bs, 10, true);
    bitset_set(&bs, 63, true);

    printf("Size (count of set bits): %zu\n", bitset_size(&bs));

    bitset_set(&bs, 5, !bitset_get(&bs, 5));
    bitset_set(&bs, 7, !bitset_get(&bs, 7));
    printf("After flipping bits 5 and 7: size = %zu\n", bitset_size(&bs));

    bitset_set(&bs, 10, false);
    printf("After clearing bit 10: size = %zu\n", bitset_size(&bs));

    bitset_debug(&bs, dc_debug_fmt_new(), stdout);
}

static void union_and_intersection() {
    printf("\n=== Union and Intersection ===\n");
    DC_SCOPED(bitset) evens = bitset_new();
    DC_SCOPED(bitset) odds = bitset_new();

    for (uint8_t i = 0; i < 16; i += 2) {
        bitset_set(&evens, i, true);
    }
    for (uint8_t i = 1; i < 16; i += 2) {
        bitset_set(&odds, i, true);
    }

    printf("Evens: %zu set, Odds: %zu set\n", bitset_size(&evens), bitset_size(&odds));

    DC_SCOPED(bitset) combined = bitset_clone(&evens);
    for (uint8_t i = 0; i < 128; i++) {
        if (bitset_get(&odds, i)) {
            bitset_set(&combined, i, true);
        }
    }
    printf("Union: %zu set\n", bitset_size(&combined));
    DC_ASSERT(bitset_size(&combined) == 16);

    bool has_common = false;
    for (uint8_t i = 0; i < 128; i++) {
        if (bitset_get(&evens, i) && bitset_get(&odds, i)) {
            has_common = true;
            break;
        }
    }
    printf("Intersection: %s\n", has_common ? "not empty" : "empty");
    DC_ASSERT(!has_common);
}

#define EXCLUSIVE_END_INDEX 64
#define NAME flags
#include <derive-c/container/bitset/static/template.h>

enum feature {
    FEATURE_LOGGING = 0,
    FEATURE_CACHING = 1,
    FEATURE_COMPRESSION = 2,
    FEATURE_ENCRYPTION = 3,
};

static void feature_flags() {
    printf("\n=== Feature Flags ===\n");
    DC_SCOPED(flags) features = flags_new();

    flags_set(&features, FEATURE_LOGGING, true);
    flags_set(&features, FEATURE_CACHING, true);
    flags_set(&features, FEATURE_ENCRYPTION, true);

    printf("Logging: %s\n", flags_get(&features, FEATURE_LOGGING) ? "ON" : "OFF");
    printf("Compression: %s\n", flags_get(&features, FEATURE_COMPRESSION) ? "ON" : "OFF");
    printf("Total enabled: %zu\n", flags_size(&features));

    flags_set(&features, FEATURE_COMPRESSION, !flags_get(&features, FEATURE_COMPRESSION));
    printf("Toggled compression: %s\n", flags_get(&features, FEATURE_COMPRESSION) ? "ON" : "OFF");
}

int main() {
    basic_operations();
    union_and_intersection();
    feature_flags();
    return 0;
}
