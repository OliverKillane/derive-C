/// @file
/// @example alloc/chunkedbump.c
/// @brief Chunked bump allocator for fast sequential allocations.

#include <stdio.h>
#include <string.h>

#include <derive-c/alloc/std.h>
#include <derive-c/prelude.h>

#define ALLOC stdalloc
#define BLOCK_SIZE 256
#define NAME arena
#include <derive-c/alloc/chunkedbump/template.h>

static void sequential_allocations() {
    printf("=== Sequential Allocations ===\n");
    DC_SCOPED(arena) a = arena_new(stdalloc_get_ref());

    // Allocate several small objects
    for (int i = 0; i < 5; i++) {
        void* ptr = arena_allocate_uninit(&a, 32);
        printf("Allocated 32 bytes (allocation %d)\n", i + 1);
        DC_ASSERT(ptr != NULL);

        // Note: bump allocators don't reclaim memory on deallocate,
        // but we still call it for proper memory tracking
        arena_deallocate(&a, ptr, 32);
    }

    printf("All allocations served from bump pointer\n");
}

static void large_allocation_spanning_blocks() {
    printf("\n=== Large Allocation Spanning Multiple Blocks ===\n");
    DC_SCOPED(arena) a = arena_new(stdalloc_get_ref());

    // This will span 2 blocks (256 bytes each)
    void* large = arena_allocate_uninit(&a, 400);
    printf("Allocated 400 bytes (spans 2 blocks of 256 bytes)\n");
    DC_ASSERT(large != NULL);

    arena_deallocate(&a, large, 400);
}

static void in_place_reallocation() {
    printf("\n=== In-Place Reallocation ===\n");
    DC_SCOPED(arena) a = arena_new(stdalloc_get_ref());

    // Allocate and immediately reallocate the last allocation
    void* ptr = arena_allocate_uninit(&a, 50);
    printf("Allocated 50 bytes\n");
    memset(ptr, 0xAA, 50);

    // Reallocate to 80 bytes - should extend in place (it's the last allocation)
    void* ptr2 = arena_reallocate(&a, ptr, 50, 80);
    printf("Reallocated to 80 bytes: %s\n", ptr2 == ptr ? "in-place ✓" : "moved");
    DC_ASSERT(ptr2 == ptr); // Should be in-place

    // Verify original data is intact
    unsigned char* bytes = (unsigned char*)ptr2;
    bool data_intact = true;
    for (int i = 0; i < 50; i++) {
        if (bytes[i] != 0xAA) {
            data_intact = false;
            break;
        }
    }
    printf("Original data intact: %s\n", data_intact ? "yes" : "no");
    DC_ASSERT(data_intact);

    arena_deallocate(&a, ptr2, 80);
}

static void reset_and_reuse() {
    printf("\n=== Reset and Reuse ===\n");
    DC_SCOPED(arena) a = arena_new(stdalloc_get_ref());

    // First batch of allocations
    void* p1 = arena_allocate_uninit(&a, 64);
    void* p2 = arena_allocate_uninit(&a, 64);
    void* p3 = arena_allocate_uninit(&a, 64);
    printf("Allocated 3x64 bytes\n");

    arena_deallocate(&a, p1, 64);
    arena_deallocate(&a, p2, 64);
    arena_deallocate(&a, p3, 64);

    // Reset clears all allocations and reuses the blocks
    arena_reset(&a);
    printf("Called reset() - blocks are freed and can be reused\n");

    // Second batch reuses the memory
    void* p4 = arena_allocate_uninit(&a, 64);
    printf("New allocation after reset\n");
    DC_ASSERT(p4 != NULL);

    arena_deallocate(&a, p4, 64);
}

int main() {
    sequential_allocations();
    large_allocation_spanning_blocks();
    in_place_reallocation();
    reset_and_reuse();
    return 0;
}
