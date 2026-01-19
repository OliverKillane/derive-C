/// @file
/// @example alloc/wrap.c
/// @brief Allocator wrapper for composing allocator behavior.

#include <stdio.h>

#include <derive-c/alloc/std.h>
#include <derive-c/prelude.h>

#define ALLOC stdalloc
#define NAME wrapped
#include <derive-c/alloc/wrap/template.h>

static void basic_wrapping() {
    printf("=== Basic Allocator Wrapping ===\n");
    DC_SCOPED(wrapped) alloc = wrapped_new(stdalloc_get_ref());

    void* ptr = wrapped_allocate_uninit(&alloc, 256);
    printf("Allocated 256 bytes through wrapper\n");
    DC_ASSERT(ptr != NULL);

    ptr = wrapped_reallocate(&alloc, ptr, 256, 512);
    printf("Reallocated to 512 bytes\n");
    DC_ASSERT(ptr != NULL);

    wrapped_deallocate(&alloc, ptr, 512);
    printf("Deallocated\n");
}

#define ALLOC stdalloc
#define NAME dbg
#include <derive-c/alloc/debug/template.h>

#define ALLOC dbg
#define NAME monitored
#include <derive-c/alloc/wrap/template.h>

static void with_debug() {
    printf("\n=== Wrapping Debug Allocator ===\n");

    DC_SCOPED(dbg) debug_alloc = dbg_new("wrapped", stdout, stdalloc_get_ref());
    DC_SCOPED(monitored) alloc = monitored_new(dbg_ref_deref(&debug_alloc));

    void* p1 = monitored_allocate_uninit(&alloc, 100);
    void* p2 = monitored_allocate_uninit(&alloc, 200);
    printf("Made 2 allocations through wrapped debug allocator\n");

    printf("\nDebug allocator state:\n");
    dbg_debug(&debug_alloc, dc_debug_fmt_new(), stdout);

    monitored_deallocate(&alloc, p1, 100);
    monitored_deallocate(&alloc, p2, 200);
}

int main() {
    basic_wrapping();
    with_debug();
    return 0;
}
