# README_PRODUCTION.md

## Scope
Production execution for 4 simulation variants with one SLURM array per variant (`0-274`), one snapshot per array task, 4 MPI ranks.

## Production Workspace
- `/home/bkoh/pgalf/work/production_runs/zoom`
- `/home/bkoh/pgalf/work/production_runs/zoom2`
- `/home/bkoh/pgalf/work/production_runs/zoom3`
- `/home/bkoh/pgalf/work/production_runs/zoom4`

Each run directory is isolated and contains:
- `FoF_Data/`
- `FoF_Garbage/`
- `logs/`
- `runs/`
- `slurm/`
- `PRODUCTION_RUN_MANIFEST.md`

## Discovered Executable Paths (no guessing)
- `NewDD`: `/home/bkoh/pgalf/NewDD/newdd.exe`
- `opFoF`: `/home/bkoh/pgalf/opFoF/opfof.exe`
- `gfind`: `/home/bkoh/pgalf/NewGalFinder/gfind.exe`
- `galcenter`: `/home/bkoh/pgalf/GalCenter/galcenter.exe`

Validated pipeline order per snapshot:
1. `newdd.exe <idx> 4`
2. `opfof.exe <idx> 4`
3. `gfind.exe <idx>`
4. `galcenter.exe <idx>`

MPI launcher:
- `mpirun -np 4`

## Required Module Stack (exact)
Every array task runs with:
```bash
source /etc/profile.d/lmod.sh
module purge
module load intel/compiler-intel-llvm/2025.3.0 intel/compiler-rt/2025.3.0 intel/umf/1.0.2 intel/tbb/2022.3 intel/mpi/2021.17
```

## SLURM Array Scripts
- `/home/bkoh/pgalf/work/production_runs/slurm/zoom_array.sbatch`
- `/home/bkoh/pgalf/work/production_runs/slurm/zoom2_array.sbatch`
- `/home/bkoh/pgalf/work/production_runs/slurm/zoom3_array.sbatch`
- `/home/bkoh/pgalf/work/production_runs/slurm/zoom4_array.sbatch`

Default per-task walltime in scripts: `08:00:00`.
To change at submission time:
```bash
sbatch --time=12:00:00 /home/bkoh/pgalf/work/production_runs/slurm/zoom_array.sbatch
```

## Launch Commands
```bash
sbatch /home/bkoh/pgalf/work/production_runs/slurm/zoom_array.sbatch
sbatch /home/bkoh/pgalf/work/production_runs/slurm/zoom2_array.sbatch
sbatch /home/bkoh/pgalf/work/production_runs/slurm/zoom3_array.sbatch
sbatch /home/bkoh/pgalf/work/production_runs/slurm/zoom4_array.sbatch
```

## Logging and Provenance
Per snapshot task (`snap_<idx>`):
- stdout: `<variant>/logs/snap_<idx>.out`
- stderr: `<variant>/logs/snap_<idx>.err`
- output size summary: `<variant>/runs/snap_<idx>_outputs.txt`
- task status record: `<variant>/runs/snapshot_status/snap_<idx>.status`

Each task logs:
- `SLURM job id`, `hostname`
- module list
- `which mpirun`
- `mpirun --version`
- input snapshot path
- key outputs + sizes

Append-only manifests:
- `<variant>/PRODUCTION_RUN_MANIFEST.md`

## Monitoring
Global status:
- `/home/bkoh/pgalf/work/production_runs/STATUS.md`

Update script:
- `/home/bkoh/pgalf/work/production_runs/scripts/update_status.sh`

Useful commands:
```bash
squeue -u $USER
watch -n 30 cat /home/bkoh/pgalf/work/production_runs/STATUS.md
```

Inspect a snapshot task:
```bash
tail -n 200 /home/bkoh/pgalf/work/production_runs/zoom/logs/snap_0000.out
tail -n 200 /home/bkoh/pgalf/work/production_runs/zoom/logs/snap_0000.err
```
