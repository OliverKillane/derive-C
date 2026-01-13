/// @file
/// @example container/vector/static.c
/// @brief Examples for using static vectors (in-place storage, up to a fixed size).

#include <stdint.h>
#include <stdio.h>

#include <derive-c/core/prelude.h>

#define MAX_CAPACITY 8

#define ITEM unsigned char
#define CAPACITY MAX_CAPACITY
#define NAME staticvec_chars
#include <derive-c/container/vector/static/template.h>

static void push_example() {
    staticvec_chars vec = staticvec_chars_new();

    // Push characters into the static vector
    for (unsigned char i = 0; i < MAX_CAPACITY; i++) {
        staticvec_chars_push(&vec, i);
    }

    // Cannot push past the in-place capacity
    DC_ASSERT(!staticvec_chars_try_push(&vec, 8));

    // Check that the first 8 characters are in place
    for (unsigned char i = 0; i < MAX_CAPACITY; i++) {
        DC_ASSERT(*staticvec_chars_read(&vec, i) == i);
    }

    // The next two should be NULL since they exceed the in-place capacity
    DC_ASSERT(staticvec_chars_try_read(&vec, MAX_CAPACITY) == NULL);
    DC_ASSERT(staticvec_chars_try_read(&vec, MAX_CAPACITY + 1) == NULL);

    staticvec_chars_debug(&vec, dc_debug_fmt_new(), stdout);

    staticvec_chars_delete(&vec);
}

static void iter_example() {
    staticvec_chars vec = staticvec_chars_new();

    // Push characters into the static vector
    for (unsigned char i = 0; i < MAX_CAPACITY; i++) {
        staticvec_chars_push(&vec, 'a' + i);
    }

    // Iterate over the vector and print the items
    staticvec_chars_iter_const iter = staticvec_chars_get_iter_const(&vec);
    unsigned char const* item = NULL;
    while (item = staticvec_chars_iter_const_next(&iter), item != NULL) {
        printf("%u ", *item);
    }
    printf("\n");

    staticvec_chars_debug(&vec, dc_debug_fmt_new(), stdout);

    staticvec_chars_delete(&vec);
}

int main() {
    push_example();
    iter_example();
}
