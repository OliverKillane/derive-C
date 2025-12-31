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
    #define ENTRIES_VECTOR NS(NAME, entries)
    #define TRACKED_ENTRY NS(EXPAND(ENTRIES), entry)

typedef struct {
    void* ptr;
    bool freed;
} TRACKED_ENTRY;

    #pragma push_macro("ALLOC")

    // JUSTIFY: Using a vector rather than a faster lookup map.
    //           - Give this will be used for test & debug, performance matters less
    //           - Much easier to explore a vector, than a hashmap in gdb.
    // JUSTIFY: Always use the std allocator for test book keeping
    //          - keeps the observed behaviour (e.g. allocator usage) the same as in release
    #define ITEM TRACKED_ENTRY           // [DERIVE-C] for template
    #define ALLOC stdalloc               // [DERIVE-C] for template
    #define INTERNAL_NAME ENTRIES_VECTOR // [DERIVE-C] for template
    #include <derive-c/container/vector/dynamic/template.h>

    #pragma pop_macro("ALLOC")

typedef struct {
    ALLOC* alloc;
    ENTRIES_VECTOR entries;
} SELF;

static SELF NS(SELF, new)(ALLOC* alloc) {
    return (SELF){.alloc = alloc, .entries = NS(ENTRIES_VECTOR, new)(stdalloc_get())};
}

static ENTRIES_VECTOR const* NS(SELF, get_entries)(SELF const* self) {
    DC_ASSUME(self);
    return &self->entries;
}

static void NS(SELF, delete)(SELF* self) { NS(ENTRIES_VECTOR, delete)(&self->entries); }

static void NS(SELF, unleak_and_delete)(SELF* self) {
    NS(ENTRIES_VECTOR, iter) iter = NS(ENTRIES_VECTOR, get_iter)(&self->entries);
    TRACKED_ENTRY* entry;

    while ((entry = NS(ENTRIES_VECTOR, iter_next)(&iter))) {
        if (!entry->freed) {
            NS(ALLOC, free)(self->alloc, entry->ptr);
        }
    }

    NS(SELF, delete)(self);
}

static void* NS(SELF, calloc)(SELF* self, size_t count, size_t size) {
    DC_ASSUME(self);
    void* ptr = NS(ALLOC, calloc)(self->alloc, count, size);
    if (ptr) {
        NS(ENTRIES_VECTOR, push)(&self->entries, (TRACKED_ENTRY){
                                                     .ptr = ptr,
                                                     .freed = false,
                                                 });
    }
    return ptr;
}

static void* NS(SELF, malloc)(SELF* self, size_t size) {
    DC_ASSUME(self);
    void* ptr = NS(ALLOC, malloc)(self->alloc, size);
    if (ptr) {
        NS(ENTRIES_VECTOR, push)(&self->entries, (TRACKED_ENTRY){
                                                     .ptr = ptr,
                                                     .freed = false,
                                                 });
    }
    return ptr;
}

static void* NS(SELF, realloc)(SELF* self, void* ptr, size_t size) {
    DC_ASSUME(self);
    DC_ASSUME(ptr);
    return NS(ALLOC, realloc)(self->alloc, ptr, size);
}

static void NS(SELF, free)(SELF* self, void* ptr) {
    DC_ASSUME(ptr);
    DC_ASSUME(self);

    NS(ENTRIES_VECTOR, iter) iter = NS(ENTRIES_VECTOR, get_iter)(&self->entries);
    TRACKED_ENTRY* entry;

    while ((entry = NS(ENTRIES_VECTOR, iter_next)(&iter))) {
        if (entry->ptr == ptr) {
            DC_ASSUME(!entry->freed);
            entry->freed = true;
            break;
        }
    }

    NS(ALLOC, free)(self->alloc, ptr);
}

static void NS(SELF, debug)(SELF const* self, dc_debug_fmt fmt, FILE* stream) {
    fprintf(stream, STRINGIFY(SELF) " @%p {", self);
    fmt = dc_debug_fmt_scope_begin(fmt);
    dc_debug_fmt_print(fmt, stream, "base: " STRINGIFY(ALLOC) "@%p,\n", self->alloc);
    NS(ENTRIES_VECTOR, debug)(&self->entries, fmt, stream);
    fmt = dc_debug_fmt_scope_end(fmt);
    dc_debug_fmt_print(fmt, stream, "}");
}

    #undef TRACKED_ENTRY
    #undef ENTRIES_VECTOR

DC_TRAIT_REFERENCABLE_BY_PTR(SELF);

DC_TRAIT_ALLOC(SELF);

    #include <derive-c/core/self/undef.h>
    #include <derive-c/core/alloc/undef.h>
    #include <derive-c/core/includes/undef.h>

#endif
