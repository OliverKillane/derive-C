#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <derive-c/core/debug/memory_tracker.h>
#include <derive-c/alloc/std.h>
#include <derive-c/utils/debug.h>

#include <derive-cpp/test/trampoline.hpp>
#include <derive-cpp/test/gtest_mock.hpp>
#include <derive-cpp/fmt/remove_ptrs.hpp>

#define ALLOC stdalloc
#define NAME mock_alloc
#include <derive-c/alloc/wrap/template.h>

#define ALLOC mock_alloc
#define CAPACITY 1
#define NAME hybridstaticempty
#include <derive-c/alloc/hybridstatic/template.h>

using namespace testing;

struct HybridStaticEmptyTests : Test {
    FIXTURE_MOCK(HybridStaticEmptyTests, void*, mock_alloc_allocate_uninit,
                 (mock_alloc * self, size_t size), ());
    FIXTURE_MOCK(HybridStaticEmptyTests, void*, mock_alloc_allocate_zeroed,
                 (mock_alloc * self, size_t size), ());
    FIXTURE_MOCK(HybridStaticEmptyTests, void*, mock_alloc_reallocate,
                 (mock_alloc * self, void* ptr, size_t old_size, size_t new_size), ());
    FIXTURE_MOCK(HybridStaticEmptyTests, void, mock_alloc_deallocate,
                 (mock_alloc * self, void* ptr, size_t size), ());
};

TEST_F(HybridStaticEmptyTests, EmptyAllocator) {
    DC_SCOPED(mock_alloc) mock = mock_alloc_new(stdalloc_get_ref());
    hybridstaticempty_buffer buf;
    DC_SCOPED(hybridstaticempty) alloc = hybridstaticempty_new(&buf, &mock);

    char mocked_allocation_storage[100] = {};
    void* mocked_allocation = mocked_allocation_storage;
    EXPECT_CALL(*this, mock_alloc_allocate_uninit_mock(_, 100)).WillOnce(Return(mocked_allocation));
    void* ptr1 = hybridstaticempty_allocate_uninit(&alloc, 100);
    EXPECT_EQ(ptr1, mocked_allocation);
}

#define ALLOC stdalloc
#define CAPACITY 30
#define NAME hybridstatic
#include <derive-c/alloc/hybridstatic/template.h>

TEST(HybridStaticTests, Debug) {
    hybridstatic_buffer buf;
    DC_SCOPED(hybridstatic) alloc = hybridstatic_new(&buf, stdalloc_get_ref());

    void* ptr1 = hybridstatic_allocate_uninit(&alloc, 10);
    void* ptr2 = hybridstatic_allocate_zeroed(&alloc, 5);
    void* ptr3 = hybridstatic_allocate_uninit(&alloc, 15);

    DC_SCOPED(dc_debug_string_builder) sb = dc_debug_string_builder_new(stdalloc_get_ref());
    hybridstatic_debug(&alloc, dc_debug_fmt_new(), dc_debug_string_builder_stream(&sb));

    EXPECT_EQ(
        // clang-format off
        "hybridstatic@" DC_PTR_REPLACE " {\n"
        "  capacity: 30,\n"
        "  used: 30,\n"
        "  buffer: " DC_PTR_REPLACE ",\n"
        "  last_alloc: " DC_PTR_REPLACE ",\n"
        "  alloc: stdalloc@" DC_PTR_REPLACE " { }\n"
        "}",
        // clang-format on
        derivecpp::fmt::pointer_replace(dc_debug_string_builder_string(&sb)));

    hybridstatic_deallocate(&alloc, ptr1, 10);
    hybridstatic_deallocate(&alloc, ptr2, 5);
    hybridstatic_deallocate(&alloc, ptr3, 15);
}

TEST(HybridStaticTests, DeallocateLastAllocationPoisonsMemory) {
    hybridstatic_buffer buf;
    DC_SCOPED(hybridstatic) alloc = hybridstatic_new(&buf, stdalloc_get_ref());

    // Allocate and write to memory
    void* ptr = hybridstatic_allocate_uninit(&alloc, 10);
    ASSERT_NE(ptr, nullptr);
    char* byte_ptr = static_cast<char*>(ptr);
    byte_ptr[0] = 42; // Should be allowed - we have write access

    // Deallocate the last (and only) allocation
    hybridstatic_deallocate(&alloc, ptr, 10);

// Memory should now be poisoned - access should trigger memory tracker violation
#if DC_MEMORY_TRACKER
    EXPECT_DEATH(byte_ptr[0] = 99, ""); // Writing should fail
#endif
}

TEST(HybridStaticTests, DeallocateLastAllocationReclaimsSpace) {
    hybridstatic_buffer buf;
    DC_SCOPED(hybridstatic) alloc = hybridstatic_new(&buf, stdalloc_get_ref());

    // Allocate 20 bytes from static buffer
    void* ptr1 = hybridstatic_allocate_uninit(&alloc, 20);
    ASSERT_NE(ptr1, nullptr);

    // Verify ptr1 is from the static buffer
    void* buffer_start = &buf[0];
    void* buffer_end = &buf[30];
    EXPECT_GE(ptr1, buffer_start);
    EXPECT_LT(ptr1, buffer_end);

    // Buffer now has 10 bytes free (20 used out of 30)
    // Try to allocate another 15 bytes - will succeed using fallback allocator
    void* ptr2 = hybridstatic_allocate_uninit(&alloc, 15);
    EXPECT_NE(ptr2, nullptr);
    // ptr2 should NOT be from the static buffer (fallback to heap)
    EXPECT_TRUE(ptr2 < buffer_start || ptr2 >= buffer_end);

    // Deallocate the last static allocation
    hybridstatic_deallocate(&alloc, ptr1, 20);

    // Now we should be able to allocate 20 bytes again from the static buffer
    void* ptr3 = hybridstatic_allocate_uninit(&alloc, 20);
    EXPECT_NE(ptr3, nullptr);
    EXPECT_EQ(ptr3, ptr1); // Should get the same address (space was reclaimed)

    // Clean up
    hybridstatic_deallocate(&alloc, ptr2, 15);
    hybridstatic_deallocate(&alloc, ptr3, 20);
}

TEST(HybridStaticTests, DeallocateNonLastAllocationDoesNotReclaimSpace) {
    hybridstatic_buffer buf;
    DC_SCOPED(hybridstatic) alloc = hybridstatic_new(&buf, stdalloc_get_ref());

    void* buffer_start = &buf[0];
    void* buffer_end = &buf[30];

    // Allocate two blocks from the static buffer
    void* ptr1 = hybridstatic_allocate_uninit(&alloc, 10);
    void* ptr2 = hybridstatic_allocate_uninit(&alloc, 10);
    ASSERT_NE(ptr1, nullptr);
    ASSERT_NE(ptr2, nullptr);

    // Deallocate the first allocation (not the last)
    hybridstatic_deallocate(&alloc, ptr1, 10);

// Memory should be poisoned
#if DC_MEMORY_TRACKER
    char* byte_ptr = static_cast<char*>(ptr1);
    EXPECT_DEATH(byte_ptr[0] = 99, ""); // Writing should fail
#endif

    // We can't reclaim the first 10 bytes since ptr2 is after it
    // Only 10 bytes left in static buffer (capacity is 30, we used 20, freed 10 but can't reclaim
    // it) Try to allocate 15 bytes - will succeed using fallback allocator
    void* ptr3 = hybridstatic_allocate_uninit(&alloc, 15);
    EXPECT_NE(ptr3, nullptr);
    // ptr3 should NOT be from the static buffer (fallback to heap)
    EXPECT_TRUE(ptr3 < buffer_start || ptr3 >= buffer_end);

    // Can still allocate 10 bytes from the static buffer
    void* ptr4 = hybridstatic_allocate_uninit(&alloc, 10);
    EXPECT_NE(ptr4, nullptr);
    // ptr4 should be from the static buffer (exactly 10 bytes remaining)
    EXPECT_GE(ptr4, buffer_start);
    EXPECT_LT(ptr4, buffer_end);

    hybridstatic_deallocate(&alloc, ptr2, 10);
    hybridstatic_deallocate(&alloc, ptr3, 15);
    hybridstatic_deallocate(&alloc, ptr4, 10);
}

TEST(HybridStaticTests, ReallocateLastAllocationGrowsInPlace) {
    hybridstatic_buffer buf;
    DC_SCOPED(hybridstatic) alloc = hybridstatic_new(&buf, stdalloc_get_ref());

    // Allocate 10 bytes
    void* ptr = hybridstatic_allocate_uninit(&alloc, 10);
    ASSERT_NE(ptr, nullptr);
    char* byte_ptr = static_cast<char*>(ptr);
    byte_ptr[0] = 42;

    // Reallocate to 20 bytes (growing)
    void* new_ptr = hybridstatic_reallocate(&alloc, ptr, 10, 20);
    EXPECT_EQ(new_ptr, ptr);                       // Should grow in place
    EXPECT_EQ(static_cast<char*>(new_ptr)[0], 42); // Data preserved

    // Can write to new space
    static_cast<char*>(new_ptr)[19] = 99;

    hybridstatic_deallocate(&alloc, new_ptr, 20);
}

TEST(HybridStaticTests, ReallocateLastAllocationShrinksInPlace) {
    hybridstatic_buffer buf;
    DC_SCOPED(hybridstatic) alloc = hybridstatic_new(&buf, stdalloc_get_ref());

    // Allocate 20 bytes
    void* ptr = hybridstatic_allocate_uninit(&alloc, 20);
    ASSERT_NE(ptr, nullptr);
    char* byte_ptr = static_cast<char*>(ptr);
    byte_ptr[0] = 42;
    byte_ptr[10] = 99;

    // Reallocate to 10 bytes (shrinking)
    void* new_ptr = hybridstatic_reallocate(&alloc, ptr, 20, 10);
    EXPECT_EQ(new_ptr, ptr);                       // Should shrink in place
    EXPECT_EQ(static_cast<char*>(new_ptr)[0], 42); // Data preserved

// The last 10 bytes should be poisoned
#if DC_MEMORY_TRACKER
    EXPECT_DEATH(byte_ptr[15] = 77, ""); // Writing to freed space should fail
#endif

    // Should be able to allocate 10 more bytes now
    void* ptr2 = hybridstatic_allocate_uninit(&alloc, 10);
    EXPECT_NE(ptr2, nullptr);

    hybridstatic_deallocate(&alloc, new_ptr, 10);
    hybridstatic_deallocate(&alloc, ptr2, 10);
}

TEST(HybridStaticTests, ReallocateNonLastAllocationCopies) {
    hybridstatic_buffer buf;
    DC_SCOPED(hybridstatic) alloc = hybridstatic_new(&buf, stdalloc_get_ref());

    // Allocate two blocks
    void* ptr1 = hybridstatic_allocate_uninit(&alloc, 5);
    void* ptr2 = hybridstatic_allocate_uninit(&alloc, 5);
    ASSERT_NE(ptr1, nullptr);
    ASSERT_NE(ptr2, nullptr);

    char* byte_ptr1 = static_cast<char*>(ptr1);
    byte_ptr1[0] = 42;

    // Reallocate first block to larger size - should copy since it's not the last
    void* new_ptr1 = hybridstatic_reallocate(&alloc, ptr1, 5, 10);
    EXPECT_NE(new_ptr1, ptr1);                      // Should be a new allocation
    EXPECT_EQ(static_cast<char*>(new_ptr1)[0], 42); // Data preserved

// Original ptr1 should be poisoned
#if DC_MEMORY_TRACKER
    EXPECT_DEATH(byte_ptr1[0] = 99, "");
#endif

    hybridstatic_deallocate(&alloc, ptr2, 5);
    hybridstatic_deallocate(&alloc, new_ptr1, 10);
}
