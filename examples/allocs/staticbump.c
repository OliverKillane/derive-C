
#include <assert.h>
#include <stdio.h>

#define CAPACITY 1024
#define SELF foopool
#include <derive-c/allocs/staticbump/template.h>

void foopool_raw_example() {
    foopool pool = foopool_new();

    void* ptr1 = foopool_malloc(&pool, 100);
    assert(ptr1 != NULL);

    void* ptr2 = foopool_malloc(&pool, 200);
    assert(ptr2 != NULL);

    // Check the used size
    size_t used = foopool_get_used(&pool);
    assert(used == 100 + 200 + foopool_metadata_size * 2);

    foopool_free(&pool, ptr1);

    // reallocated in place
    void* ptr2_realloc = foopool_realloc(&pool, ptr2, 300);
    assert(ptr2_realloc == ptr2);

    foopool_free(&pool, ptr2_realloc);

    foopool_clear(&pool);

    // we can allocate at capacity:
    void* ptr3 = foopool_malloc(&pool, 1024 - foopool_metadata_size);
    assert(ptr3 != NULL);
    foopool_free(&pool, ptr3);
    foopool_clear(&pool);

    // But we cannot allocate more than capacity
    void* ptr4 = foopool_malloc(&pool, 1024 - foopool_metadata_size + 1);
    assert(ptr4 == NULL);
    assert(foopool_get_used(&pool) == 0);
}

int main() { foopool_raw_example(); }
