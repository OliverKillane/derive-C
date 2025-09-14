/// @file
/// @example structures/option.c
/// @brief Examples for using the optional type.

#include <assert.h>
#include <stdlib.h>
#include <string.h>

struct complex_data {
    int x;
    double y;
    char* description;
};

void complex_data_delete(struct complex_data* self) { free(self->description); }

#define T struct complex_data
#define T_DELETE complex_data_delete
#define NAME complex_data_option
#include <derive-c/structures/option/template.h>

void option_example() {
    complex_data_option opt = complex_data_option_empty();
    assert(!complex_data_option_is_present(&opt));

    // when accessing a value, you get a pointer. Not present = NULL
    assert(!complex_data_option_get(&opt));
    assert(!complex_data_option_get_const(&opt));

    bool was_present_1 = complex_data_option_replace(
        &opt, (struct complex_data){.x = 42, .y = 3.14, .description = strdup("A complex data")});
    assert(!was_present_1);

    assert(complex_data_option_is_present(&opt));
    assert(complex_data_option_get(&opt));

    bool was_present_2 = complex_data_option_replace(
        &opt,
        (struct complex_data){.x = 100, .y = 2.71, .description = strdup("Another complex data")});
    assert(was_present_2);

    complex_data_option_delete(&opt);
}

int main() { option_example(); }