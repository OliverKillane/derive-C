#include <gtest/gtest.h>

namespace staticbumpalloc {

extern "C" {
#define CAPACITY 1024
#define NAME staticbumpalloc
#include <derive-c/allocs/staticbump/template.h>
}

TEST(StaticBumpAlloc, BasicAllocation) {
    staticbumpalloc alloc = staticbumpalloc_new();

    void* ptr1 = staticbumpalloc_malloc(&alloc, 100);
    ASSERT_NE(ptr1, nullptr);

    void* ptr2 = staticbumpalloc_malloc(&alloc, 200);
    ASSERT_NE(ptr2, nullptr);

    // Check the used size
    size_t used = staticbumpalloc_get_used(&alloc);
    ASSERT_EQ(used, 100 + 200 + staticbumpalloc_metadata_size * 2);

    staticbumpalloc_free(&alloc, ptr1);

    // reallocated in place
    void* ptr2_realloc = staticbumpalloc_realloc(&alloc, ptr2, 300);
    ASSERT_EQ(ptr2_realloc, ptr2);

    staticbumpalloc_free(&alloc, ptr2_realloc);

    staticbumpalloc_clear(&alloc);

    // we can allocate at capacity:
    void* ptr3 = staticbumpalloc_malloc(&alloc, 1024 - staticbumpalloc_metadata_size);
    ASSERT_NE(ptr3, nullptr);
    staticbumpalloc_free(&alloc, ptr3);
    staticbumpalloc_clear(&alloc);

    // But we cannot allocate more than capacity
    void* ptr4 = staticbumpalloc_malloc(&alloc, 1024 - staticbumpalloc_metadata_size + 1);
    ASSERT_EQ(ptr4, nullptr);
    ASSERT_EQ(staticbumpalloc_get_used(&alloc), 0);
}

} // namespace staticbumpalloc