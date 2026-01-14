
#define OK int
#define ERROR char
#define NAME expand_1
#include <derive-c/utils/result/template.h>

#define OK double
#define ERROR const char*
#define NAME expand_2
#include <derive-c/utils/result/template.h>

#define OK double
#define ERROR const char*
#define ERROR_THROW(_) abort()
#define NAME expand_3
#include <derive-c/utils/result/template.h>

int main() {}
