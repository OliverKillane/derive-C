/// @file
/// @example utils/result.c
/// @brief Examples for using the result type.

#include <derive-c/prelude.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum error_kind {
    ERROR_KIND_NOT_FOUND,
    ERROR_KIND_INVALID,
    ERROR_KIND_PERMISSION_DENIED,
};

static char const* error_kind_to_str(enum error_kind self) {
    switch (self) {
    case ERROR_KIND_NOT_FOUND:
        return "NOT_FOUND";
    case ERROR_KIND_INVALID:
        return "INVALID";
    case ERROR_KIND_PERMISSION_DENIED:
        return "PERMISSION_DENIED";
    }
    return "UNKNOWN";
}

static void error_kind_debug(enum error_kind const* self, dc_debug_fmt /* fmt */, FILE* stream) {
    fprintf(stream, "error_kind::%s", error_kind_to_str(*self));
}

struct example_data {
    int value;
    char* description;
};

static void example_data_delete(struct example_data* self) { free(self->description); }

static void example_data_debug(struct example_data const* self, dc_debug_fmt fmt, FILE* stream) {
    fprintf(stream, "example_data@%p {\n", (void*)self);
    fmt = dc_debug_fmt_scope_begin(fmt);
    dc_debug_fmt_print(fmt, stream, "value: %d,\n", self->value);
    dc_debug_fmt_print(fmt, stream, "description: %s,\n", self->description);
    fmt = dc_debug_fmt_scope_end(fmt);
    dc_debug_fmt_print(fmt, stream, "}");
}

#define OK struct example_data
#define OK_DELETE example_data_delete
#define OK_DEBUG example_data_debug
#define ERROR enum error_kind
#define ERROR_DEBUG error_kind_debug
#define NAME example_result
#include <derive-c/utils/result/template.h>

static void example_result_example() {
    DC_DEBUG_TRACE;

    DC_SCOPED(example_result)
    success_result = example_result_from_ok((struct example_data){
        .value = 42,
        .description = strdup("A successful result"),
    });

    DC_SCOPED(example_result) error_result = example_result_from_error(ERROR_KIND_NOT_FOUND);
    DC_SCOPED(example_result) invalid_result = example_result_from_error(ERROR_KIND_INVALID);
    DC_SCOPED(example_result)
    permission_result = example_result_from_error(ERROR_KIND_PERMISSION_DENIED);

    example_result_debug(&success_result, dc_debug_fmt_new(), stdout);
    fprintf(stdout, "\n");
    example_result_debug(&error_result, dc_debug_fmt_new(), stdout);
    fprintf(stdout, "\n");
    example_result_debug(&invalid_result, dc_debug_fmt_new(), stdout);
    fprintf(stdout, "\n");
    example_result_debug(&permission_result, dc_debug_fmt_new(), stdout);
    fprintf(stdout, "\n");

    DC_ASSERT(!example_result_is_error(&success_result));
    DC_ASSERT(example_result_is_error(&error_result));
    DC_ASSERT(example_result_is_error(&invalid_result));
    DC_ASSERT(example_result_is_error(&permission_result));

    struct example_data const* ok_data = example_result_get_okay(&success_result);
    DC_ASSERT(ok_data != NULL);
    DC_ASSERT(ok_data->value == 42);
    DC_ASSERT(strcmp(ok_data->description, "A successful result") == 0);

    enum error_kind const* error_data = example_result_get_error(&error_result);
    DC_ASSERT(error_data != NULL);
    DC_ASSERT(*error_data == ERROR_KIND_NOT_FOUND);

    enum error_kind const* invalid_data = example_result_get_error(&invalid_result);
    DC_ASSERT(invalid_data != NULL);
    DC_ASSERT(*invalid_data == ERROR_KIND_INVALID);

    enum error_kind const* permission_data = example_result_get_error(&permission_result);
    DC_ASSERT(permission_data != NULL);
    DC_ASSERT(*permission_data == ERROR_KIND_PERMISSION_DENIED);

    DC_ASSERT(example_result_get_okay(&error_result) == NULL);
    DC_ASSERT(example_result_get_error(&success_result) == NULL);
}

int main() {
    example_result_example();
    return 0;
}
