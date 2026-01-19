/// @file
/// @example container/vector/dynamic.c
/// @brief Examples for inserting, iterating, and deleting from vectors.

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <derive-c/alloc/std.h>
#include <derive-c/prelude.h>

#define ITEM int32_t
#define NAME vec_ints
#include <derive-c/container/vector/dynamic/template.h>

static void ints_example() {
    printf("=== Integer Vector Example ===\n");
    vec_ints ints = vec_ints_new_with_capacity(10, stdalloc_get_ref());
    const int32_t upto = 100;

    // Push values 0-99
    for (int32_t i = 0; i < upto; i++) {
        vec_ints_push(&ints, i);
    }
    printf("After pushing %d values, size: %zu\n", upto, vec_ints_size(&ints));
    DC_ASSERT(vec_ints_size(&ints) == upto);

    // Increment all values
    for (int32_t i = 0; i < upto; i++) {
        int* value = vec_ints_write(&ints, (size_t)i);
        *value += 1;
    }

    // Verify all values were incremented
    for (int32_t i = 0; i < upto; i++) {
        DC_ASSERT(*vec_ints_read(&ints, (size_t)i) == i + 1);
    }
    printf("Successfully incremented all values\n");

    // Pop the last value
    int32_t last_value = vec_ints_pop(&ints);
    printf("Popped last value: %d\n", last_value);
    DC_ASSERT(last_value == upto);
    DC_ASSERT(vec_ints_size(&ints) == upto - 1);

    // Show some values
    printf("First 5 values: ");
    for (size_t i = 0; i < 5; i++) {
        printf("%d ", *vec_ints_read(&ints, i));
    }
    printf("\n");

    vec_ints_debug(&ints, dc_debug_fmt_new(), stdout);
    printf("\n");

    vec_ints_delete(&ints);
}

struct complex_data {
    char* description;
    size_t score;
};

static void complex_data_debug(struct complex_data const* self, dc_debug_fmt fmt, FILE* stream) {
    fprintf(stream, "complex_data@%p {\n", self);
    fmt = dc_debug_fmt_scope_begin(fmt);
    dc_debug_fmt_print(fmt, stream, "description: %s,\n", self->description);
    dc_debug_fmt_print(fmt, stream, "score: %lu,\n", self->score);
    fmt = dc_debug_fmt_scope_end(fmt);
    dc_debug_fmt_print(fmt, stream, "}");
}

static void complex_data_delete(struct complex_data* self) { free(self->description); }

#define ITEM struct complex_data
#define ITEM_DELETE complex_data_delete
#define ITEM_DEBUG complex_data_debug
#define NAME vec_complex_data
#include <derive-c/container/vector/dynamic/template.h>

static void complex_data_example() {
    printf("\n=== Complex Data Vector Example ===\n");
    vec_complex_data vec = vec_complex_data_new_with_capacity(5, stdalloc_get_ref());
    size_t entries = 5;

    // Push complex data with owned strings
    for (size_t i = 0; i < entries; i++) {
        char buffer[50];
        snprintf(buffer, sizeof(buffer), "Item %zu", i);
        struct complex_data item = {.description = strdup(buffer), .score = i * 10};
        vec_complex_data_push(&vec, item);
    }

    printf("Pushed %zu items\n", entries);
    DC_ASSERT(vec_complex_data_size(&vec) == entries);

    // Modify first item
    struct complex_data* first_item = vec_complex_data_write(&vec, 0);
    first_item->score += 5;
    printf("Modified first item score: %zu\n", first_item->score);

    DC_ASSERT(vec_complex_data_read(&vec, 0)->score == 5);

    // Display all items
    printf("\nAll items:\n");
    for (size_t i = 0; i < vec_complex_data_size(&vec); i++) {
        struct complex_data const* item = vec_complex_data_read(&vec, i);
        printf("  [%zu]: \"%s\" - score: %zu\n", i, item->description, item->score);
    }

    // Pop last item
    struct complex_data popped = vec_complex_data_pop(&vec);
    printf("\nPopped item: \"%s\" - score: %zu\n", popped.description, popped.score);
    DC_ASSERT(popped.score == 40);

    vec_complex_data_debug(&vec, dc_debug_fmt_new(), stdout);
    printf("\n");

    vec_complex_data_delete(&vec);
    complex_data_delete(&popped);
}

#define ITEM char
#define NAME char_vec
#include <derive-c/container/vector/dynamic/template.h>

static void iterate_example() {
    printf("\n=== Vector Iterator Example ===\n");
    char_vec vec = char_vec_new(stdalloc_get_ref());

    const char* message = "Hello World";
    printf("Pushing characters: \"%s\"\n", message);
    for (const char* c = message; *c != '\0'; c++) {
        char_vec_push(&vec, *c);
    }

    DC_ASSERT(char_vec_size(&vec) == 11);

    // Iterate and print as string
    printf("Vector contents: ");
    {
        char_vec_iter_const iter = char_vec_get_iter_const(&vec);
        char const* item = NULL;
        while (item = char_vec_iter_const_next(&iter), item != NULL) {
            printf("%c", *item);
        }
        printf("\n");
    }

    // Iterate with indices
    printf("\nWith indices:\n");
    {
        char_vec_iter_const iter = char_vec_get_iter_const(&vec);
        char const* c = NULL;
        size_t index = 0;
        while ((c = char_vec_iter_const_next(&iter))) {
            printf("  [%zu]: '%c'\n", index, *c);
            index++;
        }
    }

    // Mutable iteration to uppercase
    printf("\nConverting to uppercase using mutable iterator:\n");
    {
        char_vec_iter iter = char_vec_get_iter(&vec);
        char* c = NULL;
        while ((c = char_vec_iter_next(&iter))) {
            if (*c >= 'a' && *c <= 'z') {
                *c = (char)((*c - 'a') + 'A');
            }
        }
    }

    // Print again
    printf("After uppercase conversion: ");
    {
        char_vec_iter_const iter = char_vec_get_iter_const(&vec);
        char const* item = NULL;
        while (item = char_vec_iter_const_next(&iter), item != NULL) {
            printf("%c", *item);
        }
        printf("\n\n");
    }

    char_vec_delete(&vec);
}

int main() {
    ints_example();
    complex_data_example();
    iterate_example();
}
