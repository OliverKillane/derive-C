/// @file
/// @example container/vector/dynamic.c
/// @brief Examples for inserting, iterating, and deleting from vectors.

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <derive-c/alloc/std.h>

#define ITEM int32_t
#define NAME vec_ints
#include <derive-c/container/vector/dynamic/template.h>

void ints_example() {
    vec_ints ints = vec_ints_new_with_capacity(10, stdalloc_get());
    const int32_t upto = 100;

    for (int32_t i = 0; i < upto; i++) {
        vec_ints_push(&ints, i);
    }
    assert(vec_ints_size(&ints) == upto);

    for (int32_t i = 0; i < upto; i++) {
        int* value = vec_ints_write(&ints, i);
        *value += 1; // Increment each value by 1
    }

    for (int32_t i = 0; i < upto; i++) {
        assert(*vec_ints_read(&ints, i) == i + 1);
    }

    // Pop the last value
    int32_t last_value = vec_ints_pop(&ints);
    assert(last_value == upto); // Last value should be 99 + 1
    assert(vec_ints_size(&ints) == upto - 1);

    vec_ints_delete(&ints);
}

struct complex_data {
    char* description;
    size_t score;
};

void complex_data_delete(struct complex_data* self) { free(self->description); }

#define ITEM struct complex_data
#define ITEM_DELETE complex_data_delete
#define NAME vec_complex_data
#include <derive-c/container/vector/dynamic/template.h>

void complex_data_example() {
    vec_complex_data vec = vec_complex_data_new_with_capacity(5, stdalloc_get());
    size_t entries = 5;
    for (size_t i = 0; i < entries; i++) {
        struct complex_data item = {.description = strdup("Complex item"), .score = i * 10};
        vec_complex_data_push(&vec, item);
    }

    assert(vec_complex_data_size(&vec) == entries);

    struct complex_data* first_item = vec_complex_data_write(&vec, 0);
    first_item->score += 5;

    assert(vec_complex_data_read(&vec, 0)->score == 5);

    struct complex_data popped = vec_complex_data_pop(&vec);
    assert(popped.score == 40); // Last item's score should be 40

    vec_complex_data_delete(&vec);
    complex_data_delete(&popped);
}

#define ITEM char
#define NAME char_vec
#include <derive-c/container/vector/dynamic/template.h>

void iterate_example() {
    char_vec vec = char_vec_new(stdalloc_get());
    char_vec_push(&vec, 'H');
    char_vec_push(&vec, 'e');
    char_vec_push(&vec, 'l');
    char_vec_push(&vec, 'l');
    char_vec_push(&vec, 'o');
    char_vec_push(&vec, ' ');
    char_vec_push(&vec, 'W');
    char_vec_push(&vec, 'o');
    char_vec_push(&vec, 'r');
    char_vec_push(&vec, 'l');
    char_vec_push(&vec, 'd');

    assert(char_vec_size(&vec) == 11);
    {
        // Iterate over the vector and print the items
        char_vec_iter_const iter = char_vec_get_iter_const(&vec);
        char const* item = NULL;
        while (item = char_vec_iter_const_next(&iter), item != NULL) {
            printf("%c", *item);
        }
        printf("\n");
    }

    {
        char_vec_iter_const iter = char_vec_get_iter_const(&vec);

        char const* c = NULL;
        size_t index = 0;
        while ((c = char_vec_iter_const_next(&iter))) {
            printf("entry for '%c' at index %zu\n", *c, index);
            index++;
        }
    }

    char_vec_delete(&vec);
}

int main() {
    ints_example();
    complex_data_example();
    iterate_example();
}
