#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 4 ]]; then
  echo "Usage: $0 <sim_key> <snapshot_idx> <cores> <nsplit>" >&2
  exit 2
fi

SIM_KEY="$1"
SNAP_IDX="$2"
CORES="$3"
NSPLIT="$4"

ROOT="/home/bkoh/pgalf"
RUNS_ROOT="$ROOT/runs"

case "$SIM_KEY" in
  zoomnull|zoom)
    SRC_DIR="/gpfs/dbi224/new-sim-cv22-zoom"
    SIM_DIR="$RUNS_ROOT/zoomnull"
    ;;
  zoom2)
    SRC_DIR="/gpfs/dbi224/new-sim-cv22-zoom2"
    SIM_DIR="$RUNS_ROOT/zoom2"
    ;;
  zoom3)
    SRC_DIR="/gpfs/dbi224/new-sim-cv22-zoom3"
    SIM_DIR="$RUNS_ROOT/zoom3"
    ;;
  zoom4)
    SRC_DIR="/gpfs/dbi224/new-sim-cv22-zoom4"
    SIM_DIR="$RUNS_ROOT/zoom4"
    ;;
  *)
    echo "Unknown sim_key: $SIM_KEY" >&2
    exit 2
    ;;
esac

SNAP=$(printf "%04d" "$SNAP_IDX")
STEP=$(printf "%05d" "$SNAP_IDX")

set +u
source /etc/profile.d/lmod.sh
set -u
module load intel/umf/1.0.2 intel/compiler-rt/2025.3.0 intel/tbb/2022.3 intel/compiler-intel-llvm/2025.3.0 intel/mpi/2021.17

mkdir -p "$SIM_DIR" "$SIM_DIR/FoF_Data" "$SIM_DIR/FoF_Garbage" "$SIM_DIR/logs/${CORES}x${NSPLIT}"
cd "$SIM_DIR"
ln -snf "$SRC_DIR/snap_${SNAP}.hdf5" "snap_${SNAP}.hdf5"

mkdir -p "FoF_Data/NewDD.${STEP}" "FoF_Data/FoF.${STEP}" "FoF_Garbage/Garb.${STEP}"

LOG_DIR="$SIM_DIR/logs/${CORES}x${NSPLIT}"
NEWDD_LOG="$LOG_DIR/newdd_${STEP}.log"
OPFOF_LOG="$LOG_DIR/opfof_${STEP}.log"
GFIND_LOG="$LOG_DIR/gfind_${STEP}.log"
GCENTER_LOG="$LOG_DIR/galcenter_${STEP}.log"

export OMP_NUM_THREADS=1

have_newdd=1
for ((i=0; i<NSPLIT; i++)); do
  r=$(printf "%05d" "$i")
  [[ -s "FoF_Data/NewDD.${STEP}/SN.${STEP}.${r}.info" ]] || have_newdd=0
  [[ -s "FoF_Data/NewDD.${STEP}/SN.${STEP}.DM.${r}.dat" ]] || have_newdd=0
  [[ -s "FoF_Data/NewDD.${STEP}/SN.${STEP}.GAS.${r}.dat" ]] || have_newdd=0
done
if [[ "$have_newdd" -eq 0 ]]; then
  mpirun -np "$CORES" "$ROOT/NewDD/newdd.exe" "$SNAP_IDX" "$NSPLIT" > "$NEWDD_LOG" 2>&1
fi

if [[ ! -s "FoF_Data/FoF.${STEP}/FoF_halo_cat.${STEP}" || ! -s "FoF_Data/FoF.${STEP}/FoF_member_particle.${STEP}" ]]; then
  mpirun -np "$CORES" "$ROOT/opFoF/opfof.exe" "$SNAP_IDX" "$NSPLIT" > "$OPFOF_LOG" 2>&1
fi

if [[ ! -s "FoF_Data/FoF.${STEP}/GALCATALOG.LIST.${STEP}" || ! -s "FoF_Data/FoF.${STEP}/GALFIND.DATA.${STEP}" ]]; then
  mpirun -np "$CORES" "$ROOT/NewGalFinder/gfind.exe" "$SNAP_IDX" > "$GFIND_LOG" 2>&1
fi

if [[ ! -s "FoF_Data/FoF.${STEP}/GALFIND.CENTER.${STEP}" ]]; then
  mpirun -np "$CORES" "$ROOT/GalCenter/galcenter.exe" "$SNAP_IDX" > "$GCENTER_LOG" 2>&1
fi

echo "completed sim=$SIM_KEY snapshot=$SNAP_IDX cores=$CORES nsplit=$NSPLIT"
