#include <gtest/gtest.h>

namespace staticbumpalloc {

extern "C" {
#include <derive-c/core/debug/memory_tracker.h>

#define CAPACITY 1024
#define NAME staticbumpalloc
#include <derive-c/alloc/staticbump/template.h>
}

TEST(StaticBumpAlloc, BasicAllocation) {
    staticbumpalloc alloc = staticbumpalloc_new();

    void* ptr1 = staticbumpalloc_malloc(&alloc, 100);
    ASSERT_NE(ptr1, nullptr);
    memory_tracker_check(MEMORY_TRACKER_LVL_ALLOC, MEMORY_TRACKER_CAP_WRITE, ptr1, 100);

    void* ptr2 = staticbumpalloc_malloc(&alloc, 200);
    ASSERT_NE(ptr2, nullptr);
    memory_tracker_check(MEMORY_TRACKER_LVL_ALLOC, MEMORY_TRACKER_CAP_WRITE, ptr2, 200);

    // Check the used size
    size_t used = staticbumpalloc_get_used(&alloc);
    ASSERT_EQ(used, 100 + 200 + (staticbumpalloc_metadata_size * 2));

    staticbumpalloc_free(&alloc, ptr1);
    memory_tracker_check(MEMORY_TRACKER_LVL_ALLOC, MEMORY_TRACKER_CAP_NONE, ptr1, 100);

    // reallocated in place
    void* ptr2_realloc = staticbumpalloc_realloc(&alloc, ptr2, 300);
    ASSERT_EQ(ptr2_realloc, ptr2);
    memory_tracker_check(MEMORY_TRACKER_LVL_ALLOC, MEMORY_TRACKER_CAP_WRITE, ptr2, 300);

    staticbumpalloc_free(&alloc, ptr2_realloc);
    memory_tracker_check(MEMORY_TRACKER_LVL_ALLOC, MEMORY_TRACKER_CAP_NONE, ptr2_realloc, 300);

    staticbumpalloc_clear(&alloc);

    // we can allocate at capacity:
    void* ptr3 = staticbumpalloc_malloc(&alloc, 1024 - staticbumpalloc_metadata_size);
    ASSERT_NE(ptr3, nullptr);
    memory_tracker_check(MEMORY_TRACKER_LVL_ALLOC, MEMORY_TRACKER_CAP_WRITE, ptr3,
                         1024 - staticbumpalloc_metadata_size);
    staticbumpalloc_free(&alloc, ptr3);
    memory_tracker_check(MEMORY_TRACKER_LVL_ALLOC, MEMORY_TRACKER_CAP_NONE, ptr3,
                         1024 - staticbumpalloc_metadata_size);
    staticbumpalloc_clear(&alloc);

    // But we cannot allocate more than capacity
    void* ptr4 = staticbumpalloc_malloc(&alloc, 1024 - staticbumpalloc_metadata_size + 1);
    memory_tracker_check(MEMORY_TRACKER_LVL_ALLOC, MEMORY_TRACKER_CAP_WRITE, ptr4,
                         1024 - staticbumpalloc_metadata_size + 1);
    ASSERT_EQ(ptr4, nullptr);
    ASSERT_EQ(staticbumpalloc_get_used(&alloc), 0);
}

} // namespace staticbumpalloc