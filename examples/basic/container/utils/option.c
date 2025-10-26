/// @file
/// @example utils/option.c
/// @brief Examples for using the optional type.

#include <stdlib.h>
#include <string.h>

#include <derive-c/core/prelude.h>

struct complex_data {
    int x;
    double y;
    char* description;
};

void complex_data_delete(struct complex_data* self) { free(self->description); }
struct complex_data complex_data_clone(struct complex_data const* self) {
    return (struct complex_data){
        .x = self->x,
        .y = self->y,
        .description = strdup(self->description),
    };
}
void complex_data_debug(struct complex_data const* self, debug_fmt fmt, FILE* stream) {
    fprintf(stream, "complex_data@%p {\n", self);
    fmt = debug_fmt_scope_begin(fmt);
    debug_fmt_print(fmt, stream, "x: %d,\n", self->x);
    debug_fmt_print(fmt, stream, "y: %lf,\n", self->y);
    debug_fmt_print(fmt, stream, "description: %lf,\n", self->description);
    fmt = debug_fmt_scope_end(fmt);
    debug_fmt_print(fmt, stream, "}");
}

#define ITEM struct complex_data
#define ITEM_DELETE complex_data_delete
#define ITEM_CLONE complex_data_clone
#define ITEM_DEBUG complex_data_debug
#define NAME complex_data_option
#include <derive-c/utils/option/template.h>

void option_example() {
    complex_data_option opt = complex_data_option_empty();
    ASSERT(!complex_data_option_is_present(&opt));

    // when accessing a value, you get a pointer. Not present = NULL
    ASSERT(!complex_data_option_get(&opt));
    ASSERT(!complex_data_option_get_const(&opt));

    complex_data_option_debug(&opt, debug_fmt_new(), stdout);

    bool was_present_1 = complex_data_option_replace(
        &opt, (struct complex_data){.x = 42, .y = 3.14, .description = strdup("A complex data")});
    ASSERT(!was_present_1);

    ASSERT(complex_data_option_is_present(&opt));
    ASSERT(complex_data_option_get(&opt));

    complex_data_option_debug(&opt, debug_fmt_new(), stdout);

    bool was_present_2 = complex_data_option_replace(
        &opt,
        (struct complex_data){.x = 100, .y = 2.71, .description = strdup("Another complex data")});
    ASSERT(was_present_2);

    complex_data_option_delete(&opt);
}

int main() { option_example(); }