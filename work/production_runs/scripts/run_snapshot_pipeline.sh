#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 3 ]]; then
  echo "Usage: $0 <variant> <input_root> <snapshot_idx>" >&2
  exit 2
fi

VARIANT="$1"
INPUT_ROOT="$2"
SNAP_IDX="$3"

ROOT="/home/bkoh/pgalf"
PROD_ROOT="$ROOT/work/production_runs"
RUN_DIR="$PROD_ROOT/$VARIANT"
STATUS_DIR="$RUN_DIR/runs/snapshot_status"
MANIFEST="$RUN_DIR/PRODUCTION_RUN_MANIFEST.md"
MANIFEST_LOCK="$RUN_DIR/runs/manifest.lock"

SNAP4=$(printf "%04d" "$SNAP_IDX")
STEP5=$(printf "%05d" "$SNAP_IDX")
SNAP_NAME="snap_${SNAP4}"
INPUT_SNAPSHOT="$INPUT_ROOT/${SNAP_NAME}.hdf5"

TASK_OUT="$RUN_DIR/logs/${SNAP_NAME}.out"
TASK_ERR="$RUN_DIR/logs/${SNAP_NAME}.err"
SLURM_TAG="${SLURM_JOB_ID:-nojob}_${SLURM_ARRAY_TASK_ID:-na}"
META_FILE="$RUN_DIR/runs/${SNAP_NAME}.meta"
OUTPUT_SUMMARY="$RUN_DIR/runs/${SNAP_NAME}_outputs.txt"
STATUS_FILE="$STATUS_DIR/${SNAP_NAME}.status"

mkdir -p "$RUN_DIR" "$RUN_DIR/FoF_Data" "$RUN_DIR/FoF_Garbage" "$RUN_DIR/logs" "$RUN_DIR/slurm" "$RUN_DIR/runs" "$STATUS_DIR"

# Per-task logs are append-only.
exec >>"$TASK_OUT" 2>>"$TASK_ERR"

echo "========================================"
echo "START $(date '+%Y-%m-%d %H:%M:%S %Z (%z)')"
echo "VARIANT=$VARIANT SNAPSHOT=$SNAP4 STEP=$STEP5 JOB=${SLURM_TAG} HOST=$(hostname)"
echo "INPUT_SNAPSHOT=$INPUT_SNAPSHOT"

if [[ ! -f "$INPUT_SNAPSHOT" ]]; then
  echo "ERROR: missing input snapshot: $INPUT_SNAPSHOT" >&2
  FAILED_STAGE="INPUT_CHECK"
  PIPELINE_RC=3
else
  FAILED_STAGE=""
  PIPELINE_RC=0
fi

# Required production module stack.
if (( PIPELINE_RC == 0 )); then
  set +e
  set +u
  source /etc/profile.d/lmod.sh
  set -u
  rc=$?
  if (( rc == 0 )); then
    module purge
    module load intel/compiler-intel-llvm/2025.3.0 intel/compiler-rt/2025.3.0 intel/umf/1.0.2 intel/tbb/2022.3 intel/mpi/2021.17
    rc=$?
  fi
  if (( rc != 0 )); then
    # Some clusters require oneAPI dependencies preloaded before compiler-intel-llvm.
    module purge
    module load intel/tbb/2022.3 intel/compiler-rt/2025.3.0 intel/umf/1.0.2
    module load intel/compiler-intel-llvm/2025.3.0 intel/compiler-rt/2025.3.0 intel/umf/1.0.2 intel/tbb/2022.3 intel/mpi/2021.17
    rc=$?
  fi
  set -e
  if (( rc != 0 )); then
    PIPELINE_RC=$rc
    FAILED_STAGE="MODULE_INIT"
  fi
fi

if (( PIPELINE_RC == 0 )); then
  echo "--- MODULE LIST ---"
  module list
  echo "--- MPIRUN ---"
  which mpirun
  mpirun --version
fi

if (( PIPELINE_RC == 0 )); then
  ln -sfn "$INPUT_SNAPSHOT" "$RUN_DIR/${SNAP_NAME}.hdf5"
  cd "$RUN_DIR"
fi

export OMP_NUM_THREADS=1
CORES=4
NSPLIT=4

run_stage() {
  local stage="$1"; shift
  echo "--- STAGE ${stage} START $(date '+%Y-%m-%d %H:%M:%S %Z (%z)') ---"
  "$@"
  local rc=$?
  echo "--- STAGE ${stage} END rc=${rc} $(date '+%Y-%m-%d %H:%M:%S %Z (%z)') ---"
  return $rc
}

if (( PIPELINE_RC == 0 )); then
  set +e
  run_stage "NewDD" mpirun -np "$CORES" "$ROOT/NewDD/newdd.exe" "$SNAP_IDX" "$NSPLIT"
  rc=$?
  if (( rc != 0 )); then PIPELINE_RC=$rc; FAILED_STAGE="NewDD"; fi

  if (( PIPELINE_RC == 0 )); then
    run_stage "opFoF" mpirun -np "$CORES" "$ROOT/opFoF/opfof.exe" "$SNAP_IDX" "$NSPLIT"
    rc=$?
    if (( rc != 0 )); then PIPELINE_RC=$rc; FAILED_STAGE="opFoF"; fi
  fi

  if (( PIPELINE_RC == 0 )); then
    run_stage "gfind" mpirun -np "$CORES" "$ROOT/NewGalFinder/gfind.exe" "$SNAP_IDX"
    rc=$?
    if (( rc != 0 )); then PIPELINE_RC=$rc; FAILED_STAGE="gfind"; fi
  fi

  if (( PIPELINE_RC == 0 )); then
    run_stage "galcenter" mpirun -np "$CORES" "$ROOT/GalCenter/galcenter.exe" "$SNAP_IDX"
    rc=$?
    if (( rc != 0 )); then PIPELINE_RC=$rc; FAILED_STAGE="galcenter"; fi
  fi
  set -e
fi

# Capture key outputs and sizes (for provenance).
{
  echo "# output summary for ${SNAP_NAME}"
  echo "# generated $(date '+%Y-%m-%d %H:%M:%S %Z (%z)')"
  for d in "FoF_Data/NewDD.${STEP5}" "FoF_Data/FoF.${STEP5}" "FoF_Garbage/Garb.${STEP5}"; do
    if [[ -d "$d" ]]; then
      find "$d" -maxdepth 1 -type f -print0 | sort -z | xargs -0 -I{} stat -c '%s %n' "{}"
    else
      echo "MISSING_DIR $d"
    fi
  done
} > "$OUTPUT_SUMMARY" 2>&1 || true

# Determine PASS/FAIL
STATE="PASS"
if (( PIPELINE_RC != 0 )); then
  STATE="FAIL"
fi

{
  echo "SNAPSHOT=$SNAP4"
  echo "JOB=$SLURM_TAG"
  echo "HOST=$(hostname)"
  echo "STATE=$STATE"
  echo "FAILED_STAGE=$FAILED_STAGE"
  echo "RC=$PIPELINE_RC"
  echo "INPUT=$INPUT_SNAPSHOT"
  echo "LOG_OUT=$TASK_OUT"
  echo "LOG_ERR=$TASK_ERR"
  echo "OUTPUT_SUMMARY=$OUTPUT_SUMMARY"
  echo "UPDATED=$(date '+%Y-%m-%d %H:%M:%S %Z (%z)')"
} > "$STATUS_FILE"

# Append manifest entry (append-only; flock for array concurrency).
(
  flock -x 200
  {
    echo
    echo "## Snapshot ${SNAP4} | Job ${SLURM_TAG} | ${STATE}"
    echo "- Timestamp: $(date '+%Y-%m-%d %H:%M:%S %Z (%z)')"
    echo "- Input: ${INPUT_SNAPSHOT}"
    echo "- Logs: ${TASK_OUT}, ${TASK_ERR}"
    echo "- Outputs: ${OUTPUT_SUMMARY}"
    if [[ "$STATE" == "FAIL" ]]; then
      echo "- Failed stage: ${FAILED_STAGE} (rc=${PIPELINE_RC})"
      echo ""
      echo "### Failure stderr excerpt (last 200 lines)"
      echo '```text'
      tail -n 200 "$TASK_ERR"
      echo '```'
    fi
  } >> "$MANIFEST"
) 200>"$MANIFEST_LOCK"

# Refresh global status after each task.
"$PROD_ROOT/scripts/update_status.sh" || true

if [[ "$STATE" == "FAIL" ]]; then
  echo "END FAIL $(date '+%Y-%m-%d %H:%M:%S %Z (%z)') stage=$FAILED_STAGE rc=$PIPELINE_RC" >&2
  exit "$PIPELINE_RC"
fi

echo "END PASS $(date '+%Y-%m-%d %H:%M:%S %Z (%z)')"
exit 0
