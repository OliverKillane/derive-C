// stdalloc is the default allocator, no need to specify ALLOC
#define BLOCK_SIZE 16
#define SLAB_SIZE 64
#define NAME slab_16_64
#include <derive-c/alloc/slab/template.h>

#define BLOCK_SIZE 64
#define SLAB_SIZE 4096
#define NAME slab_64_4k
#include <derive-c/alloc/slab/template.h>

#define BLOCK_SIZE 128
#define SLAB_SIZE 8192
#define NAME slab_128_8k
#include <derive-c/alloc/slab/template.h>

#define BLOCK_SIZE sizeof(void*)
#define SLAB_SIZE (sizeof(void*) * 16)
#define NAME slab_ptr_sized
#include <derive-c/alloc/slab/template.h>

// Example: using a larger slab allocator as the backing allocator for a smaller one
#define ALLOC slab_128_8k
#define BLOCK_SIZE 32
#define SLAB_SIZE 256
#define NAME slab_nested
#include <derive-c/alloc/slab/template.h>

int main() {
    return 0;
}
