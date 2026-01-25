#include <stdio.h>

#include <derive-c/alloc/std.h>
#include <derive-c/prelude.h>

static void example_std() {
    DC_DEBUG_TRACE;
    void* ptr = stdalloc_allocate_uninit(stdalloc_get_ref(), 256);
    ptr = stdalloc_reallocate(stdalloc_get_ref(), ptr, 256, 512);
    stdalloc_deallocate(stdalloc_get_ref(), ptr, 512);
}

#define CAPACITY 512
#define NAME hybrid
#include <derive-c/alloc/hybridstatic/template.h>

static void example_hybridstatic() {
    DC_DEBUG_TRACE;
    hybrid_buffer buf;
    DC_SCOPED(hybrid) alloc = hybrid_new(&buf, stdalloc_get_ref());

    void* ptr = hybrid_allocate_uninit(&alloc, 100);
    ptr = hybrid_reallocate(&alloc, ptr, 100, 200);
    hybrid_deallocate(&alloc, ptr, 200);
}

// We can only unleak in debug builds
#if !defined(NDEBUG)
#define NAME test_alloc
#include <derive-c/alloc/test/template.h>

static void example_test() {
    DC_DEBUG_TRACE;
    DC_SCOPED(test_alloc) alloc = test_alloc_new(stdalloc_get_ref());

    test_alloc_allocate_uninit(&alloc, 128);
    test_alloc_debug(&alloc, dc_debug_fmt_new(), stdout);
    test_alloc_unleak(&alloc);
}
#else
static void example_test() {}
#endif

#define NAME dbg
#include <derive-c/alloc/debug/template.h>

#define ALLOC dbg
#define BLOCK_SIZE 1024
#define SLAB_SIZE 8096
#define NAME slab_large
#include <derive-c/alloc/slab/template.h>

#define ALLOC slab_large
#define NAME slab_large_alloc_dbg
#include <derive-c/alloc/debug/template.h>

#define ALLOC slab_large_alloc_dbg
#define BLOCK_SIZE 32
#define SLAB_SIZE 8096
#define NAME slab_small
#include <derive-c/alloc/slab/template.h>

#define ALLOC slab_small
#define NAME slab_small_alloc_dbg
#include <derive-c/alloc/debug/template.h>

static void example_slab() {
    DC_DEBUG_TRACE;
    DC_SCOPED(dbg) user_alloc = dbg_new("user_alloc", stdout, stdalloc_get_ref());
    DC_SCOPED(slab_large) large_slab = slab_large_new(&user_alloc);
    DC_SCOPED(slab_large_alloc_dbg)
    slab_large_alloc = slab_large_alloc_dbg_new("slab_large alloc", stdout, &large_slab);
    DC_SCOPED(slab_small) small_slab = slab_small_new(&slab_large_alloc);
    DC_SCOPED(slab_small_alloc_dbg)
    slab_small_alloc = slab_small_alloc_dbg_new("slab_small alloc", stdout, &small_slab);

    void* ptr1 = slab_small_alloc_dbg_allocate_uninit(&slab_small_alloc, 32);
    void* ptr2 =
        slab_small_alloc_dbg_allocate_uninit(&slab_small_alloc, 64); // Larger than block size (32)
    void* ptr3 = slab_small_alloc_dbg_allocate_uninit(&slab_small_alloc,
                                                      2048); // Larger than large block size (1024)

    dbg_debug(&user_alloc, dc_debug_fmt_new(), stdout);

    slab_small_alloc_dbg_deallocate(&slab_small_alloc, ptr1, 32);
    slab_small_alloc_dbg_deallocate(&slab_small_alloc, ptr2, 64);
    slab_small_alloc_dbg_deallocate(&slab_small_alloc, ptr3, 2048);
}

#define ALLOC dbg
#define BLOCK_SIZE 256
#define NAME chunked
#include <derive-c/alloc/chunkedbump/template.h>

static void example_chunkedbump() {
    DC_DEBUG_TRACE;
    DC_SCOPED(dbg) debug_alloc = dbg_new("chunked_example", stdout, stdalloc_get_ref());
    DC_SCOPED(chunked) alloc = chunked_new(&debug_alloc);

    void* small = chunked_allocate_uninit(&alloc, 64);
    void* large = chunked_allocate_uninit(&alloc, 512);

    dbg_debug(&debug_alloc, dc_debug_fmt_new(), stdout);

    chunked_deallocate(&alloc, small, 64);
    chunked_deallocate(&alloc, large, 512);
}

int main() {
    example_std();
    example_hybridstatic();
    example_test();
    example_slab();
    example_chunkedbump();
    return 0;
}
