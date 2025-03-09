## What is this
A small toolbox of generic C code.
 - ab(using) the preprocessor to write generic code

## Develop
```bash
cmake -S . -B build -GNinja -DDEBUG=On -DCMAKE_EXPORT_COMPILE_COMMANDS=ON 
ninja -C build
ninja -C build format
ctest --test-dir build/test
```

For using `infer`
```bash
cmake -S . -B build -DEXTERNALS=Off
cd build

```

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
 - infer on examples?