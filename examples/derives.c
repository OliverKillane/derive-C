#include <derive-c/derives/std.h>

#include <derive-c/derives/clone.h>
#include <derive-c/derives/eq.h>
#include <derive-c/derives/ord.h>
#include <derive-c/derives/struct.h>

#define PANIC abort()

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
