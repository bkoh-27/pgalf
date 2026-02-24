# Engineering Continuation Dossier — pgalf/NewDD SWIFT Compatibility

**Status as of 2026-02-23**: All primary work **COMPLETE**. The code successfully reads and processes SWIFT HDF5 snapshots.

---

## 1. Problem Definition

**pgalf** is a Galaxy Finding Pipeline. Its first stage, **NewDD** (`/home/bkoh/pgalf/NewDD/`), reads particle simulation snapshots and splits particles into X-axis spatial domain slabs for downstream FoF finding. It previously supported only GADGET HDF5 snapshots. The goal was to extend it to support **SWIFT HDF5 snapshots** located at `/gpfs/dbi224/new-sim-cv22-zoom/snap_NNNN.hdf5`.

---

## 2. Repository & Build System

- **Source root**: `/home/bkoh/pgalf/`
- **NewDD source**: `/home/bkoh/pgalf/NewDD/`
- **Key source files**:
  - `newdd.c` — main entry point
  - `rd_gadget.c` — GADGET/SWIFT HDF5 reader
  - `utils.c` — `SplitDump()` (domain decomposition writer)
  - `Memory2.c` — custom stack allocator
  - `Memory.h` — allocator API
  - `ramses.h` — type definitions (shared with parent dir)
  - `Makefile` — build rules

**Build command** (requires Intel OneAPI modules):
```bash
source /etc/profile.d/lmod.sh
module load intel/tbb/2022.3 intel/umf/1.0.2 intel/compiler-rt/2025.3.0 intel/compiler/2025.3.0 intel/mpi/2021.17
cd /home/bkoh/pgalf/NewDD
make newdd   # or: touch newdd.c rd_gadget.c && make all
```

**Key Makefile variables** (as of completion):
```makefile
CC       = mpiicx
OPT      = -g ... -DGADGET_HDF5    # -DGADGET_HDF5 is REQUIRED
HDF5_INC = /home/bkoh/miniconda3/envs/pgalf/include
HDF5_LIB = /home/kjhan/local/lib
INCLUDES = -I./ -I../ -I$(HDF5_INC)
OSTLIB   = -L. -lmyram -L$(HDF5_LIB) -lhdf5 -Wl,-rpath,$(HDF5_LIB) -lm
TIMEROBJ = rd_amr.o rd_info.o rd_part.o header.o utils.o Memory2.o find_leaf_gas.o rd_hydro.o rd_sink.o rd_gadget.o
```

> ⚠️ If `-DGADGET_HDF5` is missing from OPT, `rd_gadget.c` compiles to empty stubs and nothing works.

**Test run**:
```bash
cd /home/bkoh/pgalf/NewDD/test_swift   # has snap_0000.hdf5 symlink
mpirun -n 1 ../newdd.exe 0 4           # istep=0, nsplit=4
```
Expected output in `FoF_Data/NewDD.00000/`: `SN.00000.{GAS,DM}.0000{0..3}.dat` and `.info` files.

---

## 3. SWIFT vs GADGET HDF5 Format Differences

| Attribute/Dataset | GADGET | SWIFT |
|---|---|---|
| `NumPart_ThisFile` | 6-element int32 | 7-element int64 |
| `MassTable` | 6-element float64 | 7-element float64 |
| `BoxSize` | scalar double | 3-element float64 array |
| Scale factor | `Time` attribute | `Scale-factor` attribute |
| `HubbleParam` | present | absent |
| Unit system | In `Header` group | In `InternalCodeUnits` group |
| Unit attr names | `UnitLength_in_cm`, etc. | `"Unit length in cgs (U_L)"`, etc. |
| Gas density | `Density` | `Densities` |
| Gas thermal energy | `InternalEnergy` | `InternalEnergies` |
| Gas smoothing | `SmoothingLength` | `SmoothingLengths` |
| Metallicity | `Metallicity` | `MetalMassFractions` |
| Star formation time | `StellarFormationTime` | `BirthScaleFactors` |
| BH mass | `BH_Mass` | `SubgridMasses` (fallback: `Masses`) |
| BH accretion rate | `BH_Mdot` | `AccretionRates` |
| Velocity convention | `v_pec / sqrt(a)` | physical peculiar velocity |

**SWIFT InternalCodeUnits** (for `/gpfs/dbi224/new-sim-cv22-zoom/`):
- `U_L = 3.08567758e24 cm` (1 Mpc)
- `U_M = 1.98841e43 g`
- `U_t = 3.08567758e19 s`
- → `unit_velocity = U_L / U_t ≈ 1 km/s`

---

## 4. Stack Allocator Invariant (CRITICAL)

`Malloc`/`Free` in `Memory2.c` implement a **stack allocator**. The allocator supports out-of-order frees via `memmove` (see `EraseMemory`), but with a critical side effect:

> When `Free(X)` is called and `X` is not the stack top, the allocator **moves all data above X downward** and **updates all live pointers** above X via their `PtrToVariable` back-references.

**This means**: if you do `ptr = Malloc(n)` and later store `saved = ptr` before freeing things below `ptr`, then `saved` becomes stale after those frees — the allocator updated `ptr` (the local variable) but not `saved`.

**Pattern to follow** (correct):
```c
// Allocate result buffer FIRST (bottom), then temporaries on top
result = Malloc(result_size, PPTR(result));   // stack bottom
tmp1   = Malloc(tmp_size,    PPTR(tmp1));     // above result
tmp2   = Malloc(tmp_size,    PPTR(tmp2));     // top

// Fill result from tmps
for(i=0;i<n;i++) result[i] = f(tmp1[i], tmp2[i]);

// Free temporaries top-down (LIFO order)
Free(tmp2); Free(tmp1);

// NOW save result pointer (after all tmps freed, result is top and stable)
ram->result = result;
```

**Anti-pattern** (what caused the GAS SplitDump segfault):
```c
result = Malloc(result_size);
// ... fill result ...
ram->result = result;             // ← WRONG: saved before freeing tmps
Free(tmp1); Free(tmp2);           // memmove shifts result, ptr goes stale
// SplitDump reads from old ram->result → SEGFAULT
```

**Similarly**: allocations in a loop over particle types (DM loop, STAR loop in `rd_gadget_particles`) are fine because the main `particle` buffer is at the stack bottom and temporaries within each loop iteration are freed in strict LIFO order (last allocated = first freed).

---

## 5. All Changes Made

### `ramses.h` (line ~175)
Added `is_swift` field to `GadgetHeaderType`:
```c
int is_swift;   /* 1 if SWIFT format, 0 if GADGET */
```

### `rd_gadget.c`
1. **`gadget_units`**: SWIFT velocity convention (no `sqrt(a)` factor):
   ```c
   ram->kmscale_v = header->is_swift ? header->unit_velocity / 1.0e5
                                     : header->unit_velocity / 1.0e5 * sqrt(a);
   ```

2. **`rd_gadget_info`**: Safe header reading:
   - `NumPart_ThisFile` → read into `long long[7]` buffer, copy 6 to int
   - `MassTable` → read into `double[8]` buffer, copy 6
   - `BoxSize` → read into `double[3]` buffer, use `[0]`
   - `Time` → read first, then override with `Scale-factor` if present and `0 < val <= 2`
   - After `H5Gclose(header_id)`: read `InternalCodeUnits` group for SWIFT units
   - `boxlen_ini = boxsize * unit_length / Mpc * hubble`

3. **mass_table buffers**: All 3 reader functions use `double mass_table[8]` with loop to 8.

4. **`rd_gadget_gas`** — dataset name fallbacks + **LIFO fix**:
   - `Density` → fallback `Densities`
   - `InternalEnergy` → fallback `InternalEnergies`
   - `SmoothingLength` → fallback `SmoothingLengths`
   - `Metallicity` → fallback `MetalMassFractions`
   - **CRITICAL**: `Free(zmet...x)` called BEFORE `ram->gas = gas`

5. **`rd_gadget_bh`** — dataset name fallbacks + **LIFO fix**:
   - `BH_Mass` → fallback `SubgridMasses` → fallback `Masses`
   - `BH_Mdot` → fallback `AccretionRates`
   - **CRITICAL**: `Free(ids...x)` called BEFORE `ram->sink = sink`

6. **`rd_gadget_particles`** (stars):
   - `StellarFormationTime` → fallback `BirthScaleFactors`
   - `Metallicity` → fallback `MetalMassFractions`

### `Makefile`
- Added `-DGADGET_HDF5` to OPT
- Added `HDF5_INC = /home/bkoh/miniconda3/envs/pgalf/include`
- Added `HDF5_LIB = /home/kjhan/local/lib`
- Updated INCLUDES to add `-I$(HDF5_INC)`
- Updated OSTLIB to add `-L$(HDF5_LIB) -lhdf5 -Wl,-rpath,$(HDF5_LIB)`
- Added `rd_gadget.o` to TIMEROBJ

### `newdd.c` (GADGET block)
- Added `snap_%04d` as third path fallback
- Restructured GADGET loop: all `SplitDump` calls first, then free in LIFO order
  - Allocation order: `particle` → `gas` → `sink` → `dm` (inside loop)
  - Free order: `dm` (inside loop) → `sink` → `gas` → `particle`

---

## 6. Known Remaining Issues

### Debug `printf/fflush` statements
`newdd.c` still contains verbose debug prints added during development (e.g., "P0: qsort GAS", "P0: Free dm"). These are harmless but noisy; remove before production use.

### SINK crash with synthetic GADGET test data
The synthetic GADGET test snapshot (`snapdir_000/snapshot_000.hdf5`) with 5 BH particles crashes in `SplitDump SINK`. This is a pre-existing bug, not related to SWIFT changes. SWIFT snapshots at early times (snap_0000 at z≈45) have `nsink=0` so this path is never hit.

---

## 7. Snapshot Paths

The code tries these patterns in order:
1. `./snapdir_%03d/snapshot_%03d` (GADGET multi-file)
2. `./snapshot_%03d` (GADGET single file)
3. `./snap_%04d` (SWIFT)

SWIFT snapshots live at:
- `/gpfs/dbi224/new-sim-cv22-zoom/snap_NNNN.hdf5`
- `/gpfs/dbi224/new-sim-cv22-zoom2/snap_NNNN.hdf5`
- `/gpfs/dbi224/new-sim-cv22-zoom3/snap_NNNN.hdf5`
- `/gpfs/dbi224/new-sim-cv22-zoom4/snap_NNNN.hdf5`

Run from a directory containing a `snap_NNNN.hdf5` symlink or copy.

---

## 8. Acceptance Criteria (All Met)

- [x] `rd_gadget_info` correctly reads SWIFT header without buffer overflow
- [x] `boxlen_ini` set to 37.25 Mpc for the zoom simulation
- [x] `aexp` set from `Scale-factor` attribute (not `Time`)
- [x] SWIFT units read from `InternalCodeUnits` group
- [x] Gas/DM/Star coordinates converted to Mpc correctly
- [x] `SplitDump GAS` succeeds (262144 particles → 2 non-empty bins)
- [x] `SplitDump DM` succeeds (349056 particles → 4 bins)
- [x] Memory stack returns to 0 after processing
- [x] Output: `FoF_Data/NewDD.00000/SN.00000.{GAS,DM}.000{00..03}.dat`
- [x] Build clean with `make newdd`

---

## 9. Next Steps (Not Yet Done)

1. **Remove debug printf statements** from `newdd.c`
2. **Test with other zoom snapshots** (`zoom2`, `zoom3`, `zoom4`) and later timesteps
3. **Run on full production data** (multiple snapshots via job array)
4. **Fix SINK crash** for GADGET data with BH particles (separate pre-existing bug)
5. **Commit to bkoh-27/pgalf GitHub** repository
