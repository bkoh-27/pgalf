#ifndef RD_GADGET_H
#define RD_GADGET_H

#ifdef GADGET_HDF5
#include <hdf5.h>
#include "ramses.h"

/* GADGET particle type indices */
#define GADGET_TYPE_GAS   0
#define GADGET_TYPE_HALO  1
#define GADGET_TYPE_DISK  2
#define GADGET_TYPE_BULGE 3
#define GADGET_TYPE_STAR  4
#define GADGET_TYPE_BH    5

/* Function prototypes */
int rd_gadget_info(RamsesType *ram, char *basename, int ifile);
int rd_gadget_particles(RamsesType *ram, char *basename, int ifile);
int rd_gadget_gas(RamsesType *ram, char *basename, int ifile);
int rd_gadget_bh(RamsesType *ram, char *basename, int ifile);
void gadget_units(RamsesType *ram, GadgetHeaderType *header);

/* Helper functions */
int gadget_get_nfiles(char *basename);
int gadget_check_dataset(hid_t group_id, const char *name);

#endif /* GADGET_HDF5 */
#endif /* RD_GADGET_H */
