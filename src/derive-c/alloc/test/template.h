/// @brief For unit tests expected to throw, as C has no unwind, we cannot free allocated memory.
///        This macro wraps the allocator in debug, to allow clearing leaks after an exception.
///
/// In release, it is a no-op / pass through.
///
/// As this is entirely C, we do not get the niceties of a C++ RAII allocator guard shebang.
// However, this is usable inside unit tests written in C.

#include <stddef.h>

#include <derive-c/alloc/std.h>
#include <derive-c/core/helpers.h>
#include <derive-c/core/panic.h>

#include <derive-c/core/alloc/def.h>
#include <derive-c/core/self/def.h>

#ifdef NDEBUG
typedef struct {
    ALLOC* alloc;
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

static void NS(SELF, free)(SELF* self, void* ptr) { return NS(ALLOC, free)(self->alloc, ptr); }

#else
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
    #define ITEM TRACKED_ENTRY
    #define ALLOC stdalloc
    #define INTERNAL_NAME ENTRIES_VECTOR
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
    DEBUG_ASSERT(self);
    return &self->entries;
}

static void NS(SELF, unleak_and_delete)(SELF* self) {
    NS(ENTRIES_VECTOR, iter) iter = NS(ENTRIES_VECTOR, get_iter)(&self->entries);
    TRACKED_ENTRY* entry;

    while ((entry = NS(ENTRIES_VECTOR, iter_next)(&iter))) {
        if (!entry->freed) {
            NS(ALLOC, free)(self->alloc, entry->ptr);
        }
    }

    NS(ENTRIES_VECTOR, delete)(&self->entries);
}

static void* NS(SELF, calloc)(SELF* self, size_t count, size_t size) {
    DEBUG_ASSERT(self);
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
    DEBUG_ASSERT(self);
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
    DEBUG_ASSERT(self);
    DEBUG_ASSERT(ptr);
    return NS(ALLOC, realloc)(self->alloc, ptr, size);
}

static void NS(SELF, free)(SELF* self, void* ptr) {
    DEBUG_ASSERT(ptr);
    DEBUG_ASSERT(self);

    NS(ENTRIES_VECTOR, iter) iter = NS(ENTRIES_VECTOR, get_iter)(&self->entries);
    TRACKED_ENTRY* entry;

    while ((entry = NS(ENTRIES_VECTOR, iter_next)(&iter))) {
        if (entry->ptr == ptr) {
            DEBUG_ASSERT(!entry->freed);
            entry->freed = true;
            break;
        }
    }

    NS(ALLOC, free)(self->alloc, ptr);
}

    #undef ENTRIES_VECTOR
    #undef TRACKED_ENTRY
#endif

#include <derive-c/core/alloc/undef.h>
#include <derive-c/core/self/undef.h>
