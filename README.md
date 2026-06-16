# Compressed Sparse Arrays (CSA)

## 1. What this project does

This project implements a **Compressed Sparse Array (CSA)** in C.

A normal array allocates storage for every index between `0` and the largest index in use. That is wasteful when only a small number of positions actually contain values. This project stores only the indices that have been written, while still allowing the user to treat the structure like an integer array.

The CSA supports:

- creating an empty sparse array;
- setting or overwriting values by index;
- getting values by index;
- converting the structure to a readable string;
- freeing all allocated memory;
- optional extension operations for iterating over stored values and deleting stored indices.

The public API is declared in `include/csa.h`. The main operations are:

| Function | Purpose |
| --- | --- |
| `csa_init()` | Allocate and initialise an empty CSA |
| `csa_set()` | Add or overwrite a value at a given index |
| `csa_get()` | Read a value if the index has been set |
| `csa_tostring()` | Produce a string representation of the CSA |
| `csa_free()` | Free the CSA and set the caller's pointer to `NULL` |
| `csa_foreach()` | Extension: apply a callback to every stored value |
| `csa_delete()` | Extension: remove a stored value by index |

The repository also includes example programs that use the CSA for tests, Fibonacci memoisation, factorial lookup, and prime-number sieving.

## 2. Technical approach

### 2.1 Data structure

The CSA is built from an array of blocks. Each block covers 64 consecutive logical array indices.

```c
struct block {
   int* vals;
   mask_t msk;
   unsigned int offset;
};

struct csa {
   block* b;
   int n;
};
```

Each block stores:

- `offset`: the first logical index covered by the block, such as `0`, `64`, `128`, and so on;
- `msk`: a 64-bit mask showing which of the 64 positions are currently set;
- `vals`: a compact integer array containing only the values that actually exist.

For example, if the user stores:

```text
csa[202] = 25
csa[203] = 30
csa[263] = 100
```

then the structure can be represented as:

```text
2 blocks {2|[202]=25:[203]=30}{1|[263]=100}
```

This means:

- one block stores the two values at indices `202` and `203`;
- another block stores the value at index `263`;
- all unset indices use no space in `vals`.

### 2.2 Bit masks and `popcount`

The implementation uses a 64-bit unsigned integer as the mask type:

```c
#define MSKLEN 64
typedef uint64_t mask_t;
```

For a logical index `idx`, the block and bit position are calculated as:

```text
offset = (idx / 64) * 64
bit    = idx % 64
```

For example, index `202` maps to:

```text
offset = 192
bit    = 10
```

If bit `10` is set in the block's mask, then `csa[202]` exists. If that bit is clear, the index has not been written.

The compact `vals` array does not have one slot per possible bit. Instead, the code counts how many set bits appear before the target bit. That count is the position in `vals`.

The implementation uses:

```c
__builtin_popcountll(msk)
```

to count set bits efficiently.

For a mask where bits `2`, `5`, and `6` are set:

```text
bit 2 -> vals[0]
bit 5 -> vals[1]
bit 6 -> vals[2]
```

This is the key compression technique: the logical array may have large gaps, but the physical storage only contains real values.

### 2.3 Sorted block array

Blocks are kept in ascending `offset` order.

When a new block is needed, the implementation:

1. finds the correct insertion position;
2. expands the block array with `realloc()`;
3. shifts later blocks with `memmove()`;
4. initialises the new block with an empty mask and no values.

Keeping blocks sorted makes the output deterministic and keeps related values grouped by index range.

### 2.4 Dynamic memory management

The project allocates memory only when it is needed:

- the block array grows when a new 64-index region is first used;
- a block's `vals` array grows when a new value is inserted;
- overwriting an existing value does not allocate a new block;
- deleting a value can shrink or free a block's `vals` array;
- deleting the last value in a block removes the whole block;
- `csa_free()` releases every nested allocation.

This design solves the main sparse-array problem: it avoids allocating memory for unset indices.

### 2.5 Algorithms and example programs

The repository contains these main example files:

| File | Role |
| --- | --- |
| `examples/driver.c` | Runs assertions for core CSA behaviour and extension behaviour |
| `examples/fibmemo.c` | Uses the CSA as a memo table for recursive Fibonacci |
| `examples/isfactorial.c` | Stores factorial numbers in a CSA and prints them later |
| `examples/sieve.c` | Uses CSA deletion and iteration to implement the Sieve of Eratosthenes |

The Fibonacci example demonstrates memoisation: once a Fibonacci value has been calculated, it is stored in the CSA and reused later instead of being recomputed recursively.

The sieve example demonstrates deletion: numbers are inserted first, and composite numbers are removed until only primes remain.

## 3. Results and version differences

### 3.1 Build targets

The `Makefile` defines several useful targets:

| Target | Compiler flags | Purpose |
| --- | --- | --- |
| `csa` | `-O3` | Optimised core test executable |
| `csa_s` | `-g3 -fsanitize=address -fsanitize=undefined` | Debug/sanitizer test executable |
| `fibmemo` | `-O3` | Fibonacci memoisation demo |
| `csa_ext` | `-DEXT -O3` | Driver tests with extension features enabled |
| `factorials` | `-DEXT -O3` | Factorial-number demo |
| `primes` | `-DEXT -O3` | Prime-number sieve demo |

### 3.2 Difference between the two main versions

The two main executables are `csa` and `csa_s`.

#### `csa`: optimised version

- built with `-O3`;
- intended for normal execution;
- runs quickly;
- does not include runtime sanitizer checks.

#### `csa_s`: sanitizer version

- built with `-g3`, AddressSanitizer, and UndefinedBehaviorSanitizer;
- checks for memory errors and undefined behaviour;
- is slower than the optimised version because of the extra instrumentation;
- is useful during debugging and validation.

There is also an extension build enabled with `-DEXT`. It adds:

- `csa_foreach()`, which visits every stored value and applies a callback;
- `csa_delete()`, which removes an index and deletes empty blocks.

### 3.3 Runtime results

In the current environment, the full run was tested with:

```bash
make clean && time make runall
```

The complete build-and-run sequence succeeded. The total runtime was approximately:

```text
real    0m6.565s
user    0m4.761s
sys     0m1.055s
```

Individual executable timings measured in the same environment were:

| Program | Real time | Result |
| --- | ---: | --- |
| `./csa` | `0.009s` | Core assertions passed with no output |
| `./csa_s` | `0.086s` | Sanitizer build passed with no output |
| `./fibmemo` | `0.008s` | Printed Fibonacci values from 1 to 40 |
| `./factorials` | `2.960s` | Printed factorial numbers up to `479001600` |
| `./primes` | `0.010s` | Printed primes from 2 to 271 |
| `./csa_ext` | `0.009s` | Extension assertions passed with no output |

The most expensive example is `factorials`, because it checks integers from `0` to `499999999`. The sanitizer executable is also slower than the optimised executable because it performs additional runtime checks.

### 3.4 Example outputs

The end of the `fibmemo` output is:

```text
36 => 14930352
37 => 24157817
38 => 39088169
39 => 63245986
40 => 102334155
```

The `factorials` program prints:

```text
1
2
6
24
120
720
5040
40320
362880
3628800
39916800
479001600
```

The end of the `primes` output is:

```text
239
241
251
257
263
269
271
```

## 4. Project layout

The repository is organised by purpose:

| Path | Contents |
| --- | --- |
| `src/` | CSA implementation source files |
| `include/` | Public and internal headers |
| `examples/` | Driver, demo, and extension example programs |
| `docs/` | Assignment/reference PDF documents |
| `notes/` | Development notes and alternative/improved draft files |
| `check.sh` | Submission/checking helper script |
| `Makefile` | Build targets for the core, sanitizer, and extension executables |

## 5. How to run

Run the full set:

```bash
make clean
make runall
```

Run the optimised core test only:

```bash
make csa
./csa
```

Run the sanitizer build:

```bash
make csa_s
./csa_s
```

Run the extension programs:

```bash
make csa_ext factorials primes
./csa_ext
./factorials
./primes
```

## 6. Summary

This project implements a compact sparse integer array using **64-bit masks, compact value arrays, sorted blocks, and dynamic allocation**.

The result is an array-like structure that avoids storing unset indices. The example programs show that the structure can be used for memoisation, value filtering, iteration, and deletion while keeping memory usage proportional to the number of stored values rather than the largest index.
