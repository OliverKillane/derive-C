#include <derive-c/prelude.h>

#define CAPACITY 2048
#define NAME alloc_2048
#include <derive-c/alloc/hybridstatic/template.h>

#define ITEM char
#define ALLOC alloc_2048
#define NAME vec_char
#include <derive-c/container/vector/dynamic/template.h>

static void example_readme(DC_LOGGER* parent) {
    DC_SCOPED(DC_LOGGER) log = DC_LOGGER_NEW(parent, "%s", __func__);

    alloc_2048_buffer buf;
    alloc_2048 alloc = alloc_2048_new(&buf, stdalloc_get_ref());
    vec_char vec = vec_char_new(&alloc);

    DC_LOG(log, DC_INFO, "pushing digits 1-9");
    for (char x = 1; x <= 9; x++) {
        vec_char_push(&vec, (char)('0' + x));
    }

    vec_char_iter_const iter = vec_char_get_iter_const(&vec);
    char const* entry;
    while ((entry = vec_char_iter_const_next(&iter))) {
        DC_LOG(log, DC_INFO, "entry: %c", *entry);
    }

    vec_char_delete(&vec);
}

int main() {
    DC_SCOPED(DC_LOGGER)
    root = NS(DC_LOGGER,
              new_global)((NS(DC_LOGGER, global_config)){.stream = stdout, .ansi_colours = true},
                          (dc_log_id){"readme"});

    example_readme(&root);
    return 0;
}
