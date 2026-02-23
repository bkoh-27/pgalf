# SWIFT Compatibility Changes (Stage 2–4)

This document summarizes the raw code changes made to support SWIFT HDF5 snapshots and ensure consistent cosmology handling across NewDD (Stage 2), opFoF (Stage 3), gfind/galcenter (Stage 4).

## 1) NewDD: GADGET/SWIFT HDF5 parsing and memory safety

Files:
- `NewDD/rd_gadget.c`
- `NewDD/rd_gadget.h`
- `NewDD/newdd.c`
- `NewDD/Makefile`
- `ramses.h`

Changes and rationale:
- **SWIFT header handling** (`NewDD/rd_gadget.c`):
  - SWIFT stores some header arrays with 7 elements; GADGET expects 6. A 7‑element temporary buffer is now used for `NumPart_ThisFile` and `MassTable`, then copied into the 6‑element structures to avoid overflow.
  - `Time` is interpreted differently in SWIFT. We now read `Scale-factor` when present and valid, and use it as `header.time`.
  - `BoxSize` can be a 3‑element array in SWIFT. We read the array and use the first element.
  - SWIFT stores physical unit system under `InternalCodeUnits`. We read `U_L/U_M/U_t` and derive velocity; this is required for consistent unit conversions.
  - SWIFT can use different dataset names (e.g., `BirthScaleFactors`, `MetalMassFractions`, `Densities`, `InternalEnergies`, `SmoothingLengths`, `SubgridMasses`, `AccretionRates`). These are now supported as fallbacks.

- **Velocity scaling** (`NewDD/rd_gadget.c`):
  - GADGET stores `v_pec/sqrt(a)` while SWIFT stores physical peculiar velocities. We now scale by `sqrt(a)` only for GADGET; SWIFT uses unscaled values.

- **Memory2 stack allocator safety** (`NewDD/rd_gadget.c` + `NewDD/newdd.c`):
  - Temporary arrays are freed **before** storing `ram->gas`/`ram->sink` because the stack allocator can relocate memory during frees. This prevents stale pointers.
  - `newdd.c` now frees in strict LIFO order (dm → sink → gas → particle) to keep Memory2’s stack consistent.

- **Snapshot basename fallback** (`NewDD/newdd.c`):
  - Added fallback to `snap_####` after `snapdir_####` and `snapshot_###` to match SWIFT file naming.

- **Build changes** (`NewDD/Makefile` + `ramses.h`):
  - `-DGADGET_HDF5` is always enabled and `rd_gadget.o` is included.
  - HDF5 include/lib paths are set for the current environment.
  - `ramses.h` now has include guards, `GadgetHeaderType.npart` is `int`, and a `is_swift` flag is added.

## 2) Cosmology overrides for consistency

Files:
- `opFoF/opfof.c`
- `NewGalFinder/gfind.c`
- `GalCenter/galcenter.c`

Changes and rationale:
- SWIFT headers may contain inconsistent or unexpected cosmology values. The pipeline requires:
  - `Omega_m = 0.3`
  - `Omega_b = 0.049`
  - `Omega_L = 0.7`
  - `H0 = 67.11`
- Each stage explicitly overwrites the read header values with the required parameters to keep FoF linking length, mass conversions, and centers consistent across stages.

## 3) FoF robustness and modern toolchain fixes

Files:
- `opFoF/Treewalk.fof.ordered.c`
- `opFoF/fof.h`
- `opFoF/Rules.make`

Changes and rationale:
- **Missing file handling**: if `BottomFaceContactHalo` is missing, we create an empty placeholder and return gracefully instead of hard exiting.
- **Boolean enum**: fix invalid YES/NO macro definition to a proper enum.
- **Compiler updates**: switch to `mpiicx/mpiifx` and add `-fcommon` for modern toolchain compatibility.

## 4) Galaxy finding robustness and diagnostics

Files:
- `NewGalFinder/gfind.c`
- `NewGalFinder/subhaloden.mod6.c`
- `NewGalFinder/tree.h`
- `NewGalFinder/Makefile`

Changes and rationale:
- **Cosmology overrides** and safe handling of zero/invalid `Omega_m` when computing linking length.
- **mpeak guardrails**: clamp negative/out‑of‑range `mpeak` in MPI receive path to avoid invalid array accesses.
- **Null/empty guards** in `write_data` when no particles are present.
- **finddenpeak diagnostics**: log counters for density‑threshold behavior and explicitly mark all particles as `NOT_HALO_MEMBER` when no core is found.
- **Precision**: `HaloQ` mass fields upgraded to `double` to avoid precision loss in accumulation.
- **Build**: `NCHEM=9`, `NDUST=4` enforced per requirement.

## 5) Summary of Why These Changes Were Necessary

- SWIFT HDF5 snapshot layouts, unit conventions, and dataset naming differ from legacy GADGET assumptions.
- The Memory2 stack allocator requires strict LIFO freeing order; previous frees could leave dangling pointers.
- Stage‑3/4 physics needs fixed cosmology parameters; relying on headers caused inconsistency.
- Modern Intel toolchains require updated compiler names and `-fcommon` to link legacy code.

