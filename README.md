# Derive C
## What is this?
A library of generic data structures & macro helpers for C.
 - templated data structures
 - gdb pretty printers
 - derives for equality, clone, ord, etc

## Develop
[nix-shell](./shell.nix) is included to setup the C/C++ toolchain.
```bash
nix-shell # from repo root
```

```bash
cmake -S . -B build -GNinja -DDEBUG=On -DCMAKE_EXPORT_COMPILE_COMMANDS=ON 

cd build
ninja
ninja format
ninja docs
ctest
ninja coverage
```

For using `infer`, infer must be installed separately (it is not yet packaged with nix - [see here](https://github.com/NixOS/nixpkgs/issues/148048))
 - Build with clang
```bash
cmake -S . -B build -DEXTERNALS=Off -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
infer run --compilation-database build/compile_commands.json --bufferoverrun --liveness --pulse
infer explore
```
 - Statically detects generic bugs (e.g. use after freed, buffer overrun, integer overflow)

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
- [moving-fast-with-software-verification](https://research.facebook.com/publications/moving-fast-with-software-verification/)