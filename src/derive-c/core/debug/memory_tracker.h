/// @brief a wrapper over asan & msan
/// Containers and allocators can use this for custom asan & msan poisoning, provided:
///  - The appropriate level is set (so that container, or allocator poisoning can be disabled)
///  - Allocators / containers setting capabilities must set capabilities when providing to the
///    level below. e.g. a container may set CAP to None for user access checks, but set to uninit
///    before passing to allocator free.
#pragma once

#include <derive-c/core/panic.h>
#include <derive-c/core/math.h>

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#if defined CUSTOM_MEMORY_TRACKING
    // For clang, we detect asan with feature
    // - See: https://clang.llvm.org/docs/LanguageExtensions.html#has-feature-and-has-extension
    #if defined __has_feature
        #if __has_feature(address_sanitizer)
            #define DC_ASAN_ON
        #endif
        #if __has_feature(memory_sanitizer)
            #define DC_MSAN_ON
        #endif
    #endif

    // For detection under gcc
    // - See: https://gcc.gnu.org/onlinedocs/gcc-15.2.0/cpp.pdf
    // - No support for msan as of GCC 15.2.0
    #if defined __SANITIZE_ADDRESS__
        #define DC_ASAN_ON
    #endif
#endif

#if defined DC_ASAN_ON
    #if defined DC_MSAN_ON
        #error "cannot support asan and msan simultaneously"
    #endif
    #include <sanitizer/asan_interface.h>
#elif defined DC_MSAN_ON
    #include <sanitizer/msan_interface.h>
#endif

/// Capabilities to assign to memory regions
typedef enum { // NOLINT(performance-enum-size)
    DC_MEMORY_TRACKER_CAP_NONE,
    DC_MEMORY_TRACKER_CAP_WRITE,
    DC_MEMORY_TRACKER_CAP_READ_WRITE,
} dc_memory_tracker_capability;

// JUSTIFY: Different tracker levels
//  - So we can disable container, or allocator level tracking when testing/debugging tracking
//  itself.
/// The level at which to track memory state:
/// - when testing allocators, none
/// - when testing containers, at the allocator level
/// - when testing users code, at the container and below level
typedef enum { // NOLINT(performance-enum-size)
    DC_MEMORY_TRACKER_LVL_NONE = 0,
    DC_MEMORY_TRACKER_LVL_ALLOC = 1,
    DC_MEMORY_TRACKER_LVL_CONTAINER = 2,
} dc_memory_tracker_level;

#if defined CUSTOM_MEMORY_TRACKING
static const dc_memory_tracker_level dc_memory_tracker_global_level =
    (dc_memory_tracker_level)CUSTOM_MEMORY_TRACKING;
#else
// JUSTIFY: Default as container
//  - Library users assume correct library implementation, so expect tracking from the container
//  level
static const dc_memory_tracker_level dc_memory_tracker_global_level =
    DC_MEMORY_TRACKER_LVL_CONTAINER;
#endif

PUBLIC static void dc_memory_tracker_set(dc_memory_tracker_level level,
                                         dc_memory_tracker_capability cap,
                                         const volatile void* addr, size_t size) {
    if (level <= dc_memory_tracker_global_level) {
#if defined(DC_MSAN_ON)
        // msan tracks the initialised state, so for none & write we want poisoned / unreadable.
        switch (cap) {
        case DC_MEMORY_TRACKER_CAP_NONE:
        case DC_MEMORY_TRACKER_CAP_WRITE: {
            __msan_poison(addr, size);
            return;
        }
        case DC_MEMORY_TRACKER_CAP_READ_WRITE: {
            __msan_unpoison(addr, size);
            return;
        }
        }
        DC_UNREACHABLE("Invalid capability");
#elif defined(DC_ASAN_ON)
        switch (cap) {
        case DC_MEMORY_TRACKER_CAP_NONE: {
            __asan_poison_memory_region(addr, size);
            return;
        }
        case DC_MEMORY_TRACKER_CAP_WRITE:
        case DC_MEMORY_TRACKER_CAP_READ_WRITE: {
            __asan_unpoison_memory_region(addr, size);
            return;
        }
        }
        DC_UNREACHABLE("Invalid capability");
#else
        (void)addr;
        (void)size;
        (void)cap;
#endif
    }
}

PUBLIC static void dc_memory_tracker_check(dc_memory_tracker_level level,
                                           dc_memory_tracker_capability cap, const void* addr,
                                           size_t size) {
    DC_ASSERT(size > 0, "Cannot check zero sized region");
    if (level <= dc_memory_tracker_global_level) {
#if defined(DC_MSAN_ON)
        // msan tracks the initialised state, so for none & write we want poisoned / unreadable.
        switch (cap) {
        case DC_MEMORY_TRACKER_CAP_NONE:
        case DC_MEMORY_TRACKER_CAP_WRITE: {
            for (size_t offset = 0; offset < size; offset++) {
                const unsigned char* p = (const unsigned char*)addr + offset;

                // For size == 1:
                //  - return -1  => unpoisoned (initialized)  -> ERROR here
                //  - return 0   => poisoned (uninitialized)  -> OK
                if (__msan_test_shadow(p, 1) == -1) {
                    DC_PANIC("Memory region %p (%zu bytes) is not fully uninitialised: "
                             "byte %zu (%p) is unpoisoned",
                             addr, size, offset, p);
                }
            }
            return;
        }
        case DC_MEMORY_TRACKER_CAP_READ_WRITE: {
            for (size_t offset = 0; offset < size; offset++) {
                const unsigned char* p = (const unsigned char*)addr + offset;
                if (__msan_test_shadow(p, 1) != -1) {
                    DC_PANIC("Memory region %p (%zu bytes) is not initialised: "
                             "byte %zu (%p) is poisoned",
                             addr, size, offset, p);
                }
            }
            return;
        }
        }
        DC_UNREACHABLE("Invalid capability");
#elif defined(DC_ASAN_ON)
        bool const region_is_poisoned = __asan_region_is_poisoned((void*)(uintptr_t)addr, size);
        switch (cap) {
        case DC_MEMORY_TRACKER_CAP_NONE: {
            if (!region_is_poisoned) {
                const char* addr_char = (const char*)addr;
                bool const is_at_end_of_granule = dc_math_is_aligned_pow2(addr_char - 7, 8);
                bool const is_next_byte_poisoned =
                    __asan_region_is_poisoned((void*)(uintptr_t)(addr_char + 1), 1);

                // JUSTIFY: panic conditionally
                //  - Asan tracks poisoning at the granule/8 byte level, with the number of bytes
                //    poisoned from the end of the granule
                //  - Hence if we have a poisoned byte, in the middle of a granule, with the rest
                //    unpoisoned, asan will mark that byte as unpoisoned
                // Therefore, we only throw if we are certain the poisoning is wrong (e.g. it would
                // be continuing contiguous poisoned part of a granule, or its at the start of a
                // granule)
                if (is_at_end_of_granule || is_next_byte_poisoned) {

                    DC_PANIC("Memory region %p (%zu bytes) is not poisoned, but should be", addr,
                             size);
                }
            }
            return;
        }
        case DC_MEMORY_TRACKER_CAP_WRITE:
        case DC_MEMORY_TRACKER_CAP_READ_WRITE: {
            if (region_is_poisoned) {
                DC_PANIC("Memory region %p (%zu bytes) is poisoned, but should be accessible", addr,
                         size);
            }
            return;
        }
        }
        DC_UNREACHABLE("Invalid capability");
#else
        (void)addr;
        (void)size;
        (void)cap;
#endif
    }
}

PUBLIC static void dc_memory_tracker_debug(FILE* stream, const void* addr, size_t size) {
    fprintf(stream, "memory tracker debug (%zu bytes) at %p ", size, addr);
#if defined DC_MSAN_ON
    fprintf(stream, "[MSAN]:");
    // msan tracks the initialised state, so for none & write we want poisoned / unreadable.
    for (size_t i = 0; i < size; i++) {
        char const* ptr = (char const*)addr + i;
        fprintf(stream, "\n%p: ", ptr);
        if (__msan_test_shadow(ptr, 1) != -1) {
            __msan_unpoison(ptr, 1);
            fprintf(stream, "U [%02x]", *((unsigned char*)ptr));
            __msan_poison(ptr, 1);
        } else {
            fprintf(stream, "I [%02x]", *((unsigned char*)ptr));
        }
    }
#elif defined DC_ASAN_ON
    // Each shadow memory entry covers 8 bytes of memory, aligned to 8 bytes, so we print this.
    const char* addr_start = (const char*)addr;
    const char* addr_end = addr_start + size;
    const char* granule_base =
        (const char*)((uintptr_t)addr & ~(uintptr_t)0x7U); // NOLINT(performance-no-int-to-ptr)
    const char* granule_end = (const char*)(((uintptr_t)addr + size + 7U) &
                                            ~(uintptr_t)0x7U); // NOLINT(performance-no-int-to-ptr)

    fprintf(stream, "[ASAN]:");
    fprintf(stream,
            "\ndisplaying each 8 byte grandule (asan tracks poisoning as 0-8 bytes from the end)");
    fprintf(stream, "\n");
    fprintf(stream, "\n                ");
    for (size_t b = 0; b < 8; b++) {
        fprintf(stream, "    %lu  ", b);
    }
    fprintf(stream, "\n");
    for (const char* p = granule_base; p < granule_end; p += 8) {
        fprintf(stream, "%p: ", p);
        for (const char* b = p; b < p + 8; b++) {
            bool const poisoned = __asan_region_is_poisoned((void*)(uintptr_t)b, 1);
            bool const in_selected = (b >= addr_start && b < addr_end);
            uint8_t value;
            if (poisoned) {
                __asan_unpoison_memory_region((void*)(uintptr_t)b, 1);
                value = (uint8_t)*b;
                __asan_poison_memory_region((void*)(uintptr_t)b, 1);
            } else {
                value = (uint8_t)*b;
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
