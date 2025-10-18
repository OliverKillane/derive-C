/// @brief A simple, portable string builder that can be templated by allocator.
///  - Uses GNU additional feature fopencookie, so not available without glibc.

#if !defined _GNU_SOURCE
    // Enable gnu specific features for fopencookie
    //  - Must be set before stdio include
    //  - see: https://www.man7.org/linux/man-pages/man7/feature_test_macros.7.html
    #define _GNU_SOURCE
#endif

#include <derive-c/core/prelude.h>

#include <errno.h>
#include <stdio.h>

#include <derive-c/core/alloc/def.h>
#include <derive-c/core/self/def.h>
#include <string.h>

typedef struct {
    FILE* stream;
    char* buf;
    size_t size_without_null;
    size_t capacity;
    ALLOC* alloc;
} SELF;

#define INVARIANT_CHECK(self)                                                                      \
    ASSUME(self);                                                                                  \
    ASSUME((self)->alloc);                                                                         \
    ASSUME(WHEN((self)->buf, (self)->stream && (self)->capacity > 0));                             \
    ASSUME(WHEN((self)->capacity == 0, !(self)->buf));                                             \
    ASSUME(WHEN((self)->buf, (self)->size_without_null + 1 <= (self)->capacity));

static ssize_t PRIV(NS(SELF, read))(void* cookie,
                                    char* buf /*NOLINT(readability-non-const-parameter)*/,
                                    size_t size) {
    (void)cookie;
    (void)buf;
    (void)size;
    PANIC("Cannot read on a string builder");
}

static ssize_t PRIV(NS(SELF, write))(void* capture, const char* data, size_t size) {
    SELF* self = (SELF*)capture;
    INVARIANT_CHECK(self);

    size_t const for_subsequent_small_inserts = 32;
    if (self->buf != NULL) {
        if (self->size_without_null + size + 1 > self->capacity) {
            size_t const growth_factor = 2;

            self->capacity = (self->capacity * growth_factor) + size + for_subsequent_small_inserts;
            self->buf = (char*)NS(ALLOC, realloc)(self->alloc, self->buf, self->capacity);
        }
    } else {
        ASSUME(self->capacity == 0);
        self->capacity = size + 1 + for_subsequent_small_inserts;
        self->buf = (char*)NS(ALLOC, malloc)(self->alloc, self->capacity);
    }

    if (!self->buf) {
        errno = ENOMEM;
        return -1;
    }

    memcpy(self->buf + self->size_without_null, data, size);
    self->size_without_null += size;
    self->buf[self->size_without_null] = '\0';
    return (ssize_t)size;
}

static int PRIV(NS(SELF, seek))(void* capture,
                                off_t* offset /*NOLINT(readability-non-const-parameter)*/,
                                int whence) {
    SELF* self = (SELF*)capture;
    INVARIANT_CHECK(self);
    (void)offset;
    (void)whence;

    errno = ESPIPE;
    return -1;
}

static int PRIV(NS(SELF, close))(void* capture) {
    SELF* self = (SELF*)capture;
    INVARIANT_CHECK(self);
    return 0;
}

static SELF NS(SELF, new)(ALLOC* alloc) {
    return (SELF){
        .stream = NULL,
        .buf = NULL,
        .size_without_null = 0,
        .capacity = 0,
        .alloc = alloc,
    };
}

/// Opens a file for
static FILE* NS(SELF, stream)(SELF* self) {
    INVARIANT_CHECK(self);

    if (self->stream == NULL) {
        cookie_io_functions_t /* NOLINT(misc-include-cleaner) */ const io = {
            .read = PRIV(NS(SELF, read)),
            .write = PRIV(NS(SELF, write)),
            .seek = PRIV(NS(SELF, seek)),
            .close = PRIV(NS(SELF, close)),
        };
        self->stream = fopencookie(self, "w", io);
        ASSERT(self->stream != NULL, "Failed to open stream for string builder, with error: %s",
               strerror(errno));

        // JUSTIFY: No buffering
        // - No performance advantage to buffering writes - this already writes to an in memory
        //   buffer
        // - No need to fflush
        setvbuf(self->stream, NULL, _IONBF, 0);
    }

    return self->stream;
}

/// Resets the string, but keps the same stream pointer alive.
static void NS(SELF, reset)(SELF* self) {
    INVARIANT_CHECK(self);
    self->size_without_null = 0;
}

/// Gets access to the null terminated string
static char const* NS(SELF, string)(SELF const* self) {
    INVARIANT_CHECK(self);
    if (self->size_without_null == 0) {
        return "";
    }
    return self->buf;
}

/// Disowns the current string, free/management with chosen allocator determined by user.
static char* NS(SELF, release_string)(SELF* self) {
    INVARIANT_CHECK(self);
    char* buf = self->buf;
    self->size_without_null = 0;
    self->buf = NULL;
    self->capacity = 0;

    return buf;
}

static size_t NS(SELF, string_size)(SELF* self) {
    INVARIANT_CHECK(self);
    return self->size_without_null;
}

static void NS(SELF, delete)(SELF* self) {
    INVARIANT_CHECK(self);
    if (self->stream) {
        fclose(self->stream);
    }
    if (self->buf) {
        NS(ALLOC, free)(self->alloc, self->buf);
    }
}

#undef INVARIANT_CHECK

#include <derive-c/core/alloc/undef.h>
#include <derive-c/core/self/undef.h>
