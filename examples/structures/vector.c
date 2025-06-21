#include <assert.h>
#include <stdint.h>

#include <derive-c/macros/iterators.h>

/// @defgroup Pushing and Popping
/// @brief Storing integers in a vector
/// @{

#define T int32_t
#define SELF vec_ints
#include <derive-c/structures/vector/template.h>

void ints_example() {
    vec_ints ints = vec_ints_new_with_capacity(10);
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
    vec_ints_popped_entry popped = vec_ints_pop(&ints);
    assert(popped.present);
    assert(popped.value == upto); // Last value should be 99 + 1
    assert(vec_ints_size(&ints) == upto - 1);

    vec_ints_delete(&ints);
}

/// @}

/// @defgroup Moving complex values
/// @brief Storing complex values, removing them (and owning their data)
/// @{

struct complex {
    char* description;
    size_t score;
};

void complex_delete(struct complex* self) { free(self->description); }

#define T struct complex
#define T_DELETE complex_delete
#define SELF vec_complex
#include <derive-c/structures/vector/template.h>

void complex_example() {
    vec_complex vec = vec_complex_new_with_capacity(5);
    size_t entries = 5;
    for (size_t i = 0; i < entries; i++) {
        struct complex item = {.description = strdup("Complex item"), .score = i * 10};
        vec_complex_push(&vec, item);
    }

    assert(vec_complex_size(&vec) == entries);

    struct complex* first_item = vec_complex_write(&vec, 0);
    first_item->score += 5;

    assert(vec_complex_read(&vec, 0)->score == 5);

    vec_complex_popped_entry popped = vec_complex_pop(&vec);
    assert(popped.present);
    assert(popped.value.score == 40); // Last item's score should be 40

    vec_complex_delete(&vec);

    complex_delete(&popped.value);
}

/// @}

/// @defgroup Iterating over vectors
/// @brief Iterating over values in a loop, demonstrating the iterator interface
/// @{

#define T char
#define SELF char_vec
#include <derive-c/structures/vector/template.h>

void iterate_example() {
    char_vec vec = char_vec_new();
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
        ITER_LOOP(char_vec_iter_const, iter, char const*, foo) { printf("%c", *foo); }
        printf("\n");
    }

    char_vec_delete(&vec);
}

/// @}

int main() {
    ints_example();
    complex_example();
    iterate_example();
}
