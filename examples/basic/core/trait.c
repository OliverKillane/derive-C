#include <stdint.h>

#include <derive-c/prelude.h>

#define Foo_REFLECT(F)                                                                             \
    F(uint32_t, bing)                                                                              \
    F(char, baz)                                                                                   \
    F(char, zing)

DC_DERIVE_STRUCT(Foo)
DC_DERIVE_EQ(Foo)
DC_DERIVE_CLONE(Foo)
DC_DERIVE_ORD(Foo)

static void example_trait(DC_LOGGER* parent) {
    DC_SCOPED(DC_LOGGER) log = DC_LOGGER_NEW(parent, "%s", __func__);

    Foo f = {.bing = 23, .baz = 'c', .zing = 'z'};
    Foo g = {.bing = 23, .baz = 'c', .zing = 'z'};

    DC_LOG(log, DC_INFO, "checking equality of f and g");
    DC_ASSERT(Foo_eq(&f, &g));
    DC_LOG(log, DC_INFO, "checking ordering of f and g");
    DC_ASSERT(!Foo_gt(&f, &g) && !Foo_lt(&f, &g));

    DC_LOG(log, DC_INFO, "cloning f to z and incrementing bing");
    Foo z = Foo_clone(&f);
    z.bing += 10;
    DC_LOG(log, DC_INFO, "checking z > f");
    DC_ASSERT(Foo_gt(&z, &f));
}

int main() {
    DC_SCOPED(DC_LOGGER)
    root = NS(DC_LOGGER,
              new_global)((NS(DC_LOGGER, global_config)){.stream = stdout, .ansi_colours = true},
                          (dc_log_id){"trait"});

    example_trait(&root);
    return 0;
}
