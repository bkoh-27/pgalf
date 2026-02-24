# Release Notes

## 2026-02-24 Production Gate Release

### Scope
- Finalized SWIFT pipeline hardening and production-gate closure across `NewDD -> opFoF -> gfind -> galcenter`.
- Consolidated pre-production and MPI validation evidence with freeze metadata.

### What Changed (by stage/task)
- Task 1 (`NewDD/utils.c`): guarded split-bin indexing with early `continue` for invalid bins across particle types.
- Task 3 (`opFoF/Treewalk.fof.ordered.c`): fixed `BIG_MPI_Send/Recv` datatype dispatch (`==` vs assignment), plus unknown-type fallback.
- Task 5 (`NewDD/rd_gadget.c`, `NewDD/newdd.c`): sink/BH data-path validation and deterministic runtime checks (no algorithm redesign).
- Task 6 (`NewDD/rd_gadget.c`): restricted `Scale-factor` override to SWIFT-only branch and preserved non-SWIFT behavior.
- Task 7 (`NewGalFinder/gfind.c`): added missing `MPI_Bcast(&omepb, ...)` for cosmology consistency.
- Task 8 (`NewDD/rd_gadget.c`): added HDF5 1D dataset extent validation to prevent dimension-mismatch misuse.
- Task 9 (`NewDD/newdd.c`): moved MPI init ahead of GADGET discovery and synchronized basename/file-count broadcasts.
- Task 10 (`params.h`, `opFoF/opfof.c`, `NewGalFinder/gfind.c`, `GalCenter/galcenter.c`): centralized required cosmology constants; removed duplicated local macro blocks.
- Task 11 (`opFoF/read.c`, `opFoF/Rules.make`): removed duplicate `DEFINE_SIM_PARA` ownership and dropped `-fcommon`.
- Preprod B1 (`GalCenter/galcenter.c`): added missing `omepb` broadcast in galcenter.
- Step 5 fix #2 (`GalCenter/galcenter.c`): added symmetric `Memory2` cleanup (`sbp`, `rbp`, `rbuffer`) before final `CurMemStack()` check.

### Why It Changed
- Production gate failures identified two root causes:
  - Missing cosmology broadcast (`omepb`) in MPI paths.
  - Non-zero shutdown memory stack in galcenter due unmatched `Memory2` allocations.
- Additional hardening removed known MPI and data-ingest correctness risks that could impact multi-rank scale runs.

### Validation Completed
- Checklist Steps 1-7 executed with evidence in `PREPROD_RUN_REPORT.md`.
- Step 5 failure reproduced, RCA documented, minimal fix applied, and Steps 5-7 rerun to PASS.
- B3 non-empty-input galcenter validation executed on snapshot `00009`:
  - 4-rank run repeated twice.
  - Output hash deterministic across reruns.
  - `Current memory stack 0` observed.
  - Non-empty center output confirmed.
- MPI runtime matrix validation completed for tasks 3/7/9/11 (1/2/4 ranks):
  - no deadlocks,
  - collective consistency preserved,
  - deterministic outputs where expected.

### Production Freeze
- Final freeze metadata and archive references are recorded in `PRODUCTION_FREEZE.md`.
- Freeze artifact bundle: `runs/production_freeze/20260224T073643Z/`.

### Final Gate Decision
- GO
- Basis: Pre-production checklist complete with rerun closure and `B3 PASS -> CLEARED FOR PRODUCTION`.
