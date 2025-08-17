#include <stdio.h>

#define CAPACITY 2048
#define SELF alloc_2048
#include <derive-c/allocs/staticbump/template.h>

#define T char
#define ALLOC alloc_2048
#define SELF vec_char
#include <derive-c/structures/vector/template.h>

int main() {
    alloc_2048 alloc = alloc_2048_new();
    vec_char vec = vec_char_new(&alloc);

    for (char x = '1'; x <= '9'; x++) {
        vec_char_push(&vec, x);
    }

    vec_char_iter_const iter = vec_char_get_iter_const(&vec);
    char const* entry;
    while ((entry = vec_char_iter_const_next(&iter))) {
        printf("entry: %c\n", *entry);
    }

    vec_char_delete(&vec);
}
