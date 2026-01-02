
#include <stdio.h>
#include <derive-c/core/prelude.h>
#include <derive-c/alloc/std.h>

#define ALLOC stdalloc
#define CAPACITY 1024
#define NAME foopool
#include <derive-c/alloc/hybridstatic/template.h>

void foopool_raw_example() {
    foopool_buffer buf;
    foopool pool = foopool_new(&buf, stdalloc_get_ref());

    void* ptr1 = foopool_malloc(&pool, 100);
    DC_ASSERT(ptr1 != NULL);

    void* ptr2 = foopool_malloc(&pool, 200);
    DC_ASSERT(ptr2 != NULL);

    foopool_free(&pool, ptr1);

    // reallocated in place
    void* ptr2_realloc = foopool_realloc(&pool, ptr2, 300);
    DC_ASSERT(ptr2_realloc == ptr2);
}

int main() { foopool_raw_example(); }
