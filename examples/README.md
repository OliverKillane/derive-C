# Examples Overview

This directory contains comprehensive examples demonstrating the usage of derive-C containers and utilities.

## Basic Examples

### Container Examples

#### Vectors (`container/vector/`)
- **`dynamic.c`** - Dynamic vectors with automatic resizing
  - Integer vectors with capacity management
  - Complex data types with owned resources
  - Iterator examples (const and mutable)
  - Demonstrates: push, pop, read, write, iteration

- **`static.c`** - Original static vector example
  - Fixed-capacity vectors allocated on stack

- **`static_enhanced.c`** - Enhanced static vector demonstrations
  - Basic operations with capacity limits
  - Using static vectors as fixed-size buffers
  - Ring buffer simulation for sensor data
  - Stack implementation using static vector

#### Arenas (`container/arena/`)
- **`basic.c`** - Contiguous arena fundamentals
  - Integer arena with different index sizes
  - Custom struct with cleanup functions
  - Insert, remove, read, write operations
  - Index-based access patterns

- **`chunked.c`** - Chunked arena for growing collections
  - Multi-chunk allocation strategy
  - Reusing freed slots
  - Handling owned data with cleanup
  - Demonstrates growth across chunks

- **`geometric.c`** - Geometric growth arena
  - Efficient reallocation with geometric growth factor
  - Capacity doubling demonstration
  - 2D grid of points example
  - Sparse arena with removals and reuse

#### Queues (`container/queue/`)
- **`circular.c`** - Circular queue (ring buffer)
  - FIFO operations with fixed capacity
  - Using hybrid static allocator

- **`deque.c`** - Double-ended queue
  - Push/pop from both ends
  - Priority queue simulation
  - Palindrome checker algorithm
  - Sliding window maximum problem

#### Maps (`container/map/`)
- **`decomposed.c`** - Hash map with key-value storage
  - Simple integer-to-string mapping
  - Complex keys with custom hash and equality
  - Owned resources in keys and values
  - Iterator patterns

#### Sets (`container/set/`)
- **`swiss.c`** - Swiss table hash set
  - Integer set with membership testing
  - String set with custom equality
  - Set operations (union, intersection via iteration)
  - Struct (point) set with custom hash

#### Bitsets (`container/bitset/`)
- **`static.c`** - Fixed-size bitset
  - Basic bit operations (set, get, toggle, clear)
  - Set operations (union, intersection)
  - Feature flags pattern
  - Efficient boolean array representation

### Utility Examples

#### Options (`container/utils/`)
- **`option.c`** - Optional value handling
  - Safe null-pointer alternatives
  - Error handling patterns

### Allocator Examples

#### Custom Allocators (`alloc/`)
- **`hybridstatic.c`** - Hybrid static/dynamic allocator
  - Stack buffer with heap fallback
  - Memory-efficient small allocations

- **`chunkedbump.c`** - Chunked bump allocator
  - Fast sequential allocations
  - In-place reallocation for last allocation
  - Reset and reuse memory blocks

- **`slab.c`** - Slab allocator with freelist
  - Fixed-size object pools
  - Freelist for fast reuse
  - Automatic fallback for oversized allocations

### Core Examples

#### Traits (`core/trait/`)
- **`general.c`** - Trait system demonstration
  - Clone, Debug, Delete, Eq traits
  - Custom trait implementations

## Complex Examples

### Real-World Applications

- **`prime_sieve.c`** - Sieve of Eratosthenes
  - Efficient prime number generation
  - Bitset usage for large ranges

- **`employees.c`** - Employee database
  - Multiple containers working together
  - Data relationships and queries

- **`readme.c`** - Quick start example
  - Featured in main README
  - Simple introduction to basic containers

## Building and Running

All examples are automatically discovered and built by CMake:

```bash
cmake -B build
cmake --build build

# Run specific example
./build/examples/basic/container/vector/dynamic

# Run all examples as tests
ctest --test-dir build -L examples
```

## Example Structure

Each example follows this pattern:

1. **Includes** - Required headers for the container/utility
2. **Template Instantiation** - Define parameters and include template
3. **Helper Functions** - Custom debug, delete, equality functions if needed
4. **Example Functions** - Demonstrate specific features
5. **Main** - Calls all example functions

## Key Patterns Demonstrated

### Resource Management
- Automatic cleanup with `_delete` functions
- Owned vs borrowed data
- Custom `ITEM_DELETE` for complex types

### Iterators
- Const iteration (`_iter_const`)
- Mutable iteration (`_iter`)
- Using `DC_FOR` macros for convenience

### Custom Types
- Implementing equality (`ITEM_EQ`)
- Implementing hash (`ITEM_HASH`)
- Implementing debug (`ITEM_DEBUG`)
- Implementing cleanup (`ITEM_DELETE`)

### Performance Considerations
- Capacity pre-allocation
- Growth strategies (contiguous, chunked, geometric)
- Index size selection for arenas
- Static vs dynamic allocation trade-offs

## Adding New Examples

To add a new example:

1. Create a `.c` file in the appropriate subdirectory
2. Follow the naming convention: `examples/basic/container/TYPE/NAME.c`
3. Add documentation comments at the top
4. CMake will automatically discover and build it

The build system uses `file(GLOB_RECURSE)` to find all `.c` files, so no CMakeLists.txt modifications are needed.
