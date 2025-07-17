## TODO / extreme wip cringe show and tell
1. Nix for toolchain setup (including for CI)
```bash
nix-shell
```

2. No externals scripts required, can just use cmake
```bash
cmake -S . -B build -GNinja -DDEVELOP=On -DCMAKE_EXPORT_COMPILE_COMMANDS=ON 

ninja -C build
```

3. templates in C
Yep, they're just templates, but not as good.
 - good enough intellisense though

3. Lets introduce a bug
```c
free_entry->present = false; // switch to true
```

We can check statically:
```bash
cmake -S . -B build -DEXTERNALS=Off -DCMAKE_EXPORT_COMPILE_COMMANDS=On
infer run --compilation-database build/compile_commands.json  --liveness --pulse
infer explore
```

Hmm, one false positive, the others seem correct?
 - Not super readable, we can try with chatgpt?

Lets run the tests instead?

```bash
ctest --test-dir build
./build/test/derivec_tests --gtest_filter=HashMapTests.General 
```

We can then fix the bug.
 - Undo the bug I added 👍

Now we see infer pass, and we can see the tests pass, but need some confidence.
```bash
ninja -C build coverage
```
Open live preview.

4. Lets document our code
```bash
ninja -C build docs
```

5. Lets try analysing something
Add heaptrack to the nix shell
```bash
heaptrack # is pkgs.heaptrack
```

```bash
cmake -S . -B build -DEXTERNALS=Off -DDEVELOP=off -DCMAKE_EXPORT_COMPILE_COMMANDS=On
heaptrack build/examples/complex-employees
```

5. Lets make a basic CI
intellisense for github actions
