/// @file
/// @example container/arena/chunked.c
/// @brief Chunked arena with slot reuse.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <derive-c/alloc/std.h>
#include <derive-c/prelude.h>
#include <derive-c/utils/for.h>

#define INDEX_BITS 16
#define BLOCK_INDEX_BITS 2
#define VALUE int
#define NAME int_arena
#include <derive-c/container/arena/chunked/template.h>

static void basic_chunked() {
    printf("=== Chunked Arena ===\n");
    DC_SCOPED(int_arena) arena = int_arena_new(stdalloc_get_ref());

    int_arena_index_t indices[16];
    for (int i = 0; i < 16; i++) {
        indices[i] = int_arena_insert(&arena, i * 10);
    }

    printf("Size: %zu\n", int_arena_size(&arena));

    *int_arena_write(&arena, indices[5]) = 999;
    printf("Modified index[5]: %d\n", *int_arena_read(&arena, indices[5]));

    int removed = int_arena_remove(&arena, indices[3]);
    printf("Removed: %d, new size: %zu\n", removed, int_arena_size(&arena));

    int_arena_index_t reused = int_arena_insert(&arena, 777);
    printf("Reused slot [%u]: %d\n", reused.index, *int_arena_read(&arena, reused));
}

struct data {
    char* msg;
    int val;
};

static void data_delete(struct data* d) { free(d->msg); }

static void data_debug(struct data const* d, dc_debug_fmt fmt, FILE* stream) {
    (void)fmt;
    fprintf(stream, "{ \"%s\", %d }", d->msg, d->val);
}

#define INDEX_BITS 8
#define BLOCK_INDEX_BITS 2
#define VALUE struct data
#define VALUE_DELETE data_delete
#define VALUE_DEBUG data_debug
#define NAME data_arena
#include <derive-c/container/arena/chunked/template.h>

static void owned_data() {
    printf("\n=== Chunked Arena with Owned Data ===\n");
    DC_SCOPED(data_arena) arena = data_arena_new(stdalloc_get_ref());

    data_arena_insert(&arena, (struct data){.msg = strdup("First"), .val = 1});
    data_arena_index_t idx2 =
        data_arena_insert(&arena, (struct data){.msg = strdup("Second"), .val = 2});
    data_arena_insert(&arena, (struct data){.msg = strdup("Third"), .val = 3});

    printf("Size: %zu\n", data_arena_size(&arena));

    printf("\nData:\n");
    DC_FOR_CONST(data_arena, &arena, iter, entry) {
        printf("  [%u]: \"%s\" = %d\n", entry.index.index, entry.value->msg, entry.value->val);
    }

    struct data removed = data_arena_remove(&arena, idx2);
    printf("\nRemoved: \"%s\" = %d\n", removed.msg, removed.val);
    data_delete(&removed);
}

int main() {
    basic_chunked();
    owned_data();
    return 0;
}
