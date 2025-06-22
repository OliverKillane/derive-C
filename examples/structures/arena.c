/// @file
/// @example structures/arena.c
/// @brief Examples using arenas with different index sizes.

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <derive-c/macros/iterators.h>

#define INDEX_BITS 32
#define V uint32_t
#define SELF ints
#include <derive-c/structures/arena/template.h>

void int_example() {
    ints arena = ints_new_with_capacity_for(12);
    ints_insert(&arena, 23);
    ints_insert(&arena, 42);
    ints_insert(&arena, 1000);
    ints_insert(&arena, 1001);

    assert(ints_size(&arena) == 4);

    ints_iter_const print_ints = ints_get_iter_const(&arena);
    ITER_LOOP(ints_iter_const, print_ints, ints_iv_const, entry) {
        printf("entry for %d at %d", *entry.value, entry.index.index);
    }

    ints_iter inc_ints = ints_get_iter(&arena);
    ITER_LOOP(ints_iter, inc_ints, ints_iv, entry) {
        printf("incrementing for %d = %d + 1 at %d", *entry.value, *entry.value, entry.index.index);
        *entry.value += 1;
    }

    ints_delete(&arena);
}

struct foo {
    int x;
    char const* y;
    int* owned_data;
};

void my_foo_delete(struct foo* self) { free(self->owned_data); }

#define INDEX_BITS 8
#define V struct foo
#define V_DELETE my_foo_delete
#define SELF foo_arena
#include <derive-c/structures/arena/template.h>

void foo_example() {
    foo_arena arena = foo_arena_new_with_capacity_for(12);
    foo_arena_index index_a = foo_arena_insert(
        &arena, (struct foo){.x = 42, .y = "A", .owned_data = (int*)malloc(sizeof(int))});
    foo_arena_index index_b = foo_arena_insert(
        &arena, (struct foo){.x = 41, .y = "B", .owned_data = (int*)malloc(sizeof(int))});

    assert(foo_arena_size(&arena) == 2);
    assert(foo_arena_full(&arena) == false);
    assert(foo_arena_read(&arena, index_a)->x == 42);
    assert(foo_arena_read(&arena, index_b)->x == 41);

    foo_arena_write(&arena, index_b)->x = 100;
    assert(foo_arena_read(&arena, index_b)->x == 100);

    // we remove the entry, improtantly - we now own this data
    struct foo entry_a = foo_arena_remove(&arena, index_a);

    // entry_b was deleted
    foo_arena_delete(&arena);

    // entry a has not yet been deleted, so we can still access and then delete it
    assert(entry_a.x == 42);

    my_foo_delete(&entry_a);
}

int main() {
    int_example();
    foo_example();
}
