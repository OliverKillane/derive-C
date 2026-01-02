// JUSTIFY: No mocking from within a struct
//  - Cannot forward declare the function within the struct body.
//  - Cannot put the function pointer after the real definition
//    without making the function body a macro argument
// So we just disable.
#if defined EXPAND_IN_STRUCT
    #if defined MOCKABLE
        // JUSTIFY: just checking mockable
        //  - All are defined in `mock.h`
        #pragma push_macro("MOCKABLE_DECLARE")
        #pragma push_macro("MOCKABLE_DEFINE")

        #undef MOCKABLE_DECLARE // [DERIVE-C] argument
        #undef MOCKABLE_DEFINE  // [DERIVE-C] argument

        #define MOCKABLE_DECLARE(ret, name, args) 
        #define MOCKABLE_DEFINE(ret, name, args) static ret name args
    #endif
#endif
