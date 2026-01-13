#include <derive-c/algorithm/hash/default.h>
#include <derive-c/core/prelude.h>

#define KEY int
#define VALUE double
#define CAPACITY 1
#define NAME expand_1
#include <derive-c/container/map/staticlinear/template.h>

#define KEY const char*
#define VALUE float
#define CAPACITY 7
#define NAME expand_2
#include <derive-c/container/map/staticlinear/template.h>

#define KEY const char*
#define VALUE long
#define CAPACITY 8
#define NAME expand_3
#include <derive-c/container/map/staticlinear/template.h>

#define KEY char*
#define KEY_EQ(str_1_ptr, str_2_ptr) (strcmp(*str_1_ptr, *str_2_ptr) == 0)
#define KEY_DELETE(value_ptr) free(*value_ptr)
#define VALUE float
#define CAPACITY 255
#define NAME expand_4
#include <derive-c/container/map/staticlinear/template.h>

#define KEY char*
#define KEY_EQ dc_str_eq
#define KEY_DELETE(value_ptr) free(*value_ptr)
#define VALUE float
#define CAPACITY 256
#define NAME expand_5
#include <derive-c/container/map/staticlinear/template.h>

int main() {}
