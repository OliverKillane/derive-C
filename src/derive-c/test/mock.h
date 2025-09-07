/// @brief Free function mocking support.
#pragma once

#if defined MOCKING_ENABLED

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
    #define MOCKABLE_DECLARE(ret, name, ...)                                                       \
        typedef ret (*MOCK_TYPE(name))(__VA_ARGS__);                                               \
        extern MOCK_TYPE(name) name;                                                               \
        void MOCK_SET(name)(MOCK_TYPE(name) func);                                                 \
        ret MOCK_REAL(name)(__VA_ARGS__)

    /// Defines a function as mockable.
    /// - Should be followed by attributes & the body of the function.
    #define MOCKABLE_DEFINE(ret, name, ...)                                                        \
        MOCK_TYPE(name) name = MOCK_REAL(name);                                                    \
        void MOCK_SET(name)(MOCK_TYPE(name) func) { name = func; }                                 \
        ret MOCK_REAL(name)(__VA_ARGS__)

#else

    #define MOCKABLE_DECLARE(ret, name, ...) ret name(__VA_ARGS__)
    #define MOCKABLE_DEFINE(ret, name, ...) ret name(__VA_ARGS__)

#endif

// JUSTIFY: Semicolon after MOCKABLE_DECLARE
//  - We cannnot take two __VA_ARGS__ arguments for parameters and attributes, and we need to allow
//    the user to add attributes and a code block for MOCKABLE_DEFINITION, so when using the
//    combined macro we cannot additional attributes to the definition
/// Defines a function as mockable, combining declaration & definition.
#define MOCKABLE(ret, name, ...)                                                                   \
    MOCKABLE_DECLARE(ret, name, __VA_ARGS__);                                                      \
    MOCKABLE_DEFINE(ret, name, __VA_ARGS__)
