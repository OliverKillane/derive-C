/// @brief A wrapper allocator, that can also be used as a mock.
/// A transparent wrapper around any other allocator.
/// - Used in fuzz tests to match the SutNS/templated struct naming interface the fuzz tests rely
/// on.
/// - Used in release by the test allocator.
/// - Not entirely zero cost, as the reference type is pointer size, even if the wrapped allocator's
/// reference is not.
///

#include <derive-c/core/includes/def.h>
#if !defined(SKIP_INCLUDES)
    #include "includes.h"
#endif

#include <derive-c/test/mock_in_struct/def.h>
#include <derive-c/core/alloc/def.h>
#include <derive-c/core/self/def.h>

typedef struct {
    NS(ALLOC, ref) alloc_ref;
} SELF;

static SELF NS(SELF, new)(NS(ALLOC, ref) alloc_ref) { return (SELF){.alloc_ref = alloc_ref}; }

MOCKABLE(void*, NS(SELF, malloc), (SELF * self, size_t size)) {
    return NS(ALLOC, malloc)(NS(NS(ALLOC, ref), write)(&self->alloc_ref), size);
}

MOCKABLE(void*, NS(SELF, calloc), (SELF * self, size_t count, size_t size)) {
    return NS(ALLOC, calloc)(NS(NS(ALLOC, ref), write)(&self->alloc_ref), count, size);
}

MOCKABLE(void*, NS(SELF, realloc), (SELF * self, void* ptr, size_t size)) {
    return NS(ALLOC, realloc)(NS(NS(ALLOC, ref), write)(&self->alloc_ref), ptr, size);
}

MOCKABLE(void, NS(SELF, free), (SELF * self, void* ptr)) {
    NS(ALLOC, free)(NS(NS(ALLOC, ref), write)(&self->alloc_ref), ptr);
}

static void NS(SELF, debug)(SELF const* self, dc_debug_fmt fmt, FILE* stream) {
    fprintf(stream, STRINGIFY(SELF) " @%p {", self);
    fmt = dc_debug_fmt_scope_begin(fmt);
    dc_debug_fmt_print(fmt, stream, "alloc: " STRINGIFY(ALLOC) "@%p,\n",
                       NS(NS(ALLOC, ref), read)(&self->alloc_ref));
    fmt = dc_debug_fmt_scope_end(fmt);
    dc_debug_fmt_print(fmt, stream, "}");
}

static void NS(SELF, delete)(SELF* self) { DC_ASSUME(self); }

DC_TRAIT_REFERENCABLE_BY_PTR(SELF);

DC_TRAIT_ALLOC(SELF);

#include <derive-c/core/self/undef.h>
#include <derive-c/core/alloc/undef.h>
#include <derive-c/test/mock_in_struct/undef.h>
#include <derive-c/core/includes/undef.h>
