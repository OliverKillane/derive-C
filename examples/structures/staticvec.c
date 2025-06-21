#include <assert.h>
#include <stdint.h>

#include <derive-c/macros/iterators.h>

/// @defgroup Pushing Up to Capacity
/// @brief Storing characters in place
/// @{

#define MAX_CAPACITY 8

#define T unsigned char
#define SELF staticvec_chars
#define INPLACE_CAPACITY MAX_CAPACITY
#include <derive-c/structures/staticvec/template.h>

void push_example() {
    staticvec_chars vec = staticvec_chars_new();
    
    // Push characters into the static vector
    for (unsigned char i = 0; i < MAX_CAPACITY; i++) {
        staticvec_chars_push(&vec, i);
    }

    // Cannot push past the in-place capacity
    assert(!staticvec_chars_push_optional(&vec, 8));
    
    // Check that the first 8 characters are in place
    for (unsigned char i = 0; i < MAX_CAPACITY; i++) {
        assert(*staticvec_chars_read(&vec, i) == i);
    }
    
    // The next two should be NULL since they exceed the in-place capacity
    assert(staticvec_chars_try_read(&vec, MAX_CAPACITY) == NULL);
    assert(staticvec_chars_try_read(&vec, MAX_CAPACITY + 1) == NULL);
    
    staticvec_chars_delete(&vec);    
}

/// @}

/// @defgroup Pushing Up to Capacity
/// @brief Storing characters in place
/// @{

void iter_example() {
    staticvec_chars vec = staticvec_chars_new();
    
    // Push characters into the static vector
    for (unsigned char i = 0; i < MAX_CAPACITY; i++) {
        staticvec_chars_push(&vec, 'a' +i);
    }

    // Iterate over the vector and print the items
    staticvec_chars_iter_const iter = staticvec_chars_get_iter_const(&vec);
    unsigned char const * item = NULL;
    while (item = staticvec_chars_iter_const_next(&iter), item != NULL) {
        printf("%u ", *item);
    }
    printf("\n");

    staticvec_chars_delete(&vec);
}

/// @}

int main() {
    push_example();
    iter_example();
}