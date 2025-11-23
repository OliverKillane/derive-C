#pragma once

#if !defined _GNU_SOURCE
    // Enable gnu specific features for fopencookie
    //  - Must be set before stdio include
    //  - see: https://www.man7.org/linux/man-pages/man7/feature_test_macros.7.html
    #define _GNU_SOURCE
#endif

// stdlib includes
#include <errno.h>  // IWYU pragma: export
#include <stdio.h>  // IWYU pragma: export
#include <string.h> // IWYU pragma: export

// derive-c includes
#include <derive-c/core/prelude.h> // IWYU pragma: export
