# Stage 5 MPI Runtime Validation (Tasks 3/7/9/11)

Date: 2026-02-24

## Environment

Requested module stack loaded:

- `intel/tbb`
- `intel/compiler-rt`
- `intel/umf`
- `intel/compiler-intel-llvm/2025.3.0`
- `intel/mpi/2021.17`

`mpirun` verification:

```bash
which mpirun
/opt/ohpc/pub/intel/oneapi-2025.03/mpi/2021.17/bin/mpirun

mpirun --version
Intel(R) MPI Library for Linux* OS, Version 2021.17 Build 20251009 (id: 0345ffb)
```

## Build Validation

Built with Intel MPI toolchain under module stack:

- `NewDD/newdd.exe`
- `opFoF/opfof.exe`
- `NewGalFinder/gfind.exe`

Task 11 compile/link check:

- `opFoF/Rules.make` has `-fcommon` removed.
- `opFoF` link completed with no multiply-defined symbol diagnostics (`multiple definition` not present in build log).

## Runtime Matrix (1/2/4 ranks)

All runs executed with timeout guards (`timeout 600`) to detect hangs/deadlocks.

### Task 9 path (`newdd.exe 0 4`)

| Ranks | Exit | Deadlock/Timeout | Hash vs rank-1 | Memory stack |
|---|---:|---|---|---|
| 1 | 0 | No | baseline | `Current memory stack 0` present |
| 2 | 0 | No | identical (`diff` empty) | `Current memory stack 0` present |
| 4 | 0 | No | identical (`diff` empty) | `Current memory stack 0` present |

Hash files compared:

- `/tmp/stage5_mpi/r1_newdd/newdd.sha`
- `/tmp/stage5_mpi/r2_newdd/newdd.sha`
- `/tmp/stage5_mpi/r4_newdd/newdd.sha`

### Task 11 + Task 3 runtime path (`opfof.exe 0 4`)

| Ranks | Exit | Deadlock/Timeout | Hash vs rank-1 |
|---|---:|---|---|
| 1 | 0 | No | baseline |
| 2 | 0 | No | identical (`diff` empty) |
| 4 | 0 | No | identical (`diff` empty) |

Hash files compared:

- `/tmp/stage5_mpi/r1_opfof/opfof.sha`
- `/tmp/stage5_mpi/r2_opfof/opfof.sha`
- `/tmp/stage5_mpi/r4_opfof/opfof.sha`

### Task 7 runtime path (`gfind.exe 0`)

| Ranks | Exit | Deadlock/Timeout | Hash vs rank-1 | Cosmology log |
|---|---:|---|---|---|
| 1 | 0 | No | baseline | `omepb=0.049` |
| 2 | 0 | No | identical (`diff` empty) | `omepb=0.049` |
| 4 | 0 | No | identical (`diff` empty) | `omepb=0.049` |

Hash files compared:

- `/tmp/stage5_mpi/r1_gfind/gfind.sha`
- `/tmp/stage5_mpi/r2_gfind/gfind.sha`
- `/tmp/stage5_mpi/r4_gfind/gfind.sha`

`omepb` evidence from runtime logs:

- `/tmp/stage5_mpi/r1_gfind/gfind.err`
- `/tmp/stage5_mpi/r2_gfind/gfind.err`
- `/tmp/stage5_mpi/r4_gfind/gfind.err`

## Dedicated Task 3 MPI Round-Trip Validation

Because the `BIG_MPI_Send/Recv` bug manifests in chunked pointer stepping for non-byte types, a focused MPI harness was executed using the corrected dispatch logic and forced chunking (`mpi_nchunk=17`) with `MPI_DOUBLE`.

Run results:

| Ranks | Exit | Result |
|---|---:|---|
| 1 | 0 | `BIG_MPI_DOUBLE_ROUNDTRIP_OK` |
| 2 | 0 | `BIG_MPI_DOUBLE_ROUNDTRIP_OK` |
| 4 | 0 | `BIG_MPI_DOUBLE_ROUNDTRIP_OK` |

Artifacts:

- `/tmp/stage5_bigmpi_n1.out`
- `/tmp/stage5_bigmpi_n2.out`
- `/tmp/stage5_bigmpi_n4.out`

Compiler warning check for assignment-vs-comparison pattern:

- `mpiicx -Wall -Wextra -Wparentheses -fsyntax-only opFoF/Treewalk.fof.ordered.c`
- No `-Wparentheses` / assignment-in-condition warnings observed.

## Collective/Branching Safety Audit

### No rank-dependent collective placement in affected paths

- `NewDD/newdd.c`:
  - `MPI_Bcast(&gadget_found...)`, `MPI_Bcast(basename...)`, `MPI_Bcast(&ram.nfiles...)` are executed before non-root early return.
  - Non-root branch returns only after matching the Bcast sequence.
- `NewGalFinder/gfind.c`:
  - Root (`myid==0`) reads/overrides cosmology.
  - All ranks execute identical Bcast sequence:
    `ng`, `nspace`, `omep`, `omepb`, `omeplam`, `hubble`, `size`, `amax`, `a`.
- `opFoF/opfof.c`:
  - Bcasts are in argc-based control-flow (`argc==4` or `argc==3`), which is uniform across ranks for a given launch.
  - No task-specific new collectives were introduced.

### Mismatched `MPI_Bcast` / `MPI_Reduce` checks

- No `MPI_Reduce` calls were added in tasks 3/7/9/11 paths.
- Bcast sequences in affected code were reviewed and exercised in 1/2/4 rank runs with no hangs or MPI runtime errors.

### Runtime error scan

No `MPI_ERR`, Hydra, assertion, or deadlock signatures found in matrix run logs.

## Memory Error Check

`valgrind` is not installed in this environment:

```bash
which valgrind
# not found
```

Therefore rank-1 valgrind execution could not be performed.

## Behavior vs Single-Rank Logic

No runtime behavior differences were observed between 1-rank and multi-rank (2/4) executions for affected task paths:

- Exit status: consistent success.
- Produced outputs: byte-identical where deterministic output is expected.
- No deadlocks/timeouts.

## Acceptance Criteria Confirmation

- **Task 3**: PASS (runtime MPI_DOUBLE round-trip succeeds in 1/2/4 ranks; no assignment-in-condition warning pattern remains).
- **Task 7**: PASS (new `omepb` broadcast is active; 1/2/4 rank gfind runs complete and produce deterministic outputs; runtime log shows expected `omepb=0.049`).
- **Task 9**: PASS (`MPI_Init` precedes discovery; 1/2/4 rank NewDD runs complete; output hashes identical; memory stack returns to zero).
- **Task 11**: PASS (`-fcommon` removed; single-definition model retained; opFoF builds and runs correctly in 1/2/4 ranks with deterministic outputs).

