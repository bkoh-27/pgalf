#!/usr/bin/env bash
#SBATCH --job-name=prod_zoom
#SBATCH --partition=normal
#SBATCH --nodes=1
#SBATCH --ntasks=4
#SBATCH --cpus-per-task=1
#SBATCH --time=08:00:00
#SBATCH --array=0-274
#SBATCH --output=/home/bkoh/pgalf/work/production_runs/zoom/slurm/%x_%A_%a.out
#SBATCH --error=/home/bkoh/pgalf/work/production_runs/zoom/slurm/%x_%A_%a.err

set -euo pipefail

# Single-snapshot default walltime (override at submit with: sbatch --time=HH:MM:SS ...)
DEFAULT_WALLTIME="08:00:00"

SNAP_IDX="${SLURM_ARRAY_TASK_ID:?missing array index}"
/home/bkoh/pgalf/work/production_runs/scripts/run_snapshot_pipeline.sh "zoom" \
  "/gpfs/dbi224/new-sim-cv22-zoom" "$SNAP_IDX"
