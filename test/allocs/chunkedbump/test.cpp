#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <derive-c/core/debug/memory_tracker.h>
#include <derive-c/alloc/std.h>
#include <derive-c/utils/debug/string.h>

#include <derive-cpp/test/trampoline.hpp>
#include <derive-cpp/test/gtest_mock.hpp>
#include <derive-cpp/fmt/remove_ptrs.hpp>

#define ALLOC stdalloc
#define NAME mock_alloc
#include <derive-c/alloc/wrap/template.h>

#define ALLOC mock_alloc
#define BLOCK_SIZE 64
#define NAME chunkedbumpsmall
#include <derive-c/alloc/chunkedbump/template.h>

using namespace testing;

struct ChunkedBumpSmallTests : Test {
    FIXTURE_MOCK(ChunkedBumpSmallTests, void*, mock_alloc_allocate_uninit,
                 (mock_alloc * self, size_t size), ());
    FIXTURE_MOCK(ChunkedBumpSmallTests, void*, mock_alloc_allocate_zeroed,
                 (mock_alloc * self, size_t size), ());
    FIXTURE_MOCK(ChunkedBumpSmallTests, void*, mock_alloc_reallocate,
                 (mock_alloc * self, void* ptr, size_t old_size, size_t new_size), ());
    FIXTURE_MOCK(ChunkedBumpSmallTests, void, mock_alloc_deallocate,
                 (mock_alloc * self, void* ptr, size_t size), ());
};

TEST_F(ChunkedBumpSmallTests, FirstAllocationUsesOneBlock) {
    using testing::InSequence;
    InSequence s;

    // Buffers must be declared BEFORE allocator so they outlive it
    char mocked_block[64] = {};
    char mocked_tracking_vector[128] = {};
    void* mocked_ptr = mocked_block;
    void* mocked_vector_data = mocked_tracking_vector;

    DC_SCOPED(mock_alloc) mock = mock_alloc_new(stdalloc_get_ref());
    DC_SCOPED(chunkedbumpsmall) alloc = chunkedbumpsmall_new(&mock);

    // Block is allocated first, then vector internal storage (capacity=8, item_size=16, so 128
    // bytes)
    EXPECT_CALL(*this, mock_alloc_allocate_uninit_mock(_, 64)).WillOnce(Return(mocked_ptr));
    EXPECT_CALL(*this, mock_alloc_allocate_uninit_mock(_, 128))
        .WillOnce(Return(mocked_vector_data));

    void* ptr1 = chunkedbumpsmall_allocate_uninit(&alloc, 32);
    EXPECT_EQ(ptr1, mocked_ptr);

    // After test, delete() will free in order: blocks first, then vector storage
    EXPECT_CALL(*this, mock_alloc_deallocate_mock(_, mocked_ptr, 64));
    EXPECT_CALL(*this, mock_alloc_deallocate_mock(_, mocked_vector_data, 128));
}

TEST_F(ChunkedBumpSmallTests, OversizedAllocationSpansMultipleBlocks) {
    using testing::InSequence;
    InSequence s;

    // Buffers must be declared BEFORE allocator so they outlive it
    char mocked_blocks[128] = {};
    char mocked_tracking_vector[128] = {};
    void* mocked_ptr = mocked_blocks;
    void* mocked_vector_data = mocked_tracking_vector;

    DC_SCOPED(mock_alloc) mock = mock_alloc_new(stdalloc_get_ref());
    DC_SCOPED(chunkedbumpsmall) alloc = chunkedbumpsmall_new(&mock);

    // Block is allocated first, then vector storage
    EXPECT_CALL(*this, mock_alloc_allocate_uninit_mock(_, 128))
        .WillOnce(Return(mocked_ptr))
        .WillOnce(Return(mocked_vector_data));

    void* ptr1 = chunkedbumpsmall_allocate_uninit(&alloc, 100);
    EXPECT_EQ(ptr1, mocked_ptr);

    // After test, delete() will free in order: blocks first, then vector storage
    EXPECT_CALL(*this, mock_alloc_deallocate_mock(_, mocked_ptr, 128));
    EXPECT_CALL(*this, mock_alloc_deallocate_mock(_, mocked_vector_data, 128));
}

TEST_F(ChunkedBumpSmallTests, ResetFreesAllBlocks) {
    using testing::InSequence;
    InSequence s;

    // Buffers must be declared BEFORE allocator so they outlive it
    char mocked_block1[64] = {};
    char mocked_block2[128] = {};
    char mocked_tracking_vector[128] = {};
    void* mocked_ptr1 = mocked_block1;
    void* mocked_ptr2 = mocked_block2;
    void* mocked_vector_data = mocked_tracking_vector;

    DC_SCOPED(mock_alloc) mock = mock_alloc_new(stdalloc_get_ref());
    DC_SCOPED(chunkedbumpsmall) alloc = chunkedbumpsmall_new(&mock);

    // First allocation: block first, then vector storage (128 bytes for capacity=8 of 16-byte
    // items)
    EXPECT_CALL(*this, mock_alloc_allocate_uninit_mock(_, 64)).WillOnce(Return(mocked_ptr1));
    EXPECT_CALL(*this, mock_alloc_allocate_uninit_mock(_, 128))
        .WillOnce(Return(mocked_vector_data));
    chunkedbumpsmall_allocate_uninit(&alloc, 32);

    // Second allocation: just the block (vector already has capacity)
    EXPECT_CALL(*this, mock_alloc_allocate_uninit_mock(_, 128)).WillOnce(Return(mocked_ptr2));
    chunkedbumpsmall_allocate_uninit(&alloc, 100);

    // Reset should free both block allocations but NOT the vector storage
    EXPECT_CALL(*this, mock_alloc_deallocate_mock(_, mocked_ptr1, 64));
    EXPECT_CALL(*this, mock_alloc_deallocate_mock(_, mocked_ptr2, 128));
    chunkedbumpsmall_reset(&alloc);

    // After test, delete() will free the vector storage
    EXPECT_CALL(*this, mock_alloc_deallocate_mock(_, mocked_vector_data, 128));
}

#define ALLOC stdalloc
#define BLOCK_SIZE 128
#define NAME chunkedbump
#include <derive-c/alloc/chunkedbump/template.h>

TEST(ChunkedBumpTests, Debug) {
    DC_SCOPED(chunkedbump) alloc = chunkedbump_new(stdalloc_get_ref());

    void* ptr1 = chunkedbump_allocate_uninit(&alloc, 50);
    void* ptr2 = chunkedbump_allocate_zeroed(&alloc, 50);
    void* ptr3 = chunkedbump_allocate_uninit(&alloc, 150); // Spans 2 blocks

    DC_SCOPED(dc_debug_string_builder) sb = dc_debug_string_builder_new(stdalloc_get_ref());
    chunkedbump_debug(&alloc, dc_debug_fmt_new(), dc_debug_string_builder_stream(&sb));

    EXPECT_EQ(
        // clang-format off
        "chunkedbump@" DC_PTR_REPLACE " {\n"
        "  block_size: 128,\n"
        "  num_blocks: 3,\n"
        "  current_block_idx: 2,\n"
        "  current_block_offset: 22,\n"
        "  total_allocated: 384,\n"
        "  alloc: stdalloc@" DC_PTR_REPLACE " { }\n"
        "}",
        // clang-format on
        derivecpp::fmt::pointer_replace(dc_debug_string_builder_string(&sb)));

    chunkedbump_deallocate(&alloc, ptr1, 50);
    chunkedbump_deallocate(&alloc, ptr2, 50);
    chunkedbump_deallocate(&alloc, ptr3, 150);
}

TEST(ChunkedBumpTests, BumpWithinBlock) {
    DC_SCOPED(chunkedbump) alloc = chunkedbump_new(stdalloc_get_ref());

    // Allocate 50 bytes - should fit in one 128-byte block
    void* ptr1 = chunkedbump_allocate_uninit(&alloc, 50);
    ASSERT_NE(ptr1, nullptr);

    // Allocate another 50 bytes - should use same block
    void* ptr2 = chunkedbump_allocate_uninit(&alloc, 50);
    ASSERT_NE(ptr2, nullptr);

    // ptr2 should be 50 bytes after ptr1
    EXPECT_EQ(static_cast<char*>(ptr2), static_cast<char*>(ptr1) + 50);

    chunkedbump_deallocate(&alloc, ptr1, 50);
    chunkedbump_deallocate(&alloc, ptr2, 50);
}

TEST(ChunkedBumpTests, AllocationSpanningMultipleBlocks) {
    DC_SCOPED(chunkedbump) alloc = chunkedbump_new(stdalloc_get_ref());

    // Allocate 200 bytes with 128-byte blocks -> needs 2 blocks (256 bytes)
    void* ptr1 = chunkedbump_allocate_uninit(&alloc, 200);
    ASSERT_NE(ptr1, nullptr);

    // Write to verify memory is accessible
    char* byte_ptr = static_cast<char*>(ptr1);
    byte_ptr[0] = 42;
    byte_ptr[199] = 99;
    EXPECT_EQ(byte_ptr[0], 42);
    EXPECT_EQ(byte_ptr[199], 99);

    chunkedbump_deallocate(&alloc, ptr1, 200);
}

TEST(ChunkedBumpTests, AllocateZeroedInitializesMemory) {
    DC_SCOPED(chunkedbump) alloc = chunkedbump_new(stdalloc_get_ref());

    void* ptr = chunkedbump_allocate_zeroed(&alloc, 100);
    ASSERT_NE(ptr, nullptr);

    // Check all bytes are zero
    char const* byte_ptr = static_cast<char const*>(ptr);
    for (size_t i = 0; i < 100; ++i) {
        EXPECT_EQ(byte_ptr[i], 0);
    }

    chunkedbump_deallocate(&alloc, ptr, 100);
}

TEST(ChunkedBumpTests, ReallocateCopiesData) {
    DC_SCOPED(chunkedbump) alloc = chunkedbump_new(stdalloc_get_ref());

    // Allocate and fill with data
    void* ptr = chunkedbump_allocate_uninit(&alloc, 50);
    ASSERT_NE(ptr, nullptr);
    char* byte_ptr = static_cast<char*>(ptr);
    for (size_t i = 0; i < 50; ++i) {
        byte_ptr[i] = static_cast<char>(i);
    }

    // Reallocate to larger size
    void* new_ptr = chunkedbump_reallocate(&alloc, ptr, 50, 100);
    ASSERT_NE(new_ptr, nullptr);

    // Check data was copied
    char* new_byte_ptr = static_cast<char*>(new_ptr);
    for (size_t i = 0; i < 50; ++i) {
        EXPECT_EQ(new_byte_ptr[i], static_cast<char>(i));
    }

    chunkedbump_deallocate(&alloc, new_ptr, 100);
}

TEST(ChunkedBumpTests, ResetClearsAllMemory) {
    DC_SCOPED(chunkedbump) alloc = chunkedbump_new(stdalloc_get_ref());

    // Allocate several blocks
    void* ptr1 = chunkedbump_allocate_uninit(&alloc, 100);
    void* ptr2 = chunkedbump_allocate_uninit(&alloc, 100);
    void* ptr3 = chunkedbump_allocate_uninit(&alloc, 200);
    ASSERT_NE(ptr1, nullptr);
    ASSERT_NE(ptr2, nullptr);
    ASSERT_NE(ptr3, nullptr);

    // Reset
    chunkedbump_reset(&alloc);

    // Should be able to allocate again
    void* ptr4 = chunkedbump_allocate_uninit(&alloc, 50);
    ASSERT_NE(ptr4, nullptr);

    chunkedbump_deallocate(&alloc, ptr4, 50);
}

TEST(ChunkedBumpTests, ExactBlockSizeAllocation) {
    DC_SCOPED(chunkedbump) alloc = chunkedbump_new(stdalloc_get_ref());

    // Allocate exactly one block size
    void* ptr1 = chunkedbump_allocate_uninit(&alloc, 128);
    ASSERT_NE(ptr1, nullptr);

    // Next allocation should use a new block
    void* ptr2 = chunkedbump_allocate_uninit(&alloc, 10);
    ASSERT_NE(ptr2, nullptr);

    // ptr2 should not be adjacent to ptr1 (different block)
    EXPECT_NE(static_cast<char*>(ptr2), static_cast<char*>(ptr1) + 128);

    chunkedbump_deallocate(&alloc, ptr1, 128);
    chunkedbump_deallocate(&alloc, ptr2, 10);
}

TEST(ChunkedBumpTests, MultipleResetsWork) {
    DC_SCOPED(chunkedbump) alloc = chunkedbump_new(stdalloc_get_ref());

    for (int round = 0; round < 3; ++round) {
        void* ptr1 = chunkedbump_allocate_uninit(&alloc, 50);
        void* ptr2 = chunkedbump_allocate_uninit(&alloc, 100);
        ASSERT_NE(ptr1, nullptr);
        ASSERT_NE(ptr2, nullptr);

        chunkedbump_deallocate(&alloc, ptr1, 50);
        chunkedbump_deallocate(&alloc, ptr2, 100);
        chunkedbump_reset(&alloc);
    }
}

TEST(ChunkedBumpTests, ReallocateLastAllocationInPlace) {
    DC_SCOPED(chunkedbump) alloc = chunkedbump_new(stdalloc_get_ref());

    // Allocate 50 bytes - should fit in one 128-byte block
    void* ptr1 = chunkedbump_allocate_uninit(&alloc, 50);
    ASSERT_NE(ptr1, nullptr);

    // Write a pattern to the first 50 bytes
    memset(ptr1, 0xAA, 50);

    // Reallocate to 80 bytes - should extend in place since:
    // 1. It's the last allocation (at the head)
    // 2. There's room in the current block (128 - 50 = 78 bytes remaining, need 30 more)
    void* ptr2 = chunkedbump_reallocate(&alloc, ptr1, 50, 80);

    // Should return the SAME pointer (in-place reallocation)
    EXPECT_EQ(ptr2, ptr1);

    // Verify the original data is intact
    char* bytes = static_cast<char*>(ptr2);
    for (size_t i = 0; i < 50; i++) {
        EXPECT_EQ((unsigned char)bytes[i], 0xAA);
    }

    // Verify we can write to the extended region
    memset(bytes + 50, 0xBB, 30);
    for (size_t i = 50; i < 80; i++) {
        EXPECT_EQ((unsigned char)bytes[i], 0xBB);
    }

    chunkedbump_deallocate(&alloc, ptr2, 80);
}

TEST(ChunkedBumpTests, ReallocateNonLastAllocationAllocatesNew) {
    DC_SCOPED(chunkedbump) alloc = chunkedbump_new(stdalloc_get_ref());

    // Allocate two items
    void* ptr1 = chunkedbump_allocate_uninit(&alloc, 30);
    void* ptr2 = chunkedbump_allocate_uninit(&alloc, 30);
    ASSERT_NE(ptr1, nullptr);
    ASSERT_NE(ptr2, nullptr);

    memset(ptr1, 0xCC, 30);

    // Reallocate ptr1 (not the last allocation) - should allocate new memory
    void* ptr3 = chunkedbump_reallocate(&alloc, ptr1, 30, 50);

    // Should return a DIFFERENT pointer
    EXPECT_NE(ptr3, ptr1);

    // Verify data was copied
    char* bytes = static_cast<char*>(ptr3);
    for (size_t i = 0; i < 30; i++) {
        EXPECT_EQ((unsigned char)bytes[i], 0xCC);
    }

    chunkedbump_deallocate(&alloc, ptr1, 30);
    chunkedbump_deallocate(&alloc, ptr2, 30);
    chunkedbump_deallocate(&alloc, ptr3, 50);
}

#if DC_MEMORY_TRACKER
TEST(ChunkedBumpTests, MemoryTrackerPoisonsAfterReset) {
    DC_SCOPED(chunkedbump) alloc = chunkedbump_new(stdalloc_get_ref());

    void* ptr = chunkedbump_allocate_uninit(&alloc, 50);
    ASSERT_NE(ptr, nullptr);
    char* byte_ptr = static_cast<char*>(ptr);
    byte_ptr[0] = 42; // Should be allowed

    chunkedbump_reset(&alloc);

    // Memory should now be poisoned - access should trigger memory tracker violation
    EXPECT_DEATH(byte_ptr[0] = 99, "");
}
#endif
