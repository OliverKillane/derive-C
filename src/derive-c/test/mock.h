/// @brief Free function mocking support.
#pragma once

#include <derive-c/core/namespace.h>

// JUSTIFY: No macro parens
//  - So we can take a single argument `args` that is bracketed.
// NOLINTBEGIN(bugprone-macro-parentheses)

#if defined ENABLE_MOCKING
    #define DC_MOCKABLE_REAL(name) __real_##name
    #define DC_MOCKABLE_SET(name) __mock_set_##name
    #define DC_MOCKABLE_TYPE(name) __type_##name

    // JUSTIFY: No Semicolon after
    //  - Users may want to add additional attributes to the declaration
    //    so we leave the termination of the real declaration to the user.
    /// Declares the mockable function, when `MOCKING_ENABLED` this includes
    /// declaring the function pointer and the `DC_MOCKABLE_SET` function.
    ///
    /// Can be used from a header (with `DC_MOCKABLE_DEFINE` in the `.c`) or as a
    /// forward declaration in the same file for a header only library.
    ///
    /// Additional attributes can be added after this declaration.
    #define DC_MOCKABLE_DECLARE(ret, name, args)                                                   \
        typedef ret(*DC_MOCKABLE_TYPE(name)) args;                                                 \
        extern DC_MOCKABLE_TYPE(name) name;                                                        \
        void DC_MOCKABLE_SET(name)(DC_MOCKABLE_TYPE(name) func);                                   \
        ret DC_MOCKABLE_REAL(name) args

    #define DC_MOCKABLE_DEFINE(ret, name, args)                                                    \
        DC_MOCKABLE_TYPE(name) name = DC_MOCKABLE_REAL(name);                                      \
        void DC_MOCKABLE_SET(name)(DC_MOCKABLE_TYPE(name) func) { name = func; }                   \
        ret DC_MOCKABLE_REAL(name) args

    #define DC_MOCKABLE_ENABLED(name) (name != DC_MOCKABLE_REAL(name))
#else
    #define DC_MOCKABLE_DECLARE(ret, name, args) PUBLIC ret name args
    #define DC_MOCKABLE_DEFINE(ret, name, args) PUBLIC ret name args
    #define DC_MOCKABLE_ENABLED(name) false
#endif

/// Defines a function as mockable, combining declaration & definition.
#define DC_MOCKABLE(ret, name, args)                                                               \
    DC_MOCKABLE_DECLARE(ret, name, args);                                                          \
    DC_MOCKABLE_DEFINE(ret, name, args)

// NOLINTEND(bugprone-macro-parentheses)
