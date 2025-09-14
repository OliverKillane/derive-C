#undef SELF

#if defined SELF_PUSHED
    #pragma pop_macro("SELF")
    #undef SELF_PUSHED
#endif

#if defined INTERNAL_NAME
    #undef INTERNAL_NAME
#elif defined NAME
    #undef NAME
#elif !defined __clang_analyzer__
    #error "If `INTERNAL_NAME` is not defined, `NAME` must be defined"
#endif
