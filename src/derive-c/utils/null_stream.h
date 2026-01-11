#pragma once

#include <derive-c/core/prelude.h>

#include <stdio.h>
#include <sys/types.h>

#if !defined _GNU_SOURCE
    #error "_GNU_SOURCE must be defined (is in the src/derive-c CMakeLists.txt) to use cookie_io"
#endif

static ssize_t PRIV(dc_null_read)(void* /* capture */,
                                  char* buf /*NOLINT(readability-non-const-parameter)*/,
                                  size_t size) {
    DC_PANIC("Attempted to read %zu bytes into %p from null stream, which is not readable", size,
             buf);
}

static ssize_t PRIV(dc_null_write)(void* cookie, const char* buf, size_t size) {
    (void)cookie;
    (void)buf;
    return (ssize_t)size; // report "all bytes written"
}

static int PRIV(dc_null_seek)(void* /* capture */,
                              off_t* /* offset */ /*NOLINT(readability-non-const-parameter)*/,
                              int /* whence */) {
    errno = EPERM; // NOLINT(misc-include-cleaner)
    return -1;
}

static int PRIV(dc_null_close)(void* /* capture */) { return 0; }

FILE* dc_null_stream(void) {
    cookie_io_functions_t io = {
        .read = PRIV(dc_null_read),
        .write = PRIV(dc_null_write),
        .seek = PRIV(dc_null_seek),
        .close = PRIV(dc_null_close),
    };

    FILE* f = fopencookie(NULL, "w", io);
    DC_ASSERT(f, "received nullptr from fopencookie");

    // Set io as unbuffered
    setvbuf(f, NULL, _IONBF, 0);

    return f;
}
