/// @file
/// @example container/arena.c
/// @brief Examples for using arena containers.

#include <derive-c/alloc/std.h>
#include <derive-c/algorithm/hash/default.h>
#include <derive-c/prelude.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct example_data {
    char* description;
    int value;
};

static void example_data_delete(struct example_data* self) { free(self->description); }

static void example_data_debug(struct example_data const* self, dc_debug_fmt fmt, FILE* stream) {
    fprintf(stream, "example_data@%p {\n", (void*)self);
    fmt = dc_debug_fmt_scope_begin(fmt);
    dc_debug_fmt_print(fmt, stream, "description: %s,\n", self->description);
    dc_debug_fmt_print(fmt, stream, "value: %d,\n", self->value);
    fmt = dc_debug_fmt_scope_end(fmt);
    dc_debug_fmt_print(fmt, stream, "}");
}

#define INDEX_BITS 16
#define VALUE struct example_data
#define VALUE_DELETE example_data_delete
#define VALUE_DEBUG example_data_debug
#define NAME unstable_arena
#include <derive-c/container/arena/contiguous/template.h>

static void example_unstable_arena() {
    DC_DEBUG_TRACE;
    DC_SCOPED(unstable_arena) arena = unstable_arena_new_with_capacity_for(3, stdalloc_get_ref());

    unstable_arena_insert(&arena,
                          (struct example_data){.description = strdup("First"), .value = 1});
    unstable_arena_index_t idx2 = unstable_arena_insert(
        &arena, (struct example_data){.description = strdup("Second"), .value = 2});
    unstable_arena_insert(&arena,
                          (struct example_data){.description = strdup("Third"), .value = 3});

    DC_FOR(unstable_arena, &arena, iter, entry) {
        printf("Entry at index %u: %s = %d\n", entry.index.index, entry.value->description,
               entry.value->value);
    }

    unstable_arena_debug(&arena, dc_debug_fmt_new(), stdout);

    struct example_data removed = unstable_arena_remove(&arena, idx2);
    example_data_delete(&removed);
}

#define NAME dbg
#include <derive-c/alloc/debug/template.h>

#define CAPACITY 512
#define ALLOC dbg
#define NAME hybrid
#include <derive-c/alloc/hybridstatic/template.h>

#define INDEX_BITS 16
#define VALUE struct example_data
#define VALUE_DELETE example_data_delete
#define VALUE_DEBUG example_data_debug
#define ALLOC hybrid
#define NAME custom_arena
#include <derive-c/container/arena/contiguous/template.h>

static void example_custom_allocator_arena() {
    DC_DEBUG_TRACE;
    DC_SCOPED(dbg) debug_alloc = dbg_new("custom_arena", stdout, stdalloc_get_ref());
    hybrid_buffer buf;
    DC_SCOPED(hybrid) hybrid_alloc = hybrid_new(&buf, &debug_alloc);
    DC_SCOPED(custom_arena) arena = custom_arena_new_with_capacity_for(3, &hybrid_alloc);

    custom_arena_insert(&arena, (struct example_data){.description = strdup("A"), .value = 1});
    custom_arena_insert(&arena, (struct example_data){.description = strdup("B"), .value = 2});
    custom_arena_insert(&arena, (struct example_data){.description = strdup("C"), .value = 3});
}

#define INDEX_BITS 32
#define VALUE struct example_data
#define VALUE_DELETE example_data_delete
#define VALUE_DEBUG example_data_debug
#define NAME geometric_arena
#include <derive-c/container/arena/geometric/template.h>

static size_t geometric_arena_index_hash(geometric_arena_index_t const* idx) {
    return DC_DEFAULT_HASH(&idx->index);
}

static bool geometric_arena_index_eq(geometric_arena_index_t const* a,
                                     geometric_arena_index_t const* b) {
    return a->index == b->index;
}

#define KEY geometric_arena_index_t
#define KEY_HASH geometric_arena_index_hash
#define KEY_EQ geometric_arena_index_eq
#define VALUE struct example_data const*
#define NAME index_to_data_map
#include <derive-c/container/map/swiss/template.h>

static void example_geometric_arena() {
    DC_DEBUG_TRACE;
    DC_SCOPED(geometric_arena) arena = geometric_arena_new(stdalloc_get_ref());

    geometric_arena_index_t idx1 = geometric_arena_insert(
        &arena, (struct example_data){.description = strdup("Alpha"), .value = 10});
    geometric_arena_index_t idx2 = geometric_arena_insert(
        &arena, (struct example_data){.description = strdup("Beta"), .value = 20});
    geometric_arena_index_t idx3 = geometric_arena_insert(
        &arena, (struct example_data){.description = strdup("Gamma"), .value = 30});

    DC_SCOPED(index_to_data_map) map = index_to_data_map_new(stdalloc_get_ref());
    index_to_data_map_insert(&map, idx1, geometric_arena_read(&arena, idx1));
    index_to_data_map_insert(&map, idx2, geometric_arena_read(&arena, idx2));
    index_to_data_map_insert(&map, idx3, geometric_arena_read(&arena, idx3));

    geometric_arena_debug(&arena, dc_debug_fmt_new(), stdout);
}

#define INDEX_BITS 8
#define BLOCK_INDEX_BITS 2
#define VALUE struct example_data
#define VALUE_DELETE example_data_delete
#define VALUE_DEBUG example_data_debug
#define NAME chunked_arena
#include <derive-c/container/arena/chunked/template.h>

static void example_chunked_arena() {
    DC_DEBUG_TRACE;
    DC_STATIC_ASSERT(sizeof(chunked_arena_index_t) == sizeof(uint8_t), "Index size must be 8 bits");

    DC_SCOPED(chunked_arena) arena = chunked_arena_new(stdalloc_get_ref());

    chunked_arena_insert(&arena, (struct example_data){.description = strdup("X"), .value = 100});
    chunked_arena_insert(&arena, (struct example_data){.description = strdup("Y"), .value = 200});
}

int main() {
    example_unstable_arena();
    example_custom_allocator_arena();
    example_geometric_arena();
    example_chunked_arena();
    return 0;
}
