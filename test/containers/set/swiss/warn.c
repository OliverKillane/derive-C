#include <derive-c/algorithm/hash/default.h>
#include <derive-c/prelude.h>

#define ITEM int
#define ITEM_HASH DC_DEFAULT_HASH
#define NAME expand_1
#include <derive-c/container/set/swiss/template.h>

#define ITEM const char*
#define ITEM_HASH DC_DEFAULT_HASH
#define NAME expand_2
#include <derive-c/container/set/swiss/template.h>

#define ITEM char*
#define ITEM_HASH DC_DEFAULT_HASH
#define ITEM_EQ(str_1_ptr, str_2_ptr) (strcmp(*str_1_ptr, *str_2_ptr) == 0)
#define ITEM_DELETE(value_ptr) free(*value_ptr)
#define NAME expand_3
#include <derive-c/container/set/swiss/template.h>

#define ITEM char*
#define ITEM_HASH DC_DEFAULT_HASH
#define ITEM_EQ dc_str_eq
#define ITEM_DELETE(value_ptr) free(*value_ptr)
#define NAME expand_4
#include <derive-c/container/set/swiss/template.h>

int main() {}
