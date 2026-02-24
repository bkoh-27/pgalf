"""
Generate a minimal GADGET HDF5 snapshot for testing newdd.exe.
Creates: snapdir_000/snapshot_000.hdf5
  - 1000 DM particles (PartType1)
  -  200 star particles (PartType4)
  -  300 gas particles  (PartType0)
  -    5 black holes    (PartType5)
Units: GADGET defaults (kpc/h, 1e10 Msun/h, km/s)
"""
import os
import numpy as np
import h5py

np.random.seed(42)

BOXSIZE   = 10000.0   # kpc/h
NDM       = 1000
NSTAR     = 200
NGAS      = 300
NBH       = 5

os.makedirs("snapdir_000", exist_ok=True)
fname = "snapdir_000/snapshot_000.hdf5"

with h5py.File(fname, "w") as f:

    # --- Header ---
    hdr = f.create_group("Header")
    npart = np.array([NGAS, NDM, 0, 0, NSTAR, NBH], dtype=np.int32)
    hdr.attrs["NumPart_ThisFile"]     = npart
    hdr.attrs["NumPart_Total"]        = npart.astype(np.uint32)
    hdr.attrs["NumPart_Total_HighWord"] = np.zeros(6, dtype=np.uint32)
    hdr.attrs["MassTable"]            = np.array([0, 1e-3, 0, 0, 0, 0], dtype=np.float64)
    hdr.attrs["Time"]                 = np.float64(0.5)      # a = 0.5 (z=1)
    hdr.attrs["Redshift"]             = np.float64(1.0)
    hdr.attrs["BoxSize"]              = np.float64(BOXSIZE)
    hdr.attrs["NumFilesPerSnapshot"]  = np.int32(1)
    hdr.attrs["Omega0"]               = np.float64(0.3)
    hdr.attrs["OmegaLambda"]          = np.float64(0.7)
    hdr.attrs["HubbleParam"]          = np.float64(0.7)
    hdr.attrs["Flag_Sfr"]             = np.int32(1)
    hdr.attrs["Flag_Cooling"]         = np.int32(1)
    hdr.attrs["Flag_StellarAge"]      = np.int32(1)
    hdr.attrs["Flag_Metals"]          = np.int32(1)
    hdr.attrs["Flag_Feedback"]        = np.int32(1)
    hdr.attrs["UnitLength_in_cm"]     = np.float64(3.085678e21)   # 1 kpc
    hdr.attrs["UnitMass_in_g"]        = np.float64(1.989e43)      # 1e10 Msun
    hdr.attrs["UnitVelocity_in_cm_per_s"] = np.float64(1.0e5)    # 1 km/s

    # --- PartType1: Dark matter ---
    dm = f.create_group("PartType1")
    coords = np.random.uniform(0, BOXSIZE, (NDM, 3)).astype(np.float64)
    vels   = np.random.normal(0, 200, (NDM, 3)).astype(np.float64)
    ids    = np.arange(1, NDM+1, dtype=np.uint64)
    dm.create_dataset("Coordinates", data=coords)
    dm.create_dataset("Velocities",  data=vels)
    dm.create_dataset("ParticleIDs", data=ids)
    # Mass from MassTable[1] — no per-particle dataset

    # --- PartType4: Stars ---
    st = f.create_group("PartType4")
    coords = np.random.uniform(0.1*BOXSIZE, 0.9*BOXSIZE, (NSTAR, 3)).astype(np.float64)
    vels   = np.random.normal(0, 150, (NSTAR, 3)).astype(np.float64)
    masses = np.random.uniform(1e-5, 5e-5, NSTAR).astype(np.float64)
    tform  = np.random.uniform(0.2, 0.5, NSTAR).astype(np.float64)
    zmet   = np.random.uniform(0.001, 0.02, NSTAR).astype(np.float64)
    ids    = np.arange(NDM+1, NDM+NSTAR+1, dtype=np.uint64)
    st.create_dataset("Coordinates",          data=coords)
    st.create_dataset("Velocities",           data=vels)
    st.create_dataset("Masses",               data=masses)
    st.create_dataset("ParticleIDs",          data=ids)
    st.create_dataset("StellarFormationTime", data=tform)
    st.create_dataset("Metallicity",          data=zmet)

    # --- PartType0: Gas ---
    gs = f.create_group("PartType0")
    coords = np.random.uniform(0, BOXSIZE, (NGAS, 3)).astype(np.float64)
    vels   = np.random.normal(0, 100, (NGAS, 3)).astype(np.float64)
    masses = np.random.uniform(5e-6, 2e-5, NGAS).astype(np.float64)
    rho    = np.random.exponential(1e-3, NGAS).astype(np.float64)
    u      = np.random.uniform(1e4, 1e6, NGAS).astype(np.float64)   # (km/s)^2
    hsml   = np.random.uniform(0.5, 5.0, NGAS).astype(np.float64)   # kpc/h
    zmet   = np.random.uniform(0, 0.01, NGAS).astype(np.float64)
    ids    = np.arange(NDM+NSTAR+1, NDM+NSTAR+NGAS+1, dtype=np.uint64)
    gs.create_dataset("Coordinates",    data=coords)
    gs.create_dataset("Velocities",     data=vels)
    gs.create_dataset("Masses",         data=masses)
    gs.create_dataset("ParticleIDs",    data=ids)
    gs.create_dataset("Density",        data=rho)
    gs.create_dataset("InternalEnergy", data=u)
    gs.create_dataset("SmoothingLength",data=hsml)
    gs.create_dataset("Metallicity",    data=zmet)

    # --- PartType5: Black holes ---
    bh = f.create_group("PartType5")
    coords = np.random.uniform(0.4*BOXSIZE, 0.6*BOXSIZE, (NBH, 3)).astype(np.float64)
    vels   = np.random.normal(0, 50, (NBH, 3)).astype(np.float64)
    mbh    = np.random.uniform(1e-4, 1e-2, NBH).astype(np.float64)
    mdot   = np.random.uniform(1e-6, 1e-4, NBH).astype(np.float64)
    ids    = np.arange(NDM+NSTAR+NGAS+1, NDM+NSTAR+NGAS+NBH+1, dtype=np.uint64)
    bh.create_dataset("Coordinates", data=coords)
    bh.create_dataset("Velocities",  data=vels)
    bh.create_dataset("ParticleIDs", data=ids)
    bh.create_dataset("BH_Mass",     data=mbh)
    bh.create_dataset("BH_Mdot",     data=mdot)

print(f"Written: {fname}")
print(f"  DM:    {NDM}")
print(f"  Stars: {NSTAR}")
print(f"  Gas:   {NGAS}")
print(f"  BH:    {NBH}")
