/// @brief For unit tests expected to throw, as C has no unwind, we cannot free allocated memory.
///        This macro wraps the allocator in debug, to allow clearing leaks after an exception.
///
/// In release, it is a no-op / pass through.
///
/// As this is entirely C, we do not get the niceties of a C++ RAII allocator guard shebang.
// However, this is usable inside unit tests written in C.

#include <stddef.h>

#include <derive-c/allocs/std.h>
#include <derive-c/core.h>
#include <derive-c/panic.h>

#include <derive-c/self/def.h>

#if !defined ALLOC
    #if !defined __clang_analyzer__
        #error "The allocator being wrapped must be defined"
    #endif
    #include <derive-c/allocs/null.h> // NOLINT(misc-include-cleaner)
    #define ALLOC nullalloc
#endif

#if !defined ENTRIES
    #if !defined __clang_analyzer__
        #error "The name of the type to generate for storing entries must be defined"
    #endif
    #define ENTRIES derive_c_entries_placeholder_name
#endif

#ifdef NDEBUG
    typedef struct {
        ALLOC* alloc;
    } SELF;

    static SELF NAME(SELF, new)(ALLOC* alloc) { return (SELF){.alloc = alloc}; }

    static void* NAME(SELF, malloc)(SELF* self, size_t size) {
        return NAME(ALLOC, malloc)(self->alloc, size);
    }

    static void* NAME(SELF, calloc)(SELF* self, size_t count, size_t size) {
        return NAME(ALLOC, calloc)(self->alloc, count, size);
    }

    static void* NAME(SELF, realloc)(SELF* self, void* ptr, size_t size) {
        return NAME(ALLOC, realloc)(self->alloc, ptr, size);
    }

    static void NAME(SELF, free)(SELF* self, void* ptr) { return NAME(ALLOC, free)(self->alloc, ptr); }

#else
    #define TRACKED_ENTRY NAME(ENTRIES, entry)

    typedef struct {
        void* ptr;
        bool freed;
    } TRACKED_ENTRY;

    #pragma push_macro("SELF")
    #undef SELF

    // JUSTIFY: Using a vector rather than a faster lookup map.
    //           - Give this will be used for test & debug, performance matters less
    //           - Much easier to explore a vector, than a hashmap in gdb.
    #define T TRACKED_ENTRY
    #define ALLOC stdalloc
    #define SELF ENTRIES
    #include <derive-c/structures/vector/template.h>

    #pragma pop_macro("SELF")

    typedef struct {
        ALLOC* alloc;
        ENTRIES entries;
    } SELF;

    static SELF NAME(SELF, new)(ALLOC* alloc) {
        return (SELF){.alloc = alloc, .entries = NAME(ENTRIES, new)(stdalloc_get())};
    }

    static ENTRIES const* NAME(SELF, get_entries)(SELF const* self) {
        DEBUG_ASSERT(self);
        return &self->entries;
    }

    static void NAME(SELF, unleak_and_delete)(SELF* self) {
        NAME(ENTRIES, iter) iter = NAME(ENTRIES, get_iter)(&self->entries);
        TRACKED_ENTRY* entry;

        while ((entry = NAME(ENTRIES, iter_next)(&iter))) {
            if (!entry->freed) {
                NAME(ALLOC, free)(self->alloc, entry->ptr);
            }
        }

        NAME(ENTRIES, delete)(&self->entries);
    }

    static void* NAME(SELF, calloc)(SELF* self, size_t count, size_t size) {
        DEBUG_ASSERT(self);
        void* ptr = NAME(ALLOC, calloc)(self->alloc, count, size);
        if (ptr) {
            NAME(ENTRIES, push)(&self->entries, (TRACKED_ENTRY){
                                                    .ptr = ptr,
                                                    .freed = false,
                                                });
        }
        return ptr;
    }

    static void* NAME(SELF, malloc)(SELF* self, size_t size) {
        DEBUG_ASSERT(self);
        void* ptr = NAME(ALLOC, malloc)(self->alloc, size);
        if (ptr) {
            NAME(ENTRIES, push)(&self->entries, (TRACKED_ENTRY){
                                                    .ptr = ptr,
                                                    .freed = false,
                                                });
        }
        return ptr;
    }

    static void* NAME(SELF, realloc)(SELF* self, void* ptr, size_t size) {
        DEBUG_ASSERT(self);
        DEBUG_ASSERT(ptr);
        return NAME(ALLOC, realloc)(self->alloc, ptr, size);
    }

    static void NAME(SELF, free)(SELF* self, void* ptr) {
        DEBUG_ASSERT(ptr);
        DEBUG_ASSERT(self);

        NAME(ENTRIES, iter) iter = NAME(ENTRIES, get_iter)(&self->entries);
        TRACKED_ENTRY* entry;

        while ((entry = NAME(ENTRIES, iter_next)(&iter))) {
            if (entry->ptr == ptr) {
                DEBUG_ASSERT(!entry->freed);
                entry->freed = true;
                break;
            }
        }

        NAME(ALLOC, free)(self->alloc, ptr);
    }

    #undef ENTRIES
    #undef TRACKED_ENTRY
#endif

#include <derive-c/self/undef.h>
