# STAGE3_4_CHANGELOG

## Entry 1 (Chronological): DD Verification + Planning Checkpoints
### 1. Change Summary
File modified: None
Lines changed: None
Type: Runtime config
Stage affected: Both
Reason for change: DD verification and staged planning were executed before Stage 3-4 edits.

### 2. Before -> After Diff (Human-readable)
- No code modifications performed in this stage.
+ No code modifications performed in this stage.

### 3. Why This Change Was Necessary
What was broken or incompatible?
- No code issue addressed in this step.
What would happen without this change?
- Pipeline risk would remain unverified before Stage 3-4 takeover.
Is this a temporary workaround or permanent fix?
- Not a code fix.

### 4. Validation Performed
How was it tested?
- Ran NewDD verification twice and compared output hashes.
On which snapshot?
- `snap_0000` (`zoom<null>` representative path via `test_swift/snap_0000.hdf5`).
With which core count?
- 4 cores, `nsplit=4`.
What outputs confirmed correctness?
- `test_swift/dd_run1.log`, `test_swift/dd_run2.log`, `test_swift/dd_run1.sha`, `test_swift/dd_run2.sha`.
- Exact output/hash match and `Current memory stack 0`.

### 5. Side Effects / Risks
Does this affect reproducibility?
- No.
Does this affect memory usage?
- No.
Does this change output formats?
- No.
Could it break other stages?
- No.

### 6. Reversion Instructions
- No reversion needed.

---

## Entry 2 (Chronological): NewGalFinder NCHEM/NDUST Compatibility
### 1. Change Summary
File modified: `../NewGalFinder/Makefile`
Lines changed: ~29
Type: Build config
Stage affected: Galaxy
Reason for change: `gfind` struct layout must match `opFoF` output. Previous build used `-DNCHEM=3` and omitted `-DNDUST=4`.

### 2. Before -> After Diff (Human-readable)
- `OPT = ... -DREAD_SINK -DNCHEM=3 -DADV`
+ `OPT = ... -DREAD_SINK -DNCHEM=9 -DNDUST=4 -DADV`

### 3. Why This Change Was Necessary
What was broken or incompatible?
- Binary I/O struct mismatch between Stage 3 and Stage 4 particle records.
What would happen without this change?
- Corrupt reads / undefined behavior in `gfind`.
Is this a temporary workaround or permanent fix?
- Permanent compatibility fix.

### 4. Validation Performed
How was it tested?
- Rebuilt `gfind.exe`; executed smoke run after `opFoF` output generation.
On which snapshot?
- `snap_0000`.
With which core count?
- 4 cores.
What outputs confirmed correctness?
- Build success: `/home/bkoh/pgalf/NewGalFinder/gfind.exe`.
- Runtime reached galaxy-finding logic and wrote expected output filenames (`GALCATALOG.LIST.00000`, `GALFIND.DATA.00000`), though run later failed due separate cosmology/runtime issues.

### 5. Side Effects / Risks
Does this affect reproducibility?
- Improves cross-stage determinism by aligning struct definitions.
Does this affect memory usage?
- Slight increase due larger per-particle structs.
Does this change output formats?
- No format redesign; enforces correct expected format.
Could it break other stages?
- Could break if another tool still compiles against old `NCHEM=3` assumptions.

### 6. Reversion Instructions
- Edit `../NewGalFinder/Makefile` and restore `-DNCHEM=3` and remove `-DNDUST=4`.
- Rebuild: `cd ../NewGalFinder && make clean && make all`.

---

## Entry 3 (Chronological): opFoF Intel Wrapper Modernization
### 1. Change Summary
File modified: `../opFoF/Rules.make`
Lines changed: ~18-20
Type: Build config
Stage affected: Halo
Reason for change: Align Stage 3 compiler wrappers with current Intel oneAPI toolchain used by other components.

### 2. Before -> After Diff (Human-readable)
- `FC = mpiifc`
+ `FC = mpiifx`
- `CC = mpiicc`
+ `CC = mpiicx`
- `F90C = mpiifort`
+ `F90C = mpiifx`

### 3. Why This Change Was Necessary
What was broken or incompatible?
- Legacy wrappers are deprecated and inconsistent with active environment.
What would happen without this change?
- Build/toolchain inconsistencies and harder maintenance.
Is this a temporary workaround or permanent fix?
- Intended as permanent modernization.

### 4. Validation Performed
How was it tested?
- Rebuilt `opfof.exe` and ran snapshot smoke test.
On which snapshot?
- `snap_0000`.
With which core count?
- 4 cores.
What outputs confirmed correctness?
- Build success after compatibility fixes; runtime produced `FoF_halo_cat.00000` and `FoF_member_particle.00000`.

### 5. Side Effects / Risks
Does this affect reproducibility?
- Potentially compiler-dependent floating-point differences.
Does this affect memory usage?
- No direct change.
Does this change output formats?
- No.
Could it break other stages?
- Could expose latent C compatibility issues (observed and fixed separately below).

### 6. Reversion Instructions
- Restore wrappers in `../opFoF/Rules.make` to `mpiifc/mpiicc/mpiifort`.
- Rebuild: `cd ../opFoF && make clean && make all`.

---

## Entry 4 (Chronological): opFoF Missing boolean Enum Definition
### 1. Change Summary
File modified: `../opFoF/fof.h`
Lines changed: ~18
Type: Code logic
Stage affected: Halo
Reason for change: `mpiicx` compile failed because `enum boolean`/`NO`/`YES` used by code was not actually defined (commented-out block).

### 2. Before -> After Diff (Human-readable)
- `/* enum boolean {YES=01, NO=02}; ... */`
+ `enum boolean {NO=0, YES=1};`

### 3. Why This Change Was Necessary
What was broken or incompatible?
- Compile errors: incomplete enum type and undeclared `NO` symbol.
What would happen without this change?
- `opfof.exe` cannot compile with modern wrapper path.
Is this a temporary workaround or permanent fix?
- Permanent correctness fix for header consistency.

### 4. Validation Performed
How was it tested?
- Recompiled `opFoF` after patch.
On which snapshot?
- `snap_0000` smoke run.
With which core count?
- 4 cores.
What outputs confirmed correctness?
- Compilation progressed beyond prior enum errors and produced executable.

### 5. Side Effects / Risks
Does this affect reproducibility?
- No expected impact.
Does this affect memory usage?
- No.
Does this change output formats?
- No.
Could it break other stages?
- Low risk; uses already-referenced symbols.

### 6. Reversion Instructions
- Re-comment/remove the enum line in `../opFoF/fof.h`.
- Rebuild `opFoF`.

---

## Entry 5 (Chronological): opFoF Link Compatibility (`-fcommon`)
### 1. Change Summary
File modified: `../opFoF/Rules.make`
Lines changed: ~22 (`OPT`)
Type: Build config
Stage affected: Halo
Reason for change: Link failed with multiple global definitions (legacy common-symbol pattern) under modern toolchain defaults.

### 2. Before -> After Diff (Human-readable)
- `OPT = -DINTEL -g`
+ `OPT = -DINTEL -g -fcommon`

### 3. Why This Change Was Necessary
What was broken or incompatible?
- Link error: multiple definition of `simpar`, `cputime0`, `cputime1`.
What would happen without this change?
- `opfof.exe` link fails; Stage 3 blocked.
Is this a temporary workaround or permanent fix?
- Compatibility workaround for legacy global definitions; medium-term preferred fix is explicit `extern` hygiene in source.

### 4. Validation Performed
How was it tested?
- Rebuilt `opFoF`; ran Stage 3 smoke test.
On which snapshot?
- `snap_0000`.
With which core count?
- 4 cores.
What outputs confirmed correctness?
- Link succeeded; runtime produced `FoF_halo_cat.00000` and `FoF_member_particle.00000`.

### 5. Side Effects / Risks
Does this affect reproducibility?
- No expected numerical change.
Does this affect memory usage?
- No.
Does this change output formats?
- No.
Could it break other stages?
- Low; confined to `opFoF` build behavior.

### 6. Reversion Instructions
- Remove `-fcommon` from `../opFoF/Rules.make`.
- Rebuild `opFoF`; expect link failure unless global symbol declarations are refactored.

---

## Entry 6 (Chronological): opFoF Cosmology Enforcement (Required Simulation Values)
### 1. Change Summary
File modified: `../opFoF/opfof.c`
Lines changed: ~24-37, ~483
Type: Cosmology handling
Stage affected: Halo
Reason for change: Input `.info` currently provides zero cosmology values for SWIFT snapshots, causing invalid FoF link-scale behavior (`omep=0`, `link02=inf`).

### 2. Before -> After Diff (Human-readable)
- `hubble = simpar.H0; omegp = simpar.omega_m; ...` (using raw header values)
+ Added `apply_required_cosmology(&simpar)` before deriving FoF physics values.
+ Enforced: `Omega_m=0.3`, `Omega_b=0.049`, `Omega_L=0.7`, `H0=67.11`.

### 3. Why This Change Was Necessary
What was broken or incompatible?
- Stage 3 read zero cosmology from upstream header info and propagated nonphysical values.
What would happen without this change?
- Invalid linking lengths and unstable downstream Stage 4 behavior.
Is this a temporary workaround or permanent fix?
- Permanent for this simulation family requiring fixed cosmology.

### 4. Validation Performed
How was it tested?
- Rebuild + rerun Stage 3 smoke on snapshot 0000.
On which snapshot?
- `snap_0000`.
With which core count?
- 4 cores.
What outputs confirmed correctness?
- Stage 3 completed with FoF outputs and no `link02=inf` regressions expected after override path.

### 5. Side Effects / Risks
Does this affect reproducibility?
- Improves reproducibility across snapshots with malformed header cosmology.
Does this affect memory usage?
- No.
Does this change output formats?
- No format change; header scalar values are corrected.
Could it break other stages?
- Only if a different cosmology is intentionally required in the same binary.

### 6. Reversion Instructions
- Remove `apply_required_cosmology()` and related `REQUIRED_*` macros in `../opFoF/opfof.c`.
- Rebuild `opFoF`.

---

## Entry 7 (Chronological): opFoF Missing Contact-File Robustness
### 1. Change Summary
File modified: `../opFoF/Treewalk.fof.ordered.c`
Lines changed: ~555-566
Type: Code logic
Stage affected: Halo
Reason for change: Stage 3 aborted when `BottomFaceContactHalo.*.dat` files were absent.

### 2. Before -> After Diff (Human-readable)
- `if(fopen(...) == NULL) { fprintf(...); exit(0); }`
+ Try open `rb`; if missing, create empty file and reopen.
+ If still failing, return safely instead of hard process exit.

### 3. Why This Change Was Necessary
What was broken or incompatible?
- Runtime assumed contact files always pre-exist.
What would happen without this change?
- `opFoF` crashes on valid early snapshots with empty/no contact boundary data.
Is this a temporary workaround or permanent fix?
- Permanent runtime robustness fix.

### 4. Validation Performed
How was it tested?
- Reran Stage 3 smoke where missing-file abort was previously observed.
On which snapshot?
- `snap_0000`.
With which core count?
- 4 cores.
What outputs confirmed correctness?
- No abort at contact-file read step; FoF outputs produced.

### 5. Side Effects / Risks
Does this affect reproducibility?
- No numerical impact expected.
Does this affect memory usage?
- Negligible.
Does this change output formats?
- No.
Could it break other stages?
- Low risk; only affects error path behavior.

### 6. Reversion Instructions
- Restore original `fopen(..., "r")` + `exit(0)` block in `../opFoF/Treewalk.fof.ordered.c`.
- Rebuild `opFoF`.

---

## Entry 8 (Chronological): gfind Cosmology Enforcement + Safe Omega Guard
### 1. Change Summary
File modified: `../NewGalFinder/gfind.c`
Lines changed: ~25-31, ~52/65/78/91, ~238-244
Type: Cosmology handling
Stage affected: Galaxy
Reason for change: Stage 4 inherited invalid cosmology from FoF header and computed link scales with `omep=0`, destabilizing galaxy finding.

### 2. Before -> After Diff (Human-readable)
- `p->link02 = ... /omep ...`
+ `const float omega_safe = (omep > 0 ? omep : 0.3)`
+ `p->link02 = ... /omega_safe ...`
- Raw read values used for `omep/omepb/omeplam/hubble`
+ Overrode to required values after header read (`0.3/0.049/0.7/67.11`).

### 3. Why This Change Was Necessary
What was broken or incompatible?
- `omep=0` propagated into link-length and physical conversions.
What would happen without this change?
- High risk of NaN/Inf physics and segmentation failures during subhalo finding.
Is this a temporary workaround or permanent fix?
- Permanent for required simulation cosmology.

### 4. Validation Performed
How was it tested?
- Rebuild + Stage 4 smoke rerun on Stage 3 output.
On which snapshot?
- `snap_0000`.
With which core count?
- 4 cores.
What outputs confirmed correctness?
- Log now reflects corrected cosmology path; no invalid-omega link calculation path remains.

### 5. Side Effects / Risks
Does this affect reproducibility?
- Improves consistency where upstream headers are incorrect.
Does this affect memory usage?
- No.
Does this change output formats?
- No.
Could it break other stages?
- Only for workflows expecting dynamic cosmology from file headers.

### 6. Reversion Instructions
- Remove `REQUIRED_*` macros and `omega_safe` guard from `../NewGalFinder/gfind.c`.
- Restore raw-header assignment behavior.
- Rebuild `gfind`.

---

## Entry 9 (Chronological): galcenter Cosmology Enforcement
### 1. Change Summary
File modified: `../GalCenter/galcenter.c`
Lines changed: ~20-23, ~216-219
Type: Cosmology handling
Stage affected: Galaxy
Reason for change: Ensure Stage 4 center-refinement stage uses the same required cosmology constants as Stage 3 and gfind.

### 2. Before -> After Diff (Human-readable)
- Header-read values passed through directly.
+ Added required constants (`Omega_m=0.3`, `Omega_b=0.049`, `Omega_L=0.7`, `H0=67.11`) and override after read.

### 3. Why This Change Was Necessary
What was broken or incompatible?
- Cross-stage cosmology inconsistency risk.
What would happen without this change?
- Potential mismatch between galaxy identification and center refinement unit conversions.
Is this a temporary workaround or permanent fix?
- Permanent consistency fix.

### 4. Validation Performed
How was it tested?
- Rebuild + smoke execution after gfind output generation.
On which snapshot?
- `snap_0000`.
With which core count?
- 4 cores.
What outputs confirmed correctness?
- `galcenter` runs successfully and writes `GALFIND.CENTER.00000`.

### 5. Side Effects / Risks
Does this affect reproducibility?
- Improves deterministic consistency across Stage 4 sub-steps.
Does this affect memory usage?
- No.
Does this change output formats?
- No.
Could it break other stages?
- Low risk; scoped to in-memory cosmology parameters.

### 6. Reversion Instructions
- Remove `REQUIRED_*` macros and override block from `../GalCenter/galcenter.c`.
- Rebuild `galcenter`.

---

## Entry 10 (Chronological): gfind Zero-Core Return Fix (Crash Prevention)
### 1. Change Summary
File modified: `../NewGalFinder/subhaloden.mod6.c`
Lines changed: ~2929-2939
Type: Code logic
Stage affected: Galaxy
Reason for change: `subhalo_den()` returned `1` when `numcore==0`, incorrectly signaling one detected subhalo and causing downstream instability/segfault in `gfind` output handling.

### 2. Before -> After Diff (Human-readable)
- `for(i=0;i<np;i++) p2halo[i] = 0;`
- `return 1;`
+ `for(i=0;i<np;i++) p2halo[i] = NOT_HALO_MEMBER;`
+ `return 0;`

### 3. Why This Change Was Necessary
What was broken or incompatible?
- Logical inconsistency: zero cores was reported as one subhalo.
What would happen without this change?
- `gfind` could crash on halos with no density peaks.
Is this a temporary workaround or permanent fix?
- Permanent correctness fix for zero-core branch semantics.

### 4. Validation Performed
How was it tested?
- Rebuilt `gfind` and reran Stage 4 on snapshot 0274.
On which snapshot?
- `snap_0274`.
With which core count?
- 4 cores.
What outputs confirmed correctness?
- `gfind` completed without segmentation fault on previously failing input path.

### 5. Side Effects / Risks
Does this affect reproducibility?
- Improves determinism by removing invalid branch behavior.
Does this affect memory usage?
- No meaningful impact.
Does this change output formats?
- No format change; only subhalo count semantics in zero-core case.
Could it break other stages?
- Low risk; downstream now receives consistent zero-subhalo signals.

### 6. Reversion Instructions
- In `../NewGalFinder/subhaloden.mod6.c`, restore zero-core block to previous `return 1` behavior.
- Rebuild `gfind`.

---

## Entry 11 (Chronological): gfind Master-Path Defensive Guards (Rank-0 Crash Hardening)
### 1. Change Summary
File modified: `../NewGalFinder/gfind.c`
Lines changed: ~440, ~478, ~707-709
Type: Code logic
Stage affected: Galaxy
Reason for change: Persistent rank-0 segmentation fault during master receive/write stage required defensive validation of received `mpeak` and write preconditions.

### 2. Before -> After Diff (Human-readable)
- `MPI_Recv(&mpeak,...)` followed by `write_data(...)` directly.
+ Clamp invalid values: `if(mpeak < 0 || mpeak > rnp) mpeak = 0;`
- `write_data(...)` had no precondition checks.
+ Added guards:
+ `if(rrnp <= 0 || ssbp == NULL || rptl2halonum == NULL) return;`
+ `if(wrp == NULL || wbp == NULL || wlist == NULL) return;`
+ `if(mpeak < 0 || mpeak > rrnp) mpeak = 0;`

### 3. Why This Change Was Necessary
What was broken or incompatible?
- Master path could consume invalid `mpeak` values and proceed into unsafe write logic.
What would happen without this change?
- Rank-0 crashes terminate all MPI ranks and block Stage 4 reproducibility tests.
Is this a temporary workaround or permanent fix?
- Defensive hardening; may remain permanent, but deeper root-cause cleanup is still desirable.

### 4. Validation Performed
How was it tested?
- Rebuild `gfind`, rerun snapshot-0274 Stage 4 (`4/4` and `8/8`).
On which snapshot?
- `snap_0274`.
With which core count?
- 4 and 8 cores.
What outputs confirmed correctness?
- Verified absence/presence of segmentation in updated logs and artifact creation.

### 5. Side Effects / Risks
Does this affect reproducibility?
- Should only affect invalid-message edge cases; normal cases unchanged.
Does this affect memory usage?
- No.
Does this change output formats?
- No.
Could it break other stages?
- Low risk; localized to Stage 4 master write path.

### 6. Reversion Instructions
- Remove new `mpeak` clamps and write precondition guards in `../NewGalFinder/gfind.c`.
- Rebuild `gfind`.

---

## Entry 12 (Chronological): HaloQ Binary Layout Compatibility (opFoF -> gfind)
### 1. Change Summary
File modified: `../NewGalFinder/tree.h`
Lines changed: ~58
Type: Code logic
Stage affected: Both
Reason for change: `HaloQ` struct in NewGalFinder used `float` halo mass fields while opFoF writes `double` fields, causing binary read misalignment and rank-0 crashes in gfind.

### 2. Before -> After Diff (Human-readable)
- `float mass, mstar, mgas, mdm, msink;`
+ `double mass, mstar, mgas, mdm, msink;`

### 3. Why This Change Was Necessary
What was broken or incompatible?
- Stage 4 read Stage 3 halo catalog with wrong struct layout.
What would happen without this change?
- Misparsed halo records, corrupted stream offsets, and segmentation faults.
Is this a temporary workaround or permanent fix?
- Permanent binary-compatibility fix.

### 4. Validation Performed
How was it tested?
- Rebuilt `gfind`; reran Stage 4 on snapshot-0274 (`4/4` and `8/8`).
On which snapshot?
- `snap_0274`.
With which core count?
- 4 and 8 cores.
What outputs confirmed correctness?
- `gfind` no longer fails immediately after first halo due struct desync.

### 5. Side Effects / Risks
Does this affect reproducibility?
- Improves reproducibility by enforcing correct binary interpretation.
Does this affect memory usage?
- Slight increase in in-memory HaloQ footprint.
Does this change output formats?
- No writer-format change; reader now matches existing writer format.
Could it break other stages?
- Low risk; aligns with opFoF canonical format.

### 6. Reversion Instructions
- In `../NewGalFinder/tree.h`, revert HaloQ mass fields to `float`.
- Rebuild `gfind` (not recommended unless writer format also changed).

---

## Entry 13 (Chronological): gfind/galcenter Quality Diagnostics (No Code Change)
### 1. Change Summary
File modified: None
Lines changed: None
Type: Runtime config
Stage affected: Galaxy
Reason for change: Measured whether sentinel centers come from galcenter failure or expected behavior given gfind subhalo composition.

### 2. Before -> After Diff (Human-readable)
- No code modifications performed in this stage.
+ No code modifications performed in this stage.

### 3. Why This Change Was Necessary
What was broken or incompatible?
- Needed to verify if sentinel centers indicated algorithm failure vs insufficient stellar content.
What would happen without this change?
- Risk of misdiagnosing galcenter as primary failure source.
Is this a temporary workaround or permanent fix?
- Diagnostic only.

### 4. Validation Performed
How was it tested?
- Parsed `GALCATALOG.LIST.00274` and `GALFIND.CENTER.00274` for both `4/4` and `8/8` runs.
On which snapshot?
- `snap_0274`.
With which core count?
- 4 and 8 cores.
What outputs confirmed correctness?
- In both configs, valid centers appear only for subhalos with `npstar > 0`; all `npstar=0` cases map to sentinel center values.

### 5. Side Effects / Risks
Does this affect reproducibility?
- No code-side effect.
Does this affect memory usage?
- No.
Does this change output formats?
- No.
Could it break other stages?
- No.

### 6. Reversion Instructions
- No reversion needed.

---

## Entry 14 (Chronological): Stage 3-4 Statistics Document (Snapshot 0274)
### 1. Change Summary
File modified: `STAGE3_4_STATS.md`
Lines changed: New file (all lines)
Type: Directory structure
Stage affected: Both
Reason for change: User-requested consolidated statistics report for FoF star occupancy and galcenter validity, plus Stage 4 star-retention diagnostics.

### 2. Before -> After Diff (Human-readable)
- (no file)
+ `STAGE3_4_STATS.md` created with:
+ snapshot star inventory (`N_star_total=9514`),
+ stars in/out of FoF halos (`9463/51`),
+ halo concentration statistics,
+ valid center counts (`gal/dmhalo/gas`),
+ Stage 4 retention audit (`530 -> 341` stars across gfind outputs).

### 3. Why This Change Was Necessary
What was broken or incompatible?
- No single, auditable document existed for the current Stage 3-4 quantitative status.
What would happen without this change?
- Harder to review pipeline status and diagnose why galcenter outputs many sentinel galaxy centers.
Is this a temporary workaround or permanent fix?
- Permanent documentation artifact.

### 4. Validation Performed
How was it tested?
- Recomputed all reported metrics from current artifacts:
  - `snap_0274.hdf5`
  - `FoF_halo_cat.00274`
  - `GALCATALOG.LIST.00274`
  - `GALFIND.CENTER.00274`
- Cross-checked both `4x4` and `8x8` runs.
On which snapshot?
- `snap_0274` (zoomnull).
With which core count?
- 4 and 8 cores.
What outputs confirmed correctness?
- Matching totals and consistency checks reported in `STAGE3_4_STATS.md`.

### 5. Side Effects / Risks
Does this affect reproducibility?
- No runtime effect; documentation only.
Does this affect memory usage?
- No.
Does this change output formats?
- No.
Could it break other stages?
- No.

### 6. Reversion Instructions
- Remove `STAGE3_4_STATS.md`.
- Remove this entry from `STAGE3_4_CHANGELOG.md` if full rollback is required.

---

## Entry 15 (Chronological): Stage 4 Peak-Threshold Sensitivity Sweep (Reverted)
### 1. Change Summary
File modified: `STAGE3_4_STATS.md`
Lines changed: Appended sensitivity-sweep section at end of file
Type: Runtime config
Stage affected: Galaxy
Reason for change: Validate whether low valid-galaxy-center counts were caused by an overly high active Stage 4 peak threshold.

### 2. Before -> After Diff (Human-readable)
- (no sensitivity-sweep section)
+ Added a section documenting temporary `PEAKTHRESHOLD 500 -> 200` test on snapshot `0274` (`4x4`) and resulting null impact.
+ Added explicit conclusion that this single-threshold change does not resolve the dominant loss mode.

### 3. Why This Change Was Necessary
What was broken or incompatible?
- Stage 4 outputs had sparse star-bearing subhalos and many sentinel galaxy centers.
What would happen without this change?
- We would lack evidence on whether threshold sensitivity alone explains the behavior.
Is this a temporary workaround or permanent fix?
- Diagnostic experiment only; temporary code tweak was reverted.
- No code modifications performed in this stage.

### 4. Validation Performed
How was it tested?
- Temporarily lowered `subhaloden` peak threshold, rebuilt `gfind`, and reran Stage 4 in isolated directory `cmp_zoomnull_s0274_4x4_pth200`.
- Compared `GALCATALOG.LIST` and `GALFIND.CENTER` against baseline `4x4` outputs.
On which snapshot?
- `snap_0274` (zoomnull).
With which core count?
- 4 cores.
What outputs confirmed correctness?
- Metrics were unchanged: halos `30`, subs `30`, `halo_nstar=530`, `sub_npstar=341`, valid `gal centers=3`.

### 5. Side Effects / Risks
Does this affect reproducibility?
- No persistent impact; code was reverted.
Does this affect memory usage?
- No persistent impact.
Does this change output formats?
- No.
Could it break other stages?
- No persistent risk after reversion.

### 6. Reversion Instructions
- Already reverted.
- If needed, remove the isolated test directory `cmp_zoomnull_s0274_4x4_pth200`.

---

## Entry 16 (Chronological): gfind Rejection-Reason Instrumentation in subhalo_den
### 1. Change Summary
File modified: `../NewGalFinder/subhaloden.mod6.c`
Lines changed: ~2867-2942
Type: Code logic
Stage affected: Galaxy
Reason for change: Add explicit per-halo diagnostics to identify why halos produce `numcore=0` and therefore do not contribute star-bearing subhalos/valid galaxy centers.

### 2. Before -> After Diff (Human-readable)
- `if(findstarnum(bp,np)<= NUMNEIGHBOR || findstarmass(bp,np)<MINSTELLARMASS){`
+ `star_num = findstarnum(bp,np);`
+ `star_mass = findstarmass(bp,np);`
+ `use_all_path = (star_num <= NUMNEIGHBOR || star_mass < MINSTELLARMASS);`
+ `if(use_all_path){`

- (no per-halo summary log)
+ `LOGPRINT("SUBHALO_DIAG np=%lld star_num=%d star_mass=%g branch=%s numcore=%d\\n", ...)`

- `if(numcore == 0) { ... return 0; }`
+ `if(numcore == 0) {`
+ `  LOGPRINT("SUBHALO_ZERO_CORE np=%lld star_num=%d star_mass=%g low_star_num=%d low_star_mass=%d branch=%s\\n", ...)`
+ `  ... return 0;`
+ `}`

### 3. Why This Change Was Necessary
What was broken or incompatible?
- Stage 4 behavior was opaque: many halos returned zero cores with no direct reason trace.
What would happen without this change?
- Root-cause analysis of star-loss path would remain speculative.
Is this a temporary workaround or permanent fix?
- Diagnostic instrumentation; can remain for debugging or be removed later once pipeline tuning stabilizes.

### 4. Validation Performed
How was it tested?
- Rebuilt `gfind` with instrumentation and reran snapshot `0274` in isolated run dir:
  - `cmp_zoomnull_s0274_4x4_diag`
- Parsed `SUBHALO_DIAG` and `SUBHALO_ZERO_CORE` lines from `run_gfind_0274_4x4_diag.log`.
On which snapshot?
- `snap_0274` (zoomnull).
With which core count?
- 4 cores.
What outputs confirmed correctness?
- New diagnostic tags were emitted and aggregated successfully.
- Baseline science outputs (`GALFIND.CENTER` validity counts) remained unchanged.

### 5. Side Effects / Risks
Does this affect reproducibility?
- Numerical outputs unchanged in validation run; only additional logs emitted.
Does this affect memory usage?
- Negligible.
Does this change output formats?
- No file-format change; log stream only.
Could it break other stages?
- Low risk; local to Stage 4 diagnostic logging.

### 6. Reversion Instructions
- Remove added `star_num/star_mass/use_all_path` vars and both `LOGPRINT` calls from `../NewGalFinder/subhaloden.mod6.c`.
- Rebuild `gfind`.

---

## Entry 17 (Chronological): Stage-4 Rejection Diagnostic Report Update
### 1. Change Summary
File modified: `STAGE3_4_STATS.md`
Lines changed: Appended "Focused Stage-4 Rejection Diagnostics" section
Type: Runtime config
Stage affected: Galaxy
Reason for change: Record quantified rejection-path evidence from instrumented run and connect it to sparse valid galaxy centers.

### 2. Before -> After Diff (Human-readable)
- (no focused rejection diagnostic section)
+ Added counts for total halos processed, zero-core fraction, branch breakdown, and low-star-number reason breakdown.
+ Added baseline-vs-instrumented output consistency check.

### 3. Why This Change Was Necessary
What was broken or incompatible?
- Prior report lacked direct per-halo rejection accounting.
What would happen without this change?
- Inability to prioritize the correct next tuning target in Stage 4.
Is this a temporary workaround or permanent fix?
- Permanent diagnostic documentation.

### 4. Validation Performed
How was it tested?
- Parsed instrumented log tags and cross-checked against output artifacts.
On which snapshot?
- `snap_0274` (zoomnull).
With which core count?
- 4 cores.
What outputs confirmed correctness?
- `SUBHALO_DIAG`/`SUBHALO_ZERO_CORE` aggregates and unchanged center-validity counts.

### 5. Side Effects / Risks
Does this affect reproducibility?
- No runtime change from documentation update.
Does this affect memory usage?
- No.
Does this change output formats?
- No.
Could it break other stages?
- No.

### 6. Reversion Instructions
- Remove the appended diagnostic section from `STAGE3_4_STATS.md`.
- Remove this changelog entry if fully rolling back docs.

---

## Entry 18 (Chronological): Temporary NUMNEIGHBOR Sweep Configuration
### 1. Change Summary
File modified: `../NewGalFinder/params.h`
Lines changed: ~37
Type: Runtime config
Stage affected: Galaxy
Reason for change: Controlled sensitivity test to check whether reducing neighbor-count threshold increases Stage-4 core detection success.

### 2. Before -> After Diff (Human-readable)
- `#define NUMNEIGHBOR 32`
+ `#define NUMNEIGHBOR 16`

### 3. Why This Change Was Necessary
What was broken or incompatible?
- Needed to test whether `star_num <= NUMNEIGHBOR` gate was dominating zero-core outcomes.
What would happen without this change?
- No direct evidence on neighbor-threshold sensitivity.
Is this a temporary workaround or permanent fix?
- Temporary experimental change.

### 4. Validation Performed
How was it tested?
- Rebuilt `gfind`; reran instrumented snapshot `0274` `4x4` in isolated dir `cmp_zoomnull_s0274_4x4_diag_n16`.
On which snapshot?
- `snap_0274`.
With which core count?
- 4 cores.
What outputs confirmed correctness?
- `SUBHALO_DIAG`/`SUBHALO_ZERO_CORE` and center-validity metrics compared against baseline.

### 5. Side Effects / Risks
Does this affect reproducibility?
- Experimental run only; no persistent production impact after reversion.
Does this affect memory usage?
- No meaningful effect observed.
Does this change output formats?
- No.
Could it break other stages?
- Low risk in isolated test context.

### 6. Reversion Instructions
- Set `#define NUMNEIGHBOR` back to `32` in `../NewGalFinder/params.h`.
- Rebuild `gfind`.

---

## Entry 19 (Chronological): NUMNEIGHBOR Sweep Result Documentation
### 1. Change Summary
File modified: `STAGE3_4_STATS.md`
Lines changed: Appended "Controlled Sweep A" section
Type: Runtime config
Stage affected: Galaxy
Reason for change: Record outcome of controlled `NUMNEIGHBOR 32 -> 16` test.

### 2. Before -> After Diff (Human-readable)
- (no NUMNEIGHBOR sweep section)
+ Added `NUMNEIGHBOR` sweep setup and measured no-change result across rejection diagnostics and center validity.

### 3. Why This Change Was Necessary
What was broken or incompatible?
- Needed empirical test data for parameter sensitivity.
What would happen without this change?
- Tuning decisions would remain assumption-based.
Is this a temporary workaround or permanent fix?
- Permanent documentation of experiment.

### 4. Validation Performed
How was it tested?
- Parsed instrumented logs and compared output files to baseline.
On which snapshot?
- `snap_0274`.
With which core count?
- 4 cores.
What outputs confirmed correctness?
- Identical aggregate counts and center-validity metrics.

### 5. Side Effects / Risks
Does this affect reproducibility?
- Documentation only.
Does this affect memory usage?
- No.
Does this change output formats?
- No.
Could it break other stages?
- No.

### 6. Reversion Instructions
- Remove the appended section from `STAGE3_4_STATS.md` if desired.

---


## Entry 20 (Chronological): Restore NUMNEIGHBOR Baseline + Temporary MINSTELLARMASS Sweep Config
### 1. Change Summary
File modified: `../NewGalFinder/params.h`
Lines changed: ~37, ~60
Type: Runtime config
Stage affected: Galaxy
Reason for change: Transition from Sweep A to Sweep B by restoring `NUMNEIGHBOR` baseline and applying temporary `MINSTELLARMASS=0` for sensitivity test.

### 2. Before -> After Diff (Human-readable)
- `#define NUMNEIGHBOR 16`
+ `#define NUMNEIGHBOR 32`
- `#define MINSTELLARMASS 2.e6`
+ `#define MINSTELLARMASS 0`

### 3. Why This Change Was Necessary
What was broken or incompatible?
- Needed isolated test of stellar-mass gate effect while keeping neighbor threshold at baseline.
What would happen without this change?
- Could not separate mass-threshold sensitivity from neighbor-threshold effects.
Is this a temporary workaround or permanent fix?
- Temporary experimental change.

### 4. Validation Performed
How was it tested?
- Rebuilt `gfind`; reran instrumented snapshot `0274` `4x4` in `cmp_zoomnull_s0274_4x4_diag_mstar0`.
On which snapshot?
- `snap_0274`.
With which core count?
- 4 cores.
What outputs confirmed correctness?
- Instrumented logs collected and compared against baseline.

### 5. Side Effects / Risks
Does this affect reproducibility?
- Experimental run only; no persistent production impact after reversion.
Does this affect memory usage?
- No meaningful effect observed.
Does this change output formats?
- No.
Could it break other stages?
- Low risk in isolated test context.

### 6. Reversion Instructions
- Set `#define MINSTELLARMASS` back to `2.e6` in `../NewGalFinder/params.h`.
- Rebuild `gfind`.

---

## Entry 21 (Chronological): MINSTELLARMASS Sweep Result Documentation
### 1. Change Summary
File modified: `STAGE3_4_STATS.md`
Lines changed: Appended "Controlled Sweep B" section
Type: Runtime config
Stage affected: Galaxy
Reason for change: Record outcome of controlled `MINSTELLARMASS 2e6 -> 0` test.

### 2. Before -> After Diff (Human-readable)
- (no MINSTELLARMASS sweep section)
+ Added `MINSTELLARMASS` sweep setup and measured no-change result across rejection diagnostics and center validity.

### 3. Why This Change Was Necessary
What was broken or incompatible?
- Needed empirical test data for stellar-mass-threshold sensitivity.
What would happen without this change?
- Tuning decisions would remain incomplete.
Is this a temporary workaround or permanent fix?
- Permanent documentation of experiment.

### 4. Validation Performed
How was it tested?
- Parsed instrumented logs and compared output files to baseline.
On which snapshot?
- `snap_0274`.
With which core count?
- 4 cores.
What outputs confirmed correctness?
- Identical aggregate counts and center-validity metrics.

### 5. Side Effects / Risks
Does this affect reproducibility?
- Documentation only.
Does this affect memory usage?
- No.
Does this change output formats?
- No.
Could it break other stages?
- No.

### 6. Reversion Instructions
- Remove the appended section from `STAGE3_4_STATS.md` if desired.

---

## Entry 22 (Chronological): Restore MINSTELLARMASS Baseline
### 1. Change Summary
File modified: `../NewGalFinder/params.h`
Lines changed: ~60
Type: Runtime config
Stage affected: Galaxy
Reason for change: Return codebase to baseline Stage-4 runtime parameters after controlled sweeps.

### 2. Before -> After Diff (Human-readable)
- `#define MINSTELLARMASS 0`
+ `#define MINSTELLARMASS 2.e6`

### 3. Why This Change Was Necessary
What was broken or incompatible?
- Temporary experimental setting should not persist in baseline pipeline configuration.
What would happen without this change?
- Future runs would unintentionally use non-baseline mass threshold.
Is this a temporary workaround or permanent fix?
- Reversion to baseline.

### 4. Validation Performed
How was it tested?
- Rebuilt `gfind` after restoring baseline value.
On which snapshot?
- Build-level validation (no additional runtime rerun in this step).
With which core count?
- N/A (build step).
What outputs confirmed correctness?
- Successful `gfind` rebuild with baseline parameter definitions.

### 5. Side Effects / Risks
Does this affect reproducibility?
- Restores intended reproducible baseline configuration.
Does this affect memory usage?
- No.
Does this change output formats?
- No.
Could it break other stages?
- No known risk.

### 6. Reversion Instructions
- Set `#define MINSTELLARMASS` back to `0` only if repeating Sweep B.
- Rebuild `gfind`.

---

## Entry 23 (Chronological): finddenpeak-Level Candidate/Rejection Diagnostics
### 1. Change Summary
File modified: `../NewGalFinder/subhaloden.mod6.c`
Lines changed: ~416-501
Type: Code logic
Stage affected: Galaxy
Reason for change: Add direct counters in `finddenpeak()` to quantify whether cores fail due to no threshold-crossing candidates or due to local-maximum rejection.

### 2. Before -> After Diff (Human-readable)
- `int finddenpeak(...)` only returned `numcore` without summary counters.
+ Added per-call counters:
+ `n_above`, `n_localmax`, `n_reject_higher`.
+ Added log line:
+ `LOGPRINT("FINDDENPEAK_DIAG jflag=%d np=%d above=%lld localmax=%lld reject_higher=%lld numcore=%d\\n", ...)`

### 3. Why This Change Was Necessary
What was broken or incompatible?
- Prior diagnostics identified zero-core outcomes but not where candidate loss occurred inside peak finding.
What would happen without this change?
- Could not distinguish “no candidates above threshold” from “candidates rejected by higher-density neighbors”.
Is this a temporary workaround or permanent fix?
- Diagnostic instrumentation; intended for root-cause analysis.

### 4. Validation Performed
How was it tested?
- Rebuilt `gfind`; reran snapshot `0274` `4x4` in isolated directory `cmp_zoomnull_s0274_4x4_diag_peak`.
- Parsed `FINDDENPEAK_DIAG` with `SUBHALO_DIAG`.
On which snapshot?
- `snap_0274`.
With which core count?
- 4 cores.
What outputs confirmed correctness?
- Log counters emitted and aggregated; Stage-4 output metrics unchanged.

### 5. Side Effects / Risks
Does this affect reproducibility?
- Numerical outputs unchanged in validation run; only additional logging.
Does this affect memory usage?
- Negligible.
Does this change output formats?
- No file-format change.
Could it break other stages?
- Low risk; localized to logging and counters in Stage 4.

### 6. Reversion Instructions
- Remove added counters and `FINDDENPEAK_DIAG` `LOGPRINT` line from `../NewGalFinder/subhaloden.mod6.c`.
- Rebuild `gfind`.

---

## Entry 24 (Chronological): finddenpeak Diagnostic Report Update
### 1. Change Summary
File modified: `STAGE3_4_STATS.md`
Lines changed: Appended "Focused Stage-4 Rejection Diagnostics v2" section
Type: Runtime config
Stage affected: Galaxy
Reason for change: Record fine-grained rejection-stage evidence from `finddenpeak` counters.

### 2. Before -> After Diff (Human-readable)
- (no finddenpeak-level section)
+ Added paired diagnostics showing that all `TYPE_ALL` zero-core halos have `above=0` (no threshold-crossing particles).
+ Added note about 14 `TYPE_STAR` halos not entering `finddenpeak` in current branch path.

### 3. Why This Change Was Necessary
What was broken or incompatible?
- Root cause remained ambiguous between threshold gating and local-max rejection.
What would happen without this change?
- Next tuning step could target wrong mechanism.
Is this a temporary workaround or permanent fix?
- Permanent diagnostic documentation.

### 4. Validation Performed
How was it tested?
- Parsed instrumented logs and cross-checked center-file outputs against baseline diagnostic run.
On which snapshot?
- `snap_0274`.
With which core count?
- 4 cores.
What outputs confirmed correctness?
- Aggregated counters and unchanged `GALFIND.CENTER` validity counts.

### 5. Side Effects / Risks
Does this affect reproducibility?
- Documentation only.
Does this affect memory usage?
- No.
Does this change output formats?
- No.
Could it break other stages?
- No.

### 6. Reversion Instructions
- Remove appended v2 section from `STAGE3_4_STATS.md` if desired.

---

## Entry 25 (Chronological): Temporary TYPE_ALL Threshold Override in finddenpeak
### 1. Change Summary
File modified: `../NewGalFinder/subhaloden.mod6.c`
Lines changed: ~421, ~428, ~462, ~497
Type: Code logic
Stage affected: Galaxy
Reason for change: Controlled test to determine whether TYPE_ALL zero-core failures are primarily due to overly high threshold in `finddenpeak`.

### 2. Before -> After Diff (Human-readable)
- `float peak_thr = PEAKTHRESHOLD;`
+ `float peak_thr = (jflag == 0) ? 50.L : PEAKTHRESHOLD;`

- `LOGPRINT("FINDDENPEAK_DIAG ... np=%d ...")`
+ `LOGPRINT("FINDDENPEAK_DIAG ... np=%d peak_thr=%g ...")`

### 3. Why This Change Was Necessary
What was broken or incompatible?
- Needed direct recovery test after confirming zero-core TYPE_ALL halos had no particles above threshold.
What would happen without this change?
- Could not quantify whether threshold-only relaxation materially improves galaxy outputs.
Is this a temporary workaround or permanent fix?
- Temporary experimental change.

### 4. Validation Performed
How was it tested?
- Rebuilt `gfind`; reran snapshot `0274` `4x4` in isolated dir `cmp_zoomnull_s0274_4x4_diag_thr50`.
On which snapshot?
- `snap_0274`.
With which core count?
- 4 cores.
What outputs confirmed correctness?
- `SUBHALO_DIAG`, `SUBHALO_ZERO_CORE`, and `FINDDENPEAK_DIAG` metrics compared against baseline.

### 5. Side Effects / Risks
Does this affect reproducibility?
- Experimental behavior only; reverted after test.
Does this affect memory usage?
- Negligible.
Does this change output formats?
- No file-format change; additional log field only.
Could it break other stages?
- Low risk in isolated run context.

### 6. Reversion Instructions
- Replace conditional threshold expression with baseline `peak_thr = PEAKTHRESHOLD`.
- Rebuild `gfind`.

---

## Entry 26 (Chronological): TYPE_ALL Threshold Sweep Result Documentation
### 1. Change Summary
File modified: `STAGE3_4_STATS.md`
Lines changed: Appended "Controlled Sweep C" section
Type: Runtime config
Stage affected: Galaxy
Reason for change: Record measured impact of TYPE_ALL threshold override.

### 2. Before -> After Diff (Human-readable)
- (no TYPE_ALL threshold sweep section)
+ Added Sweep C with side-by-side metrics and interpretation.

### 3. Why This Change Was Necessary
What was broken or incompatible?
- Needed empirical evidence on whether TYPE_ALL threshold relaxation improves star-bearing galaxy recovery.
What would happen without this change?
- Risk of over-investing in a low-impact tuning direction.
Is this a temporary workaround or permanent fix?
- Permanent documentation of experiment.

### 4. Validation Performed
How was it tested?
- Compared isolated sweep outputs/logs against baseline instrumented run.
On which snapshot?
- `snap_0274`.
With which core count?
- 4 cores.
What outputs confirmed correctness?
- Small nonzero-core gain (+2 halos) but no increase in star-bearing subhalos or valid galaxy centers.

### 5. Side Effects / Risks
Does this affect reproducibility?
- Documentation only.
Does this affect memory usage?
- No.
Does this change output formats?
- No.
Could it break other stages?
- No.

### 6. Reversion Instructions
- Remove appended Sweep C section if desired.

---

## Entry 27 (Chronological): Restore Baseline finddenpeak Threshold
### 1. Change Summary
File modified: `../NewGalFinder/subhaloden.mod6.c`
Lines changed: ~421
Type: Code logic
Stage affected: Galaxy
Reason for change: Revert temporary experimental threshold override after completion of controlled test.

### 2. Before -> After Diff (Human-readable)
- `float peak_thr = (jflag == 0) ? 50.L : PEAKTHRESHOLD;`
+ `float peak_thr = PEAKTHRESHOLD;`

### 3. Why This Change Was Necessary
What was broken or incompatible?
- Experimental setting should not persist as baseline behavior.
What would happen without this change?
- Future runs would silently use altered TYPE_ALL sensitivity.
Is this a temporary workaround or permanent fix?
- Reversion to baseline.

### 4. Validation Performed
How was it tested?
- Rebuilt `gfind` after reversion.
On which snapshot?
- Build-level validation.
With which core count?
- N/A.
What outputs confirmed correctness?
- Successful `gfind` rebuild with reverted expression.

### 5. Side Effects / Risks
Does this affect reproducibility?
- Restores intended baseline reproducibility.
Does this affect memory usage?
- No.
Does this change output formats?
- No.
Could it break other stages?
- No known risk.

### 6. Reversion Instructions
- Re-apply conditional threshold only for repeating Sweep C.
- Rebuild `gfind`.

---

## Entry 28 (Chronological): Add Per-Snapshot Production Runner Script
### 1. Change Summary
File modified: `scripts/stage3_4_run_snapshot.sh`
Lines changed: New file (all lines)
Type: Runtime config
Stage affected: Both
Reason for change: Create a single, idempotent snapshot runner executing Stage 2–4 sequence in required order with standardized paths and per-stage logs.

### 2. Before -> After Diff (Human-readable)
- (no file)
+ Added `stage3_4_run_snapshot.sh` with:
+ dataset mapping (`zoomnull/2/3/4`),
+ symlink creation (`snap_NNNN.hdf5`),
+ directory bootstrapping,
+ stage-by-stage execution (`newdd -> opfof -> gfind -> galcenter`),
+ skip-if-output-exists behavior,
+ per-stage logs under `runs/<sim>/logs/<cores>x<nsplit>/`.

### 3. Why This Change Was Necessary
What was broken or incompatible?
- No standardized production driver existed for snapshot-by-snapshot execution.
What would happen without this change?
- Manual runs would be error-prone and difficult to resume.
Is this a temporary workaround or permanent fix?
- Permanent operational script.

### 4. Validation Performed
How was it tested?
- Executed preflight snapshot run (`zoomnull`, snapshot `0274`, `4x4`) end-to-end.
On which snapshot?
- `snap_0274`.
With which core count?
- 4 cores.
What outputs confirmed correctness?
- Successful completion message and expected Stage 3/4 artifacts in run directory.

### 5. Side Effects / Risks
Does this affect reproducibility?
- Improves reproducibility via standardized stage order and paths.
Does this affect memory usage?
- No direct change.
Does this change output formats?
- No.
Could it break other stages?
- Low risk; wrapper only.

### 6. Reversion Instructions
- Remove `scripts/stage3_4_run_snapshot.sh`.
- Revert to manual per-stage commands.

---

## Entry 29 (Chronological): Add SLURM Array Wrapper Script
### 1. Change Summary
File modified: `scripts/stage3_4_array.sbatch`
Lines changed: New file (all lines)
Type: MPI configuration
Stage affected: Both
Reason for change: Implement documented one-task-per-snapshot SLURM array strategy.

### 2. Before -> After Diff (Human-readable)
- (no file)
+ Added `stage3_4_array.sbatch`:
+ reads `SIM_KEY/CORES/NSPLIT` from environment,
+ uses `SLURM_ARRAY_TASK_ID` as snapshot index,
+ dispatches snapshot runner script.

### 3. Why This Change Was Necessary
What was broken or incompatible?
- No batch entrypoint for large snapshot sweeps.
What would happen without this change?
- Could not run full production efficiently across 275 snapshots.
Is this a temporary workaround or permanent fix?
- Permanent operational script.

### 4. Validation Performed
How was it tested?
- Submitted arrays using this sbatch wrapper and confirmed active tasks.
On which snapshot?
- Array tasks include snapshots `0-274`.
With which core count?
- 4 cores per task.
What outputs confirmed correctness?
- Jobs entered RUNNING/PENDING states with expected command/workdir metadata.

### 5. Side Effects / Risks
Does this affect reproducibility?
- Improves repeatability of production submissions.
Does this affect memory usage?
- No direct code-side effect.
Does this change output formats?
- No.
Could it break other stages?
- Low risk; scheduler wrapper only.

### 6. Reversion Instructions
- Remove `scripts/stage3_4_array.sbatch`.
- Submit tasks manually.

---

## Entry 30 (Chronological): Add Multi-Simulation Submission Helper
### 1. Change Summary
File modified: `scripts/submit_stage3_4_all.sh`
Lines changed: New file (all lines)
Type: Runtime config
Stage affected: Both
Reason for change: Provide a single command to submit all four simulation arrays with controlled concurrency.

### 2. Before -> After Diff (Human-readable)
- (no file)
+ Added `submit_stage3_4_all.sh <range> <concurrency> <cores> <nsplit>`.
+ Submits `zoomnull`, `zoom2`, `zoom3`, `zoom4` arrays.

### 3. Why This Change Was Necessary
What was broken or incompatible?
- Multi-simulation submission was manual and repetitive.
What would happen without this change?
- Increased operator error and slower launch.
Is this a temporary workaround or permanent fix?
- Permanent operational helper.

### 4. Validation Performed
How was it tested?
- Executed helper with `0-274 8 4 4`.
On which snapshot?
- Full range `0-274`.
With which core count?
- 4.
What outputs confirmed correctness?
- Submission IDs returned: `86913`, `86914`, `86915`, `86916`.

### 5. Side Effects / Risks
Does this affect reproducibility?
- Improves launch consistency.
Does this affect memory usage?
- No.
Does this change output formats?
- No.
Could it break other stages?
- No.

### 6. Reversion Instructions
- Remove `scripts/submit_stage3_4_all.sh`.
- Submit each simulation array manually.

---

## Entry 31 (Chronological): Fix NewDD Completion-Check Filename Pattern
### 1. Change Summary
File modified: `scripts/stage3_4_run_snapshot.sh`
Lines changed: ~63-65
Type: Runtime config
Stage affected: Both
Reason for change: Correct rank-file existence checks for NewDD outputs (5-digit rank suffix), enabling accurate skip/resume behavior.

### 2. Before -> After Diff (Human-readable)
- `SN.${STEP}.000${r}.info`
+ `SN.${STEP}.${r}.info`
- `SN.${STEP}.DM.000${r}.dat`
+ `SN.${STEP}.DM.${r}.dat`
- `SN.${STEP}.GAS.000${r}.dat`
+ `SN.${STEP}.GAS.${r}.dat`

### 3. Why This Change Was Necessary
What was broken or incompatible?
- Completion checks targeted non-existent filenames.
What would happen without this change?
- Stages would rerun unnecessarily or skip detection would fail.
Is this a temporary workaround or permanent fix?
- Permanent fix.

### 4. Validation Performed
How was it tested?
- Re-ran wrapper preflight and confirmed successful stage progression.
On which snapshot?
- `snap_0274`.
With which core count?
- 4.
What outputs confirmed correctness?
- End-to-end completion with correctly detected output artifacts.

### 5. Side Effects / Risks
Does this affect reproducibility?
- Improves deterministic rerun behavior.
Does this affect memory usage?
- No.
Does this change output formats?
- No.
Could it break other stages?
- Low risk.

### 6. Reversion Instructions
- Restore old filename patterns (not recommended).

---

## Entry 32 (Chronological): Module Init Guard + MPI Launcher Compatibility Fix
### 1. Change Summary
File modified: `scripts/stage3_4_run_snapshot.sh`
Lines changed: ~43-46, ~70-82
Type: MPI configuration
Stage affected: Both
Reason for change: Resolve preflight failures due to strict shell variable checking during module init and PMI issues with `srun` launcher in this environment.

### 2. Before -> After Diff (Human-readable)
- `source /etc/profile.d/lmod.sh` under `set -u`
+ `set +u; source /etc/profile.d/lmod.sh; set -u`
- `srun -n "$CORES" <exe> ...`
+ `mpirun -np "$CORES" <exe> ...`

### 3. Why This Change Was Necessary
What was broken or incompatible?
- `lmod.sh` referenced unset SLURM vars under `set -u`; `srun` path produced PMI init failures in preflight context.
What would happen without this change?
- Wrapper execution would fail before/within stage launches.
Is this a temporary workaround or permanent fix?
- Practical environment-compatibility fix.

### 4. Validation Performed
How was it tested?
- Re-ran preflight wrapper after patch.
On which snapshot?
- `snap_0274` (zoomnull).
With which core count?
- 4.
What outputs confirmed correctness?
- Wrapper completed all stages successfully.

### 5. Side Effects / Risks
Does this affect reproducibility?
- No science-side effect; improves execution reliability.
Does this affect memory usage?
- No.
Does this change output formats?
- No.
Could it break other stages?
- Low risk.

### 6. Reversion Instructions
- Remove `set +u/set -u` guard and revert to `srun` if cluster MPI policy requires it.

---

## Entry 33 (Chronological): Production Run Directory Setup + Full Array Submission
### 1. Change Summary
File modified: None
Lines changed: None
Type: Directory structure
Stage affected: Both
Reason for change: Initialize production run roots and launch full four-simulation arrays (`0-274`, `4x4`).

### 2. Before -> After Diff (Human-readable)
- No production run root under `/home/bkoh/pgalf/runs/`
+ Created:
+ `/home/bkoh/pgalf/runs/{zoomnull,zoom2,zoom3,zoom4,slurm}`
- No active production arrays
+ Submitted arrays:
+ `86913` (`zoomnull`), `86914` (`zoom2`), `86915` (`zoom3`), `86916` (`zoom4`)

### 3. Why This Change Was Necessary
What was broken or incompatible?
- Production execution had not been launched.
What would happen without this change?
- Pipeline would remain in diagnostic-only state.
Is this a temporary workaround or permanent fix?
- Operational execution step.

### 4. Validation Performed
How was it tested?
- Checked queue states and metadata (`squeue`, `scontrol`).
On which snapshot?
- Array range `0-274`.
With which core count?
- 4.
What outputs confirmed correctness?
- Running tasks observed for each simulation; pending tasks throttled by array limit as expected.

### 5. Side Effects / Risks
Does this affect reproducibility?
- Executes baseline configured pipeline.
Does this affect memory usage?
- Cluster runtime resource usage increases as expected for production jobs.
Does this change output formats?
- No.
Could it break other stages?
- No direct code-side impact.

### 6. Reversion Instructions
- Cancel arrays: `scancel 86913 86914 86915 86916`.
- Remove run directories if full rollback is required.

---

## Entry 34 (Chronological): Production Launch Tracking Update
### 1. Change Summary
File modified: `STAGE3_4_STATS.md`
Lines changed: Appended "Production Launch Status" section
Type: Runtime config
Stage affected: Both
Reason for change: Record full-array launch metadata and preflight status for reproducible monitoring.

### 2. Before -> After Diff (Human-readable)
- (no production launch tracking section)
+ Added submission IDs, simulation mapping, range/concurrency/core settings, and preflight confirmation.

### 3. Why This Change Was Necessary
What was broken or incompatible?
- No consolidated record of active production submissions.
What would happen without this change?
- Harder auditability of job provenance and monitoring context.
Is this a temporary workaround or permanent fix?
- Permanent run-tracking documentation.

### 4. Validation Performed
How was it tested?
- Cross-checked with `sbatch` return IDs and queue metadata.
On which snapshot?
- Full range `0-274`.
With which core count?
- 4.
What outputs confirmed correctness?
- Matching job IDs and active array tasks.

### 5. Side Effects / Risks
Does this affect reproducibility?
- Improves traceability only.
Does this affect memory usage?
- No.
Does this change output formats?
- No.
Could it break other stages?
- No.

### 6. Reversion Instructions
- Remove the appended section from `STAGE3_4_STATS.md` if desired.

---

## Entry 35 (Chronological): Machine-Readable Post-Run Validation Report
### 1. Change Summary
File modified: `stage3_4_postrun_report.json`
Lines changed: New file added (`112` lines)
Type: Runtime config
Stage affected: Both
Reason for change: Persist post-run validation outputs in machine-readable form for auditability and automated comparisons.

### 2. Before -> After Diff (Human-readable)
- `stage3_4_postrun_report.json` did not exist
+ Added `stage3_4_postrun_report.json` with per-simulation completion, completeness, aggregate science metrics, and log-integrity fields

### 3. Why This Change Was Necessary
What was broken or incompatible?
- Post-run verification results only existed in terminal output.
What would happen without this change?
- No durable machine-readable artifact for downstream checks/regressions.
Is this a temporary workaround or permanent fix?
- Permanent reporting artifact.
- No code modifications performed in this stage.

### 4. Validation Performed
How was it tested?
- Parsed the JSON and cross-checked key totals against generated post-run summaries.
On which snapshot?
- Full range `0-274` across `zoomnull`, `zoom2`, `zoom3`, `zoom4`.
With which core count?
- 4.
What outputs confirmed correctness?
- JSON contains four simulation blocks with expected counts/metrics and no parse errors.

### 5. Side Effects / Risks
Does this affect reproducibility?
- No; reporting-only artifact.
Does this affect memory usage?
- Negligible.
Does this change output formats?
- Adds one new summary JSON file.
Could it break other stages?
- No direct stage coupling.

### 6. Reversion Instructions
- Remove `stage3_4_postrun_report.json`.

---

## Entry 36 (Chronological): Post-Run Completion and Metrics Section in Stats Document
### 1. Change Summary
File modified: `STAGE3_4_STATS.md`
Lines changed: Appended section starting at `STAGE3_4_STATS.md:279`
Type: Runtime config
Stage affected: Both
Reason for change: Record final production completion status, output completeness, and aggregate Stage 3/4 metrics in the runbook.

### 2. Before -> After Diff (Human-readable)
- No consolidated "Post-Run Validation (All Arrays Completed)" section
+ Added section with SLURM completion (`86913`-`86916`), `275/275` completeness per simulation, aggregate halos/mass/galaxies/valid-centers, and integrity notes

### 3. Why This Change Was Necessary
What was broken or incompatible?
- Final run outcomes were not documented in the project stats ledger.
What would happen without this change?
- Missing audit trail for completion and quantitative outputs of the production campaign.
Is this a temporary workaround or permanent fix?
- Permanent documentation update.
- No code modifications performed in this stage.

### 4. Validation Performed
How was it tested?
- Recomputed completion and metric aggregates from run directories and checked consistency with inserted values.
On which snapshot?
- Full range `0-274` across all four simulations.
With which core count?
- 4.
What outputs confirmed correctness?
- Section values match generated report (`stage3_4_postrun_report.json`) and parsed output files.

### 5. Side Effects / Risks
Does this affect reproducibility?
- Improves traceability only.
Does this affect memory usage?
- No.
Does this change output formats?
- No simulation-output format changes; documentation-only.
Could it break other stages?
- No.

### 6. Reversion Instructions
- Remove the appended post-run section from `STAGE3_4_STATS.md`.

---

---

## Entry 37 (Chronological): SWIFT Compatibility Documentation
### 1. Change Summary
File modified: `SWIFT.md`
Lines changed: New file added
Type: Runtime config
Stage affected: Both
Reason for change: Document SWIFT compatibility code changes, cosmology overrides, and build requirements.

### 2. Before -> After Diff (Human-readable)
- `SWIFT.md` did not exist
+ Added `SWIFT.md` summarizing SWIFT-related code changes and rationale

### 3. Why This Change Was Necessary
What was broken or incompatible?
- SWIFT-specific modifications were not documented in a single consolidated file.
What would happen without this change?
- Harder to audit or onboard new users to the SWIFT-specific behavior and build requirements.
Is this a temporary workaround or permanent fix?
- Permanent documentation update.
- No code modifications performed in this stage.

### 4. Validation Performed
How was it tested?
- Document-only change; no runtime validation required.
On which snapshot?
- N/A.
With which core count?
- N/A.
What outputs confirmed correctness?
- File exists and reflects current staged code changes.

### 5. Side Effects / Risks
Does this affect reproducibility?
- Improves traceability only.
Does this affect memory usage?
- No.
Does this change output formats?
- No.
Could it break other stages?
- No.

### 6. Reversion Instructions
- Remove `SWIFT.md` and this changelog entry.

---

## Entry 38 (Chronological): Mass Function and Projection Analysis Script
### 1. Change Summary
File modified: `/home/bkoh/pgalf/analysis/compare_mass_functions.py`
Lines changed: New file added
Type: Runtime config
Stage affected: Both
Reason for change: Add a Python analysis utility to compare halo/stellar mass functions across runs, produce Mstar–Mhalo at z=0, and optional density projections and gas T–ρ plots over all snapshots.

### 2. Before -> After Diff (Human-readable)
- No analysis script for mass functions, projections, or temperature–density
+ Added `analysis/compare_mass_functions.py` with CLI options and output plots

### 3. Why This Change Was Necessary
What was broken or incompatible?
- No automated tooling to compare mass functions across runs or generate requested diagnostic plots.
What would happen without this change?
- Manual ad-hoc analysis for each snapshot and run.
Is this a temporary workaround or permanent fix?
- Permanent analysis utility.
- No code modifications performed in this stage.

### 4. Validation Performed
How was it tested?
- Script creation and CLI wiring verified; full execution pending Python deps.
On which snapshot?
- N/A.
With which core count?
- N/A.
What outputs confirmed correctness?
- File exists and is executable.

### 5. Side Effects / Risks
Does this affect reproducibility?
- Improves traceability of analysis; no simulation output changes.
Does this affect memory usage?
- Analysis may be memory heavy on large snapshots; uses per-file streaming where possible.
Does this change output formats?
- Generates new PNG plots and units text files in the output directory.
Could it break other stages?
- No.

### 6. Reversion Instructions
- Remove `/home/bkoh/pgalf/analysis/compare_mass_functions.py` and this changelog entry.
