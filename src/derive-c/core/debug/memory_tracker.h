/// @brief a wrapper over asan & msan
/// Containers and allocators can use this for custom asan & msan poisoning, provided:
///  - The appropriate level is set (so that container, or allocator poisoning can be disabled)
///  - Allocators / containers setting capabilities must set capabilities when providing to the
///    level below. e.g. a container may set CAP to None for user access checks, but set to uninit
///    before passing to allocator free.
#pragma once

#include <derive-c/core/panic.h>
#include <stddef.h>

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
static const memory_tracker_level memory_tracker_global_level =
    (memory_tracker_level)MEMORY_TRACKER_LVL_CONTAINER;
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
                                 void* addr, size_t size) {
    if (level <= memory_tracker_global_level) {
#if defined(MSAN_ON)
        // msan tracks the initialised state, so for none & write we want poisoned / unreadable.
        switch (cap) {
        case MEMORY_TRACKER_CAP_NONE:
        case MEMORY_TRACKER_CAP_WRITE: {
            if (__msan_test_shadow(addr, size) != -1) {
                PANIC("Memory region %p (%zu bytes) is not uninitialised, but should be", addr,
                      size);
            }
            return;
        }
        case MEMORY_TRACKER_CAP_READ_WRITE: {
            if (__msan_test_shadow(addr, size) == -1) {
                PANIC("Memory region %p (%zu bytes) is not uninitialised, but should be", addr,
                      size);
            }
            return;
        }
        }
        UNREACHABLE("Invalid capability");
#elif defined(ASAN_ON)
        switch (cap) {
        case MEMORY_TRACKER_CAP_NONE: {
            if (!__asan_region_is_poisoned(addr, size)) {
                PANIC("Memory region %p (%zu bytes) is not poisoned, but should be", addr, size);
            }
            return;
        }
        case MEMORY_TRACKER_CAP_WRITE:
        case MEMORY_TRACKER_CAP_READ_WRITE: {
            if (__asan_region_is_poisoned(addr, size)) {
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
