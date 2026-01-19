#define BLOCK_SIZE 1
#define NAME expand_1
#include <derive-c/alloc/chunkedbump/template.h>

#define ALLOC stdalloc
#define BLOCK_SIZE 64
#define NAME expand_2
#include <derive-c/alloc/chunkedbump/template.h>

#define BLOCK_SIZE 4096
#define NAME expand_3
#include <derive-c/alloc/chunkedbump/template.h>

#define BLOCK_SIZE 65536
#define NAME expand_4
#include <derive-c/alloc/chunkedbump/template.h>

#define BLOCK_SIZE 1048576 /* 1MB */
#define NAME expand_5
#include <derive-c/alloc/chunkedbump/template.h>

int main() {}
