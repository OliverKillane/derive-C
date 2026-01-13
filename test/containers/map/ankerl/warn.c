#include <derive-c/algorithm/hash/default.h>
#include <derive-c/prelude.h>

#define KEY int
#define KEY_HASH DC_DEFAULT_HASH
#define VALUE double
#define NAME expand_1
#include <derive-c/container/map/ankerl/template.h>

#define KEY int
#define KEY_HASH DC_DEFAULT_HASH
#define VALUE double
#define SMALL_BUCKETS
#define NAME expand_2
#include <derive-c/container/map/ankerl/template.h>

#define KEY const char*
#define KEY_HASH DC_DEFAULT_HASH
#define VALUE float
#define NAME expand_3
#include <derive-c/container/map/ankerl/template.h>

#define KEY const char*
#define KEY_HASH DC_DEFAULT_HASH
#define VALUE float
#define SMALL_BUCKETS
#define NAME expand_4
#include <derive-c/container/map/ankerl/template.h>

#define KEY const char*
#define KEY_HASH DC_DEFAULT_HASH
#define VALUE long
#define NAME expand_5
#include <derive-c/container/map/ankerl/template.h>

#define KEY const char*
#define KEY_HASH DC_DEFAULT_HASH
#define VALUE long
#define SMALL_BUCKETS
#define NAME expand_6
#include <derive-c/container/map/ankerl/template.h>

#define KEY char*
#define KEY_HASH DC_DEFAULT_HASH
#define KEY_EQ(str_1_ptr, str_2_ptr) (strcmp(*str_1_ptr, *str_2_ptr) == 0)
#define KEY_DELETE(value_ptr) free(*value_ptr)
#define VALUE float
#define NAME expand_7
#include <derive-c/container/map/ankerl/template.h>

#define KEY char*
#define KEY_HASH DC_DEFAULT_HASH
#define KEY_EQ(str_1_ptr, str_2_ptr) (strcmp(*str_1_ptr, *str_2_ptr) == 0)
#define KEY_DELETE(value_ptr) free(*value_ptr)
#define VALUE float
#define SMALL_BUCKETS
#define NAME expand_8
#include <derive-c/container/map/ankerl/template.h>

#define KEY char*
#define KEY_HASH DC_DEFAULT_HASH
#define KEY_EQ dc_str_eq
#define KEY_DELETE(value_ptr) free(*value_ptr)
#define VALUE float
#define NAME expand_9
#include <derive-c/container/map/ankerl/template.h>

#define KEY char*
#define KEY_HASH DC_DEFAULT_HASH
#define KEY_EQ dc_str_eq
#define KEY_DELETE(value_ptr) free(*value_ptr)
#define VALUE float
#define SMALL_BUCKETS
#define NAME expand_10
#include <derive-c/container/map/ankerl/template.h>

int main() {}
