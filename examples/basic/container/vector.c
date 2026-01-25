/// @file
/// @example container/vector.c
/// @brief Examples for using vector containers.

#include <derive-c/alloc/std.h>
#include <derive-c/algorithm/hash/default.h>
#include <derive-c/prelude.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ITEM int
#define NAME basic_vec
#include <derive-c/container/vector/dynamic/template.h>

static void example_basic() {
    DC_DEBUG_TRACE;
    DC_SCOPED(basic_vec) vec = basic_vec_new(stdalloc_get_ref());

    for (int i = 0; i < 10; i++) {
        basic_vec_push(&vec, i);
    }

    basic_vec_debug(&vec, dc_debug_fmt_new(), stdout);
    basic_vec_pop(&vec);
    basic_vec_debug(&vec, dc_debug_fmt_new(), stdout);
}

#define ITEM int
#define NAME dynamic_vec
#include <derive-c/container/vector/dynamic/template.h>

static void example_dynamic() {
    DC_DEBUG_TRACE;
    DC_SCOPED(dynamic_vec) vec = dynamic_vec_new(stdalloc_get_ref());

    for (int i = 0; i < 10; i++) {
        dynamic_vec_push(&vec, i);
    }

    dynamic_vec_debug(&vec, dc_debug_fmt_new(), stdout);
}

#define ITEM int
#define CAPACITY 3
#define NAME static_vec
#include <derive-c/container/vector/static/template.h>

static void example_static() {
    DC_DEBUG_TRACE;
    DC_SCOPED(static_vec) vec = static_vec_new();

    static_vec_push(&vec, 1);
    static_vec_push(&vec, 2);
    static_vec_push(&vec, 3);

    int* result = static_vec_try_push(&vec, 4);
    DC_ASSERT(result == NULL);

    DC_FOR_CONST(static_vec, &vec, iter, item) { printf("%d ", *item); }
    printf("\n");
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

static void example_map() {
    DC_DEBUG_TRACE;
    DC_SCOPED(str_to_vec_map) map = str_to_vec_map_new(stdalloc_get_ref());

    char_vec empty_vec = char_vec_new(stdalloc_get_ref());
    char* key = strdup("key1");
    str_to_vec_map_insert(&map, key, empty_vec);

    char_vec* vec_ptr = str_to_vec_map_write(&map, key);
    char_vec_push(vec_ptr, strdup("value1"));
    char_vec_push(vec_ptr, strdup("value2"));
    char_vec_push(vec_ptr, strdup("value3"));

    str_to_vec_map_debug(&map, dc_debug_fmt_new(), stdout);
}

int main() {
    example_basic();
    example_dynamic();
    example_static();
    example_map();
    return 0;
}
