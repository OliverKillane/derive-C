/// @file
/// @example alloc/hybridstatic.c
/// @brief Hybrid static/dynamic allocator with stack buffer fallback.

#include <stdio.h>

#include <derive-c/alloc/std.h>
#include <derive-c/prelude.h>

#define ALLOC stdalloc
#define CAPACITY 512
#define NAME pool
#include <derive-c/alloc/hybridstatic/template.h>

static void small_allocations() {
    printf("=== Small Allocations (stays in stack buffer) ===\n");
    pool_buffer buf;
    DC_SCOPED(pool) p = pool_new(&buf, stdalloc_get_ref());

    void* ptr1 = pool_allocate_uninit(&p, 100);
    printf("Allocated 100 bytes\n");
    DC_ASSERT(ptr1 != NULL);

    void* ptr2 = pool_allocate_uninit(&p, 200);
    printf("Allocated 200 bytes\n");
    DC_ASSERT(ptr2 != NULL);

    pool_deallocate(&p, ptr1, 100);
    printf("Deallocated first 100 bytes\n");

    void* ptr2_realloc = pool_reallocate(&p, ptr2, 200, 300);
    printf("Reallocated to 300 bytes: %s\n", ptr2_realloc == ptr2 ? "in-place" : "moved");

    pool_deallocate(&p, ptr2_realloc, 300);
}

static void overflow_to_heap() {
    printf("\n=== Overflow to Heap ===\n");
    pool_buffer buf;
    DC_SCOPED(pool) p = pool_new(&buf, stdalloc_get_ref());

    void* large = pool_allocate_uninit(&p, 1024);
    printf("Allocated 1024 bytes (exceeds 512 capacity, uses heap)\n");
    DC_ASSERT(large != NULL);

    pool_deallocate(&p, large, 1024);
    printf("Deallocated heap allocation\n");
}

static void zeroed_allocation() {
    printf("\n=== Zeroed Allocation ===\n");
    pool_buffer buf;
    DC_SCOPED(pool) p = pool_new(&buf, stdalloc_get_ref());

    uint32_t* nums = pool_allocate_zeroed(&p, sizeof(uint32_t) * 10);
    printf("Allocated 10 uint32_t, checking if zeroed: ");
    bool all_zero = true;
    for (int i = 0; i < 10; i++) {
        if (nums[i] != 0) {
            all_zero = false;
            break;
        }
    }
    printf("%s\n", all_zero ? "yes" : "no");
    DC_ASSERT(all_zero);

    pool_deallocate(&p, nums, sizeof(uint32_t) * 10);
}

int main() {
    small_allocations();
    overflow_to_heap();
    zeroed_allocation();
    return 0;
}
