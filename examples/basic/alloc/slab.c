/// @file
/// @example alloc/slab.c
/// @brief Slab allocator for fixed-size object pools with freelist.

#include <stdio.h>
#include <string.h>

#include <derive-c/alloc/std.h>
#include <derive-c/prelude.h>

#define ALLOC stdalloc
#define BLOCK_SIZE 64
#define SLAB_SIZE 256
#define NAME object_pool
#include <derive-c/alloc/slab/template.h>

typedef struct {
    int id;
    char name[56]; // Total size fits in 64-byte blocks
} Object;

static void basic_slab_usage() {
    printf("=== Basic Slab Usage ===\n");
    DC_SCOPED(object_pool) pool = object_pool_new(stdalloc_get_ref());

    // Allocate from slab (size <= BLOCK_SIZE)
    Object* obj1 = object_pool_allocate_uninit(&pool, sizeof(Object));
    obj1->id = 1;
    strcpy(obj1->name, "First");
    printf("Allocated object 1 from slab\n");

    Object* obj2 = object_pool_allocate_uninit(&pool, sizeof(Object));
    obj2->id = 2;
    strcpy(obj2->name, "Second");
    printf("Allocated object 2 from slab\n");

    object_pool_deallocate(&pool, obj1, sizeof(Object));
    object_pool_deallocate(&pool, obj2, sizeof(Object));
}

static void freelist_reuse() {
    printf("\n=== Freelist Reuse ===\n");
    DC_SCOPED(object_pool) pool = object_pool_new(stdalloc_get_ref());

    // Allocate and deallocate
    void* ptr1 = object_pool_allocate_uninit(&pool, 32);
    printf("Allocated block 1\n");

    void* ptr2 = object_pool_allocate_uninit(&pool, 32);
    printf("Allocated block 2\n");

    object_pool_deallocate(&pool, ptr1, 32);
    printf("Deallocated block 1 - added to freelist\n");

    // Next allocation should reuse freed block
    void* ptr3 = object_pool_allocate_uninit(&pool, 32);
    printf("Allocated block 3: %s\n",
           ptr3 == ptr1 ? "reused block 1 from freelist ✓" : "new block");
    DC_ASSERT(ptr3 == ptr1); // Should reuse from freelist

    object_pool_deallocate(&pool, ptr2, 32);
    object_pool_deallocate(&pool, ptr3, 32);
}

static void oversized_allocation() {
    printf("\n=== Oversized Allocation (Fallback) ===\n");
    DC_SCOPED(object_pool) pool = object_pool_new(stdalloc_get_ref());

    // Small allocation uses slab
    void* small = object_pool_allocate_uninit(&pool, 32);
    printf("Allocated 32 bytes (uses slab)\n");

    // Large allocation uses backup allocator
    void* large = object_pool_allocate_uninit(&pool, 128);
    printf("Allocated 128 bytes (exceeds BLOCK_SIZE=64, uses backup allocator)\n");

    object_pool_deallocate(&pool, small, 32);
    object_pool_deallocate(&pool, large, 128);
}

static void zeroed_allocation() {
    printf("\n=== Zeroed Allocation ===\n");
    DC_SCOPED(object_pool) pool = object_pool_new(stdalloc_get_ref());

    uint32_t* nums = object_pool_allocate_zeroed(&pool, sizeof(uint32_t) * 10);
    printf("Allocated 10 uint32_t with zeroing\n");

    bool all_zero = true;
    for (int i = 0; i < 10; i++) {
        if (nums[i] != 0) {
            all_zero = false;
            break;
        }
    }
    printf("All values zeroed: %s\n", all_zero ? "yes" : "no");
    DC_ASSERT(all_zero);

    object_pool_deallocate(&pool, nums, sizeof(uint32_t) * 10);
}

static void reset_all_slabs() {
    printf("\n=== Reset All Slabs ===\n");
    DC_SCOPED(object_pool) pool = object_pool_new(stdalloc_get_ref());

    // Allocate several objects
    for (int i = 0; i < 8; i++) {
        void* ptr = object_pool_allocate_uninit(&pool, 32);
        printf("Allocated object %d\n", i + 1);
        object_pool_deallocate(&pool, ptr, 32);
    }

    // Reset clears all slabs and frees memory
    object_pool_reset(&pool);
    printf("Called reset() - all slabs freed, freelist cleared\n");

    // Can allocate again after reset
    void* ptr = object_pool_allocate_uninit(&pool, 32);
    printf("New allocation after reset\n");
    DC_ASSERT(ptr != NULL);

    object_pool_deallocate(&pool, ptr, 32);
}

int main() {
    basic_slab_usage();
    freelist_reuse();
    oversized_allocation();
    zeroed_allocation();
    reset_all_slabs();
    return 0;
}
