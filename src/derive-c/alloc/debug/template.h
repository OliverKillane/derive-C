/// @brief An allocator that prints to stdout when it allocates or frees memory.
///  - Takes a specific instance, so we can define different printers for
///     different instances of data structures, only see the allocations we want
///     to.

#include <derive-c/core/includes/def.h>
#if !defined(SKIP_INCLUDES)
    #include "includes.h"
#endif

#include <derive-c/core/alloc/def.h>
#include <derive-c/core/self/def.h>

typedef struct {
    char const* name;
    FILE* stream;
    NS(ALLOC, ref) alloc_ref;
} SELF;

static SELF NS(SELF, new)(char const* name, FILE* stream, NS(ALLOC, ref) alloc_ref) {
    fprintf(stream, "%s: Creating debug allocator wrapping " STRINGIFY(ALLOC) "@%p\n", name,
            NS(NS(ALLOC, ref), read)(&alloc_ref));
    return (SELF){
        .name = name,
        .stream = stream,
        .alloc_ref = alloc_ref,
    };
}

static void* NS(SELF, malloc)(SELF* self, size_t size) {
    DC_ASSUME(self);
    void* ptr = NS(ALLOC, malloc)(NS(NS(ALLOC, ref), write)(&self->alloc_ref), size);
    if (ptr) {
        fprintf(self->stream, "%s allocated %zu bytes at %p\n", self->name, size, ptr);
    } else {
        fprintf(self->stream, "%s failed to allocate %zu bytes\n", self->name, size);
    }
    return ptr;
}

static void* NS(SELF, calloc)(SELF* self, size_t count, size_t size) {
    DC_ASSUME(self);
    void* ptr = NS(ALLOC, calloc)(NS(NS(ALLOC, ref), write)(&self->alloc_ref), count, size);
    if (ptr) {
        fprintf(self->stream, "%s allocated %zu bytes at %p\n", self->name, count * size, ptr);
    } else {
        fprintf(self->stream, "%s failed to allocate %zu bytes\n", self->name, count * size);
    }
    return ptr;
}

static void* NS(SELF, realloc)(SELF* self, void* ptr, size_t size) {
    DC_ASSUME(self);
    void* new_ptr = NS(ALLOC, realloc)(NS(NS(ALLOC, ref), write)(&self->alloc_ref), ptr, size);
    if (new_ptr) {
        fprintf(self->stream, "%s reallocated memory at %p to %zu bytes\n", self->name, new_ptr,
                size);
    } else {
        fprintf(self->stream, "%s failed to reallocate memory at %p to %zu bytes\n", self->name,
                ptr, size);
    }
    return new_ptr;
}

static void NS(SELF, free)(SELF* self, void* ptr) {
    DC_ASSUME(self);
    fprintf(self->stream, "%s freeing memory at %p\n", self->name, ptr);
    NS(ALLOC, free)(NS(NS(ALLOC, ref), write)(&self->alloc_ref), ptr);
}

static void NS(SELF, debug)(SELF const* self, dc_debug_fmt fmt, FILE* stream) {
    fprintf(stream, STRINGIFY(SELF) "@%p {\n", self);
    fmt = dc_debug_fmt_scope_begin(fmt);
    dc_debug_fmt_print(fmt, stream, "name: %s,\n", self->name);
    dc_debug_fmt_print(fmt, stream, "base: " STRINGIFY(ALLOC) "@%p,\n",
                       NS(NS(ALLOC, ref), read)(&self->alloc_ref));
    fmt = dc_debug_fmt_scope_end(fmt);
    dc_debug_fmt_print(fmt, stream, "}");
}

static void NS(SELF, delete)(SELF* self) {
    DC_ASSUME(self);
    fprintf(self->stream, "%s: Deleting debug allocator wrapping " STRINGIFY(ALLOC) "@%p\n",
            self->name, NS(NS(ALLOC, ref), read)(&self->alloc_ref));
}

DC_TRAIT_REFERENCABLE_BY_PTR(SELF);

DC_TRAIT_ALLOC(SELF);

#include <derive-c/core/self/undef.h>
#include <derive-c/core/alloc/undef.h>
#include <derive-c/core/includes/undef.h>