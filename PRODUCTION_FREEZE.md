# PRODUCTION_FREEZE.md

## Date/Time
- Local: 2026-02-24 16:36:52 KST (+0900)
- UTC: 2026-02-24T07:36:52Z

## Source Identity
- Git commit hash: `2a88d054e6d38f3eb090d3f64742144372d5af12`
- Source tree checksum (tracked working tree SHA256): `6f113525faa41cf37074717ff374797ca3d21cf3dc30be1e0873a7824d6b111c`

## Environment
- Modules:
  1. `intel/tbb/2022.3`
  2. `intel/compiler-rt/2025.3.0`
  3. `intel/umf/1.0.2`
  4. `intel/compiler-intel-llvm/2025.3.0`
  5. `intel/mpi/2021.17`
- `mpirun` path: `/opt/ohpc/pub/intel/oneapi-2025.03/mpi/2021.17/bin/mpirun`
- `mpirun` version: `Intel(R) MPI Library for Linux* OS, Version 2021.17 Build 20251009 (id: 0345ffb)`

## Archived Artifacts
- Freeze snapshot directory: `/home/bkoh/pgalf/runs/production_freeze/20260224T073643Z`
- Archived files:
  - `IMPLEMENTATION_LOG.md`
  - `PREPROD_RUN_REPORT.md`
  - `stage5_mpi_validation.md`
- Archive checksums file:
  - `/home/bkoh/pgalf/runs/production_freeze/20260224T073643Z/archive_sha256.txt`

## Final Decision
- **GO**
- Basis: `PREPROD_RUN_REPORT.md` concludes `B3 PASS → CLEARED FOR PRODUCTION`.
