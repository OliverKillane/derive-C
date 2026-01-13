
#define ITEM char
#define CAPACITY 1
#define NAME expand_1
#include <derive-c/container/vector/static/template.h>

#define ITEM float
#define CAPACITY 7
#define NAME expand_2
#include <derive-c/container/vector/static/template.h>

#define ITEM char*
#define CAPACITY 8
#define NAME expand_3
#include <derive-c/container/vector/static/template.h>

#define ITEM char*
#define CAPACITY 65535 /* 2**16 - 1 */
#define NAME expand_4
#include <derive-c/container/vector/static/template.h>


int main() {}
