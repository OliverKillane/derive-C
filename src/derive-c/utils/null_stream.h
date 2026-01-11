#pragma once

#include <derive-c/core/prelude.h>

#include <stdio.h>
#include <sys/types.h>

#if !defined _GNU_SOURCE
    #error "_GNU_SOURCE must be defined (is in the src/derive-c CMakeLists.txt) to use cookie_io"
#endif

FILE* dc_null_stream(void) {
    cookie_io_functions_t io = {
        .read = NULL,
        .write = NULL,
        .seek = NULL,
        .close = NULL,
    };

    FILE* f = fopencookie(NULL, "w", io);
    DC_ASSERT(f, "received nullptr from fopencookie");

    // Set io as unbuffered
    DC_ASSERT(setvbuf(f, NULL, _IONBF, 0) == 0, "setvbuf failed");

    return f;
}
