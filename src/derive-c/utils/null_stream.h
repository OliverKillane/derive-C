#pragma once

#include <derive-c/core/prelude.h>

#include <stdio.h>
#include <sys/types.h>

#if !defined _GNU_SOURCE
    #error "_GNU_SOURCE must be defined (is in the src/derive-c CMakeLists.txt) to use cookie_io"
#endif

static ssize_t PRIV(dc_null_write)(void* cookie, const char* buf, size_t size) {
    (void)cookie;
    (void)buf;
    return (ssize_t)size; // report "all bytes written"
}

FILE* dc_null_stream(void) {
    cookie_io_functions_t io = {
        .read = NULL,
        .write = PRIV(dc_null_write),
        .seek = NULL,
        .close = NULL,
    };

    FILE* f = fopencookie(NULL, "w", io);
    DC_ASSERT(f, "received nullptr from fopencookie");

    // Set io as unbuffered
    setvbuf(f, NULL, _IONBF, 0);

    return f;
}
