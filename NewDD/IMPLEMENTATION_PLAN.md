Implementation Plan: Halo Finding + Galaxy Finding (Stages 3–4)
Preliminary Assumptions
State before writing any code:

NCHEM/NDUST mismatch is a critical blocker. gfind.c reads FoF_member_particle written by opFoF using sizeof(StarType), sizeof(GasType), etc. opFoF is compiled with NCHEM=9, NDUST=4; the current gfind Makefile uses NCHEM=3 with no NDUST. This causes a 40+ byte per-particle struct size mismatch, corrupting all reads. Fix: compile gfind with NCHEM=9 -DNDUST=4.

opFoF uses mpiicc (deprecated Intel legacy wrapper). It is available on this cluster but wraps the old icx internally. Will update to mpiicx for consistency with the rest of the build.

All three stages must share one working directory per simulation since all paths are hardcoded relative to ./. The structure will be:


/home/bkoh/pgalf/runs/{zoom,zoom2,zoom3,zoom4}/
275 snapshots × 4 simulations = 1100 total snapshot-runs. Processing will use SLURM job arrays, with one job per snapshot per simulation.

nsplit = 4 (matching the validated test). This determines both NewDD output and opFoF nfiles argument.

GalCenter (center refinement) is included as a sub-step of Galaxy Finding.

No existing opFoF or gfind binaries. Both must be built.

libmyram.a already exists at /home/bkoh/pgalf/libmyram.a. opFoF links against it with -L../ -lmyram — this path assumes opFoF is compiled and run relative to the pgalf root, which is fine since we call it via absolute path.

Stage 3: Halo Finding (opFoF)
3.1 Build opFoF
File to modify: /home/bkoh/pgalf/opFoF/Rules.make

Changes required:

CC = mpiicc → CC = mpiicx
FC = mpiifc → FC = mpiifx
F90C = mpiifort → F90C = mpiifx
No changes to flags needed; NCHEM=9, NDUST=4 already match NewDD.

Build command:


cd /home/bkoh/pgalf/opFoF && make all
Success criterion: opfof.exe exists and links against libmyram.a.

3.2 opFoF execution per snapshot

cd /home/bkoh/pgalf/runs/zoom
mpirun -n 4 /home/bkoh/pgalf/opFoF/opfof.exe <ISTEP> 4
Inputs: ./FoF_Data/NewDD.<ISTEP>/SN.<ISTEP>.{DM,GAS,STAR,SINK}.0000{0..3}.dat
Outputs: ./FoF_Data/FoF.<ISTEP>/FoF_halo_cat.<ISTEP>, FoF_member_particle.<ISTEP>
Prerequisite: FoF_Data/FoF.<ISTEP>/ directory must exist.

Success criterion: FoF_halo_cat.<ISTEP> is non-empty and its 7-float header has valid cosmological parameters.

Stage 4: Galaxy Finding (NewGalFinder + GalCenter)
4.1 Build gfind
File to modify: /home/bkoh/pgalf/NewGalFinder/Makefile

Changes required:

Add -DNDUST=4 to CFLAGS/OPT
Change -DNCHEM=3 → -DNCHEM=9 (struct compatibility with opFoF)
Build command:


cd /home/bkoh/pgalf/NewGalFinder && make
Success criterion: gfind.exe links successfully with FFTW3.

4.2 Build GalCenter
File to check/modify: /home/bkoh/pgalf/GalCenter/Makefile

Verify compiler flags match (NCHEM=9, NDUST=4). No structural changes expected.

4.3 gfind execution per snapshot

cd /home/bkoh/pgalf/runs/zoom
mpirun -n <N> /home/bkoh/pgalf/NewGalFinder/gfind.exe <ISTEP>
Inputs: ./FoF_Data/FoF.<ISTEP>/FoF_halo_cat.<ISTEP>, FoF_member_particle.<ISTEP>
Outputs: ./FoF_Data/FoF.<ISTEP>/GALCATALOG.LIST.<ISTEP>, GALFIND.DATA.<ISTEP>

4.4 galcenter execution per snapshot

cd /home/bkoh/pgalf/runs/zoom
mpirun -n <N> /home/bkoh/pgalf/GalCenter/galcenter.exe <ISTEP>
Input: GALFIND.DATA.<ISTEP>
Output: GALFIND.CENTER.<ISTEP>

Directory Structure

/home/bkoh/pgalf/runs/
├── zoom/
│   ├── snap_0000.hdf5 → /gpfs/dbi224/new-sim-cv22-zoom/snap_0000.hdf5
│   ├── snap_0001.hdf5 → ...  (275 symlinks)
│   └── FoF_Data/
│       ├── NewDD.00000/    (NewDD output)
│       ├── FoF.00000/      (opFoF + gfind output)
│       └── FoF_Garbage/
├── zoom2/  (same structure)
├── zoom3/
└── zoom4/
Full Pipeline Execution Order (per snapshot)

Step 1: NewDD   → FoF_Data/NewDD.<ISTEP>/
Step 2: opFoF   → FoF_Data/FoF.<ISTEP>/FoF_halo_cat + member_particle
Step 3: gfind   → FoF_Data/FoF.<ISTEP>/GALCATALOG.LIST + GALFIND.DATA
Step 4: galcenter → FoF_Data/FoF.<ISTEP>/GALFIND.CENTER
SLURM Job Strategy
One SLURM array job per simulation. Each array task processes one snapshot:

Task index = snapshot index (0–274)
Sequentially runs all 4 stages for that snapshot
Snapshot dependencies handled within single task (no inter-task deps needed)
Potential Failure Points
Risk	Severity	Mitigation
NCHEM/NDUST struct mismatch in gfind	Critical	Recompile with NCHEM=9 NDUST=4
SWIFT header has omega0=0, hubble=1 — affects FoF link length	Medium	Verify opFoF link length calculation is robust to these
gfind NMEG=90000 (90 GB/worker) — memory over-allocation on small halos	Medium	Start with 1 worker, tune NMEG if needed
FoF_Garbage/Garb.XXXXX/ directory missing → opFoF write failure	Low	Pre-create all dirs in setup script
275×4 = 1100 snapshots × 4 stages = 4400 SLURM tasks	Low	Job array with dependency checking
Success Criteria (per snapshot, per simulation)
FoF_halo_cat.<ISTEP>: ≥ 1 halo, mass range 10⁸–10¹⁵ M☉/h
GALCATALOG.LIST.<ISTEP>: ≥ 1 subhalo entry
GALFIND.CENTER.<ISTEP>: center positions within simulation box
Memory stack returns 0 (no leaks)

You may proceed with changing NCHEM to 9.
For zoom (null seed):
Run two tests:
cores=4, nsplit=4
cores=8, nsplit=8
Compare:
Halo counts
Total halo mass
Galaxy counts
Center positions
Results should be numerically consistent within floating-point tolerance.
Report any discrepancies.
For zoom2, zoom3, zoom4:
Use cores=4 and nsplit=4 only.
Cosmological parameters for the simulation:
Omega_m = 0.3
Omega_b = 0.049
sigma_8 = 0.8
h = 0.6711
Ensure these are used consistently in:
FoF linking length calculation
Unit conversions
Galaxy mass calculations
Proceed after confirming configuration.
