# Stage 3-4 Statistics (Snapshot 0274, zoomnull)

Date: 2026-02-23

## Scope
This document summarizes verified Stage 3 (FoF) and Stage 4 (gfind + galcenter) statistics for snapshot `0274` using:
- `4x4` configuration (`cores=4`, `nsplit=4`)
- `8x8` configuration (`cores=8`, `nsplit=8`)

Data sources:
- `cmp_zoomnull_s0274_4x4/snap_0274.hdf5`
- `cmp_zoomnull_s0274_4x4/FoF_Data/FoF.00274/*`
- `cmp_zoomnull_s0274_8x8/FoF_Data/FoF.00274/*`

## Star Inventory
Total stars in simulation snapshot:
- `N_star_total = 9514`

(Verified from SWIFT header `NumPart_Total[4]` and `PartType4/ParticleIDs` length.)

## Stars In/Out of FoF Halos
From `FoF_halo_cat.00274`:

- `4x4`:
  - Stars in halos: `9463`
  - Stars outside halos: `51`
- `8x8`:
  - Stars in halos: `9463`
  - Stars outside halos: `51`

Outside-halo fraction:
- `51 / 9514 = 0.0053605` (`0.536%`)

## Star Concentration Across Halos
From per-halo `npstar` in `FoF_halo_cat.00274`:

- Halos with at least one star: `67`
- Largest halo star count: `4612` (`48.74%` of all in-halo stars)
- Top 3 halos combined: `77.32%` of in-halo stars
- Top 5 halos combined: `85.62%` of in-halo stars
- Top 10 halo star counts:
  - `[4612, 1910, 795, 523, 262, 224, 187, 161, 147, 107]`

This distribution is identical between `4x4` and `8x8` for snapshot `0274`.

## GALFIND.CENTER Validity Breakdown
`GALFIND.CENTER.00274` stores three center types per record: `gal`, `dmhalo`, `gas`.

Sentinel value for invalid centers is `NO_CENTER_POS = -999999`.

Counts of valid entries:

- `4x4` (30 records):
  - Valid `gal` centers: `3`
  - Valid `dmhalo` centers: `30`
  - Valid `gas` centers: `24`

- `8x8` (31 records):
  - Valid `gal` centers: `3`
  - Valid `dmhalo` centers: `31`
  - Valid `gas` centers: `25`

Interpretation:
- A sentinel `gal` center usually indicates no stellar member particles for that subhalo in Stage 4 output.
- The low count of valid `gal` centers is therefore primarily a subhalo stellar-content issue, not a blanket galcenter runtime failure.

## Stage 4 Star Retention Audit (gfind Output)
From `GALCATALOG.LIST.00274` (header-only output of halos/subhalos with `mpeak > 0`):

- `4x4`:
  - Output halos: `30`
  - Output subhalos: `30`
  - Stars in output halos (`sum(HaloInfo.nstar)`): `530`
  - Stars assigned to output subhalos (`sum(SubInfo.npstar)`): `341`
  - Subhalos with stars: `3`
  - Subhalos without stars: `27`

- `8x8`:
  - Output halos: `31`
  - Output subhalos: `31`
  - Stars in output halos (`sum(HaloInfo.nstar)`): `530`
  - Stars assigned to output subhalos (`sum(SubInfo.npstar)`): `341`
  - Subhalos with stars: `3`
  - Subhalos without stars: `28`

Derived retention metrics:
- Relative to FoF in-halo stars (`9463`):
  - Stars reaching gfind output halos: `530 / 9463 = 5.60%`
  - Stars assigned to gfind subhalos: `341 / 9463 = 3.60%`
- Within gfind output halos:
  - Subhalo stellar assignment efficiency: `341 / 530 = 64.34%`

Interpretation:
- The dominant loss occurs before/during subhalo-core detection (`mpeak` generation): most FoF halos with stars do not produce a positive `mpeak` and are absent from `GALCATALOG.LIST`.
- This strongly indicates Stage 4 detection sensitivity/selection behavior is the primary cause of sparse valid `gal` centers.

## Controlled Sensitivity Sweep (Stage 4)
Test performed:
- Temporary code change in `subhaloden.mod6.c`: `PEAKTHRESHOLD 5.E2L -> 2.E2L`
- Rebuilt `gfind`, reran snapshot `0274` (`4x4`) in isolated directory:
  - `cmp_zoomnull_s0274_4x4_pth200`
- Then reverted code and rebuilt baseline executable.

Result:
- No measurable output change relative to baseline `4x4`:
  - `GALCATALOG.LIST`: halos `30`, subs `30`, `halo_nstar=530`, `sub_npstar=341`
  - `GALFIND.CENTER`: records `30`, valid `gal=3`, valid `dmhalo=30`, valid `gas=24`

Conclusion:
- Lowering this single threshold does not address the dominant loss mode.
- Root cause is likely elsewhere in Stage 4 detection/assignment flow (not this peak threshold value alone).

## Focused Stage-4 Rejection Diagnostics (Instrumented gfind)
Run:
- Snapshot `0274` (zoomnull), `cores=4`, `nsplit=4`
- Isolated directory: `cmp_zoomnull_s0274_4x4_diag`
- Instrumentation tags in `run_gfind_0274_4x4_diag.log`:
  - `SUBHALO_DIAG`
  - `SUBHALO_ZERO_CORE`

Aggregated counts:
- Total halos processed by `subhalo_den`: `279`
- Halos with `numcore=0`: `238` (`85.3%`)
- Halos with `numcore>0`: `41` (`14.7%`)

Branch breakdown:
- `TYPE_ALL` and `numcore=0`: `238`
- `TYPE_ALL` and `numcore>0`: `27`
- `TYPE_STAR` and `numcore>0`: `14`
- `TYPE_STAR` and `numcore=0`: `0`

Zero-core reason breakdown (`TYPE_ALL` only):
- `low_star_num=1`, `low_star_mass=1`: `186`
- `low_star_num=1`, `low_star_mass=0`: `52`
- `low_star_num=0`, `low_star_mass=1`: `0`
- `low_star_num=0`, `low_star_mass=0`: `0`

Interpretation:
- The dominant rejection mode is low stellar count (`star_num <= NUMNEIGHBOR`, with `NUMNEIGHBOR=32`).
- Most halos never enter the `TYPE_STAR` core-finding path.
- This is consistent with sparse star-bearing subhalos and many sentinel `gal` centers.

Output consistency check (baseline vs instrumented):
- `GALFIND.CENTER` unchanged for `4x4`:
  - records `30`, valid `gal=3`, valid `dmhalo=30`, valid `gas=24`

## Controlled Sweep A: NUMNEIGHBOR (32 -> 16)
Run:
- Snapshot `0274`, `4x4`
- Isolated directory: `cmp_zoomnull_s0274_4x4_diag_n16`
- Other settings unchanged.

Result (compared to baseline instrumented run):
- `SUBHALO_DIAG total`: `279` (unchanged)
- `SUBHALO_ZERO_CORE total`: `238` (unchanged)
- Branch breakdown unchanged:
  - `TYPE_ALL, numcore=0`: `238`
  - `TYPE_ALL, numcore>0`: `27`
  - `TYPE_STAR, numcore>0`: `14`
- Zero-core reason breakdown unchanged:
  - `low_star_num=1, low_star_mass=1`: `186`
  - `low_star_num=1, low_star_mass=0`: `52`

Output metrics unchanged:
- `GALFIND.CENTER`: records `30`, valid `gal=3`, valid `dmhalo=30`, valid `gas=24`

Conclusion:
- Lowering `NUMNEIGHBOR` from 32 to 16 did not alter the dominant rejection path.

## Controlled Sweep B: MINSTELLARMASS (2e6 -> 0)
Run:
- Snapshot `0274`, `4x4`
- Isolated directory: `cmp_zoomnull_s0274_4x4_diag_mstar0`
- `NUMNEIGHBOR` kept at baseline (`32`).

Result (compared to baseline instrumented run):
- `SUBHALO_DIAG total`: `279` (unchanged)
- `SUBHALO_ZERO_CORE total`: `238` (unchanged)
- Branch breakdown unchanged:
  - `TYPE_ALL, numcore=0`: `238`
  - `TYPE_ALL, numcore>0`: `27`
  - `TYPE_STAR, numcore>0`: `14`
- Zero-core reason breakdown unchanged:
  - `low_star_num=1, low_star_mass=1`: `186`
  - `low_star_num=1, low_star_mass=0`: `52`

Output metrics unchanged:
- `GALFIND.CENTER`: records `30`, valid `gal=3`, valid `dmhalo=30`, valid `gas=24`

Conclusion:
- Lowering `MINSTELLARMASS` to 0 also did not change observed Stage-4 outcomes for this snapshot.

## Focused Stage-4 Rejection Diagnostics v2 (finddenpeak-level)
Run:
- Snapshot `0274`, `4x4`
- Isolated directory: `cmp_zoomnull_s0274_4x4_diag_peak`
- Added `FINDDENPEAK_DIAG` logging in `finddenpeak()`.

Observed log counts:
- `SUBHALO_DIAG` lines: `279`
- `FINDDENPEAK_DIAG` lines: `265`
- Paired records (`FINDDENPEAK_DIAG` + `SUBHALO_DIAG`): `265`

Note:
- The missing 14 `FINDDENPEAK_DIAG` lines correspond to `TYPE_STAR` path halos (`lagFindStellarCore` branch), which does not call `finddenpeak()` in current build path.

Paired diagnostics (TYPE_ALL / jflag=0 path):
- Zero-core halos: `238`
  - `sum_above = 0`
  - `sum_localmax = 0`
  - `sum_reject_higher = 0`
  - `zero_core_above>0`: `0`
  - `zero_core_above==0`: `238`
- Nonzero-core halos: `27`
  - `sum_above = 188` (avg `6.963` per halo)
  - `sum_localmax = 27` (avg `1.0` per halo)
  - `sum_reject_higher = 161`

Interpretation:
- For all `TYPE_ALL` halos that fail with `numcore=0`, failure occurs before local-maximum competition:
  - no particle exceeds `PEAKTHRESHOLD` in `finddenpeak`.
- This indicates the dominant bottleneck is the effective density-thresholding stage for these halos, not neighbor tie-breaking.

Output consistency check:
- `GALFIND.CENTER` remains unchanged versus previous diagnostic baseline:
  - records `30`, valid `gal=3`, valid `dmhalo=30`, valid `gas=24`

## Controlled Sweep C: TYPE_ALL finddenpeak Threshold Override (Temporary)
Test design:
- Temporary code path override in `finddenpeak()`:
  - `jflag==0 (TYPE_ALL)` uses `peak_thr=50` instead of default `PEAKTHRESHOLD`.
  - `jflag==1 (TYPE_STAR)` remains at default threshold.
- Snapshot `0274`, `4x4`
- Isolated directory: `cmp_zoomnull_s0274_4x4_diag_thr50`

Comparison vs baseline instrumented run (`cmp_zoomnull_s0274_4x4_diag_peak`):

Diagnostic counts:
- `SUBHALO_DIAG total`: `279` -> `279` (no change)
- `SUBHALO_ZERO_CORE total`: `238` -> `236` (improvement of 2 halos)
- `TYPE_ALL, numcore>0`: `27` -> `29`
- `TYPE_STAR, numcore>0`: `14` -> `14` (unchanged)

Output structure:
- `GALCATALOG.LIST`: halos `30` -> `32`, subs `30` -> `32`
- `GALFIND.CENTER`: records `30` -> `32`

Stellar outcome:
- `sum(HaloInfo.nstar)`: `530` -> `530` (unchanged)
- `sum(SubInfo.npstar)`: `341` -> `341` (unchanged)
- Subhalos with stars: `3` -> `3` (unchanged)
- Valid `gal` centers: `3` -> `3` (unchanged)

Interpretation:
- Lowering TYPE_ALL threshold recovered a small number of additional nonzero-core structures,
  but they are starless and do not improve galaxy-center completeness.
- Dominant issue remains stellar-content path, not merely TYPE_ALL threshold strictness.

Status:
- Temporary threshold override has been reverted after test completion.

## Production Launch Status (All Simulations, 4x4)
Launch date: 2026-02-23

Submitted SLURM arrays (snapshot range `0-274`, concurrency `%8`, `cores=4`, `nsplit=4`):
- `86913` : `zoomnull`
- `86914` : `zoom2`
- `86915` : `zoom3`
- `86916` : `zoom4`

Preflight validation before full submission:
- Wrapper run passed for `zoomnull`, snapshot `0274`, `4x4`.
- Output/log directory structure validated under `/home/bkoh/pgalf/runs/`.

Operational note:
- Array tasks execute snapshot-by-snapshot pipeline order:
  `NewDD -> opFoF -> gfind -> galcenter`.

## Post-Run Validation (All Arrays Completed)
Validation date: 2026-02-23

### SLURM Completion
Array completion status (from `sacct`):
- `86913` (`zoomnull`): `275/275` tasks `COMPLETED`, no non-zero exit codes
- `86914` (`zoom2`): `275/275` tasks `COMPLETED`, no non-zero exit codes
- `86915` (`zoom3`): `275/275` tasks `COMPLETED`, no non-zero exit codes
- `86916` (`zoom4`): `275/275` tasks `COMPLETED`, no non-zero exit codes

### Output Completeness Check (Existence-Based)
Using expected artifact presence per snapshot (`0-274`):
- Stage 2: `FoF_Data/NewDD.<step>/SN.<step>.{rank}.info`, `DM.<rank>.dat`, `GAS.<rank>.dat` for ranks `00000..00003`
- Stage 3: `FoF_halo_cat.<step>`, `FoF_member_particle.<step>`
- Stage 4: `GALCATALOG.LIST.<step>`, `GALFIND.DATA.<step>`, `GALFIND.CENTER.<step>`

Result:
- `zoomnull`: `275/275` snapshots complete
- `zoom2`: `275/275` snapshots complete
- `zoom3`: `275/275` snapshots complete
- `zoom4`: `275/275` snapshots complete

### Aggregate Science Outputs (Across 275 Snapshots Each)
- `zoomnull`:
  - FoF halos (sum): `128842`
  - FoF halo mass sum: `7.7177074097424464e16`
  - Galaxies (sum): `8014`
  - Valid galaxy centers: `1191` (`14.86%` of center records)
  - Zero-galaxy snapshots: `9`

- `zoom2`:
  - FoF halos (sum): `140232`
  - FoF halo mass sum: `7.370270240095536e16`
  - Galaxies (sum): `8761`
  - Valid galaxy centers: `1065` (`12.16%`)
  - Zero-galaxy snapshots: `9`

- `zoom3`:
  - FoF halos (sum): `135305`
  - FoF halo mass sum: `7.332279592636053e16`
  - Galaxies (sum): `8230`
  - Valid galaxy centers: `886` (`10.77%`)
  - Zero-galaxy snapshots: `9`

- `zoom4`:
  - FoF halos (sum): `144882`
  - FoF halo mass sum: `7.551733267461141e16`
  - Galaxies (sum): `9588`
  - Valid galaxy centers: `906` (`9.45%`)
  - Zero-galaxy snapshots: `8`

### Log Integrity Notes
- No Stage 3/4 task-level SLURM failures detected.
- No Memory2 allocator error signatures detected in production stage logs.
- `newdd` logs contain frequent HDF5 diagnostic lines due expected fallback attempts (`snapdir_*` / `snapshot_*` before `snap_*`); these did not prevent successful completion.

### Machine-Readable Report
- Consolidated report file: `stage3_4_postrun_report.json`
