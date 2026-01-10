#if defined EXPAND_IN_STRUCT
    #if defined DC_MOCKABLE
        #undef DC_MOCKABLE_ENABLED
        #undef DC_MOCKABLE_DEFINE
        #undef DC_MOCKABLE_DECLARE

        #pragma push_macro("DC_MOCKABLE_ENABLED")
        #pragma push_macro("DC_MOCKABLE_DEFINE")
        #pragma push_macro("DC_MOCKABLE_DECLARE")
    #endif
#endif
