#pragma once

// JUSTIFY: Allowing _Generic with clang
//  - Tests are in C++, we ideally want all the code to be compiled as C
//  - In the matrix build, GCC will validate the templated versions, clang the _Generic
//  - Examples also help to validate (compiled as C)
#if defined(__cplusplus)
    #if defined(__clang__)
        #define DC_STATIC_ASSERT_SUPPORTED
        #define DC_GENERIC_KEYWORD_SUPPORTED
    #endif
#else
    #define DC_STATIC_ASSERT_SUPPORTED
    #define DC_GENERIC_KEYWORD_SUPPORTED
#endif
