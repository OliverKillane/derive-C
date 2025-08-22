/// @brief Supporting templates that internally invoke new templates.
/// 
/// A impler implementation of templates would have the user `#define SELF`
///
/// ```c
/// #include <derive-c/self/def.h>
///
/// #push SELF // save the current self
/// #define SELF NAME(SELF, inner_vector)
/// #include <derive-c/structures/vector/template.h>
///
/// #pop SELF
/// // define the structure that uses the inner_vector
/// #include <derive-c/self/undef.h>
/// ```
///
/// However, there is no `pop` and `push` that works this way.
///  - `#pragma push_macro` and `#pragma pop_macro` will remove `SELF`, meaning 
///    we cannot redefine using the old `SELF`.
///  - We could take in another value (say `INTERNAL`) that is given by the user, 
///    then push_macro, redefine SELF, and pop. But this is not a nice user 
///    experience & does not scale for deeper nesting of template instantiations. 
///
/// To solve this, we add an additional layer of indirection between the user-defined 
/// `NAME`, and the `SELF` type, and build a stack of names.
/// - internally `INTERNAL_NAME_{n}` up to a fixed depth
///
/// ```c
/// #include <derive-c/self/def.h>
///
/// #define INTERNAL_NAME NAME(SELF, inner_vector)
/// #include <derive-c/structures/vector/template.h> // internally checks and 
///
/// // define the structure, as SELF, using INTERNAL_NAME
/// #include <derive-c/self/undef.h>
/// ```

#if defined INTERNAL_NAME
#   if defined INTERNAL_NAME_4
#      error "Cannot invoke a template more than 4 levels deep"
#   elif defined INTERNAL_NAME_3
#      define INTERNAL_NAME_4 INTERNAL_NAME
#      define SELF INTERNAL_NAME_4
#   elif defined INTERNAL_NAME_2
#      define INTERNAL_NAME_3 INTERNAL_NAME
#      define SELF INTERNAL_NAME_3
#   elif defined INTERNAL_NAME_1
#      define INTERNAL_NAME_2 INTERNAL_NAME
#      define SELF INTERNAL_NAME_2
#   elif defined INTERNAL_NAME_0
#      define INTERNAL_NAME_1 INTERNAL_NAME
#      define SELF INTERNAL_NAME_1
#   else
#       define INTERNAL_NAME_0 INTERNAL_NAME
#       define SELF INTERNAL_NAME_0
#   endif 
#elif defined NAME
#   define SELF NAME
#elif !defined __clang_analyzer__
#   error "The name to use for the instantiation of the template must be defined"
#else
#   define SELF derive_c_placeholder_self
#endif
