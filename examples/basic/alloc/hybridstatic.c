
#include <stdio.h>
#include <derive-c/prelude.h>
#include <derive-c/alloc/std.h>

#define ALLOC stdalloc
#define CAPACITY 1024
#define NAME foopool
#include <derive-c/alloc/hybridstatic/template.h>

static void foopool_raw_example() {
    foopool_buffer buf;
    foopool pool = foopool_new(&buf, stdalloc_get_ref());

    void* ptr1 = foopool_allocate_uninit(&pool, 100);
    DC_ASSERT(ptr1 != NULL);

    void* ptr2 = foopool_allocate_uninit(&pool, 200);
    DC_ASSERT(ptr2 != NULL);

    foopool_deallocate(&pool, ptr1, 100);

    // reallocated in place
    void* ptr2_realloc = foopool_reallocate(&pool, ptr2, 200, 300);
    DC_ASSERT(ptr2_realloc == ptr2);

    foopool_deallocate(&pool, ptr2_realloc, 300);
}

int main() { foopool_raw_example(); }
