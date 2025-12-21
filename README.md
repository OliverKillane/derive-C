# Derive C

_Easy templated data strctures in C_

## Elevator Pitch
When using C for complex projects, the lack of generics is frustrating:
 - tradeoff performance and store runtime type information (e.g. size of items in a vector)
 - tradeoff usability and restrict sizes of types, use unions (e.g. vector of only 8 byte items, user must cast to their own type, debuggers unaware of _real_ type)
 - tradeoff readability & development speed by outsourcing data structure generation to another language (cmake calling python to generate headers)
 - tradeoff tooling for formatting, linting & intellisense with complex preprocessor macros to generate data structures

Derive-C aims to avoid these, and to get an experience close to simple templates in C++:
 - templates are edittable (with intellisense), lintable, formattable independent of their usage.
 - instances of templates should be clangd friendly / show types & completions
 - no performance tradeoff
 - easy to debug with in GDB

See examples in [./examples](./examples/)

## Use
In a `CMakeLists.txt`
```cmake
include(FetchContent)

FetchContent_Declare(
    derive-c
    GIT_REPOSITORY https://github.com/OliverKillane/derive-C
    GIT_TAG <Chosen derive-c version>
)
FetchContent_MakeAvailable(derive-c)

target_compile_definitions(${my_target} PRIVATE
    # Enable free function mocking for tests
    ENABLE_MOCKING  

    # To override the default behaviour for `DC_PANIC`, `DC_ASSERT
    DC_PANIC_HEADER=<my-special-library/foo/derive_c_panic_overrides.h> 
)
```

## Develop
For development, the [clang toolchain](./toolchain/clang.nix) is recommended.
```bash
nix-shell toolchain/clang.nix # from repo root
```
```bash
cmake -S . -B build -GNinja
```
```bash
cd build
ninja
ctest -j
ninja format
ninja lint
ninja docs
ninja coverage
```
Then when opening vscode from the nix-shell, the correct clangd & library paths are used:
```bash
code .
```

For using `infer`, infer must be installed separately (it is not yet packaged with nix - [see here](https://github.com/NixOS/nixpkgs/issues/148048))
 - Build with clang
```bash
cmake -S . -B build -DEXTERNALS=Off
infer run --compilation-database build/compile_commands.json --bufferoverrun --liveness --pulse
infer explore
```
 - Statically detects generic bugs (e.g. use after free, buffer overrun, integer overflow)

To check for dependency upgrades in the cpp code:
```bash
LOG_LEVEL=debug renovate --platform=local --dry-run=full
```

### Remaining Work
In development, remaining tasks:
 - Fix remaining infer-detected casting issues
 - Increase coverage
 - Regression benchmarks
 - compare & optimise hashmap versus: [ankerl](https://github.com/martinus/unordered_dense/blob/main/include/ankerl/unordered_dense.h)

## References
- [moving-fast-with-software-verification](https://research.facebook.com/publications/moving-fast-with-software-verification/)
