#include <stdint.h>

#include <derive-c/core/panic.h>
#include <derive-c/derive/clone.h>
#include <derive-c/derive/eq.h>
#include <derive-c/derive/ord.h>
#include <derive-c/derive/std.h>
#include <derive-c/derive/struct.h>

#define Foo_REFLECT(F)                                                                             \
    F(uint32_t, bing)                                                                              \
    F(char, baz)                                                                                   \
    F(double, zing)

DERIVE_STRUCT(Foo)
DERIVE_EQ(Foo)
DERIVE_CLONE(Foo)
DERIVE_ORD(Foo)

int main() {
    Foo f = {.bing = 23, .baz = 'c', .zing = 3.14};
    Foo g = {.bing = 23, .baz = 'c', .zing = 3.14};

    ASSERT(Foo_eq(&f, &g));
    ASSERT(!Foo_gt(&f, &g) && !Foo_lt(&f, &g));

    Foo z = Foo_clone(&f);
    z.bing += 10;
    ASSERT(Foo_gt(&z, &f));
}
