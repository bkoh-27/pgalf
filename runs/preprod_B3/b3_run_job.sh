#!/usr/bin/env bash
set -euo pipefail
WORKDIR=/home/bkoh/pgalf/runs/zoom3
EXE=/home/bkoh/pgalf/GalCenter/galcenter.exe
OUTROOT=/home/bkoh/pgalf/runs/preprod_B3

cd "$WORKDIR"

# Step 1 (exact command requested)
ls -lh FoF_Data/FoF.*/GALFIND.DATA.* | awk '$5 != "0" {print}' > b3_nonempty_candidates.txt

if [ ! -s b3_nonempty_candidates.txt ]; then
  mkdir -p "$OUTROOT/NONE"
  cp b3_nonempty_candidates.txt "$OUTROOT/NONE/"
  {
    echo "B3_STEP1_STATUS:FAIL"
    echo "No non-empty GALFIND.DATA candidates found."
  } > "$OUTROOT/NONE/b3_status.txt"
  exit 0
fi

CAND_PATH=$(awk 'NR==1{print $NF}' b3_nonempty_candidates.txt)
NSTEP=$(basename "$CAND_PATH" | sed 's/^GALFIND.DATA\.//')
ARTDIR="$OUTROOT/$NSTEP"
mkdir -p "$ARTDIR"

cp b3_nonempty_candidates.txt "$ARTDIR/"

# Environment and provenance capture
{
  echo "B3_JOBID:${SLURM_JOB_ID:-none}"
  echo "B3_HOST:$(hostname)"
  echo "B3_WORKDIR:$WORKDIR"
  echo "B3_SELECTED_NSTEP:$NSTEP"
  echo "B3_SELECTED_CANDIDATE:$CAND_PATH"
  echo "--- MODULE LIST ---"
} > "$ARTDIR/b3_env.txt"

module purge >/dev/null 2>&1 || true
module load intel/tbb/2022.3 intel/compiler-rt/2025.3.0 intel/umf/1.0.2 intel/compiler-intel-llvm/2025.3.0 intel/mpi/2021.17
module list >> "$ARTDIR/b3_env.txt" 2>&1

{
  echo
  echo "--- KEY ENV ---"
  for v in LOADEDMODULES PATH LD_LIBRARY_PATH MODULEPATH; do
    eval "echo ${v}=\"\${$v:-}\""
  done
  echo
  echo "--- MPIRUN ---"
  echo "MPIRUN_PATH=$(which mpirun)"
  mpirun --version
  echo
  echo "--- EXACT COMMANDS ---"
  echo "ls -lh FoF_Data/FoF.*/GALFIND.DATA.* | awk '\$5 != \"0\" {print}' > b3_nonempty_candidates.txt"
  echo "mpirun -n 4 $EXE $NSTEP > b3_run1.out 2> b3_run1.err"
  echo "sha256sum FoF_Data/FoF.$NSTEP/GALFIND.CENTER.$NSTEP > b3_run1.sha"
  echo "mpirun -n 4 $EXE $NSTEP > b3_run2.out 2> b3_run2.err"
  echo "sha256sum FoF_Data/FoF.$NSTEP/GALFIND.CENTER.$NSTEP > b3_run2.sha"
  echo "diff b3_run1.sha b3_run2.sha"
  echo "grep \"Current memory stack 0\" b3_run1.out"
  echo "wc -c FoF_Data/FoF.$NSTEP/GALFIND.CENTER.$NSTEP"
} >> "$ARTDIR/b3_env.txt"

# Step 2 (exact sequence)
mpirun -n 4 "$EXE" "$NSTEP" > "$ARTDIR/b3_run1.out" 2> "$ARTDIR/b3_run1.err"
sha256sum "FoF_Data/FoF.$NSTEP/GALFIND.CENTER.$NSTEP" > "$ARTDIR/b3_run1.sha"

mpirun -n 4 "$EXE" "$NSTEP" > "$ARTDIR/b3_run2.out" 2> "$ARTDIR/b3_run2.err"
sha256sum "FoF_Data/FoF.$NSTEP/GALFIND.CENTER.$NSTEP" > "$ARTDIR/b3_run2.sha"

# Step 3 acceptance checks
DIFF_RC=0
MEM_RC=0
SIZE_RC=0

diff "$ARTDIR/b3_run1.sha" "$ARTDIR/b3_run2.sha" > "$ARTDIR/b3_sha_diff.txt" || DIFF_RC=$?
grep "Current memory stack 0" "$ARTDIR/b3_run1.out" > "$ARTDIR/b3_memstack_check.txt" || MEM_RC=$?
wc -c "FoF_Data/FoF.$NSTEP/GALFIND.CENTER.$NSTEP" > "$ARTDIR/b3_center_wc.txt"
SIZE_BYTES=$(awk '{print $1}' "$ARTDIR/b3_center_wc.txt")
if [ "$SIZE_BYTES" -gt 0 ]; then SIZE_RC=0; else SIZE_RC=1; fi

{
  echo "DIFF_RC:$DIFF_RC"
  echo "MEMSTACK_GREP_RC:$MEM_RC"
  echo "CENTER_SIZE_BYTES:$SIZE_BYTES"
  echo "CENTER_SIZE_GT_ZERO_RC:$SIZE_RC"
  if [ $DIFF_RC -eq 0 ] && [ $MEM_RC -eq 0 ] && [ $SIZE_RC -eq 0 ]; then
    echo "B3_STATUS:PASS"
  else
    echo "B3_STATUS:FAIL"
  fi
} > "$ARTDIR/b3_status.txt"
