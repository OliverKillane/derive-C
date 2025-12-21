/// @brief A simple, portable string builder that can be templated by allocator.
///  - Uses GNU additional feature fopencookie, so not available without glibc.

#include <derive-c/core/includes/def.h>
#if !defined(SKIP_INCLUDES)
    #include "includes.h"
#endif

#if !defined _GNU_SOURCE
    #error "_GNU_SOURCE must be defined (is in the src/derive-c CMakeLists.txt) to use cookie_io"
#endif

#include <derive-c/core/alloc/def.h>
#include <derive-c/core/self/def.h>

typedef struct {
    FILE* stream;
    char* buf;
    size_t size_without_null;
    size_t capacity;
    ALLOC* alloc;
} SELF;

static size_t const NS(SELF, additional_alloc_size) = 32;

#define INVARIANT_CHECK(self)                                                                      \
    DC_ASSUME(self);                                                                               \
    DC_ASSUME((self)->alloc);                                                                      \
    DC_ASSUME(DC_WHEN((self)->buf, (self)->stream && (self)->capacity > 0));                       \
    DC_ASSUME(DC_WHEN((self)->capacity == 0, !(self)->buf));                                       \
    DC_ASSUME(DC_WHEN((self)->buf, (self)->size_without_null + 1 <= (self)->capacity));

static ssize_t PRIV(NS(SELF, read))(void* capture,
                                    char* buf /*NOLINT(readability-non-const-parameter)*/,
                                    size_t size) {
    SELF* self = (SELF*)capture;
    INVARIANT_CHECK(self);

    (void)buf;
    (void)size;

    DC_PANIC("cookie set in write-only mode, but read was called.");
}

static ssize_t PRIV(NS(SELF, write))(void* capture, const char* data, size_t size) {
    SELF* self = (SELF*)capture;
    INVARIANT_CHECK(self);

    if (self->size_without_null + size + 1 > self->capacity) {
        size_t new_capacity;
        char* new_buf;
        if (self->buf != NULL) {
            size_t const growth_factor = 2;
            new_capacity =
                (self->capacity * growth_factor) + size + NS(SELF, additional_alloc_size);
            new_buf = (char*)NS(ALLOC, realloc)(self->alloc, self->buf, new_capacity);
        } else {
            DC_ASSUME(self->capacity == 0);
            new_capacity = size + 1 + NS(SELF, additional_alloc_size);
            new_buf = (char*)NS(ALLOC, malloc)(self->alloc, new_capacity);
        }

        if (!new_buf) {
            errno = ENOMEM; // NOLINT(misc-include-cleaner)
            return -1;
        }

        self->capacity = new_capacity;
        self->buf = new_buf;
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

    errno = EPERM; // NOLINT(misc-include-cleaner)
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
        DC_ASSERT(self->stream != NULL, "Failed to open stream for string builder, with error: %s",
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

#include <derive-c/core/self/undef.h>
#include <derive-c/core/alloc/undef.h>
#include <derive-c/core/includes/undef.h>
