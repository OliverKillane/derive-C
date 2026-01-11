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
nix-shell toolchain/renovate.nix
LOG_LEVEL=debug renovate --platform=local --dry-run=full
```

## Design Principles
This library should focus on maintaining _yeetability_.

> Only from passing CI, a PR is good to yeet to prod. 

1. All tests should run in full, in CI, in an easily reproducible environment. Manual testing is never valid evidence of testing. Reviewers should always check coverage.
2. Odd design decisions should be justified in code with `JUSTIFY:` comments
3. Derive-c specific idioms should be where possible enforced in CI, using the linting scripts.

## Remaining Work
In development, remaining tasks:
 - Fix remaining infer-detected casting issues
 - Increase coverage
 - Regression benchmarks
 - compare & optimise hashmap versus: [ankerl](https://github.com/martinus/unordered_dense/blob/main/include/ankerl/unordered_dense.h)
 - Remove all the null checks for allocations (allocators now handle this)