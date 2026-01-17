#pragma once

#include <derive-cpp/meta/labels.hpp>
#include <derive-c/alloc/std.h>
#include <derive-c/alloc/hybridstatic/includes.h>

// Standard allocator (malloc/free wrapper)
struct StdAlloc {
    LABEL_ADD(stdalloc);
    // stdalloc is a zero-sized singleton, we just use the ref
};

// Hybrid static allocator with templated buffer size
template<size_t Capacity>
struct HybridStatic {
    LABEL_ADD(hybridstatic);
#define EXPAND_IN_STRUCT
#define CAPACITY Capacity
#define NAME Alloc
#include <derive-c/alloc/hybridstatic/template.h>
};

// Common size configurations
using HybridStatic1K = HybridStatic<1024>;
using HybridStatic4K = HybridStatic<4096>;
using HybridStatic16K = HybridStatic<16384>;
