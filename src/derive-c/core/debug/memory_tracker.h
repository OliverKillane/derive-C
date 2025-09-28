/// @brief a wrapper over asan & msan
#pragma once

#include <stddef.h>

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

#if defined ASAN_ON
    #if defined MSAN_ON
        #error "cannot support asan and msan simultaneously
    #endif
    #include <sanitizer/asan_interface.h>
#elif defined MSAN_ON
    #include <sanitizer/msan_interface.h>
#endif

/// Mark a new allocation as writable, but not readable.
static void memory_tracker_new(const volatile void* addr, size_t size) {
#if defined(MSAN_ON)
    // Mark bytes uninitialized
    __msan_poison(addr, size);
#elif defined(ASAN_ON)
    // Ensure [addr, addr+size) is accessible
    __asan_unpoison_memory_region(addr, size);
#else
    (void)addr;
    (void)size;
#endif
}

/// Mark a location as readable and writeable
static void memory_tracker_init(const volatile void* addr, size_t size) {
#if defined(MSAN_ON)
    // Mark bytes initialized
    __msan_unpoison(addr, size);
#else
    (void)addr;
    (void)size;
#endif
}

// Mark a location as writable, but not readable
static void memory_tracker_uninit(const volatile void* addr, size_t size) {
#if defined(MSAN_ON)
    // Mark bytes uninitialized again
    __msan_poison(addr, size);
#else
    (void)addr;
    (void)size;
#endif
}

// Mark a location as neither writable nor readable.
static void memory_tracker_delete(const volatile void* addr, size_t size) {
#if defined(MSAN_ON)
    // Mark bytes uninitialized prior to free
    __msan_poison(addr, size);
#elif defined(ASAN_ON)
    // Make further accesses error out
    __asan_poison_memory_region(addr, size);
#else
    (void)addr;
    (void)size;
#endif
}