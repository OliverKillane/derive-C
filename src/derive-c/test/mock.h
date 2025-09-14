/// @brief Free function mocking support.
#pragma once

// JUSTIFY: No macro parens
//  - So we can take a single argument `args` that is bracketed.
// NOLINTBEGIN(bugprone-macro-parentheses)

#if defined ENABLE_MOCKING
    #define MOCK_REAL(name) __real_##name
    #define MOCK_SET(name) __mock_set_##name
    #define MOCK_TYPE(name) __type_##name

    // JUSTIFY: No Semicolon after
    //  - Users may want to add additional attributes to the declaration
    //    so we leave the termination of the real declaration to the user.
    /// Declares the mockable function, when `MOCKING_ENABLED` this includes
    /// declaring the function pointer and the `MOCK_SET` function.
    ///
    /// Can be used from a header (with `MOCKABLE_DEFINE` in the `.c`) or as a
    /// forward declaration in the same file for a header only library.
    ///
    /// Additional attributes can be added after this declaration.
    #define MOCKABLE_DECLARE(ret, name, args)                                                      \
        typedef ret(*MOCK_TYPE(name)) args;                                                        \
        extern MOCK_TYPE(name) name;                                                               \
        void MOCK_SET(name)(MOCK_TYPE(name) func);                                                 \
        ret MOCK_REAL(name) args

    #define MOCKABLE_DEFINE(ret, name, args)                                                       \
        MOCK_TYPE(name) name = MOCK_REAL(name);                                                    \
        void MOCK_SET(name)(MOCK_TYPE(name) func) { name = func; }                                 \
        ret MOCK_REAL(name) args
#else
    #define MOCKABLE_DECLARE(ret, name, args) ret name args
    #define MOCKABLE_DEFINE(ret, name, args) ret name args
#endif

/// Defines a function as mockable, combining declaration & definition.
#define MOCKABLE(ret, name, args)                                                                  \
    MOCKABLE_DECLARE(ret, name, args);                                                             \
    MOCKABLE_DEFINE(ret, name, args)

// NOLINTEND(bugprone-macro-parentheses)
