/// @file
/// @example container/set.c
/// @brief Examples for using set containers.

#include <derive-c/alloc/std.h>
#include <derive-c/algorithm/hash/default.h>
#include <derive-c/algorithm/hash/combine.h>
#include <derive-c/algorithm/hash/fnv1a.h>
#include <derive-c/prelude.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ITEM int
#define ITEM_HASH DC_DEFAULT_HASH
#define NAME int_set
#include <derive-c/container/set/swiss/template.h>

static void example_integer_set(DC_LOGGER* parent) {
    DC_SCOPED(DC_LOGGER) log = DC_LOGGER_NEW(parent, "%s", __func__);
    DC_SCOPED(int_set) set = int_set_new(stdalloc_get_ref());

    DC_LOG(log, DC_INFO, "adding integers 0-9");
    for (int i = 0; i < 10; i++) {
        int_set_add(&set, i);
    }

    DC_FOR_CONST(int_set, &set, iter, item) { DC_LOG(log, DC_INFO, "item: %d", *item); }

    DC_LOG(log, DC_INFO, "set: %s", DC_DEBUG(int_set_debug, &set));
}

struct complex_data {
    bool flag;
    char* name;
    int value;
};

static void complex_data_delete(struct complex_data* self) { free(self->name); }

static bool complex_data_eq(struct complex_data const* a, struct complex_data const* b) {
    return a->flag == b->flag && strcmp(a->name, b->name) == 0 && a->value == b->value;
}

static size_t complex_data_hash(struct complex_data const* self) {
    const char* name_ptr = self->name;
    size_t hash = dc_fnv1a_str_const(&name_ptr);
    int32_t value = (int32_t)self->value;
    hash = dc_hash_combine(hash, int32_t_hash_id(&value));
    hash = dc_hash_combine(hash, (size_t)self->flag);
    return hash;
}

static void complex_data_debug(struct complex_data const* self, dc_debug_fmt fmt, FILE* stream) {
    fprintf(stream, "complex_data@%p {\n", (void*)self);
    fmt = dc_debug_fmt_scope_begin(fmt);
    dc_debug_fmt_print(fmt, stream, "flag: %s,\n", self->flag ? "true" : "false");
    dc_debug_fmt_print(fmt, stream, "name: %s,\n", self->name);
    dc_debug_fmt_print(fmt, stream, "value: %d,\n", self->value);
    fmt = dc_debug_fmt_scope_end(fmt);
    dc_debug_fmt_print(fmt, stream, "}");
}

#define ITEM struct complex_data
#define ITEM_HASH complex_data_hash
#define ITEM_EQ complex_data_eq
#define ITEM_DELETE complex_data_delete
#define ITEM_DEBUG complex_data_debug
#define NAME complex_set
#include <derive-c/container/set/swiss/template.h>

static void example_complex_set(DC_LOGGER* parent) {
    DC_SCOPED(DC_LOGGER) log = DC_LOGGER_NEW(parent, "%s", __func__);
    DC_SCOPED(complex_set) set = complex_set_new(stdalloc_get_ref());

    DC_LOG(log, DC_INFO, "adding two complex items");
    struct complex_data item1 = {.flag = true, .name = strdup("first"), .value = 42};
    struct complex_data item2 = {.flag = false, .name = strdup("second"), .value = 100};

    complex_set_add(&set, item1);
    complex_set_add(&set, item2);

    struct complex_data lookup = {.flag = true, .name = strdup("first"), .value = 42};
    DC_LOG(log, DC_INFO, "checking if lookup item exists");
    DC_ASSERT(complex_set_contains(&set, lookup));
    free(lookup.name);

    DC_LOG(log, DC_INFO, "set: %s", DC_DEBUG(complex_set_debug, &set));
}

int main() {
    DC_SCOPED(DC_LOGGER)
    root = NS(DC_LOGGER, new_global)(
        (NS(DC_LOGGER, global_config)){.stream = stdout, .ansi_colours = true}, (dc_log_id){"set"});

    example_integer_set(&root);
    example_complex_set(&root);
    return 0;
}
