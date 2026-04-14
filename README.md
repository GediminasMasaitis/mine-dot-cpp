# mine-dot-cpp

A Minesweeper solver library implemented in C++. Combines multiple solving strategies — trivial deduction, Gaussian elimination, exhaustive combinatorial search, and mine count constraint analysis — to determine which cells are safe and which contain mines. Exposes both a C++ API and a C-compatible shared library interface for integration with other languages.

## Building

Requires CMake 3.6+ and a C++20 compiler.

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

Build outputs:
- `build/lib/` — shared library (`minedotcpp.dll` / `libminedotcpp.so`)
- `build/bin/` — test CLI executable

### Build options

Set in the root `CMakeLists.txt`:

| Option | Default | Description |
|---|---|---|
| `USE_SPARSEHASH` | `true` | Use Google sparsehash for high-performance hash maps |
| `USE_OPENCL` | `true` | Enable GPU-accelerated combination search (requires OpenCL SDK) |
| `shared` | `true` | Build as shared library (DLL/SO) with C API exports |

### Dependencies

- **[CTPL](https://github.com/vit-vit/CTPL)** by vit-vit — header-only thread pool library (bundled in `external/ctpl/`)
- **[Google sparsehash](https://github.com/sparsehash/sparsehash)** by Google — high-performance hash maps, specifically `dense_hash_map` and `dense_hash_set` (bundled in `external/sparsehash/`)
- **OpenCL** (optional) — GPU acceleration for combination validation

## Architecture

### Solver pipeline

The solver runs strategies in sequence, with early exit controlled by settings. Each strategy can produce **verdicts** (definitive mine/safe conclusions) and **probabilities** (likelihood estimates).

```
solver::solve(map, results)
  1. Trivial solver        — rule-based deduction
  2. Gaussian solver       — linear algebra over constraints
  3. Separation solver     — exhaustive combinatorial search
     a. Border separation  — split into independent subproblems
     b. Partial borders    — approximate large borders by subsets
     c. Combination search — enumerate valid mine placements (CPU or GPU)
     d. Mine count solver  — use total mine count as global constraint
  4. Guessing              — pick the least risky cell when stuck
```

Each stage can be individually enabled/disabled. Between stages, the solver checks early-stopping conditions to avoid unnecessary work when sufficient verdicts have been found.

### Trivial solver

Applies basic Minesweeper deduction rules iteratively until no new verdicts are found:

- If a numbered cell's hint equals the number of unflagged neighbors, all unflagged neighbors are mines.
- If a numbered cell's hint equals the number of flagged neighbors, all remaining unflagged neighbors are safe.

### Gaussian solver

Converts the board into a system of linear equations where each revealed number creates a constraint over its unknown neighbors. Applies Gaussian elimination with multiple reduction parameter sets (varying row direction and uniqueness constraints) to find cells that must be 0 or 1 in all solutions.

The solver tries up to four reduction strategies sequentially, stopping when verdicts are found:

| Strategy | `skip_reduction` | `reverse_rows` | `use_unique_rows` |
|---|---|---|---|
| 1 | `false` | `false` | `true` |
| 2 | `false` | `true` | `true` |
| 3 | `false` | `true` | `false` |
| 4 | `false` | `false` | `false` |

If `gaussian_resolve_on_success` is enabled (default), the solver re-runs reduction after each successful round to find additional verdicts from the simplified matrix.

### Separation solver

Handles complex board states that the Gaussian solver cannot fully resolve:

1. **Border identification** — finds all unrevealed cells adjacent to at least one revealed cell (the "border").
2. **Border separation** — splits the border into independent connected components that can be solved independently.
3. **Combination search** — for each border, enumerates all possible mine/safe assignments (2^n for n border cells) and filters to those consistent with all adjacent hints. Valid assignments are found via:
   - Single-threaded brute force (small borders)
   - Multi-threaded parallel search via thread pool (medium borders)
   - OpenCL GPU kernel (large borders, when enabled)
4. **Probability calculation** — from the valid combinations, computes the probability of each cell being a mine.
5. **Verdict extraction** — cells with probability 0 or 1 are converted to definitive verdicts.

#### Partial border solving

When a border exceeds `partial_solve_from_size` cells, exhaustive enumeration becomes infeasible (2^28 = 268M combinations at the default `give_up_from_size` of 28). The partial border solver approximates by:

1. Selecting subsets of the border up to `partial_optimal_size` cells.
2. Solving each subset exhaustively.
3. Extracting definitive verdicts from subsets.
4. Averaging probabilities across all subsets containing each cell.

Cells are selected using a lowest-use-count heuristic to ensure even coverage across the border.

#### Mine count solver

When the total remaining mine count is known, it provides an additional global constraint:

1. **Combination trimming** — eliminates valid combinations that would require more mines than remain on the board.
2. **Cross-border analysis** — for borders with variable mine counts, enumerates all inter-border mine distributions weighted by combinatorial coefficients.
3. **Non-border probabilities** — computes uniform mine probability for unrevealed cells not adjacent to any revealed cell, using the expected mines remaining after accounting for border probabilities.

### Guessing

When no deterministic verdicts are available, the solver picks the cell with the lowest mine probability as a guess. Controlled by `guess_if_no_no_mine_verdict` (guess when only mine verdicts exist but no safe verdicts) and `guess_if_no_verdict` (guess when no verdicts exist at all).

## Map format

Maps are represented as text strings where each character is a cell. Rows are separated by newlines, and the map is oriented so that each row in the text maps to an X-coordinate slice.

| Character | Meaning |
|---|---|
| `.` | Revealed, 0 adjacent mines |
| `1`-`8` | Revealed, N adjacent mines |
| `#` | Unrevealed (unknown) |
| `?` | Unrevealed (unknown, same as `#`) |
| `!` | Unrevealed, flagged as mine |
| `v` | Unrevealed, flagged as safe |
| `X` | Wall (not part of the board) |

An optional line starting with `m` followed by a number specifies the remaining mine count (e.g., `m10`).

Example:
```
###2
####
##32
##1.
m5
```

## C API

The shared library exports two C-compatible functions for integration with other languages (C#, Python, etc.):

### `init_solver`

```c
void init_solver(solver_settings settings);
```

Initializes or reinitializes the global solver instance with the given settings. Must be called before `solve`. Calling again replaces the previous solver.

### `solve`

```c
int solve(const char* map_str, solver_result* results_buffer, int* buffer_size);
```

Solves the given map string and writes results to the provided buffer.

**Parameters:**
- `map_str` — null-terminated map string in the text format described above
- `results_buffer` — pre-allocated array of `solver_result` structs
- `buffer_size` — pointer to buffer capacity; on return, set to the number of results written

**Returns:** `1` on success, `-1` if the buffer is too small.

### `solver_result`

```c
struct solver_result {
    point pt;           // {x, y} coordinates
    double probability; // mine probability [0.0, 1.0]
    solver_verdict verdict; // verdict_none (0), verdict_has_mine (1), verdict_doesnt_have_mine (2)
};
```

## C++ API

```cpp
#include "solvers/solver.h"

solver_settings settings;
// configure settings...
auto s = minedotcpp::solvers::solver(settings);

minedotcpp::common::map m;
// parse or construct the map...
auto results = minedotcpp::common::point_map<minedotcpp::solvers::solver_result>();
s.solve(m, results);

for (auto& res : results) {
    // res.second.pt, res.second.probability, res.second.verdict
}
```

## Solver settings reference

All settings are fields on the `solver_settings` struct with the defaults shown.

### Trivial solver

| Setting | Default | Description |
|---|---|---|
| `trivial_solve` | `true` | Enable the trivial deduction solver |
| `trivial_stop_on_no_mine_verdict` | `true` | Stop after trivial if a safe cell was found |
| `trivial_stop_on_any_verdict` | `false` | Stop after trivial if any verdict was found |
| `trivial_stop_always` | `false` | Always stop after trivial (skip later stages) |

### Gaussian solver

| Setting | Default | Description |
|---|---|---|
| `gaussian_solve` | `true` | Enable Gaussian elimination solver |
| `gaussian_resolve_on_success` | `true` | Re-run reduction after finding verdicts to find more |
| `gaussian_single_stop_on_no_mine_verdict` | `true` | Stop after a single Gaussian round if a safe cell was found |
| `gaussian_single_stop_on_any_verdict` | `false` | Stop after a single Gaussian round if any verdict was found |
| `gaussian_single_stop_always` | `false` | Always stop after one Gaussian round |
| `gaussian_all_stop_on_no_mine_verdict` | `true` | Stop after all Gaussian rounds if a safe cell was found |
| `gaussian_all_stop_on_any_verdict` | `false` | Stop after all Gaussian rounds if any verdict was found |
| `gaussian_all_stop_always` | `false` | Always stop after Gaussian (skip separation) |

### Separation solver

| Setting | Default | Description |
|---|---|---|
| `separation_solve` | `true` | Enable separation (combinatorial) solver |
| `separation_order_borders_by_size` | `true` | Solve smaller borders first |
| `separation_order_borders_by_size_descending` | `false` | Solve larger borders first (mutually exclusive with above) |
| `separation_single_border_stop_on_no_mine_verdict` | `true` | Stop after solving a border if a safe cell was found |
| `separation_single_border_stop_on_any_verdict` | `false` | Stop after solving a border if any verdict was found |
| `separation_single_border_stop_always` | `false` | Stop after solving one border |

### Partial border solver

| Setting | Default | Description |
|---|---|---|
| `partial_solve` | `true` | Enable partial border approximation for large borders |
| `partial_single_stop_on_no_mine_verdict` | `false` | Stop after one partial border yields a safe verdict |
| `partial_single_stop_on_any_verdict` | `false` | Stop after one partial border yields any verdict |
| `partial_all_stop_on_no_mine_verdict` | `true` | Stop all partial solving if a safe cell was found |
| `partial_all_stop_on_any_verdict` | `false` | Stop all partial solving if any verdict was found |
| `partial_stop_always` | `false` | Stop after partial solving (skip full border solve) |
| `partial_solve_from_size` | `24` | Minimum border size to trigger partial solving |
| `partial_optimal_size` | `20` | Target size for partial border subsets |
| `partial_set_probability_guesses` | `true` | Average probabilities from partial borders for unsolved cells |

### Border re-separation

| Setting | Default | Description |
|---|---|---|
| `resplit_on_partial_verdict` | `true` | After partial verdicts, re-separate the border and solve the smaller pieces |
| `resplit_on_complete_verdict` | `false` | After full solve verdicts, re-separate and solve remaining pieces |

### Mine count constraints

| Setting | Default | Description |
|---|---|---|
| `mine_count_ignore_completely` | `false` | Ignore the remaining mine count entirely (treat as unknown) |
| `mine_count_solve` | `true` | Use total mine count to trim invalid combinations and compute cross-border probabilities |
| `mine_count_solve_non_border` | `true` | Compute uniform mine probability for non-border unrevealed cells |

### Combination search limits

| Setting | Default | Description |
|---|---|---|
| `give_up_from_size` | `28` | Skip exhaustive search for borders larger than this (2^28 = ~268M combinations) |

### OpenCL (GPU acceleration)

| Setting | Default | Description |
|---|---|---|
| `valid_combination_search_open_cl` | `true` | Use OpenCL GPU for combination validation |
| `valid_combination_search_open_cl_allow_loop_break` | `false` | Allow early loop exit in GPU kernel (can hurt performance on some GPUs due to branch divergence) |
| `valid_combination_search_open_cl_use_from_size` | `16` | Minimum border size to use GPU (smaller borders are faster on CPU) |
| `valid_combination_search_open_cl_max_batch_size` | `31` | Max GPU batch as power of 2 (2^31 work items per batch) |
| `valid_combination_search_open_cl_platform_id` | `0` | OpenCL platform index |
| `valid_combination_search_open_cl_device_id` | `0` | OpenCL device index within platform |

### Multithreading

| Setting | Default | Description |
|---|---|---|
| `valid_combination_search_multithread` | `true` | Use thread pool for combination validation |
| `valid_combination_search_multithread_use_from_size` | `8` | Minimum border size to use multithreading |
| `valid_combination_search_multithread_thread_count` | `16` | Number of threads for combination search |

### Mine count cross-border analysis

| Setting | Default | Description |
|---|---|---|
| `variable_mine_count_borders_probabilities_multithread_use_from` | `128` | Total cross-border combinations before using thread pool |
| `variable_mine_count_borders_probabilities_give_up_from` | `131072` | Give up cross-border analysis if total combinations exceed this |

### Guessing

| Setting | Default | Description |
|---|---|---|
| `guess_if_no_no_mine_verdict` | `true` | Guess the safest cell if only mine verdicts were found (no safe cells identified) |
| `guess_if_no_verdict` | `false` | Guess even if no verdicts were found at all |

### Debug

| Setting | Default | Description |
|---|---|---|
| `debug_setting_1` | `0` | Reserved for development |
| `debug_setting_2` | `0` | Reserved for development |
| `debug_setting_3` | `0` | Reserved for development |
