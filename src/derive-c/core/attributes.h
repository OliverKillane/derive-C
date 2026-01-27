#pragma once

#define DC_INLINE inline __attribute__((always_inline))
#define DC_CONST __attribute__((const))
#define DC_PURE __attribute__((pure))
#define DC_NODISCARD __attribute__((warn_unused_result))
#define DC_UNUSED __attribute__((unused))

// JUSTIFY: restrict keyword for pointer aliasing optimization
//  - In C: use standard 'restrict' keyword
//  - In C++: use '__restrict__' compiler extension (GCC/Clang)
#if defined __cplusplus
    #define DC_RESTRICT __restrict__
#else
    #define DC_RESTRICT restrict
#endif

// JUSTIFY: Different values for cpp
//  - When embedded in a struct (e.g. for fuzz tests), we need to avoid ODR
#if defined __cplusplus
    #define DC_STATIC_CONSTANT inline static constexpr
#else
    #define DC_STATIC_CONSTANT static const
#endif

// JUSTIFY: Changing thread local
//  - _Thread_local not part of C++ standard, though is supported by clang & GCC
#if defined __cplusplus
    #define DC_THREAD_LOCAL thread_local
#else
    #define DC_THREAD_LOCAL _Thread_local
#endif
