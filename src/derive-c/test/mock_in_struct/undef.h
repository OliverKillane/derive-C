#if defined EXPAND_IN_STRUCT
    #if defined MOCKABLE
        #undef MOCKABLE_DEFINE
        #undef MOCKABLE_DECLARE

        #pragma push_macro("MOCKABLE_DEFINE")
        #pragma push_macro("MOCKABLE_DECLARE")
    #endif
#endif
