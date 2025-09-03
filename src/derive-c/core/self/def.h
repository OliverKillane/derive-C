#include <derive-c/core/helpers.h> // NOLINT(misc-include-cleaner)

/// @brief Supporting templates that internally invoke new templates.
///
/// Ideally we could do:
/// ```c
/// #define SELF foo
///
/// // within the template
/// #pragma push_macro("SELF")
/// #define SELF NS(SELF, internal_vector)
/// // ...
/// #include <derive-c/structures/vector/template.h>
///
/// // ...
///
/// #pragma pop_macro("SELF")
///
/// // The rest of the structure definition ...
/// ```
///
/// However, this is not possible, as macros are not eagrely expanded at definition
///  - We cannot 'save' a macro, and redefine it in using its previous/saved value
///
/// To solve this, there are a few options
/// 1. For the user to provide names for internally used data structures
/// 2. A more complex scheme of stacking internal names (name 1, 2, etc)
/// 3. Namespace all macro paramaters (e.g. `VECTOR_SELF`, `HASHMAP_SELF` etc)
///
/// To keep code simple, but allow definitions to uniformly use `SELF`:
///  - We restrict to 1 layer deep - `NAME` (from user), and `INTERNAL_NAME` (usage inside a
///  template file, one layer deep).

#if defined INTERNAL_NAME
    #if defined SELF
        #pragma push_macro("SELF")
        #define SELF_PUSHED
        #undef SELF
        #define SELF EXPAND(INTERNAL_NAME)
    #endif
#elif defined NAME
    #if defined SELF
        #error "Cannot redefine SELF"
    #endif
    #define SELF EXPAND(NAME)
#else
    #if !defined __clang_analyzer__
        #error "The `SELF` type for a data structure must be defined (by `NAME` or `INTERNAL_NAME`)"
    #endif
    #define NAME derive_c_parameter_self
    #define SELF NAME
#endif
