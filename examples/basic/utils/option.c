/// @file
/// @example utils/option.c
/// @brief Examples for using the option type.

#include <derive-c/prelude.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct example_data {
    int value;
    char* description;
};

static void example_data_delete(struct example_data* self) { free(self->description); }

static struct example_data example_data_clone(struct example_data const* self) {
    return (struct example_data){
        .value = self->value,
        .description = strdup(self->description),
    };
}

static void example_data_debug(struct example_data const* self, dc_debug_fmt fmt, FILE* stream) {
    fprintf(stream, "example_data@%p {\n", (void*)self);
    fmt = dc_debug_fmt_scope_begin(fmt);
    dc_debug_fmt_print(fmt, stream, "value: %d,\n", self->value);
    dc_debug_fmt_print(fmt, stream, "description: %s,\n", self->description);
    fmt = dc_debug_fmt_scope_end(fmt);
    dc_debug_fmt_print(fmt, stream, "}");
}

#define ITEM struct example_data
#define ITEM_DELETE example_data_delete
#define ITEM_CLONE example_data_clone
#define ITEM_DEBUG example_data_debug
#define NAME example_option
#include <derive-c/utils/option/template.h>

static void example_option_example() {
    DC_DEBUG_TRACE;

    DC_SCOPED(example_option)
    present_option = example_option_from(
        (struct example_data){.value = 42, .description = strdup("A present option")});

    DC_SCOPED(example_option) empty_option = example_option_empty();

    example_option_debug(&present_option, dc_debug_fmt_new(), stdout);
    fprintf(stdout, "\n");
    example_option_debug(&empty_option, dc_debug_fmt_new(), stdout);
    fprintf(stdout, "\n");

    DC_ASSERT(example_option_is_present(&present_option));
    DC_ASSERT(!example_option_is_present(&empty_option));

    struct example_data const* present_data = example_option_get_const(&present_option);
    DC_ASSERT(present_data != NULL);
    DC_ASSERT(present_data->value == 42);
    DC_ASSERT(strcmp(present_data->description, "A present option") == 0);

    struct example_data const* empty_data = example_option_get_const(&empty_option);
    DC_ASSERT(empty_data == NULL);

    bool was_present = example_option_replace(
        &empty_option,
        (struct example_data){.value = 100, .description = strdup("Replaced value")});
    DC_ASSERT(!was_present);
    DC_ASSERT(example_option_is_present(&empty_option));

    struct example_data const* replaced_data = example_option_get_const(&empty_option);
    DC_ASSERT(replaced_data != NULL);
    DC_ASSERT(replaced_data->value == 100);
    DC_ASSERT(strcmp(replaced_data->description, "Replaced value") == 0);
}

int main() {
    example_option_example();
    return 0;
}
