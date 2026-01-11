#pragma once

#include <stdint.h>
#include <stddef.h>

#include <derive-c/core/compiler.h>

#include "id.h"
#include "fnv1a.h"

/// Sensible default hashers for standard data types
///  - Can be passed as a `<PARAM>_HASH` argument
#if defined DC_GENERIC_KEYWORD_SUPPORTED
    #define DC_DEFAULT_HASH(obj)                                                                   \
        _Generic(*(obj),                                                                           \
            int8_t: int8_t_hash_id,                                                                \
            uint8_t: uint8_t_hash_id,                                                              \
            int16_t: int16_t_hash_id,                                                              \
            uint16_t: uint16_t_hash_id,                                                            \
            int32_t: int32_t_hash_id,                                                              \
            uint32_t: uint32_t_hash_id,                                                            \
            int64_t: int64_t_hash_id,                                                              \
            uint64_t: uint64_t_hash_id,                                                            \
            const char*: dc_fnv1a_str)(obj)
#else
namespace dc::hash {

    #include <type_traits>
    #include <cstdint>

template <class T> constexpr uint64_t default_hash_impl(T const* obj) {
    using U = std::remove_cv_t<T>;

    if constexpr (std::is_same_v<U, int8_t>)
        return int8_t_hash_id(obj);
    else if constexpr (std::is_same_v<U, uint8_t>)
        return uint8_t_hash_id(obj);
    else if constexpr (std::is_same_v<U, int16_t>)
        return int16_t_hash_id(obj);
    else if constexpr (std::is_same_v<U, uint16_t>)
        return uint16_t_hash_id(obj);
    else if constexpr (std::is_same_v<U, int32_t>)
        return int32_t_hash_id(obj);
    else if constexpr (std::is_same_v<U, uint32_t>)
        return uint32_t_hash_id(obj);
    else if constexpr (std::is_same_v<U, int64_t>)
        return int64_t_hash_id(obj);
    else if constexpr (std::is_same_v<U, uint64_t>)
        return uint64_t_hash_id(obj);
    else if constexpr (std::is_same_v<U, char const*>)
        return dc_fnv1a_str(*obj); // obj: char const* const*
    else {
        // always-false, but dependent, without a helper variable
        static_assert([]<class>() { return false; }.template operator()<U>(),
                      "DC_DEFAULT_HASH: unsupported type");
    }
}
} // namespace dc::hash
    #define DC_DEFAULT_HASH(obj) dc::hash::default_hash_impl(obj)
#endif