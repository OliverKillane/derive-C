#include <derive-c/core/namespace.h> // NOLINT(misc-include-cleaner)
#include <derive-c/core/placeholder.h>

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
/// #include <derive-c/container/vector/dynamic/template.h>
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
///  - We restrict to 1 layer deep - `NAME` (from user), and `DC_INTERNAL_NAME` (usage inside a
///  template file, one layer deep).

#if defined DC_INTERNAL_NAME
    #if defined SELF
        #pragma push_macro("SELF")
        #define SELF_PUSHED
        #undef SELF
        #define SELF DC_EXPAND(DC_INTERNAL_NAME)
    #endif
#elif defined NAME
    #if defined SELF
        #error "Cannot redefine SELF"
    #endif
    #define SELF DC_EXPAND(NAME)
#else
    #if !defined DC_PLACEHOLDERS
        #error                                                                                     \
            "The `SELF` type for a data structure must be defined (by `NAME` or `DC_INTERNAL_NAME`)"
    #endif
    #define NAME self_t
    #define SELF NAME
#endif

/// With the user provided name, even in nested templates
#define TEMPLATE_ERROR(...) _Pragma(DC_STRINGIFY(GCC error DC_EXPAND_STRING(NAME) ": " __VA_ARGS__))
