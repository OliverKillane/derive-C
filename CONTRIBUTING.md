# Contributing

## Develop
For development, the [clang toolchain](./toolchain/clang_dev.nix) is recommended.
```bash
nix-shell toolchain/clang_dev.nix # from repo root
```
```bash
cmake -S . -B build -GNinja
```
```bash
cd build
ninja
ctest -j
ninja format
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
nix-shell toolchain/renovate.nix
LOG_LEVEL=debug renovate --platform=local --dry-run=full
```

## Verifying Changes
All tests, linting & benchmarks are checked under ctest:
```bash
ctest --test-dir build -N # List the test names
ctest --test-dir build -R 'derive-c-custom-lint' -V # See results for custom derive-c lints
ctest --test-dir build -j --output-on-failure # To test all
ctest --test-dir build -L bench -R allocs -V # To just run the alloc benchmarks 
```
Individual test binaries are presenting in the:
```bash
ls build/bench # all benchmarking binaries
ls build/test # all test binaries
```

We also verify for both clang and gcc, in release and with sanitizers.
```bash
# Used for normal development (include intellisense)
nix-shell toolschain/clang_dev

# Verifying code works (e.g. cutsom poisoning under msan)
nix-shell toolschain/clang_msan --run 'cmake -S . -B build_msan -DUSE_ASAN=Off _DUSE_UBSAN=Off -DUSE_MSAN=On -DDOCS=Off && ninja -C build_msan && ctest --test-dir build_msan -j --output-on-failure'
```

Finally we have benchmarks covering:
 - Basic cases so we can check for obvious performance regressions
 - Worst case scenarios for the library

A normal development cycle should occur in [the clang dev toolchain](./toolchain/clang_dev.nix), with tests run with asan & ubsan.
 - The rest can be run in ci, and specific toolchains & builds debugged locally.

## Design Principles
This library should focus on maintaining _yeetability_.

> Only from passing CI, a PR is good to yeet to prod. 

1. All tests should run in full, in CI, in an easily reproducible environment. Manual testing is never valid evidence of testing. Reviewers should always check coverage.
2. Odd design decisions should be justified in code with `JUSTIFY:` comments
3. Derive-c specific idioms should be where possible enforced in CI, using the linting scripts.

## TODO
 - Improved testing coverage (e.g. delete in containers)
 - Roaring bitset
 - Improved set
