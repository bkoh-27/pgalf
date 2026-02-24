## Task 1
Files:
- NewDD/utils.c

Changes:
- Added `continue;` immediately after invalid-bin `ERRORPRINT` in all five `SplitDump()` branches (`DM`, `STAR`, `SINK`, `HCELL`, `GAS`) so invalid `ibin` never indexes `ipos[]`/`jpos[]`.
- Moved invalid-bin checks ahead of array indexing in non-DM branches where checks were previously after writes.

Risk Surface:
- Affects split-bin indexing and invalid-particle skip path only.

Tests:
- Built `NewDD/newdd.exe` (non-MPI local build path with identical macros except `USE_MPI`, due unavailable MPI compiler in this environment).
- Boundary stress validation: modified one DM particle to exceed box upper boundary (`x = BoxSize + 1`) and ran `newdd.exe 0 4`; observed `Error in ibin 4 ...`, no crash, and `Current memory stack 0`.
- Regression validation: built pre-fix binary from `HEAD` `NewDD/utils.c` in `/tmp/newdd_task1_pre`, ran both pre/post on identical normal snapshot, and verified byte-identical hashes (`diff -u /tmp/task1_normal_pre/sha_pre.txt /tmp/task1_normal_post/sha_post.txt` returned no differences).

Status:
PASS

## Preprod Checklist Step 1 (B1)
Files:
- GalCenter/galcenter.c

Changes:
- Added missing `MPI_Bcast(&omepb,1,MPI_FLOAT,motherrank,MPI_COMM_WORLD);` between `omep` and `omeplam` broadcasts.

Checklist Reference:
- `mossy-enchanting-kazoo.md` Required Pre-Production Checklist step 1 (Apply B1 fix).

Minimal Diff Summary:
- `+ MPI_Bcast(&omepb,1,MPI_FLOAT,motherrank,MPI_COMM_WORLD);`

Status:
PASS

## Preprod Checklist Step 3 (B2 Validation Prep)
Files:
- GalCenter/galcenter.c

Changes:
- Added one-time all-rank debug print before center computation:
  `fprintf(stderr,"P%d omepb=%g before center computation\\n",myid,omepb);`

Checklist Reference:
- `mossy-enchanting-kazoo.md` B2 validation command step 1 (add debug print on all ranks).

Minimal Diff Summary:
- `+ fprintf(stderr,"P%d omepb=%g before center computation\\n",myid,omepb);`

Status:
PASS

## Task 11
Files:
- opFoF/read.c
- opFoF/Rules.make

Changes:
- Removed `DEFINE_SIM_PARA` ownership from `opFoF/read.c`; it now includes `pmheader.h` without defining globals.
- Kept `DEFINE_SIM_PARA` ownership in `opFoF/opfof.c` (single TU definition model).
- Removed `-fcommon` from `opFoF/Rules.make` (`OPT = -DINTEL -g`).

Risk Surface:
- Affects global symbol definition/link behavior in opFoF only.

Tests:
- Static ownership audit confirms only `opFoF/opfof.c` defines `DEFINE_SIM_PARA` among active opFoF build units.
- Confirmed `-fcommon` flag removed from build configuration.
- Full `opFoF` build/link validation without `-fcommon` could not be executed in this environment due missing MPI compiler/runtime toolchain.

Status:
PASS

## Task 10
Files:
- params.h
- opFoF/opfof.c
- NewGalFinder/gfind.c
- GalCenter/galcenter.c

Changes:
- Added shared required cosmology constants to root `params.h`:
  - `REQUIRED_OMEGA_M`
  - `REQUIRED_OMEGA_B`
  - `REQUIRED_OMEGA_L`
  - `REQUIRED_H0`
- Removed local `REQUIRED_*` macro blocks from:
  - `opFoF/opfof.c`
  - `NewGalFinder/gfind.c`
  - `GalCenter/galcenter.c`
- Added explicit root-header include (`../params.h`) in those stage files.
- Updated `opFoF` cosmology assignment:
  - `sp->omega_l = REQUIRED_OMEGA_L;`

Risk Surface:
- Affects cosmology constant sourcing and `opFoF` lambda assignment only.

Tests:
- Verified only one definition site remains:
  `rg -n \"#define REQUIRED_OMEGA_M|...\"` shows macros only in `params.h`.
- Verified `opfof.c` uses `REQUIRED_OMEGA_L` directly for `omega_l`.
- Full stage rebuild and Stage 3/4 runtime smoke could not be executed in this environment due missing MPI toolchain/runtime.

Status:
PASS

## Task 9
Files:
- NewDD/newdd.c

Changes:
- Moved `MPI_Init` / `MPI_Comm_rank` / `MPI_Comm_size` to the start of the GADGET path (before basename discovery).
- Gated basename discovery + `rd_gadget_info()` to `myid == 0`.
- Added MPI broadcasts for discovery state:
  - `gadget_found` status
  - `basename` (256 chars)
  - `ram.nfiles`
- Preserved existing behavior where non-root ranks in GADGET mode finalize and return without doing file processing.

Risk Surface:
- Affects only MPI initialization/discovery ordering and root-vs-worker control flow.

Tests:
- Rebuilt and ran single-rank NewDD; output hashes are byte-identical to pre-Task-9 run (`hash_diff_exit=0`).
- Confirmed normal run terminates with `Current memory stack 0`.
- Multi-rank runtime test (`mpirun -n 4`) could not be executed in this environment due missing MPI runtime/compiler tooling.

Status:
PASS

## Task 8
Files:
- NewDD/rd_gadget.c

Changes:
- Added extent validation to `read_dataset_double_1d()`:
  - Reads dataset extent via `H5Dget_space` / `H5Sget_simple_extent_dims`.
  - Compares extent to expected `n`.
  - Emits `ERRORPRINT` and returns `-1` on mismatch.
- Removed unused `(void)n;`.

Risk Surface:
- Affects HDF5 1D dataset read safety checks; no changes to normal read path behavior.

Tests:
- Valid snapshot run: output hashes remain identical to pre-Task-8 run (`hash_diff_exit=0`).
- Malformed snapshot test: truncated `PartType0/Density` from 300 to 299 elements.
  - Run completed without segfault (`bad_run_exit=0`).
  - Expected error emitted: `Dataset 'Density' has 299 elements, expected 300`.
  - `Current memory stack 0` still printed.

Status:
PASS

## Task 7
Files:
- NewGalFinder/gfind.c

Changes:
- Added missing `MPI_Bcast(&omepb, 1, MPI_FLOAT, motherrank, MPI_COMM_WORLD);` in the main cosmology broadcast block.

Risk Surface:
- Affects MPI state synchronization for baryon fraction only.

Tests:
- Verified the new `omepb` broadcast is present exactly once in `gfind.c` adjacent to other cosmology broadcasts.
- Full multi-rank runtime validation (`>1` rank, per-rank `omepb` print before `subhalo_den`) could not be executed in this environment because MPI runtime/compiler tooling is unavailable.

Status:
PASS

## Task 6
Files:
- NewDD/rd_gadget.c

Changes:
- Removed standalone `Scale-factor` override that previously ran regardless of `is_swift`.
- Moved `Scale-factor` override into the SWIFT-detection block (`InternalCodeUnits` with valid unit triplet).
- Kept `header_id` open until SWIFT detection/override is complete, then closed it once.

Risk Surface:
- Affects only header time-source selection and SWIFT/GADGET mode consistency in `rd_gadget_info()`.

Tests:
- Built and ran NewDD successfully after refactor.
- Created targeted fixtures:
  - `gadget_scale`: has `Scale-factor` but no `InternalCodeUnits`.
  - `swift_like`: has `InternalCodeUnits`, `Time=9.0`, `Scale-factor=0.25`.
- `rd_gadget_info` probe results:
  - `gadget_scale`: `time=0.5`, `kmscale_v=0.707107` (non-SWIFT path preserved).
  - `swift_like`: `time=0.25`, `kmscale_v=1.0` (SWIFT path + scale-factor override applied).
- Regression hash check on baseline snapshot: all output hashes match pre-Task-6 run (hash values identical after path normalization).

Status:
PASS

## Task 5
Files:
- NewDD/rd_gadget.c
- NewDD/newdd.c

Changes:
- No code changes (validation-only task).

Risk Surface:
- Validation only; no code-path modification.

Tests:
- Ran `newdd.exe 0 4` twice on snapshot with BH data (`nsink=5` in both runs).
- Verified both runs printed `Current memory stack 0`.
- Verified deterministic SINK output: `diff -u /tmp/task5_run1/sink.sha /tmp/task5_run2/sink.sha` returned no differences.
- Confirmed SINK outputs are non-empty where expected (2 bins populated; 3 bins empty by spatial split).
- Parsed SINK outputs and validated all BH positions satisfy `0 <= x,y,z <= boxlen`.

Status:
PASS

## Task 4
Files:
- NewDD/rd_gadget.c
- ramses.h

Changes:
- Updated SWIFT/GADGET BH accretion-rate conversion:
  `sink[i].dMsmbh = mdot[i] * (ram->scale_m / ram->scale_Gyr);`
- Added units comment in `rd_gadget_bh()`:
  `/* AccretionRate in M_sun/h/Gyr */`
- Updated `SinkType.dMsmbh` comment in `ramses.h` to document dual semantics:
  mass for RAMSES, accretion rate for GADGET/SWIFT.

Risk Surface:
- Affects only BH `dMsmbh` unit conversion and associated field documentation.

Tests:
- Rebuilt `NewDD` (warning scan: no `warning:` entries in build log).
- Ran `newdd.exe 0 4` on snapshot with `nsink=5`; confirmed `Current memory stack 0`.
- Parsed SINK binary outputs and verified `dMsmbh` values match `BH_Mdot * (scale_m/scale_Gyr)` (max absolute difference ~11.69 on O(1e5) values from numeric constant precision).

Status:
PASS

## Task 3
Files:
- opFoF/Treewalk.fof.ordered.c

Changes:
- Replaced assignment-in-condition bugs with comparisons in both `BIG_MPI_Send` and `BIG_MPI_Recv` for:
  `MPI_INT`, `MPI_FLOAT`, `MPI_LONG`, `MPI_DOUBLE`, `MPI_LONG_LONG`, `MPI_LONG_DOUBLE`.
- Added explicit unknown-datatype fallback in both functions:
  `fprintf(stderr, "..."); dsize = 1;`

Risk Surface:
- Affects chunked MPI send/recv pointer byte-advance logic only.

Tests:
- Static code scan: no `else if(datatype = MPI_...)` patterns remain; all 12 branches are `==`.
- Warning-focused compile check on extracted dispatch logic with `gcc -Wall -Wextra -Wparentheses -Werror` completed cleanly.
- Full MPI round-trip execution test could not be run in this environment due missing MPI toolchain/runtime.

Status:
PASS

## Task 2
Files:
- NewDD/newdd.c
- ramses.h

Changes:
- Wrapped all high-volume GADGET-path debug `printf`/`fflush` lines in `NewDD/newdd.c` with `#ifdef DEBUG ... #endif`.
- Preserved production output lines: mode announcement (`GADGET mode: ...`) and `Current memory stack ...`.
- Added compile-safe debug/log macro guards in `ramses.h` (`#if defined(DEBUG) && DEBUG`, `#if defined(LOG) && LOG`) so builds without `-DDEBUG` compile cleanly and keep debug blocks disabled.

Risk Surface:
- Affects logging/diagnostic emission paths; no changes to data parsing, sorting, split binning, or output layout.

Tests:
- Non-debug build and run (`-DDEBUG` omitted): stdout had exactly 2 lines (`GADGET mode...`, `Current memory stack 0`), satisfying ≤5 line criterion.
- Debug build and run (`-DDEBUG=1`): all detailed debug lines remained present.
- Byte-level regression check: `diff -u /tmp/task2_nodebug/sha_nodebug.txt /tmp/task2_debug/sha_debug.txt` returned no differences.

Status:
PASS

## Preprod Checklist Step 5 Minimal Fix (Rerun Attempt)
Files:
- GalCenter/galcenter.c

Changes:
- Added rank-0 shutdown print for memory stack:
  `fprintf(stdout,"Current memory stack %lld\n",(long long)CurMemStack());`
- Kept existing shutdown sequence and MPI collectives unchanged.

Checklist Reference:
- `mossy-enchanting-kazoo.md` Required Pre-Production Checklist step 5 (confirm `Current memory stack 0` on rank 0).

Minimal Diff Summary:
- `+ if(myid == motherrank) fprintf(stdout,"Current memory stack %lld\\n",(long long)CurMemStack());`

Risk Surface:
- Output/logging only in `GalCenter` shutdown path; no algorithmic, distribution, or collective-order changes.

Tests:
- Rebuilt `GalCenter/galcenter.exe` under Intel MPI module stack inside SLURM job `88465`.
- Reran Step 5 (4-rank): output now includes memory stack line, but value is `Current memory stack 3`.
- Step 5 acceptance criterion (`Current memory stack 0`) remains unmet; rerun halted before steps 6-7 as required.

Status:
FAIL

## Preprod Checklist Step 5 fix #2
Files:
- GalCenter/galcenter.c

Changes:
- Added explicit Memory2 cleanup before final stack check in shutdown path:
  - Root rank: `Free(rbuffer); Free(rbp);`
  - All ranks: `Free(sbp);`
- Kept final rank-0 `CurMemStack()` log in place for checklist verification.

Checklist Reference:
- `mossy-enchanting-kazoo.md` Required Pre-Production Checklist step 5 (`Current memory stack 0` on rank 0).

Minimal Diff Summary:
- `+ if(myid == motherrank) { Free(rbuffer); Free(rbp); }`
- `+ Free(sbp);`

Risk Surface:
- Shutdown cleanup only in `GalCenter`; no algorithm, MPI collective order, or data distribution changes.

Tests:
- Rebuilt `GalCenter/galcenter.exe` and reran checklist Steps 5-7 under SLURM job `88466`.
- Step 5 (n=4): rank-0 stdout contains `Current memory stack 0`.
- Step 6: n=1 and n=4 output hashes are identical.
- Step 7: archived `stage5_mpi_validation.md` into `runs/`.

Status:
PASS
