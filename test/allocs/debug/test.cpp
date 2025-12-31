#include <gtest/gtest.h>

#include <derive-c/alloc/std.h>
#include <derive-c/utils/null_stream.h>

#define ALLOC stdalloc
#define NAME stddebugalloc
#include <derive-c/alloc/debug/template.h>

TEST(DebugAlloc, BasicAllocation) {
    FILE* stream = dc_null_stream();
    stddebugalloc alloc = stddebugalloc_new("Test Allocator", stream, stdalloc_get());

    void* malloced = stddebugalloc_malloc(&alloc, 100);
    void* calloced = stddebugalloc_calloc(&alloc, 10, 10);
    stddebugalloc_free(&alloc, malloced);
    stddebugalloc_free(&alloc, calloced);
    stddebugalloc_delete(&alloc);
    fclose(stream);
}
