/// @brief The reference trait.
/// For some objects references do not need to be the size of pointers.
///  - For singletons an empty struct is sufficient
///  - For a small (global) pool of objects, a smaller index can be used.
///
/// These are convertable to pointers for ease of use, but use less storage when stored.
#pragma once

#include <derive-c/core/namespace.h>
#include <derive-c/core/panic.h>
#include <derive-c/core/std/reflect.h>

#define DC_TRAIT_REFERENCABLE(SELF)                                                                \
    DC_REQUIRE_TYPE(SELF, ref);                                                                    \
    DC_STATIC_ASSERT(sizeof(NS(SELF, ref)) <= sizeof(SELF*),                                       \
                     "Reference type must be at most pointer sized");                              \
    DC_REQUIRE_METHOD(SELF*, NS(SELF, ref), write, (NS(SELF, ref)*));                              \
    DC_REQUIRE_METHOD(SELF const*, NS(SELF, ref), read, (NS(SELF, ref) const*));

#define DC_TRAIT_REFERENCABLE_BY_PTR(SELF)                                                         \
    typedef SELF* NS(SELF, ref);                                                                   \
    static SELF* NS(NS(SELF, ref), write)(NS(SELF, ref) * ref) {                                   \
        DC_ASSUME(ref);                                                                            \
        return *ref;                                                                               \
    }                                                                                              \
    static SELF const* NS(NS(SELF, ref), read)(NS(SELF, ref) const* ref) {                         \
        DC_ASSUME(ref);                                                                            \
        return *ref;                                                                               \
    }                                                                                              \
    DC_TRAIT_REFERENCABLE(SELF);

#define DC_TRAIT_REFERENCABLE_SINGLETON(SELF, INSTANCE)                                            \
    DC_ZERO_SIZED(NS(SELF, ref));                                                                  \
    static NS(SELF, ref) NS(SELF, get_ref)() { return (NS(SELF, ref)){}; }                         \
    static SELF* NS(NS(SELF, ref), write)(NS(SELF, ref) * ref) {                                   \
        DC_ASSUME(ref);                                                                            \
        return &INSTANCE;                                                                          \
    }                                                                                              \
    static SELF const* NS(NS(SELF, ref), read)(NS(SELF, ref) const* ref) {                         \
        DC_ASSUME(ref);                                                                            \
        return &INSTANCE;                                                                          \
    }                                                                                              \
    DC_TRAIT_REFERENCABLE(SELF);
