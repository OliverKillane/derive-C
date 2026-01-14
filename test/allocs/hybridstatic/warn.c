#define CAPACITY 1
#define NAME expand_1
#include <derive-c/alloc/hybridstatic/template.h>

#define ALLOC stdalloc
#define CAPACITY 1
#define NAME expand_2
#include <derive-c/alloc/hybridstatic/template.h>

#define CAPACITY 255
#define NAME expand_3
#include <derive-c/alloc/hybridstatic/template.h>

#define CAPACITY 256
#define NAME expand_4
#include <derive-c/alloc/hybridstatic/template.h>

#define CAPACITY 4294967295 /* (2**32 - 1) */
#define NAME expand_5
#include <derive-c/alloc/hybridstatic/template.h>

int main() {}
