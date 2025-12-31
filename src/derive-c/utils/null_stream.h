#pragma once

#include <derive-c/core/prelude.h>
#include <stdio.h>

#if !defined _GNU_SOURCE
    #error "_GNU_SOURCE must be defined (is in the src/derive-c CMakeLists.txt) to use cookie_io"
#endif

static ssize_t PRIV(dc_null_write)(void* cookie, const char* buf, size_t size) {
    (void)cookie;
    (void)buf;
    return (ssize_t)size; // report "all bytes written"
}

FILE* dc_null_stream() {
    return fopencookie(NULL, "w",
                       (cookie_io_functions_t){
                           .read = NULL,
                           .write = PRIV(dc_null_write),
                           .seek = NULL,
                           .close = NULL,
                       });
}
