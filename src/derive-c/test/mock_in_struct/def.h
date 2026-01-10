// JUSTIFY: No mocking from within a struct
//  - Cannot forward declare the function within the struct body.
//  - Cannot put the function pointer after the real definition
//    without making the function body a macro argument
// So we just disable.
#if defined EXPAND_IN_STRUCT
    #if defined DC_MOCKABLE
        // JUSTIFY: just checking mockable
        //  - All are defined in `mock.h`
        #pragma push_macro("DC_MOCKABLE_DECLARE")
        #pragma push_macro("DC_MOCKABLE_DEFINE")
        #pragma push_macro("DC_MOCKABLE_ENABLED")

        #undef DC_MOCKABLE_DECLARE // [DERIVE-C] argument
        #undef DC_MOCKABLE_DEFINE  // [DERIVE-C] argument
        #undef DC_MOCKABLE_ENABLED // [DERIVE-C] argument

        #define DC_MOCKABLE_DECLARE(ret, name, args)
        #define DC_MOCKABLE_DEFINE(ret, name, args) static ret name args
        #define DC_MOCKABLE_ENABLED(name) false
    #endif
#endif
