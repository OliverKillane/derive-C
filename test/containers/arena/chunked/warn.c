
#define VALUE int
#define INDEX_BITS 8
#define BLOCK_INDEX_BITS 7
#define NAME expand_1
#include <derive-c/container/arena/chunked/template.h>

#define VALUE int
#define INDEX_BITS 8
#define BLOCK_INDEX_BITS 1
#define NAME expand_2
#include <derive-c/container/arena/chunked/template.h>

#define VALUE int
#define INDEX_BITS 64
#define BLOCK_INDEX_BITS 1
#define NAME expand_3
#include <derive-c/container/arena/chunked/template.h>

#define VALUE int
#define INDEX_BITS 64
#define BLOCK_INDEX_BITS 32
#define NAME expand_4
#include <derive-c/container/arena/chunked/template.h>

#define VALUE const char*
#define INDEX_BITS 8
#define BLOCK_INDEX_BITS 1
#define NAME expand_5
#include <derive-c/container/arena/chunked/template.h>

int main() {}
