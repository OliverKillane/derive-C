#pragma once

#define INLINE inline __attribute__((always_inline))
#define CONST __attribute__((const))
#define PURE __attribute__((pure))
#define NODISCARD __attribute__((warn_unused_result))
