#!/usr/bin/env bash
set -euo pipefail

ARRAY_RANGE="${1:-0-274}"
CONCURRENCY="${2:-8}"
CORES="${3:-4}"
NSPLIT="${4:-4}"

mkdir -p /home/bkoh/pgalf/runs/slurm

submit_one() {
  local sim="$1"
  sbatch \
    --array="${ARRAY_RANGE}%${CONCURRENCY}" \
    --job-name="st34_${sim}_${CORES}x${NSPLIT}" \
    --ntasks="$CORES" \
    --export=ALL,SIM_KEY="$sim",CORES="$CORES",NSPLIT="$NSPLIT" \
    /home/bkoh/pgalf/NewDD/scripts/stage3_4_array.sbatch
}

submit_one zoomnull
submit_one zoom2
submit_one zoom3
submit_one zoom4
