#pragma once

#define DC_INLINE inline __attribute__((always_inline))
#define DC_CONST __attribute__((const))
#define DC_PURE __attribute__((pure))
#define DC_NODISCARD __attribute__((warn_unused_result))
#define DC_UNUSED __attribute__((unused))

// JUSTIFY: Different values for cpp
//  - When embedded in a struct (e.g. for fuzz tests), we need to avoid ODR
#if defined __cplusplus
    #define DC_STATIC_CONSTANT inline static constexpr
#else
    #define DC_STATIC_CONSTANT static const
#endif
