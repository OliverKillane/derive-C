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

int main() {
    Foo f = {.bing = 23, .baz = 'c', .zing = 'z'};
    Foo g = {.bing = 23, .baz = 'c', .zing = 'z'};

    DC_ASSERT(Foo_eq(&f, &g));
    DC_ASSERT(!Foo_gt(&f, &g) && !Foo_lt(&f, &g));

    Foo z = Foo_clone(&f);
    z.bing += 10;
    DC_ASSERT(Foo_gt(&z, &f));
}
