#!/usr/bin/env bash
#SBATCH --job-name=st34
#SBATCH --partition=normal
#SBATCH --nodes=1
#SBATCH --ntasks=4
#SBATCH --cpus-per-task=1
#SBATCH --time=08:00:00
#SBATCH --output=/home/bkoh/pgalf/runs/slurm/%x_%A_%a.out
#SBATCH --error=/home/bkoh/pgalf/runs/slurm/%x_%A_%a.err

set -euo pipefail

SIM_KEY="${SIM_KEY:?SIM_KEY required}"
CORES="${CORES:-4}"
NSPLIT="${NSPLIT:-4}"

if [[ -z "${SLURM_ARRAY_TASK_ID:-}" ]]; then
  echo "SLURM_ARRAY_TASK_ID is required" >&2
  exit 2
fi

/home/bkoh/pgalf/NewDD/scripts/stage3_4_run_snapshot.sh "$SIM_KEY" "$SLURM_ARRAY_TASK_ID" "$CORES" "$NSPLIT"
