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
    fprintf(stream, "[%s] %s(alloc=" DC_EXPAND_STRING(ALLOC) "@%p)\n", name, __func__,
            NS(NS(ALLOC, ref), deref)(alloc_ref));
    return (SELF){
        .name = name,
        .stream = stream,
        .alloc_ref = alloc_ref,
    };
}

static void* NS(SELF, allocate_uninit)(SELF* self, size_t size) {
    DC_ASSUME(self);
    void* ptr = NS(ALLOC, allocate_uninit)(self->alloc_ref, size);
    fprintf(self->stream, "[%s] %s(size=%zu) -> %p\n", self->name, __func__, size, ptr);
    return ptr;
}

static void* NS(SELF, allocate_zeroed)(SELF* self, size_t size) {
    DC_ASSUME(self);
    void* ptr = NS(ALLOC, allocate_zeroed)(self->alloc_ref, size);
    fprintf(self->stream, "[%s] %s(size=%zu) -> %p\n", self->name, __func__, size, ptr);
    return ptr;
}

static void* NS(SELF, reallocate)(SELF* self, void* ptr, size_t old_size, size_t new_size) {
    DC_ASSUME(self);
    void* new_ptr = NS(ALLOC, reallocate)(self->alloc_ref, ptr, old_size, new_size);
    // JUSTIFY: Ignoring ptr used after free for clang static analyser
    //  - We only use the pointer's value in log, do not dereference it.
    // NOLINTBEGIN(clang-analyzer-unix.Malloc)
    fprintf(self->stream, "[%s] %s(ptr=%p, old_size=%zu, new_size=%zu) -> %p\n", self->name,
            __func__, ptr, old_size, new_size, new_ptr);
    // NOLINTEND(clang-analyzer-unix.Malloc)
    return new_ptr;
}

static void NS(SELF, deallocate)(SELF* self, void* ptr, size_t size) {
    DC_ASSUME(self);
    fprintf(self->stream, "[%s] %s(ptr=%p, size=%zu)\n", self->name, __func__, ptr, size);
    NS(ALLOC, deallocate)(self->alloc_ref, ptr, size);
}

static void NS(SELF, debug)(SELF const* self, dc_debug_fmt fmt, FILE* stream) {
    fprintf(stream, DC_EXPAND_STRING(SELF) "@%p {\n", self);
    fmt = dc_debug_fmt_scope_begin(fmt);
    dc_debug_fmt_print(fmt, stream, "name: %s,\n", self->name);
    dc_debug_fmt_print(fmt, stream, "alloc: ");
    NS(ALLOC, debug)(NS(NS(ALLOC, ref), deref)(self->alloc_ref), fmt, stream);
    fprintf(stream, "\n");
    fmt = dc_debug_fmt_scope_end(fmt);
    dc_debug_fmt_print(fmt, stream, "}");
}

static void NS(SELF, delete)(SELF* self) {
    DC_ASSUME(self);
    fprintf(self->stream, "[%s]: %s\n", self->name, __func__);
}

DC_TRAIT_REFERENCABLE_BY_PTR(SELF);

DC_TRAIT_ALLOC(SELF);

#include <derive-c/core/self/undef.h>
#include <derive-c/core/alloc/undef.h>
#include <derive-c/core/includes/undef.h>