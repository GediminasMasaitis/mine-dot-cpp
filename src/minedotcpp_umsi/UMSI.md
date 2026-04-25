# UMSI — Universal Minesweeper Solving Interface

UMSI is a line-oriented text protocol for driving a Minesweeper solver as a separate process over standard input and output. It is modelled on chess's UCI: the GUI launches the engine as a subprocess, performs a handshake to discover its capabilities, configures it via named options, loads a position, and asks it to solve.

This document describes protocol version **0.1**, as implemented by the `minedotcpp_umsi` reference engine ([main.cpp](main.cpp)). A reference client lives at [ExtSolver.cs](../../../mine-dot-net/src/MineDotNet/AI/Solvers/ExtSolver.cs).

## Process model

- The GUI starts the engine as a child process with stdin, stdout, and stderr piped.
- All protocol traffic flows over stdin (GUI → engine) and stdout (engine → GUI). stderr is not used by the protocol; the GUI may discard or log it freely.
- The engine holds at most one solver configuration and one loaded position at any time. Sending a new `setoption` updates the configuration; sending a new `position` replaces the loaded map. There is no notion of multiple sessions or named slots.
- Solves run synchronously. Once a `go` is issued, the engine produces results and emits `done`; there is no way to cancel a running solve in v0.

## Wire format

- Communication is line-oriented. A line is a sequence of bytes terminated by either `\n` or `\r\n`. The engine trims a trailing `\r` if present, so clients may use either convention.
- Lines are ASCII; the engine does not interpret non-ASCII bytes.
- The engine writes its stdout unbuffered, so every response line reaches the client as soon as it is emitted. Clients should likewise flush after every command they send.
- Empty lines are ignored.
- A line whose first non-whitespace token begins with `#` is treated as a comment and ignored. Comments are useful for protocol traces and scripted test inputs.
- Tokens within a command are separated by whitespace. Values passed via `setoption … value …` are single whitespace-delimited tokens — values containing spaces are not supported.

## Commands (GUI → engine)

### `umsi`

Initiates the handshake. The engine responds with identification lines, one `option` line per supported setting, and finally `umsiok`:

```
id name <engine-name>
id author <author-name>
id version <version>
option name <name> type <type> default <default> [min <min> max <max>]
…
umsiok
```

The client should send `umsi` exactly once, immediately after launching the engine, and use the option lines that follow to discover what settings the engine accepts. No other command should be sent before `umsiok` is received.

### `isready`

Liveness and synchronisation probe. The engine responds with a single line:

```
readyok
```

`isready` is also useful as a sync barrier after a batch of `setoption` commands: the engine processes commands strictly in order, so a `readyok` confirms that every preceding `setoption` has been applied (and any error responses they produced are already in the pipe).

### `setoption name <name> value <value>`

Sets a single registered option to the given value. `<name>` must match the name reported by an `option` line during the handshake (case-sensitive). `<value>` must be a single whitespace-delimited token.

Type-specific value rules:

| Type | Valid `<value>` |
|---|---|
| `check` (boolean) | `true`, `false`, `1`, `0`, `yes`, `no`, `on`, `off` |
| `spin` (integer) | a decimal integer within the option's `min`/`max` range |

If the name is not recognised, the engine emits:

```
error unknown option: <name>
```

If the value cannot be parsed (e.g. a non-numeric value for a `spin` option), the engine emits:

```
error failed to set <name>: <reason>
```

In either case the engine continues running and the option keeps its previous value. There is no positive acknowledgement on success — a successful `setoption` produces no response. Use `isready` afterwards to draw a sync line.

### `position`

Loads a Minesweeper position. Two forms are accepted:

**Single-line form:** the map text follows the command on the same line, with rows separated by `;`:

```
position ###2;####;##32;##1.;m5
```

**Multi-line form:** the command is sent alone, then each row is sent on its own line, terminated by an `end` line:

```
position
###2
####
##32
##1.
m5
end
```

Both forms produce identical state; the engine simply substitutes `;` for newlines in the single-line form. The map text must use the format described in [Map format](#map-format) below.

`position` does not produce a response on success. Parse errors are deferred until `go` is issued (see below). Loading a new position replaces the previous one; there is no way to clear the loaded position other than overwriting it.

### `go`

Solves the currently loaded position with the current settings. The engine responds with zero or more `result` lines followed by exactly one `done` line:

```
result <x> <y> <probability> <verdict>
…
done
```

If no position has been loaded, or if the loaded position fails to parse, or if the solver throws, the engine emits a single `error` line followed by `done`:

```
error no position loaded
done
```

```
error parse failed: <reason>
done
```

```
error solve failed: <reason>
done
```

`done` always terminates the response to a `go`, error or not. Clients should read until `done` and treat its arrival as the signal that the engine is ready for the next command.

### `quit`

Terminates the engine. The engine exits with status `0` immediately; no response is emitted. Closing stdin (sending EOF) has the same effect — the engine's read loop exits cleanly.

## Engine responses

| Line prefix | Emitted by | Meaning |
|---|---|---|
| `id name <name>` | `umsi` | Engine product name. |
| `id author <name>` | `umsi` | Engine author. |
| `id version <ver>` | `umsi` | Engine version string. |
| `option name <name> type <t> default <d> [min <lo> max <hi>]` | `umsi` | One supported option. `min`/`max` are present only for `spin` options. |
| `umsiok` | `umsi` | Handshake complete. |
| `readyok` | `isready` | Engine is alive and has finished processing prior commands. |
| `result <x> <y> <prob> <verdict>` | `go` | One per cell the solver has information about. See [Result format](#result-format). |
| `done` | `go` | End of a `go` response. Always emitted, even on error. |
| `error <message>` | various | Recoverable error; the engine remains running. |

Unknown commands yield `error unknown command: <cmd>` and the engine continues running.

### Result format

```
result <x> <y> <probability> <verdict>
```

- `<x>` and `<y>` — integer coordinates of the cell, using the map's coordinate system. The map is laid out so that each text row corresponds to an X-coordinate slice and each character within a row corresponds to a Y position; see [the solver README](../../README.md#map-format) for details.
- `<probability>` — a real number in `[0.0, 1.0]` formatted with 6 decimal places (`std::fixed`, precision 6, e.g. `0.166667`). This is the solver's estimate of the probability that the cell contains a mine.
- `<verdict>` — one of:
  - `mine` — the cell definitely contains a mine. `<probability>` will be `1.000000`.
  - `safe` — the cell definitely does not contain a mine. `<probability>` will be `0.000000`.
  - `unknown` — no definitive verdict; only the probability is informative. This is the typical case for cells where the solver fell back to guessing.

The engine emits a result line for every cell on which the solver produced information; cells not mentioned have no information from the solver. The order of result lines is unspecified — clients should accumulate them into a map keyed by `(x, y)`.

## Settings reference

The set of options is fixed at handshake time and reported via `option` lines. The reference engine exposes the full `solver_settings` surface as `check` (boolean) and `spin` (integer) options, named in `snake_case`. Categories include:

- Trivial solver (`trivial_*`)
- Gaussian solver (`gaussian_*`)
- Separation solver (`separation_*`)
- Partial border solver (`partial_*`)
- Border re-separation (`resplit_*`)
- Mine count constraints (`mine_count_*`, `give_up_from_size`)
- OpenCL acceleration (`valid_combination_search_open_cl_*`)
- Multithreading (`valid_combination_search_multithread_*`)
- Combination search internals (`combination_search_*`)
- Variable mine count probabilities (`variable_mine_count_borders_probabilities_*`)
- Guessing (`guess_*`)
- Debug slots (`debug_setting_1`, `debug_setting_2`, `debug_setting_3`)

Defaults, ranges, and per-setting semantics are documented in [the solver README's settings reference](../../README.md#solver-settings-reference). The authoritative list of names a given engine actually exposes is the set of `option` lines emitted during the handshake.

The `print_trace` setting and its associated thresholds are deliberately **not** exposed: enabling them would cause the solver to write trace output to stdout, which would corrupt the protocol channel. The engine also pins `print_trace` to off internally so it cannot be re-enabled by other means.

## Map format

The text passed between `position` and `end` (or after `position ` on a single line, with `;` substituted for newlines) uses the same format as the rest of the codebase:

| Character | Meaning |
|---|---|
| `.` | Revealed, 0 adjacent mines |
| `1`–`8` | Revealed, N adjacent mines |
| `#` | Unrevealed (unknown) |
| `?` | Unrevealed (unknown, same as `#`) |
| `!` | Unrevealed, flagged as mine |
| `v` | Unrevealed, flagged as safe |
| `X` | Wall (not part of the board) |

An optional line starting with `m` followed by an integer specifies the remaining mine count for the whole board (e.g. `m5`).

See [the solver README](../../README.md#map-format) for the full specification and worked examples.

## Example session

GUI lines are prefixed with `>` and engine lines with `<` for clarity; the protocol itself does not use these prefixes.

```
> umsi
< id name minedotcpp
< id author Gediminas Masaitis
< id version 0.1
< option name trivial_solve type check default true
< option name gaussian_solve type check default true
< option name partial_solve_from_size type spin default 24 min 0 max 128
< … (more option lines) …
< umsiok

> setoption name partial_solve_from_size value 20
> setoption name guess_if_no_verdict value true
> isready
< readyok

> position ###2;####;##32;##1.;m5
> go
< result 0 0 0.166667 unknown
< result 0 1 0.500000 unknown
< result 1 0 1.000000 mine
< result 1 1 0.000000 safe
< … (more result lines) …
< done

> quit
```

## Limitations (v0.1)

- **No `stop`.** A `go` runs to completion. Clients that need responsiveness on slow boards should set tighter thresholds (`give_up_from_size`, `partial_solve_from_size`) or run the engine in a thread that the GUI can ignore once it loses interest.
- **No streaming progress.** The engine emits no output between `go` and the first `result` (or `done`). There is no `info` channel for partial probabilities or status updates.
- **Single position.** There is no way to load multiple positions in parallel within one process. Clients that need parallelism should launch additional engine processes.
- **Whitespace-only values.** `setoption` values cannot contain spaces. All currently exposed options take simple boolean or integer values, so this is not a practical limitation today, but protocol consumers should not assume future string-valued options will tolerate whitespace.
- **No success ack for `setoption` or `position`.** Use a following `isready` if you need to confirm a batch was applied without errors.
- **`print_trace` is forced off.** Tracing must be obtained via other means (e.g. linking against the C++ library directly) — it cannot be re-enabled over UMSI without breaking the channel.
