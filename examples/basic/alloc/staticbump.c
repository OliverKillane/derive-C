
#include <stdio.h>

#define CAPACITY 1024
#define NAME foopool
#include <derive-c/alloc/staticbump/template.h>

#include <derive-c/core/prelude.h>

void foopool_raw_example() {
    foopool_buffer buf;
    foopool pool = foopool_new(&buf);

    void* ptr1 = foopool_malloc(&pool, 100);
    ASSERT(ptr1 != NULL);

    void* ptr2 = foopool_malloc(&pool, 200);
    ASSERT(ptr2 != NULL);

    // Check the used size
    size_t used = foopool_get_used(&pool);
    ASSERT(used == 100 + 200 + foopool_metadata_size * 2);

    foopool_free(&pool, ptr1);

    // reallocated in place
    void* ptr2_realloc = foopool_realloc(&pool, ptr2, 300);
    ASSERT(ptr2_realloc == ptr2);

    foopool_free(&pool, ptr2_realloc);

    foopool_clear(&pool);

    // we can allocate at capacity:
    void* ptr3 = foopool_malloc(&pool, 1024 - foopool_metadata_size);
    ASSERT(ptr3 != NULL);
    foopool_free(&pool, ptr3);
    foopool_clear(&pool);

    // But we cannot allocate more than capacity
    void* ptr4 = foopool_malloc(&pool, 1024 - foopool_metadata_size + 1);
    ASSERT(ptr4 == NULL);
    ASSERT(foopool_get_used(&pool) == 0);
}

int main() { foopool_raw_example(); }
