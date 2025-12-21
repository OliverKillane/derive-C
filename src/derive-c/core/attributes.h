#pragma once

#define DC_INLINE inline __attribute__((always_inline))
#define DC_CONST __attribute__((const))
#define DC_PURE __attribute__((pure))
#define DC_NODISCARD __attribute__((warn_unused_result))
