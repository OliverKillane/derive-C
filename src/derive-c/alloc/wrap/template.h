/// @brief A wrapper allocator
/// A transparent wrapper around any other allocator.
/// - Used in fuzz tests to match the SutNS/templated struct naming interface the fuzz tests rely
/// on.
/// - Used in release by the test allocator.
/// - Not entirely zero cost, as the reference type is pointer size, even if the wrapped allocator's
/// reference is not.

#include <derive-c/core/includes/def.h>
#if !defined(SKIP_INCLUDES)
    #include "includes.h"
#endif

#include <derive-c/core/alloc/def.h>
#include <derive-c/core/self/def.h>

typedef struct {
    ALLOC* alloc; /* not owned */
} SELF;

static SELF NS(SELF, new)(ALLOC* alloc) { return (SELF){.alloc = alloc}; }

static void* NS(SELF, malloc)(SELF* self, size_t size) {
    return NS(ALLOC, malloc)(self->alloc, size);
}

static void* NS(SELF, calloc)(SELF* self, size_t count, size_t size) {
    return NS(ALLOC, calloc)(self->alloc, count, size);
}

static void* NS(SELF, realloc)(SELF* self, void* ptr, size_t size) {
    return NS(ALLOC, realloc)(self->alloc, ptr, size);
}

static void NS(SELF, free)(SELF* self, void* ptr) { NS(ALLOC, free)(self->alloc, ptr); }

static void NS(SELF, debug)(SELF const* self, dc_debug_fmt fmt, FILE* stream) {
    fprintf(stream, STRINGIFY(SELF) " @%p {", self);
    fmt = dc_debug_fmt_scope_begin(fmt);
    dc_debug_fmt_print(fmt, stream, "alloc: " STRINGIFY(ALLOC) "@%p,\n", self->alloc);
    fmt = dc_debug_fmt_scope_end(fmt);
    dc_debug_fmt_print(fmt, stream, "}");
}

static void NS(SELF, delete)(SELF* self) { DC_ASSUME(self); }

DC_TRAIT_REFERENCABLE_BY_PTR(SELF);

DC_TRAIT_ALLOC(SELF);

#include <derive-c/core/self/undef.h>
#include <derive-c/core/alloc/undef.h>
#include <derive-c/core/includes/undef.h>
