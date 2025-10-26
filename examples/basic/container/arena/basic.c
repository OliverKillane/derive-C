/// @file
/// @example container/arena/basic.c
/// @brief Examples using arenas with different index sizes.

#include <stdio.h>
#include <stdlib.h>

#include <derive-c/alloc/std.h>
#include <derive-c/core/prelude.h>

#define INDEX_BITS 32
#define VALUE uint32_t
#define NAME ints
#include <derive-c/container/arena/basic/template.h>

void int_example() {
    ints arena = ints_new_with_capacity_for(12, stdalloc_get());
    ints_insert(&arena, 23);
    ints_insert(&arena, 42);
    ints_insert(&arena, 1000);
    ints_insert(&arena, 1001);

    ASSERT(ints_size(&arena) == 4);
    {
        ints_iter_const print_ints = ints_get_iter_const(&arena);
        ints_iv_const const* entry = NULL;
        while ((entry = ints_iter_const_next(&print_ints))) {
            printf("entry for %d at %d\n", *entry->value, entry->index.index);
        }
    }

    {
        ints_iter inc_ints = ints_get_iter(&arena);
        ints_iv const* entry = NULL;
        while ((entry = ints_iter_next(&inc_ints))) {
            printf("incrementing for %d = %d + 1 at %d\n", *entry->value, *entry->value,
                   entry->index.index);
            *entry->value += 1;
        }
    }

    ints_debug(&arena, debug_fmt_new(), stdout);

    ints_delete(&arena);
}

struct foo {
    int x;
    char const* y;
    int* owned_data;
};

void my_foo_delete(struct foo* self) { free(self->owned_data); }

void foo_debug(struct foo const* self, debug_fmt fmt, FILE* stream) {
    (void)fmt;
    fprintf(stream, "foo@%p { x: %d, y: \"%s\", owned_data: @%p { %d }, }", self, self->x, self->y,
            self->owned_data, *self->owned_data);
}

int* new_owned_int(int value) {
    int* v = (int*)malloc(sizeof(int));
    *v = value;
    return v;
}

#define INDEX_BITS 8
#define VALUE struct foo
#define VALUE_DELETE my_foo_delete
#define VALUE_DEBUG foo_debug
#define NAME foo_arena
#include <derive-c/container/arena/basic/template.h>

void foo_example() {
    foo_arena arena = foo_arena_new_with_capacity_for(12, stdalloc_get());
    foo_arena_index_t index_a =
        foo_arena_insert(&arena, (struct foo){.x = 42, .y = "A", .owned_data = new_owned_int(3)});
    foo_arena_index_t index_b =
        foo_arena_insert(&arena, (struct foo){.x = 41, .y = "B", .owned_data = new_owned_int(5)});

    ASSERT(foo_arena_size(&arena) == 2);
    ASSERT(foo_arena_full(&arena) == false);
    ASSERT(foo_arena_read(&arena, index_a)->x == 42);
    ASSERT(foo_arena_read(&arena, index_b)->x == 41);

    foo_arena_write(&arena, index_b)->x = 100;
    ASSERT(foo_arena_read(&arena, index_b)->x == 100);

    foo_arena_debug(&arena, debug_fmt_new(), stdout);

    // we remove the entry, importantly - we now own this data
    struct foo entry_a = foo_arena_remove(&arena, index_a);

    // entry_b was deleted
    foo_arena_delete(&arena);

    // entry a has not yet been deleted, so we can still access and then delete it
    ASSERT(entry_a.x == 42);

    my_foo_delete(&entry_a);
}

int main() {
    int_example();
    foo_example();
}
