#include <stdio.h>

#define CAPACITY 2048
#define NAME alloc_2048
#include <derive-c/alloc/staticbump/template.h>

#define ITEM char
#define ALLOC alloc_2048
#define NAME vec_char
#include <derive-c/container/vector/dynamic/template.h>

int main() {
    alloc_2048 alloc = alloc_2048_new();
    vec_char vec = vec_char_new(&alloc);

    for (char x = 1; x <= 9; x++) {
        vec_char_push(&vec, (char)('0' + x));
    }

    vec_char_iter_const iter = vec_char_get_iter_const(&vec);
    char const* entry;
    while ((entry = vec_char_iter_const_next(&iter))) {
        printf("entry: %c\n", *entry);
    }

    vec_char_delete(&vec);
}
