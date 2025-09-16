#include <gtest/gtest.h>

namespace debugalloc {

extern "C" {
#include <derive-c/alloc/std.h>

#define ALLOC stdalloc
#define NAME stddebugalloc
#include <derive-c/alloc/debug/template.h>
}

TEST(DebugAlloc, BasicAllocation) {
    stddebugalloc alloc = stddebugalloc_new("Test Allocator", stdalloc_get());

    void* malloced = stddebugalloc_malloc(&alloc, 100);
    void* calloced = stddebugalloc_calloc(&alloc, 10, 10);
    stddebugalloc_free(&alloc, malloced);
    stddebugalloc_free(&alloc, calloced);
}

} // namespace debugalloc