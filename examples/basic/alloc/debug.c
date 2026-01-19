/// @file
/// @example alloc/debug.c
/// @brief Debug allocator wrapper for tracking allocations and detecting leaks.

#include <stdio.h>

#include <derive-c/alloc/std.h>
#include <derive-c/prelude.h>

#define ALLOC stdalloc
#define NAME dbg_alloc
#include <derive-c/alloc/debug/template.h>

static void tracked_allocations() {
    printf("=== Tracked Allocations ===\n");
    DC_SCOPED(dbg_alloc) alloc = dbg_alloc_new("test", stdout, stdalloc_get_ref());

    void* ptr1 = dbg_alloc_allocate_uninit(&alloc, 100);
    printf("Allocated 100 bytes\n");

    void* ptr2 = dbg_alloc_allocate_uninit(&alloc, 200);
    printf("Allocated 200 bytes\n");

    void* ptr3 = dbg_alloc_allocate_uninit(&alloc, 50);
    printf("Allocated 50 bytes\n");

    printf("\nAllocator state:\n");
    dbg_alloc_debug(&alloc, dc_debug_fmt_new(), stdout);

    dbg_alloc_deallocate(&alloc, ptr2, 200);
    printf("\nDeallocated 200 bytes\n");

    printf("\nFinal state:\n");
    dbg_alloc_debug(&alloc, dc_debug_fmt_new(), stdout);

    dbg_alloc_deallocate(&alloc, ptr1, 100);
    dbg_alloc_deallocate(&alloc, ptr3, 50);
}

static void reallocation_tracking() {
    printf("\n\n=== Reallocation Tracking ===\n");
    DC_SCOPED(dbg_alloc) alloc = dbg_alloc_new("realloc", stdout, stdalloc_get_ref());

    void* ptr = dbg_alloc_allocate_uninit(&alloc, 100);
    printf("Initial allocation: 100 bytes\n");

    ptr = dbg_alloc_reallocate(&alloc, ptr, 100, 500);
    printf("Reallocated to 500 bytes\n");

    printf("\nAllocator state:\n");
    dbg_alloc_debug(&alloc, dc_debug_fmt_new(), stdout);

    dbg_alloc_deallocate(&alloc, ptr, 500);
}

int main() {
    tracked_allocations();
    reallocation_tracking();
    return 0;
}
