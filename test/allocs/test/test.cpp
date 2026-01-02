
#include <gtest/gtest.h>

#include <derive-c/alloc/std.h>

#define ALLOC stdalloc
#define ENTRIES allocs
#define NAME stdtestalloc
#include <derive-c/alloc/test/template.h>

void allocate_and_throw(stdtestalloc* alloc) {
    void* a = stdtestalloc_malloc(alloc, 10000000);
    ((int*)a)[12] = 42;
    DC_PANIC("problem!");
}

#if !defined NDEBUG
TEST(TestAlloc, BasicAllocation) {
    stdtestalloc alloc = stdtestalloc_new(stdalloc_get_ref());
    EXPECT_ANY_THROW(allocate_and_throw(&alloc));
    stdtestalloc_unleak_and_delete(&alloc);
}
#endif
