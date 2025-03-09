## What is this
A small toolbox of generic C code.
 - ab(using) the preprocessor to write generic code

## Develop
[nix-shell](./shell.nix) is included to setup the C/C++ toolchain.
```bash
nix-shell # from repo root
```

Currently using clang19
```bash
export CC=clang
export CXX=clang++
```

```bash
cmake -S . -B build -GNinja -DDEBUG=On -DCMAKE_EXPORT_COMPILE_COMMANDS=ON 
ninja -C build
ninja -C build format
ctest --test-dir build/test
```

For using `infer`, infer must be installed separately (it is not yet packaged with nix - [see here](https://github.com/NixOS/nixpkgs/issues/148048))
 - Build with clang
```bash
cmake -S . -B build -DEXTERNALS=Off -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
infer run --compilation-database build/compile_commands.json
```
 - [ ] Need to work out a current error with hashmap items (looks like noise, but not 100% certain).

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

## Features
### Structures
- [x] vector
- [x] hashmap (untested)

### Derive Macros
Using a similar pattern to [xmacros](https://en.wikipedia.org/wiki/X_macro) we can derive equality, and structs (debug is more complex so not done yet).

## TODO
 - Add more tests
 - Add regression benchmarks
 - CBMC verification for basic insert/amend/delete flow on hashmap.
