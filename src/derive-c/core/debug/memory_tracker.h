/// @brief a wrapper over asan & msan
/// Containers and allocators can use this for custom asan & msan poisoning, provided:
///  - The appropriate level is set (so that container, or allocator poisoning can be disabled)
///  - Allocators / containers setting capabilities must set capabilities when providing to the
///    level below. e.g. a container may set CAP to None for user access checks, but set to uninit
///    before passing to allocator free.
#pragma once

#include <derive-c/core/helpers.h> // NOLINT(misc-include-cleaner)
#include <derive-c/core/panic.h>

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#if defined CUSTOM_MEMORY_TRACKING
    // For clang, we detect asan with feature
    // - See: https://clang.llvm.org/docs/LanguageExtensions.html#has-feature-and-has-extension
    #if defined __has_feature
        #if __has_feature(address_sanitizer)
            #define ASAN_ON
        #endif
        #if __has_feature(memory_sanitizer)
            #define MSAN_ON
        #endif
    #endif

    // For detection under gcc
    // - See: https://gcc.gnu.org/onlinedocs/gcc-15.2.0/cpp.pdf
    // - No support for msan as of GCC 15.2.0
    #if defined __SANITIZE_ADDRESS__
        #define ASAN_ON
    #endif
#endif

#if defined ASAN_ON
    #if defined MSAN_ON
        #error "cannot support asan and msan simultaneously"
    #endif
    #include <sanitizer/asan_interface.h>
#elif defined MSAN_ON
    #include <sanitizer/msan_interface.h>
#endif

/// Capabilities to assign to memory regions
typedef enum { // NOLINT(performance-enum-size)
    MEMORY_TRACKER_CAP_NONE,
    MEMORY_TRACKER_CAP_WRITE,
    MEMORY_TRACKER_CAP_READ_WRITE,
} memory_tracker_capability;

// JUSTIFY: Different tracker levels
//  - So we can disable container, or allocator level tracking when testing/debugging tracking
//  itself.
/// The level at which to track memory state:
/// - when testing allocators, none
/// - when testing containers, at the allocator level
/// - when testing users code, at the container and below level
typedef enum { // NOLINT(performance-enum-size)
    MEMORY_TRACKER_LVL_NONE = 0,
    MEMORY_TRACKER_LVL_ALLOC = 1,
    MEMORY_TRACKER_LVL_CONTAINER = 2,
} memory_tracker_level;

#if defined CUSTOM_MEMORY_TRACKING
static const memory_tracker_level memory_tracker_global_level =
    (memory_tracker_level)CUSTOM_MEMORY_TRACKING;
#else
// JUSTIFY: Default as container
//  - Library users assume correct library implementation, so expect tracking from the container
//  level
static const memory_tracker_level memory_tracker_global_level = MEMORY_TRACKER_LVL_CONTAINER;
#endif

static void memory_tracker_set(memory_tracker_level level, memory_tracker_capability cap,
                               const volatile void* addr, size_t size) {
    if (level <= memory_tracker_global_level) {
#if defined(MSAN_ON)
        // msan tracks the initialised state, so for none & write we want poisoned / unreadable.
        switch (cap) {
        case MEMORY_TRACKER_CAP_NONE:
        case MEMORY_TRACKER_CAP_WRITE: {
            __msan_poison(addr, size);
            return;
        }
        case MEMORY_TRACKER_CAP_READ_WRITE: {
            __msan_unpoison(addr, size);
            return;
        }
        }
        UNREACHABLE("Invalid capability");
#elif defined(ASAN_ON)
        switch (cap) {
        case MEMORY_TRACKER_CAP_NONE: {
            __asan_poison_memory_region(addr, size);
            return;
        }
        case MEMORY_TRACKER_CAP_WRITE:
        case MEMORY_TRACKER_CAP_READ_WRITE: {
            __asan_unpoison_memory_region(addr, size);
            return;
        }
        }
        UNREACHABLE("Invalid capability");
#else
        (void)addr;
        (void)size;
        (void)cap;
#endif
    }
}

static void memory_tracker_check(memory_tracker_level level, memory_tracker_capability cap,
                                 const void* addr, size_t size) {
    if (level <= memory_tracker_global_level) {
#if defined(MSAN_ON)
        // msan tracks the initialised state, so for none & write we want poisoned / unreadable.
        switch (cap) {
        case MEMORY_TRACKER_CAP_NONE:
        case MEMORY_TRACKER_CAP_WRITE: {
            if (__msan_test_shadow((void*)addr, size) != -1) {
                PANIC("Memory region %p (%zu bytes) is not uninitialised, but should be", addr,
                      size);
            }
            return;
        }
        case MEMORY_TRACKER_CAP_READ_WRITE: {
            if (__msan_test_shadow((void*)addr, size) == -1) {
                PANIC("Memory region %p (%zu bytes) is not uninitialised, but should be", addr,
                      size);
            }
            return;
        }
        }
        UNREACHABLE("Invalid capability");
#elif defined(ASAN_ON)
        bool const region_is_poisoned = __asan_region_is_poisoned((void*)addr, size);
        switch (cap) {
        case MEMORY_TRACKER_CAP_NONE: {
            if (!region_is_poisoned) {
                bool const is_at_end_of_granule = is_aligned_pow2_exp((char*)addr - 7, 3);
                bool const is_next_byte_poisoned =
                    __asan_region_is_poisoned((void*)((char*)addr + 1), 1);

                // JUSTIFY: panic conditionally
                //  - Asan tracks poisoning at the granule/8 byte level, with the number of bytes
                //    poisoned from the end of the granule
                //  - Hence if we have a poisoned byte, in the middle of a granule, with the rest
                //    unpoisoned, asan will mark that byte as unpoisoned
                // Therefore, we only throw if we are certain the poisoning is wrong (e.g. it would
                // be continuing contiguous poisoned part of a granule, or its at the start of a
                // granule)
                if (is_at_end_of_granule || is_next_byte_poisoned) {

                    PANIC("Memory region %p (%zu bytes) is not poisoned, but should be", addr,
                          size);
                }
            }
            return;
        }
        case MEMORY_TRACKER_CAP_WRITE:
        case MEMORY_TRACKER_CAP_READ_WRITE: {
            if (region_is_poisoned) {
                PANIC("Memory region %p (%zu bytes) is poisoned, but should be accessible", addr,
                      size);
            }
            return;
        }
        }
        UNREACHABLE("Invalid capability");
#else
        (void)addr;
        (void)size;
        (void)cap;
#endif
    }
}

static void memory_tracker_debug(FILE* stream, const void* addr, size_t size) {
    fprintf(stream, "memory tracker debug (%zu bytes) at %p ", size, addr);
#if defined(MSAN_ON)
    fprintf(stream, "[MSAN]: ")
        // msan tracks the initialised state, so for none & write we want poisoned / unreadable.
        for (size_t i = 0; i < size; i++) {
        fprintf(stream, "\n%p: ", (char*)addr + i);
        if (__msan_test_shadow((char*)addr + i, 1) == -1) {
            fprintf(stream, "U [%02x]", *((unsigned char*)addr + i));
        } else {
            fprintf(stream, "I [%02x]", *((unsigned char*)addr + i));
        }
    }
#elif defined(ASAN_ON)
    // Each shadow memory entry covers 8 bytes of memory, aligned to 8 bytes, so we print this.
    char* addr_start = (char*)addr;
    char* addr_end = addr_start + size;
    char* granule_base = (char*)((uintptr_t)addr & ~0x7); // NOLINT(performance-no-int-to-ptr)
    char* granule_end =
        (char*)(((uintptr_t)addr + size + 7) & ~0x7); // NOLINT(performance-no-int-to-ptr)

    fprintf(stream, "[ASAN]:");
    fprintf(stream,
            "\ndisplaying each 8 byte grandule (asan tracks poisoning as 0-8 bytes from the end)");
    fprintf(stream, "\n");
    fprintf(stream, "\n                ");
    for (size_t b = 0; b < 8; b++) {
        fprintf(stream, "    %lu  ", b);
    }
    fprintf(stream, "\n");
    for (char* p = granule_base; p < granule_end; p += 8) {
        fprintf(stream, "%p: ", p);
        for (char* b = p; b < p + 8; b++) {
            bool const poisoned = __asan_region_is_poisoned(b, 1);
            bool const in_selected = (b >= addr_start && b < addr_end);
            uint8_t value;
            if (poisoned) {
                __asan_unpoison_memory_region(b, 1);
                value = *b;
                __asan_poison_memory_region(b, 1);
            } else {
                value = *b;
            }

            char const status = poisoned ? 'P' : 'U';

            if (in_selected) {
                fprintf(stream, "[%c|%02x] ", status, value);
            } else {
                fprintf(stream, "|%c|%02x| ", status, value);
            }
        }
        fprintf(stream, "\n");
    }
#else
    fprintf(stream, "[NO TRACKING]");
    (void)addr;
    (void)size;
#endif
    fprintf(stream, "\n");
}
