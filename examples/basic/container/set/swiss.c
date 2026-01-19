/// @file
/// @example container/set/swiss.c
/// @brief Hash sets with integers, strings, and structs.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <derive-c/algorithm/hash/default.h>
#include <derive-c/alloc/std.h>
#include <derive-c/prelude.h>
#include <derive-c/utils/for.h>

static size_t uint32_hash(uint32_t const* item) { return DC_DEFAULT_HASH(item); }

#define ITEM uint32_t
#define ITEM_HASH uint32_hash
#define NAME uint_set
#include <derive-c/container/set/swiss/template.h>

static void integer_sets() {
    printf("=== Integer Sets ===\n");
    DC_SCOPED(uint_set) s = uint_set_new(stdalloc_get_ref());

    for (uint32_t i = 0; i < 10; i++) {
        uint_set_add(&s, i * 2);
    }

    printf("Size: %zu\n", uint_set_size(&s));

    uint_set_remove(&s, 8);
    printf("After removing 8: contains 8? %s\n", uint_set_contains(&s, 8) ? "yes" : "no");
}

static bool str_eq(char const* const* a, char const* const* b) { return strcmp(*a, *b) == 0; }

static size_t str_hash(char const* const* s) {
    size_t hash = 0;
    const char* str = *s;
    while (*str) {
        hash = hash * 31 + (size_t)(unsigned char)*str;
        str++;
    }
    return hash;
}

#define ITEM char const*
#define ITEM_EQ str_eq
#define ITEM_HASH str_hash
#define NAME str_set
#include <derive-c/container/set/swiss/template.h>

static void string_sets() {
    printf("\n=== String Sets ===\n");
    DC_SCOPED(str_set) s = str_set_new(stdalloc_get_ref());

    str_set_add(&s, "apple");
    str_set_add(&s, "banana");
    str_set_add(&s, "cherry");

    printf("Size: %zu\n", str_set_size(&s));
    DC_ASSERT(str_set_size(&s) == 3);

    const char* search = "banana";
    printf("Contains 'banana': %s\n", str_set_contains(&s, search) ? "yes" : "no");

    printf("\nStrings:\n");
    DC_FOR_CONST(str_set, &s, iter, item) { printf("  '%s'\n", *item); }
}

static void set_operations() {
    printf("\n=== Set Operations ===\n");
    DC_SCOPED(uint_set) odds = uint_set_new(stdalloc_get_ref());
    DC_SCOPED(uint_set) evens = uint_set_new(stdalloc_get_ref());

    for (uint32_t i = 0; i < 20; i++) {
        if (i % 2 == 0) {
            uint_set_add(&evens, i);
        } else {
            uint_set_add(&odds, i);
        }
    }

    bool has_common = false;
    DC_FOR_CONST(uint_set, &odds, iter1, item) {
        if (uint_set_contains(&evens, *item)) {
            has_common = true;
            break;
        }
    }
    printf("Disjoint sets: %s\n", !has_common ? "yes" : "no");

    DC_SCOPED(uint_set) combined = uint_set_clone(&odds);
    DC_FOR_CONST(uint_set, &evens, iter2, item) { uint_set_add(&combined, *item); }
    printf("Union size: %zu\n", uint_set_size(&combined));
    DC_ASSERT(uint_set_size(&combined) == 20);
}

struct point {
    int x;
    int y;
};

static bool point_eq(struct point const* a, struct point const* b) {
    return a->x == b->x && a->y == b->y;
}

static size_t point_hash(struct point const* p) { return ((size_t)p->x * 31) + (size_t)p->y; }

static void point_debug(struct point const* p, dc_debug_fmt fmt, FILE* stream) {
    (void)fmt;
    fprintf(stream, "(%d,%d)", p->x, p->y);
}

#define ITEM struct point
#define ITEM_EQ point_eq
#define ITEM_HASH point_hash
#define ITEM_DEBUG point_debug
#define NAME point_set
#include <derive-c/container/set/swiss/template.h>

static void struct_sets() {
    printf("\n=== Struct Sets ===\n");
    DC_SCOPED(point_set) s = point_set_new(stdalloc_get_ref());

    point_set_add(&s, (struct point){0, 0});
    point_set_add(&s, (struct point){1, 1});
    point_set_add(&s, (struct point){2, 3});

    printf("Size: %zu\n", point_set_size(&s));
    DC_ASSERT(point_set_size(&s) == 3);

    struct point search = {1, 1};
    printf("Contains (1,1): %s\n", point_set_contains(&s, search) ? "yes" : "no");

    printf("\nPoints:\n");
    DC_FOR_CONST(point_set, &s, iter, pt) { printf("  (%d,%d)\n", pt->x, pt->y); }
}

int main() {
    integer_sets();
    string_sets();
    set_operations();
    struct_sets();
    return 0;
}
