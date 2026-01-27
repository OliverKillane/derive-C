#include <gtest/gtest.h>

#include <derive-c/core/debug/memory_tracker.h>
#include <derive-c/alloc/std.h>
#include <derive-c/utils/debug/string.h>

#include <derive-cpp/fmt/remove_ptrs.hpp>

#define ALLOC stdalloc
#define BLOCK_SIZE 32
#define SLAB_SIZE 128
#define NAME slabsmall
#include <derive-c/alloc/slab/template.h>

using namespace testing;

TEST(SlabTests, BasicAllocation) {
    DC_SCOPED(slabsmall) alloc = slabsmall_new(stdalloc_get_ref());

    void* ptr1 = slabsmall_allocate_uninit(&alloc, 16);
    EXPECT_NE(ptr1, nullptr);

    void* ptr2 = slabsmall_allocate_uninit(&alloc, 16);
    EXPECT_NE(ptr2, nullptr);
    EXPECT_NE(ptr1, ptr2);

    slabsmall_deallocate(&alloc, ptr1, 16);
    slabsmall_deallocate(&alloc, ptr2, 16);
}

TEST(SlabTests, AllocationWithinBlockSize) {
    DC_SCOPED(slabsmall) alloc = slabsmall_new(stdalloc_get_ref());

    void* ptr = slabsmall_allocate_uninit(&alloc, 32);
    EXPECT_NE(ptr, nullptr);

    slabsmall_deallocate(&alloc, ptr, 32);
}

TEST(SlabTests, OversizedAllocationUsesBackupAllocator) {
    DC_SCOPED(slabsmall) alloc = slabsmall_new(stdalloc_get_ref());

    void* ptr = slabsmall_allocate_uninit(&alloc, 64);
    EXPECT_NE(ptr, nullptr);

    slabsmall_deallocate(&alloc, ptr, 64);
}

TEST(SlabTests, AllocateZeroedInitializesMemory) {
    DC_SCOPED(slabsmall) alloc = slabsmall_new(stdalloc_get_ref());

    void* ptr = slabsmall_allocate_zeroed(&alloc, 32);
    EXPECT_NE(ptr, nullptr);

    char* bytes = (char*)ptr;
    for (size_t i = 0; i < 32; i++) {
        EXPECT_EQ(bytes[i], 0);
    }

    slabsmall_deallocate(&alloc, ptr, 32);
}

TEST(SlabTests, DeallocateAddsToFreeList) {
    DC_SCOPED(slabsmall) alloc = slabsmall_new(stdalloc_get_ref());

    void* ptr1 = slabsmall_allocate_uninit(&alloc, 16);
    void* ptr2 = slabsmall_allocate_uninit(&alloc, 16);

    slabsmall_deallocate(&alloc, ptr1, 16);

    void* ptr3 = slabsmall_allocate_uninit(&alloc, 16);
    EXPECT_EQ(ptr3, ptr1);

    slabsmall_deallocate(&alloc, ptr2, 16);
    slabsmall_deallocate(&alloc, ptr3, 16);
}

TEST(SlabTests, ReallocateCopiesData) {
    DC_SCOPED(slabsmall) alloc = slabsmall_new(stdalloc_get_ref());

    void* ptr = slabsmall_allocate_uninit(&alloc, 16);
    memset(ptr, 0xAB, 16);

    void* new_ptr = slabsmall_reallocate(&alloc, ptr, 16, 24);
    EXPECT_NE(new_ptr, nullptr);

    char* bytes = (char*)new_ptr;
    for (size_t i = 0; i < 16; i++) {
        EXPECT_EQ((unsigned char)bytes[i], 0xAB);
    }

    slabsmall_deallocate(&alloc, new_ptr, 24);
}

TEST(SlabTests, ResetClearsAllSlabs) {
    DC_SCOPED(slabsmall) alloc = slabsmall_new(stdalloc_get_ref());

    void* ptr1 = slabsmall_allocate_uninit(&alloc, 16);
    void* ptr2 = slabsmall_allocate_uninit(&alloc, 16);

    slabsmall_reset(&alloc);

    void* ptr3 = slabsmall_allocate_uninit(&alloc, 16);
    EXPECT_NE(ptr3, nullptr);
}

TEST(SlabTests, MultipleSlabsAllocated) {
    DC_SCOPED(slabsmall) alloc = slabsmall_new(stdalloc_get_ref());

    size_t blocks_per_slab = 128 / 32;
    void* ptrs[6];

    for (size_t i = 0; i < blocks_per_slab + 2; i++) {
        ptrs[i] = slabsmall_allocate_uninit(&alloc, 16);
        EXPECT_NE(ptrs[i], nullptr);
    }

    for (size_t i = 0; i < blocks_per_slab + 2; i++) {
        slabsmall_deallocate(&alloc, ptrs[i], 16);
    }
}

TEST(SlabTests, Debug) {
    DC_SCOPED(slabsmall) alloc = slabsmall_new(stdalloc_get_ref());
    void* ptr1 = slabsmall_allocate_uninit(&alloc, 16);
    void* ptr2 = slabsmall_allocate_uninit(&alloc, 16);

    DC_SCOPED(dc_debug_string_builder) sb = dc_debug_string_builder_new(stdalloc_get_ref());
    slabsmall_debug(&alloc, dc_debug_fmt_new(), dc_debug_string_builder_stream(&sb));

    EXPECT_EQ(
        // clang-format off
        "slabsmall@" DC_PTR_REPLACE " {\n"
        "  block_size: 32,\n"
        "  slab_size: 128,\n"
        "  blocks_per_slab: 4,\n"
        "  num_slabs: 1,\n"
        "  alloc: stdalloc@" DC_PTR_REPLACE " { }\n"
        "}",
        // clang-format on
        derivecpp::fmt::pointer_replace(dc_debug_string_builder_string(&sb)));

    slabsmall_deallocate(&alloc, ptr1, 16);
    slabsmall_deallocate(&alloc, ptr2, 16);
}
