#include <derive-c/core/placeholder.h>

#undef SELF
#undef TEMPLATE_ERROR

#if defined SELF_PUSHED
    #pragma pop_macro("SELF")
    #undef SELF_PUSHED
#endif

#if defined DC_INTERNAL_NAME
    #undef DC_INTERNAL_NAME // [DERIVE-C] argument
#elif defined NAME
    #undef NAME
#elif !defined DC_PLACEHOLDERS
    #error "If `DC_INTERNAL_NAME` is not defined, `NAME` must be defined"
#endif
