/// @brief For unit tests expected to throw, as C has no unwind, we cannot free allocated memory.
///        This macro wraps the allocator in debug, to allow clearing leaks after an exception.
///
/// In release, it is a no-op / pass through.
/// - Not entirely zero cost, as the reference type is pointer size, even if the wrapped allocator's
/// reference is not.
///
/// As this is entirely C, we do not get the niceties of a C++ RAII allocator guard shebang.
/// However, this is usable inside unit tests written in C.

#ifdef NDEBUG
    #include <derive-c/alloc/wrap/template.h>
#else

    #include <derive-c/core/includes/def.h>
    #if !defined(SKIP_INCLUDES)
        #include "includes.h"
    #endif

    #include <derive-c/core/alloc/def.h>
    #include <derive-c/core/self/def.h>

    #include <derive-c/alloc/std.h>

    #define ALLOCATIONS_MAP NS(NAME, allocations)

    #pragma push_macro("ALLOC")

    // JUSTIFY: Always use the std allocator for test book keeping
    //          - keeps the observed behaviour (e.g. allocator usage) the same as in release
    #define ALLOC stdalloc                   // [DERIVE-C] for template
    #define KEY void*                        // [DERIVE-C] for template
    #define KEY_HASH(ptr) ((size_t)(*(ptr))) // [DERIVE-C] for template
    #define KEY_DEBUG dc_void_ptr_debug      // [DERIVE-C] for template
    #define VALUE size_t                     // [DERIVE-C] for template
    #define INTERNAL_NAME ALLOCATIONS_MAP    // [DERIVE-C] for template
    #include <derive-c/container/map/swiss/template.h>

    #pragma pop_macro("ALLOC")

typedef struct {
    NS(ALLOC, ref) alloc_ref;
    ALLOCATIONS_MAP allocations;
} SELF;

DC_PUBLIC static SELF NS(SELF, new)(NS(ALLOC, ref) alloc_ref) {
    return (SELF){
        .alloc_ref = alloc_ref,
        .allocations = NS(ALLOCATIONS_MAP, new)(stdalloc_get_ref()),
    };
}

DC_PUBLIC static ALLOCATIONS_MAP const* NS(SELF, get_allocations)(SELF const* self) {
    DC_ASSUME(self);
    return &self->allocations;
}

DC_PUBLIC static void NS(SELF, delete)(SELF* self) {
    NS(ALLOCATIONS_MAP, delete)(&self->allocations);
}

DC_PUBLIC static void NS(SELF, unleak)(SELF* self) {
    DC_FOR(ALLOCATIONS_MAP, &self->allocations, iter, entry) {
        NS(ALLOC, deallocate)(self->alloc_ref, *entry.key, *entry.value);
    }
}

DC_PUBLIC static void* NS(SELF, allocate_zeroed)(SELF* self, size_t size) {
    DC_ASSUME(self);
    void* ptr = NS(ALLOC, allocate_zeroed)(self->alloc_ref, size);
    size_t* alloc = NS(ALLOCATIONS_MAP, try_insert)(&self->allocations, ptr, size);
    DC_ASSERT(alloc != NULL,
              "Got zeroed allocation, that is already allocated at %p (attempted size: %zu)", ptr,
              size);
    return ptr;
}

DC_PUBLIC static void* NS(SELF, allocate_uninit)(SELF* self, size_t size) {
    DC_ASSUME(self);
    void* ptr = NS(ALLOC, allocate_uninit)(self->alloc_ref, size);
    size_t* alloc = NS(ALLOCATIONS_MAP, try_insert)(&self->allocations, ptr, size);
    DC_ASSERT(alloc != NULL,
              "Got uninit allocation, that is already allocated at %p (attempted size: %zu)", ptr,
              *alloc);
    return ptr;
}

DC_PUBLIC static void* NS(SELF, reallocate)(SELF* self, void* old_ptr, size_t old_size,
                                            size_t new_size) {
    DC_ASSUME(self);
    DC_ASSUME(old_ptr);

    size_t const* tracked_size = NS(ALLOCATIONS_MAP, try_read)(&self->allocations, old_ptr);
    DC_ASSERT(tracked_size != NULL, "Reallocating pointer that is not allocated");
    DC_ASSERT(*tracked_size == old_size,
              "Incorrect size provided for reallocation of %p (was %zu, but expected %zu)", old_ptr,
              old_size, *tracked_size);
    NS(ALLOCATIONS_MAP, delete_entry)(&self->allocations, old_ptr);

    void* new_ptr = NS(ALLOC, reallocate)(self->alloc_ref, old_ptr, old_size, new_size);

    size_t* alloc = NS(ALLOCATIONS_MAP, try_insert)(&self->allocations, new_ptr, new_size);
    DC_ASSERT(alloc != NULL,
              "Got new reallocation, that is already allocated at %p (attempted size: %zu)",
              new_ptr, new_size);

    return new_ptr;
}

DC_PUBLIC static void NS(SELF, deallocate)(SELF* self, void* ptr, size_t size) {
    DC_ASSUME(ptr);
    DC_ASSUME(self);

    size_t const* tracked_size = NS(ALLOCATIONS_MAP, try_read)(&self->allocations, ptr);
    DC_ASSERT(tracked_size != NULL,
              "Attempted to deallocate %p (size: %zu), but was not already allocated", ptr, size);
    DC_ASSERT(*tracked_size == size,
              "Incorrect size passed on %p deallocation (was: %zu, expected: %zu)", ptr,
              *tracked_size, size);

    NS(ALLOCATIONS_MAP, delete_entry)(&self->allocations, ptr);

    NS(ALLOC, deallocate)(self->alloc_ref, ptr, size);
}

DC_PUBLIC static void NS(SELF, debug)(SELF const* self, dc_debug_fmt fmt, FILE* stream) {
    fprintf(stream, DC_EXPAND_STRING(SELF) " @%p {\n", self);
    fmt = dc_debug_fmt_scope_begin(fmt);
    dc_debug_fmt_print(fmt, stream, "base: " DC_EXPAND_STRING(ALLOC) "@%p,\n",
                       (void*)NS(NS(ALLOC, ref), deref)(self->alloc_ref));

    dc_debug_fmt_print(fmt, stream, "allocations: ");
    NS(ALLOCATIONS_MAP, debug)(&self->allocations, fmt, stream);
    fprintf(stream, "\n");

    fmt = dc_debug_fmt_scope_end(fmt);
    dc_debug_fmt_print(fmt, stream, "}");
}

    #undef ALLOCATIONS_MAP

DC_TRAIT_REFERENCABLE_BY_PTR(SELF);

DC_TRAIT_ALLOC(SELF);

    #include <derive-c/core/self/undef.h>
    #include <derive-c/core/alloc/undef.h>
    #include <derive-c/core/includes/undef.h>

#endif
