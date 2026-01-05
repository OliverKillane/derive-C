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

MOCKABLE(void*, NS(SELF, allocate_uninit), (SELF * self, size_t size)) {
    return NS(ALLOC, allocate_uninit)(self->alloc_ref, size);
}

MOCKABLE(void*, NS(SELF, allocate_zeroed), (SELF * self, size_t size)) {
    return NS(ALLOC, allocate_zeroed)(self->alloc_ref, size);
}

MOCKABLE(void*, NS(SELF, reallocate), (SELF * self, void* ptr, size_t old_size, size_t new_size)) {
    return NS(ALLOC, reallocate)(self->alloc_ref, ptr, old_size, new_size);
}

MOCKABLE(void, NS(SELF, deallocate), (SELF * self, void* ptr, size_t size)) {
    NS(ALLOC, deallocate)(self->alloc_ref, ptr, size);
}

static void NS(SELF, debug)(SELF const* self, dc_debug_fmt fmt, FILE* stream) {
    fprintf(stream, EXPAND_STRING(SELF) "@%p {\n", self);
    fmt = dc_debug_fmt_scope_begin(fmt);
    dc_debug_fmt_print(fmt, stream, "alloc: ");
    NS(ALLOC, debug)(NS(NS(ALLOC, ref), deref)(self->alloc_ref), fmt, stream);
    fprintf(stream, ",\n");

    dc_debug_fmt_print(fmt, stream, "mocking: {\n");
    fmt = dc_debug_fmt_scope_begin(fmt);

#define DEBUG_MOCK_TOGGLE(name)                                                                    \
    dc_debug_fmt_print(fmt, stream, EXPAND_STRING(NS(SELF, name)) ": %s,\n",                       \
                       MOCKABLE_ENABLED(NS(SELF, name)) ? "enabled" : "disabled")

    DEBUG_MOCK_TOGGLE(allocate_uninit);
    DEBUG_MOCK_TOGGLE(allocate_zeroed);
    DEBUG_MOCK_TOGGLE(reallocate);
    DEBUG_MOCK_TOGGLE(deallocate);

#undef DEBUG_MOCK_TOGGLE

    fmt = dc_debug_fmt_scope_end(fmt);
    dc_debug_fmt_print(fmt, stream, "}\n");

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
