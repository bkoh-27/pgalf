# NewDD Quick Start Guide

Get up and running with NewDD in 5 minutes.

---

## Prerequisites Checklist

Before starting, ensure you have:

- [ ] C compiler (gcc or icc)
- [ ] MPI library (OpenMPI or MPICH)
- [ ] RAMSES simulation output (`output_XXXXX/` directory)
- [ ] Sufficient disk space for output files

---

## Step 1: Configure and Build

```bash
# From the top-level GalaxyFinder directory:
./configure
make newdd
```

Or with custom options:
```bash
./configure --debug --newdd-nmeg=10000
make newdd
```

Expected output:
```
==============================================
 GalaxyFinder Configuration
==============================================
...
Generating Makefiles...
  Generated: .../NewDD/Makefile
...
```

---

## Step 2: Verify Input Data

Check that your RAMSES output exists:

```bash
ls ../output_00005/
```

You should see:
```
info_00005.txt
amr_00005.out00001
amr_00005.out00002
...
part_00005.out00001
hydro_00005.out00001
sink_00005.out00001
```

---

## Step 3: Run (Single Processor)

For a first test, run without MPI:

```bash
./newdd.exe 5 32
```

Parameters:
- `5` = Snapshot number (reads `output_00005/`)
- `32` = Number of output slabs

---

## Step 4: Run (Parallel)

For production runs, use MPI:

```bash
mpirun -np 8 ./newdd.exe 5 64
```

**Important**: Number of MPI processes should divide `ncpu` evenly.
Check ncpu in your info file:
```bash
grep ncpu ../output_00005/info_00005.txt
```

---

## Step 5: Verify Output

Check the output directory:

```bash
ls FoF_Data/NewDD.00005/
```

Expected files:
```
SN.00005.00000.info      # Slab 0 metadata
SN.00005.DM.00000.dat    # Slab 0 dark matter
SN.00005.STAR.00000.dat  # Slab 0 stars
SN.00005.GAS.00000.dat   # Slab 0 gas
SN.00005.SINK.00000.dat  # Slab 0 sinks
SN.00005.00001.info      # Slab 1 metadata
...
```

---

## Quick Validation

### Check Info File

```bash
cat FoF_Data/NewDD.00005/SN.00005.00000.info
```

Should show particle counts and slab boundaries.

### Check Binary File Size

```bash
ls -la FoF_Data/NewDD.00005/SN.00005.DM.00000.dat
```

File size should be approximately:
```
size = num_particles * sizeof(DmType)
```

---

## Common First-Run Issues

| Issue | Solution |
|-------|----------|
| `Cannot open info file` | Check input path in `rd_info.c` |
| `Memory allocation failed` | Reduce `NMEG` in Makefile |
| `MPI error` | Ensure np divides ncpu |
| `Empty output files` | Check snapshot number exists |

---

## Next Steps

1. **Production Runs**: See [README.md](README.md) for detailed usage
2. **Configuration**: See [CONFIGURATION.md](CONFIGURATION.md) for tuning options
3. **Pipeline Integration**: See [PIPELINE.md](PIPELINE.md) for downstream tools
4. **GADGET Support**: See [README4GADGET.md](README4GADGET.md) for GADGET adaptation
5. **SWIFT/GADGET Runbook**: See [SWIFT_GADGET.md](SWIFT_GADGET.md) for build/run commands used in this repo

---

## Example: Complete Workflow

```bash
# 1. Configure and build (from GalaxyFinder directory)
./configure
make newdd

# 2. Check simulation info
grep -E "ncpu|nlevelmax|boxlen" ../output_00050/info_00050.txt

# 3. Run decomposition
mpirun -np 16 ./NewDD/newdd.exe 50 128

# 4. Verify output
ls -la FoF_Data/NewDD.00050/ | head -20

# 5. Check particle counts
cat FoF_Data/NewDD.00050/SN.00050.00000.info
```

---

## Getting Help

- Full documentation: [README.md](README.md)
- GADGET adaptation: [README4GADGET.md](README4GADGET.md)
- Configuration options: [CONFIGURATION.md](CONFIGURATION.md)
