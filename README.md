# Derive C
## What is this?
A library of generic data structures & macro helpers for C.
 - templated data structures 
 - gdb pretty printers
 - derives for equality, clone, ord, etc

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
```
 - See usage examples in [./examples](./examples/)

## Develop
[nix-shell](./shell.nix) is included to setup the C/C++ toolchain.
```bash
nix-shell # from repo root
```
```bash
cmake -S . -B build -GNinja -DDEVELOP=On -DCMAKE_EXPORT_COMPILE_COMMANDS=ON 
```
```bash
cd build
ninja
ninja format
ninja docs
ninja coverage

ctest # includes examples/ as well as test/
```

For using `infer`, infer must be installed separately (it is not yet packaged with nix - [see here](https://github.com/NixOS/nixpkgs/issues/148048))
 - Build with clang
```bash
cmake -S . -B build -DEXTERNALS=Off -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
infer run --compilation-database build/compile_commands.json --bufferoverrun --liveness --pulse
infer explore
```
 - Statically detects generic bugs (e.g. use after free, buffer overrun, integer overflow)

### TODO
In development, remaining tasks:
 - Fix remaining infer-detected casting issues
 - Finish gdb pretty printers
 - Increase coverage for hashmap
 - Regression benchmarks
 - compare & optimise hashmap versus: [ankerl](https://github.com/martinus/unordered_dense/blob/main/include/ankerl/unordered_dense.h)

## References
- [moving-fast-with-software-verification](https://research.facebook.com/publications/moving-fast-with-software-verification/)
