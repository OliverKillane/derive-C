![](./docs/banner.svg)

## How to Use Derive-C?
1. Add as a dependency
```cmake
# e.g. with fetchcontent
include(FetchContent)

FetchContent_Declare(
    derive-c
    GIT_REPOSITORY https://github.com/OliverKillane/derive-C
    GIT_TAG <Chosen derive-c version>
)
FetchContent_MakeAvailable(derive-c)
```
2. Add overrides for default panic, mocking behaviour
```cmake
target_compile_definitions(${my_target} PRIVATE
    # Enable free function mocking for tests
    ENABLE_MOCKING  

    # To override the default behaviour for `DC_PANIC`, `DC_ASSERT`, etc.
    # For example convert to std::runtime_error in google tests
    DC_PANIC_HEADER=<my-special-library/foo/derive_c_panic_overrides.h> 
)
```
3. Use the library
```c
#define KEY int32_t
#define KEY_HASH DC_DEFAULT_HASH
#define VALUE const char*
#define NAME id_to_name
#include <derive-c/container/map/ankerl/template.h>
```

See examples in [./examples](./examples/).

## Why use Derive-C?

### Type Safety
No casting, no use of void* with container interfaces.
 - compiler (& LSP) can interrogate actual types. 
 - different containers implement the same traits, allowing implementations to be switched without substantial code changes.

### Intellisense
Type Safety supports good intellisense.
Furthermore all templates work with clangd independent of instantiation, using placeholder types for ease of derive-c development.

### Composability
Containers can nest other containers
 - e.g. a hashmap of optional ints to vectors of strings.
Containers also take an allocator as a parameter.
 - e.g. using a bump allocator

### Performance
Many implementations of map, vector (and custom allocators) with a focus on performance.
 - Containers use smaller integers for indexes where possible
 - Containers parameterisable for performance features (e.g. small buckets for the ankerl map)

### Debugability
For finding bugs:
 - Custom msan and asan poisoning implemented for all containers.
 - Iterator invalidation checking done in debug mode.

And for debugging:
 - All containers can be debug printed
 - type safety makes containers easier to explore in gdb


## [Contributing](./.github/CONTRIBUTING.md)

## References
 - [moving-fast-with-software-verification](https://research.facebook.com/publications/moving-fast-with-software-verification/)
 - [klib](https://github.com/attractivechaos/klib)
 - [stc](https://github.com/stclib/STC)
