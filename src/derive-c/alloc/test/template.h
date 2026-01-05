/// @brief For unit tests expected to throw, as C has no unwind, we cannot free allocated memory.
///        This macro wraps the allocator in debug, to allow clearing leaks after an exception.
///
/// In release, it is a no-op / pass through.
/// - Not entirely zero cost, as the reference type is pointer size, even if the wrapped allocator's
/// reference is not.
///
/// As this is entirely C, we do not get the niceties of a C++ RAII allocator guard shebang.
/// However, this is usable inside unit tests written in C.

#include "derive-c/core/panic.h"
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

static void NS(TRACKED_ENTRY, debug)(TRACKED_ENTRY const* self, dc_debug_fmt /* fmt */,
                                     FILE* stream) {
    fprintf(stream, "{ ptr: %p, state: %s }", self->ptr, self->freed ? "freed" : "alive");
}

    #pragma push_macro("ALLOC")

    // JUSTIFY: Using a vector rather than a faster lookup map.
    //           - Give this will be used for test & debug, performance matters less
    //           - Much easier to explore a vector, than a hashmap in gdb.
    // JUSTIFY: Always use the std allocator for test book keeping
    //          - keeps the observed behaviour (e.g. allocator usage) the same as in release
    #define ITEM TRACKED_ENTRY // [DERIVE-C] for template
    #define ITEM_DEBUG NS(TRACKED_ENTRY, debug)
    #define ALLOC stdalloc               // [DERIVE-C] for template
    #define INTERNAL_NAME ENTRIES_VECTOR // [DERIVE-C] for template
    #include <derive-c/container/vector/dynamic/template.h>

    #pragma pop_macro("ALLOC")

typedef struct {
    NS(ALLOC, ref) alloc_ref;
    ENTRIES_VECTOR entries;
} SELF;

static SELF NS(SELF, new)(NS(ALLOC, ref) alloc_ref) {
    return (SELF){.alloc_ref = alloc_ref, .entries = NS(ENTRIES_VECTOR, new)(stdalloc_get_ref())};
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
            NS(ALLOC, free)(self->alloc_ref, entry->ptr);
        }
    }

    NS(SELF, delete)(self);
}

static void* NS(SELF, calloc)(SELF* self, size_t count, size_t size) {
    DC_ASSUME(self);
    void* ptr = NS(ALLOC, calloc)(self->alloc_ref, count, size);
    NS(ENTRIES_VECTOR, push)(&self->entries, (TRACKED_ENTRY){
                                                 .ptr = ptr,
                                                 .freed = false,
                                             });
    return ptr;
}

static void* NS(SELF, malloc)(SELF* self, size_t size) {
    DC_ASSUME(self);
    void* ptr = NS(ALLOC, malloc)(self->alloc_ref, size);
    NS(ENTRIES_VECTOR, push)(&self->entries, (TRACKED_ENTRY){
                                                 .ptr = ptr,
                                                 .freed = false,
                                             });
    return ptr;
}

static void* NS(SELF, realloc)(SELF* self, void* ptr, size_t size) {
    DC_ASSUME(self);
    DC_ASSUME(ptr);
    FOR(ENTRIES_VECTOR, &self->entries, iter, entry) {
        if (entry->ptr == ptr) {
            void* new_ptr = NS(ALLOC, realloc)(self->alloc_ref, ptr, size);
            entry->ptr = new_ptr;
            return new_ptr;
        }
    }
    DC_UNREACHABLE("ptr was not present in the entries list");
}

static void NS(SELF, free)(SELF* self, void* ptr) {
    DC_ASSUME(ptr);
    DC_ASSUME(self);

    FOR(ENTRIES_VECTOR, &self->entries, iter, entry) {
        if (entry->ptr == ptr) {
            DC_ASSUME(!entry->freed);
            entry->freed = true;
            NS(ALLOC, free)(self->alloc_ref, ptr);
            return;
        }
    }
    DC_UNREACHABLE("ptr was not present in the entries list");
}

static void NS(SELF, debug)(SELF const* self, dc_debug_fmt fmt, FILE* stream) {
    fprintf(stream, EXPAND_STRING(SELF) " @%p {\n", self);
    fmt = dc_debug_fmt_scope_begin(fmt);
    dc_debug_fmt_print(fmt, stream, "base: " EXPAND_STRING(ALLOC) "@%p,\n",
                       NS(NS(ALLOC, ref), deref)(self->alloc_ref));

    dc_debug_fmt_print(fmt, stream, "entries: ");
    NS(ENTRIES_VECTOR, debug)(&self->entries, fmt, stream);
    fprintf(stream, "\n");

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
