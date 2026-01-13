
#define ITEM int
#define NAME expand_1
#include <derive-c/utils/option/template.h>

#define ITEM char*
#define NAME expand_2
#define ITEM_DELETE(item_ptr) free(*item_ptr)
#include <derive-c/utils/option/template.h>


int main() {}
