#pragma once

#include <derive-cpp/meta/labels.hpp>
#include <derive-c/alloc/std.h>
#include <derive-c/alloc/hybridstatic/includes.h>
#include <derive-c/alloc/chunkedbump/includes.h>
#include <derive-c/alloc/slab/includes.h>

// Standard allocator (malloc/free wrapper)
struct StdAlloc {
    LABEL_ADD(stdalloc);
    static constexpr const char* impl_name = "derive-c/stdalloc";
};

// Hybrid static allocator with templated buffer size
template<size_t Capacity>
struct HybridStatic {
    LABEL_ADD(hybridstatic);
    static constexpr const char* impl_name = "derive-c/hybridstatic";
#define EXPAND_IN_STRUCT
#define CAPACITY Capacity
#define NAME Alloc
#include <derive-c/alloc/hybridstatic/template.h>
};

// Chunked bump allocator with templated block size
template<size_t BlockSize>
struct ChunkedBump {
    LABEL_ADD(chunkedbump);
    static constexpr const char* impl_name = "derive-c/chunkedbump";
#define EXPAND_IN_STRUCT
#define BLOCK_SIZE BlockSize
#define NAME Alloc
#include <derive-c/alloc/chunkedbump/template.h>
};

// Slab allocator with templated block and slab sizes
template<size_t BlockSize, size_t SlabSize>
struct Slab {
    LABEL_ADD(slab);
    static constexpr const char* impl_name = "derive-c/slab";
#define EXPAND_IN_STRUCT
#define BLOCK_SIZE BlockSize
#define SLAB_SIZE SlabSize
#define NAME Alloc
#include <derive-c/alloc/slab/template.h>
};
