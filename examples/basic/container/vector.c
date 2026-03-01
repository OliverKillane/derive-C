/// @file
/// @example container/vector.c
/// @brief Examples for using vector containers.

#include <derive-c/alloc/std.h>
#include <derive-c/algorithm/hash/default.h>
#include <derive-c/prelude.h>
#include <stdlib.h>
#include <string.h>

#define ITEM int
#define NAME basic_vec
#include <derive-c/container/vector/dynamic/template.h>

static void example_basic(DC_LOGGER* parent) {
    DC_SCOPED(DC_LOGGER) log = DC_LOGGER_NEW(parent, "%s", __func__);
    DC_SCOPED(basic_vec) vec = basic_vec_new(stdalloc_get_ref());

    DC_LOG(log, DC_INFO, "pushing 10 integers");
    for (int i = 0; i < 10; i++) {
        basic_vec_push(&vec, i);
    }

    DC_LOG(log, DC_INFO, "vector: %s", DC_DEBUG(basic_vec_debug, &vec));
    basic_vec_pop(&vec);
    DC_LOG(log, DC_INFO, "after pop: %s", DC_DEBUG(basic_vec_debug, &vec));
}

#define ITEM int
#define NAME dynamic_vec
#include <derive-c/container/vector/dynamic/template.h>

static void example_dynamic(DC_LOGGER* parent) {
    DC_SCOPED(DC_LOGGER) log = DC_LOGGER_NEW(parent, "%s", __func__);
    DC_SCOPED(dynamic_vec) vec = dynamic_vec_new(stdalloc_get_ref());

    DC_LOG(log, DC_INFO, "pushing 10 integers");
    for (int i = 0; i < 10; i++) {
        dynamic_vec_push(&vec, i);
    }

    DC_LOG(log, DC_INFO, "vector: %s", DC_DEBUG(dynamic_vec_debug, &vec));
}

#define ITEM int
#define CAPACITY 3
#define NAME static_vec
#include <derive-c/container/vector/static/template.h>

static void example_static(DC_LOGGER* parent) {
    DC_SCOPED(DC_LOGGER) log = DC_LOGGER_NEW(parent, "%s", __func__);
    DC_SCOPED(static_vec) vec = static_vec_new();

    DC_LOG(log, DC_INFO, "pushing 3 integers to static vec (capacity 3)");
    static_vec_push(&vec, 1);
    static_vec_push(&vec, 2);
    static_vec_push(&vec, 3);

    int* result = static_vec_try_push(&vec, 4);
    DC_ASSERT(result == NULL);
    DC_LOG(log, DC_INFO, "try_push returned NULL as expected (capacity full)");

    DC_FOR_CONST(static_vec, &vec, iter, item) { DC_LOG(log, DC_INFO, "item: %d", *item); }
}

#define ITEM char*
#define ITEM_DELETE(ptr_to_str) free(*ptr_to_str)
#define NAME char_vec
#include <derive-c/container/vector/dynamic/template.h>

#define KEY char*
#define KEY_DELETE(ptr_to_str) free(*ptr_to_str)
#define KEY_HASH DC_DEFAULT_HASH
#define VALUE char_vec
#define VALUE_DELETE char_vec_delete
#define VALUE_DEBUG char_vec_debug
#define VALUE_CLONE char_vec_clone
#define NAME str_to_vec_map
#include <derive-c/container/map/ankerl/template.h>

static void example_map(DC_LOGGER* parent) {
    DC_SCOPED(DC_LOGGER) log = DC_LOGGER_NEW(parent, "%s", __func__);
    DC_SCOPED(str_to_vec_map) map = str_to_vec_map_new(stdalloc_get_ref());

    DC_LOG(log, DC_INFO, "inserting key1 with empty vector");
    char_vec empty_vec = char_vec_new(stdalloc_get_ref());
    char* key = strdup("key1");
    str_to_vec_map_insert(&map, key, empty_vec);

    DC_LOG(log, DC_INFO, "pushing values to key1's vector");
    char_vec* vec_ptr = str_to_vec_map_write(&map, key);
    char_vec_push(vec_ptr, strdup("value1"));
    char_vec_push(vec_ptr, strdup("value2"));
    char_vec_push(vec_ptr, strdup("value3"));

    DC_LOG(log, DC_INFO, "map: %s", DC_DEBUG(str_to_vec_map_debug, &map));
}

int main() {
    DC_SCOPED(DC_LOGGER)
    root = NS(DC_LOGGER,
              new_global)((NS(DC_LOGGER, global_config)){.stream = stdout, .ansi_colours = true},
                          (dc_log_id){"vector"});

    example_basic(&root);
    example_dynamic(&root);
    example_static(&root);
    example_map(&root);
    return 0;
}
