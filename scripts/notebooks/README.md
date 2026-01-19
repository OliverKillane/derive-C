# Benchmark Analysis Notebooks

Interactive Jupyter notebooks for analyzing derive-C benchmark results.

## Quick Start

1. Run benchmarks for both GCC and Clang:
   ```bash
   cd scripts
   uv run bench run
   ```

2. Start Jupyter to analyze results:
   ```bash
   cd scripts/notebooks/benchmark
   jupyter notebook
   ```

## Benchmark CLI

The `bench` command provides automated benchmark execution:

```bash
# Run benchmarks with both compilers (default)
uv run bench run

# Run with specific compiler
uv run bench run --compiler gcc
uv run bench run --compiler clang

# Run in debug mode
uv run bench run --build-type debug

# Clean build directories
uv run bench clean

# List available benchmarks
uv run bench list
```

The CLI automatically:
- Enters the appropriate nix-shell environment (gcc.nix or clang_dev.nix)
- Configures and builds benchmarks
- Executes benchmarks via ctest
- Stores results in `build-{compiler}-{build_type}/benchmark_results/{compiler}-{build_type}/`

## Setup

Install dependencies (if not already done):
```bash
cd scripts
uv sync
```

## Notebooks

Located in `notebooks/benchmark/`:

- **allocs.ipynb** - Allocator performance analysis
  - Compares derive-C allocators (stdalloc, hybridstatic, chunkedbump)
  - Visualizes sequential allocation, mixed sizes, fragmentation, zeroed allocation
  - GCC vs Clang comparison

- **map.ipynb** - Hash map performance analysis
  - Compares derive-C maps (swiss, ankerl, decomposed, staticlinear)
  - Against STL (unordered_map, map)
  - Against external C++ libraries (ankerl::unordered_dense, abseil, boost)
  - Visualizes iteration performance with GCC vs Clang comparison

- **queue.ipynb** - Queue performance analysis
  - Compares derive-C queues (circular, deque)
  - Against STL (deque, queue)
  - Visualizes push/pop, access ends, iteration, and mixed operations
  - GCC vs Clang comparison

- **set.ipynb** - Hash set performance analysis
  - Compares derive-C sets (swiss)
  - Against STL (unordered_set, set)
  - Against external C++ libraries (boost::unordered_flat_set)
  - Visualizes iteration and mixed operations with GCC vs Clang comparison

- **vector.ipynb** - Vector performance analysis
  - Compares derive-C vectors (dynamic, hybrid, static)
  - Against STL (vector)
  - Visualizes push/iteration and mutable iteration with GCC vs Clang comparison

## Features

Each notebook includes:
- Automatic data loading from multiple compiler/build configurations
- Pre-configured visualizations using Plotly
- **Interactive graphs**: Click legend to toggle series, hover for details
- **Logarithmic X-axis** for all size-based comparisons
- Throughput metrics (M operations/sec) for easy comparison
- Focuses on release builds by default

## Data Processing

The notebooks use:
- **polars** for fast data manipulation
- **plotly** for interactive visualizations
- Custom loader functions that extract:
  - Implementation names
  - Compiler and build type metadata
  - Key/value/item sizes
  - Time measurements (real time, CPU time)
  - Throughput metrics
  - Benchmark parameters (size, test name)

## Tips

- Click legend items to show/hide specific implementations
- Use log scales to see trends across orders of magnitude
- Rerun `uv run bench run` after code changes to regenerate data
- Compare GCC vs Clang by looking at series with same impl name but different compiler suffix
