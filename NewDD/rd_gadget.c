#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef GADGET_HDF5
#include <hdf5.h>
#endif

#include "ramses.h"
#include "Memory.h"

#ifdef GADGET_HDF5

#define GADGET_NTYPE 6

static int open_snapshot_file(char *basename, int ifile, char *filename, size_t nfilename, hid_t *file_id){
	if(ifile == 0){
		snprintf(filename, nfilename, "%s.hdf5", basename);
		*file_id = H5Fopen(filename, H5F_ACC_RDONLY, H5P_DEFAULT);
		if(*file_id >= 0) return 0;
	}
	snprintf(filename, nfilename, "%s.%d.hdf5", basename, ifile);
	*file_id = H5Fopen(filename, H5F_ACC_RDONLY, H5P_DEFAULT);
	if(*file_id >= 0) return 0;
	return -1;
}

static int attr_exists(hid_t obj_id, const char *name){
	return (H5Aexists(obj_id, name) > 0);
}

static int read_attr_double(hid_t obj_id, const char *name, double *value){
	hid_t attr_id;
	if(!attr_exists(obj_id, name)) return -1;
	attr_id = H5Aopen(obj_id, name, H5P_DEFAULT);
	if(attr_id < 0) return -1;
	H5Aread(attr_id, H5T_NATIVE_DOUBLE, value);
	H5Aclose(attr_id);
	return 0;
}

static int read_attr_int(hid_t obj_id, const char *name, int *value){
	hid_t attr_id;
	if(!attr_exists(obj_id, name)) return -1;
	attr_id = H5Aopen(obj_id, name, H5P_DEFAULT);
	if(attr_id < 0) return -1;
	H5Aread(attr_id, H5T_NATIVE_INT, value);
	H5Aclose(attr_id);
	return 0;
}

static int read_attr_ll_array(hid_t obj_id, const char *name, long long *values){
	hid_t attr_id;
	if(!attr_exists(obj_id, name)) return -1;
	attr_id = H5Aopen(obj_id, name, H5P_DEFAULT);
	if(attr_id < 0) return -1;
	H5Aread(attr_id, H5T_NATIVE_LLONG, values);
	H5Aclose(attr_id);
	return 0;
}

static int read_attr_double_array(hid_t obj_id, const char *name, double *values){
	hid_t attr_id;
	if(!attr_exists(obj_id, name)) return -1;
	attr_id = H5Aopen(obj_id, name, H5P_DEFAULT);
	if(attr_id < 0) return -1;
	H5Aread(attr_id, H5T_NATIVE_DOUBLE, values);
	H5Aclose(attr_id);
	return 0;
}

static int dataset_exists(hid_t group_id, const char *name){
	return (H5Lexists(group_id, name, H5P_DEFAULT) > 0);
}

static int read_dataset_double_1d(hid_t group_id, const char *name, size_t n, double *out){
	hid_t dataset_id;
	dataset_id = H5Dopen2(group_id, name, H5P_DEFAULT);
	if(dataset_id < 0) return -1;
	H5Dread(dataset_id, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, out);
	H5Dclose(dataset_id);
	(void)n;
	return 0;
}

static int read_dataset_ids(hid_t group_id, const char *name, size_t n, idtype *out){
	hid_t dataset_id;
	unsigned long long *tmp;
	size_t i;

	dataset_id = H5Dopen2(group_id, name, H5P_DEFAULT);
	if(dataset_id < 0) return -1;

	tmp = (unsigned long long*)Malloc(sizeof(unsigned long long) * n, PPTR(tmp));
	H5Dread(dataset_id, H5T_NATIVE_ULLONG, H5S_ALL, H5S_ALL, H5P_DEFAULT, tmp);
	H5Dclose(dataset_id);

	for(i=0;i<n;i++) out[i] = (idtype)tmp[i];
	Free(tmp);
	return 0;
}

static int read_dataset_vec3(hid_t group_id, const char *name, size_t n, double *x, double *y, double *z){
	hid_t dataset_id, space_id;
	int ndims;
	hsize_t dims[2] = {0, 0};
	double *tmp;
	size_t i;

	dataset_id = H5Dopen2(group_id, name, H5P_DEFAULT);
	if(dataset_id < 0) return -1;
	space_id = H5Dget_space(dataset_id);
	ndims = H5Sget_simple_extent_ndims(space_id);
	if(ndims != 2){
		H5Sclose(space_id);
		H5Dclose(dataset_id);
		return -1;
	}
	H5Sget_simple_extent_dims(space_id, dims, NULL);
	H5Sclose(space_id);
	if(dims[0] != (hsize_t)n || dims[1] < 3){
		H5Dclose(dataset_id);
		return -1;
	}

	tmp = (double*)Malloc(sizeof(double) * n * 3, PPTR(tmp));
	H5Dread(dataset_id, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, tmp);
	H5Dclose(dataset_id);

	for(i=0;i<n;i++){
		x[i] = tmp[3*i + 0];
		y[i] = tmp[3*i + 1];
		z[i] = tmp[3*i + 2];
	}
	Free(tmp);
	return 0;
}

static int read_dataset_first_component(hid_t group_id, const char *name, size_t n, double *out){
	hid_t dataset_id, space_id;
	int ndims;
	hsize_t dims[2] = {0, 0};
	double *tmp;
	size_t i;

	dataset_id = H5Dopen2(group_id, name, H5P_DEFAULT);
	if(dataset_id < 0) return -1;
	space_id = H5Dget_space(dataset_id);
	ndims = H5Sget_simple_extent_ndims(space_id);
	if(ndims == 1){
		H5Sget_simple_extent_dims(space_id, dims, NULL);
		H5Sclose(space_id);
		H5Dread(dataset_id, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, out);
		H5Dclose(dataset_id);
		return 0;
	}
	if(ndims != 2){
		H5Sclose(space_id);
		H5Dclose(dataset_id);
		return -1;
	}
	H5Sget_simple_extent_dims(space_id, dims, NULL);
	H5Sclose(space_id);
	if(dims[0] != (hsize_t)n || dims[1] < 1){
		H5Dclose(dataset_id);
		return -1;
	}
	tmp = (double*)Malloc(sizeof(double) * n * dims[1], PPTR(tmp));
	H5Dread(dataset_id, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, tmp);
	H5Dclose(dataset_id);
	for(i=0;i<n;i++) out[i] = tmp[i*dims[1]];
	Free(tmp);
	return 0;
}

static size_t part_count_from_coords(hid_t file_id, const char *group_name){
	hid_t group_id, dataset_id, space_id;
	hsize_t dims[2] = {0, 0};
	size_t n = 0;
	if(H5Lexists(file_id, group_name, H5P_DEFAULT) <= 0) return 0;
	group_id = H5Gopen2(file_id, group_name, H5P_DEFAULT);
	if(group_id < 0) return 0;
	if(H5Lexists(group_id, "Coordinates", H5P_DEFAULT) > 0){
		dataset_id = H5Dopen2(group_id, "Coordinates", H5P_DEFAULT);
		space_id = H5Dget_space(dataset_id);
		H5Sget_simple_extent_dims(space_id, dims, NULL);
		n = (size_t)dims[0];
		H5Sclose(space_id);
		H5Dclose(dataset_id);
	}
	H5Gclose(group_id);
	return n;
}

void gadget_units(RamsesType *ram, GadgetHeaderType *header){
	double a = header->time;
	double h = header->hubble;
	double gamma = 5.0/3.0;
	double mu = 0.588;

	if(h <= 0.0) h = 1.0;
	if(a <= 0.0) a = 1.0;

	ram->scale_l = header->unit_length;
	ram->scale_t = header->unit_length / header->unit_velocity;
	ram->scale_v = header->unit_velocity;
	ram->scale_d = header->unit_mass /
		(header->unit_length * header->unit_length * header->unit_length);
	ram->scale_nH = X/mH * ram->scale_d;
	ram->scale_m = header->unit_mass / Msun * h;
	ram->scale_T2 = (gamma - 1.0) * (header->unit_velocity * header->unit_velocity) * mu * mH / kB;
	ram->mpcscale_l = header->unit_length / Mpc * h;
	ram->kmscale_v = header->unit_velocity / 1.0e5 * sqrt(a);
	ram->scale_Gyr = ram->scale_t / oneyear / 1.0e9;
}

int rd_gadget_info(RamsesType *ram, char *basename, int ifile){
	char filename[512];
	hid_t file_id, header_id;
	GadgetHeaderType header;
	int i;

	memset(&header, 0, sizeof(header));
	header.unit_length = 3.085678e21;
	header.unit_mass = 1.989e43;
	header.unit_velocity = 1.0e5;
	header.hubble = 1.0;
	header.time = 1.0;
	header.nfiles = 1;

	if(open_snapshot_file(basename, ifile, filename, sizeof(filename), &file_id) != 0){
		ERRORPRINT("Cannot open GADGET snapshot from base '%s'\n", basename);
		return -1;
	}

	header_id = H5Gopen2(file_id, "Header", H5P_DEFAULT);
	if(header_id < 0){
		H5Fclose(file_id);
		ERRORPRINT("Cannot open Header in '%s'\n", filename);
		return -1;
	}

	read_attr_ll_array(header_id, "NumPart_ThisFile", header.npart);
	read_attr_double_array(header_id, "MassTable", header.mass);
	read_attr_double(header_id, "Time", &header.time);
	read_attr_double(header_id, "Redshift", &header.redshift);
	read_attr_double(header_id, "BoxSize", &header.boxsize);
	read_attr_int(header_id, "NumFilesPerSnapshot", &header.nfiles);
	read_attr_double(header_id, "Omega0", &header.omega0);
	read_attr_double(header_id, "OmegaLambda", &header.omegalambda);
	read_attr_double(header_id, "HubbleParam", &header.hubble);
	read_attr_int(header_id, "Flag_Sfr", &header.flag_sfr);
	read_attr_int(header_id, "Flag_Cooling", &header.flag_cooling);
	read_attr_int(header_id, "Flag_StellarAge", &header.flag_stellarage);
	read_attr_int(header_id, "Flag_Metals", &header.flag_metals);
	read_attr_double(header_id, "UnitLength_in_cm", &header.unit_length);
	read_attr_double(header_id, "UnitMass_in_g", &header.unit_mass);
	read_attr_double(header_id, "UnitVelocity_in_cm_per_s", &header.unit_velocity);

	H5Gclose(header_id);
	H5Fclose(file_id);

	if(header.nfiles < 1) header.nfiles = 1;
	ram->nfiles = header.nfiles;
	ram->ncpu = header.nfiles;
	ram->ndim = NDIM;
	ram->twotondim = 1;
	for(i=0;i<NDIM;i++) ram->twotondim *= 2;

	ram->aexp = header.time;
	ram->time = header.time;
	ram->H0 = header.hubble * 100.0;
	ram->omega_m = header.omega0;
	ram->omega_l = header.omegalambda;
	ram->cosmo = 1;
	ram->unit_l = header.unit_length;
	ram->unit_t = header.unit_length / header.unit_velocity;
	ram->unit_d = header.unit_mass /
		(header.unit_length * header.unit_length * header.unit_length);
	ram->boxlen_ini = header.boxsize * header.unit_length / Mpc * header.hubble;
	ram->boxlen = ram->boxlen_ini;
	ram->smallr = 1.e-30;

	gadget_units(ram, &header);
	return 0;
}

int rd_gadget_particles(RamsesType *ram, char *basename, int ifile){
	char filename[512];
	hid_t file_id, header_id;
	double mass_table[GADGET_NTYPE];
	size_t ndm, nstar, npart;
	PmType *particle;
	size_t ip, i;
	const int ptypes_dm[3] = {1,2,3};
	const int ptypes_star[1] = {4};
	int it;

	for(i=0;i<GADGET_NTYPE;i++) mass_table[i] = 0.0;

	if(open_snapshot_file(basename, ifile, filename, sizeof(filename), &file_id) != 0){
		ERRORPRINT("Cannot open particle file from base '%s' at file %d\n", basename, ifile);
		return -1;
	}

	header_id = H5Gopen2(file_id, "Header", H5P_DEFAULT);
	if(header_id >= 0){
		read_attr_double_array(header_id, "MassTable", mass_table);
		H5Gclose(header_id);
	}

	ndm = part_count_from_coords(file_id, "PartType1")
		+ part_count_from_coords(file_id, "PartType2")
		+ part_count_from_coords(file_id, "PartType3");
	nstar = part_count_from_coords(file_id, "PartType4");
	npart = ndm + nstar;

	if(npart == 0){
		ram->particle = NULL;
		ram->npart = 0;
		H5Fclose(file_id);
		return 0;
	}

	particle = (PmType*)Malloc(sizeof(PmType) * npart, PPTR(particle));
	ip = 0;

	for(it=0;it<3;it++){
		int ptype = ptypes_dm[it];
		char gname[32];
		hid_t gid;
		size_t n;
		double *x,*y,*z,*vx,*vy,*vz,*m;
		idtype *ids;

		snprintf(gname, sizeof(gname), "PartType%d", ptype);
		n = part_count_from_coords(file_id, gname);
		if(n == 0) continue;

		gid = H5Gopen2(file_id, gname, H5P_DEFAULT);
		x = (double*)Malloc(sizeof(double)*n, PPTR(x));
		y = (double*)Malloc(sizeof(double)*n, PPTR(y));
		z = (double*)Malloc(sizeof(double)*n, PPTR(z));
		vx = (double*)Malloc(sizeof(double)*n, PPTR(vx));
		vy = (double*)Malloc(sizeof(double)*n, PPTR(vy));
		vz = (double*)Malloc(sizeof(double)*n, PPTR(vz));
		m = (double*)Malloc(sizeof(double)*n, PPTR(m));
		ids = (idtype*)Malloc(sizeof(idtype)*n, PPTR(ids));

		read_dataset_vec3(gid, "Coordinates", n, x, y, z);
		read_dataset_vec3(gid, "Velocities", n, vx, vy, vz);
		read_dataset_ids(gid, "ParticleIDs", n, ids);

		if(dataset_exists(gid, "Masses")){
			read_dataset_double_1d(gid, "Masses", n, m);
		}
		else{
			for(i=0;i<n;i++) m[i] = mass_table[ptype];
		}

		for(i=0;i<n;i++){
			particle[ip].x = x[i] * ram->mpcscale_l;
			particle[ip].y = y[i] * ram->mpcscale_l;
			particle[ip].z = z[i] * ram->mpcscale_l;
			particle[ip].vx = vx[i] * ram->kmscale_v;
			particle[ip].vy = vy[i] * ram->kmscale_v;
			particle[ip].vz = vz[i] * ram->kmscale_v;
			particle[ip].mass = m[i] * ram->scale_m;
			particle[ip].id = ids[i];
			particle[ip].levelp = 0;
			particle[ip].family = 1;
			particle[ip].tag = 0;
			particle[ip].tp = 0.0;
			particle[ip].zp = 0.0;
			particle[ip].mass0 = particle[ip].mass;
			particle[ip].birth_d = 0.0;
			particle[ip].partp = 0;
			ip++;
		}

		Free(ids); Free(m); Free(vz); Free(vy); Free(vx); Free(z); Free(y); Free(x);
		H5Gclose(gid);
	}

	for(it=0;it<1;it++){
		int ptype = ptypes_star[it];
		char gname[32];
		hid_t gid;
		size_t n;
		double *x,*y,*z,*vx,*vy,*vz,*m,*tform,*zmet;
		idtype *ids;

		snprintf(gname, sizeof(gname), "PartType%d", ptype);
		n = part_count_from_coords(file_id, gname);
		if(n == 0) continue;

		gid = H5Gopen2(file_id, gname, H5P_DEFAULT);
		x = (double*)Malloc(sizeof(double)*n, PPTR(x));
		y = (double*)Malloc(sizeof(double)*n, PPTR(y));
		z = (double*)Malloc(sizeof(double)*n, PPTR(z));
		vx = (double*)Malloc(sizeof(double)*n, PPTR(vx));
		vy = (double*)Malloc(sizeof(double)*n, PPTR(vy));
		vz = (double*)Malloc(sizeof(double)*n, PPTR(vz));
		m = (double*)Malloc(sizeof(double)*n, PPTR(m));
		tform = (double*)Malloc(sizeof(double)*n, PPTR(tform));
		zmet = (double*)Malloc(sizeof(double)*n, PPTR(zmet));
		ids = (idtype*)Malloc(sizeof(idtype)*n, PPTR(ids));

		read_dataset_vec3(gid, "Coordinates", n, x, y, z);
		read_dataset_vec3(gid, "Velocities", n, vx, vy, vz);
		read_dataset_ids(gid, "ParticleIDs", n, ids);

		if(dataset_exists(gid, "Masses")) read_dataset_double_1d(gid, "Masses", n, m);
		else for(i=0;i<n;i++) m[i] = mass_table[ptype];
		if(dataset_exists(gid, "StellarFormationTime"))
			read_dataset_double_1d(gid, "StellarFormationTime", n, tform);
		else
			for(i=0;i<n;i++) tform[i] = 0.0;
		if(dataset_exists(gid, "Metallicity"))
			read_dataset_first_component(gid, "Metallicity", n, zmet);
		else
			for(i=0;i<n;i++) zmet[i] = 0.0;

		for(i=0;i<n;i++){
			particle[ip].x = x[i] * ram->mpcscale_l;
			particle[ip].y = y[i] * ram->mpcscale_l;
			particle[ip].z = z[i] * ram->mpcscale_l;
			particle[ip].vx = vx[i] * ram->kmscale_v;
			particle[ip].vy = vy[i] * ram->kmscale_v;
			particle[ip].vz = vz[i] * ram->kmscale_v;
			particle[ip].mass = m[i] * ram->scale_m;
			particle[ip].id = ids[i];
			particle[ip].levelp = 0;
			particle[ip].family = 2;
			particle[ip].tag = 0;
			particle[ip].tp = tform[i];
			particle[ip].zp = zmet[i];
			particle[ip].mass0 = particle[ip].mass;
			particle[ip].birth_d = 0.0;
			particle[ip].partp = 0;
			ip++;
		}

		Free(ids); Free(zmet); Free(tform); Free(m); Free(vz); Free(vy); Free(vx); Free(z); Free(y); Free(x);
		H5Gclose(gid);
	}

	ram->particle = particle;
	ram->npart = (int)ip;
	H5Fclose(file_id);
	return ram->npart;
}

int rd_gadget_gas(RamsesType *ram, char *basename, int ifile){
	char filename[512];
	hid_t file_id, header_id, gid;
	double mass_table[GADGET_NTYPE];
	size_t n, i;
	double *x,*y,*z,*vx,*vy,*vz,*m,*rho,*u,*hsml,*zmet;
	GasType *gas;

	for(i=0;i<GADGET_NTYPE;i++) mass_table[i] = 0.0;

	if(open_snapshot_file(basename, ifile, filename, sizeof(filename), &file_id) != 0){
		ERRORPRINT("Cannot open gas file from base '%s' at file %d\n", basename, ifile);
		return -1;
	}

	header_id = H5Gopen2(file_id, "Header", H5P_DEFAULT);
	if(header_id >= 0){
		read_attr_double_array(header_id, "MassTable", mass_table);
		H5Gclose(header_id);
	}

	n = part_count_from_coords(file_id, "PartType0");
	if(n == 0){
		ram->gas = NULL;
		ram->ngas = 0;
		H5Fclose(file_id);
		return 0;
	}

	gid = H5Gopen2(file_id, "PartType0", H5P_DEFAULT);
	x = (double*)Malloc(sizeof(double)*n, PPTR(x));
	y = (double*)Malloc(sizeof(double)*n, PPTR(y));
	z = (double*)Malloc(sizeof(double)*n, PPTR(z));
	vx = (double*)Malloc(sizeof(double)*n, PPTR(vx));
	vy = (double*)Malloc(sizeof(double)*n, PPTR(vy));
	vz = (double*)Malloc(sizeof(double)*n, PPTR(vz));
	m = (double*)Malloc(sizeof(double)*n, PPTR(m));
	rho = (double*)Malloc(sizeof(double)*n, PPTR(rho));
	u = (double*)Malloc(sizeof(double)*n, PPTR(u));
	hsml = (double*)Malloc(sizeof(double)*n, PPTR(hsml));
	zmet = (double*)Malloc(sizeof(double)*n, PPTR(zmet));

	read_dataset_vec3(gid, "Coordinates", n, x, y, z);
	read_dataset_vec3(gid, "Velocities", n, vx, vy, vz);

	if(dataset_exists(gid, "Masses")) read_dataset_double_1d(gid, "Masses", n, m);
	else for(i=0;i<n;i++) m[i] = mass_table[0];

	if(dataset_exists(gid, "Density")) read_dataset_double_1d(gid, "Density", n, rho);
	else for(i=0;i<n;i++) rho[i] = 0.0;

	if(dataset_exists(gid, "InternalEnergy")) read_dataset_double_1d(gid, "InternalEnergy", n, u);
	else for(i=0;i<n;i++) u[i] = 0.0;

	if(dataset_exists(gid, "SmoothingLength")) read_dataset_double_1d(gid, "SmoothingLength", n, hsml);
	else for(i=0;i<n;i++) hsml[i] = 0.0;

	if(dataset_exists(gid, "Metallicity")) read_dataset_first_component(gid, "Metallicity", n, zmet);
	else for(i=0;i<n;i++) zmet[i] = 0.0;

	gas = (GasType*)Malloc(sizeof(GasType)*n, PPTR(gas));
	for(i=0;i<n;i++){
		gas[i].x = x[i] * ram->mpcscale_l;
		gas[i].y = y[i] * ram->mpcscale_l;
		gas[i].z = z[i] * ram->mpcscale_l;
		gas[i].vx = (float)(vx[i] * ram->kmscale_v);
		gas[i].vy = (float)(vy[i] * ram->kmscale_v);
		gas[i].vz = (float)(vz[i] * ram->kmscale_v);
		gas[i].cellsize = hsml[i] * ram->mpcscale_l;
		gas[i].den = rho[i];
		gas[i].temp = (float)(u[i] * ram->scale_T2);
		gas[i].metallicity = (float)zmet[i];
		gas[i].mass = (float)(m[i] * ram->scale_m);
		gas[i].potent = 0.0;
		gas[i].fx = gas[i].fy = gas[i].fz = 0.0;
	}

	ram->gas = gas;
	ram->ngas = (int)n;
	ram->nleafcell = (int)n;

	Free(zmet); Free(hsml); Free(u); Free(rho); Free(m); Free(vz); Free(vy); Free(vx); Free(z); Free(y); Free(x);
	H5Gclose(gid);
	H5Fclose(file_id);
	return ram->ngas;
}

int rd_gadget_bh(RamsesType *ram, char *basename, int ifile){
	char filename[512];
	hid_t file_id, header_id, gid;
	double mass_table[GADGET_NTYPE];
	size_t n, i;
	double *x,*y,*z,*vx,*vy,*vz,*mbh,*mdot;
	idtype *ids;
	SinkType *sink;

	for(i=0;i<GADGET_NTYPE;i++) mass_table[i] = 0.0;

	if(open_snapshot_file(basename, ifile, filename, sizeof(filename), &file_id) != 0){
		ERRORPRINT("Cannot open BH file from base '%s' at file %d\n", basename, ifile);
		return -1;
	}

	header_id = H5Gopen2(file_id, "Header", H5P_DEFAULT);
	if(header_id >= 0){
		read_attr_double_array(header_id, "MassTable", mass_table);
		H5Gclose(header_id);
	}

	n = part_count_from_coords(file_id, "PartType5");
	if(n == 0){
		ram->sink = NULL;
		ram->nsink = 0;
		H5Fclose(file_id);
		return 0;
	}

	gid = H5Gopen2(file_id, "PartType5", H5P_DEFAULT);
	x = (double*)Malloc(sizeof(double)*n, PPTR(x));
	y = (double*)Malloc(sizeof(double)*n, PPTR(y));
	z = (double*)Malloc(sizeof(double)*n, PPTR(z));
	vx = (double*)Malloc(sizeof(double)*n, PPTR(vx));
	vy = (double*)Malloc(sizeof(double)*n, PPTR(vy));
	vz = (double*)Malloc(sizeof(double)*n, PPTR(vz));
	mbh = (double*)Malloc(sizeof(double)*n, PPTR(mbh));
	mdot = (double*)Malloc(sizeof(double)*n, PPTR(mdot));
	ids = (idtype*)Malloc(sizeof(idtype)*n, PPTR(ids));

	read_dataset_vec3(gid, "Coordinates", n, x, y, z);
	read_dataset_vec3(gid, "Velocities", n, vx, vy, vz);
	read_dataset_ids(gid, "ParticleIDs", n, ids);

	if(dataset_exists(gid, "BH_Mass")) read_dataset_double_1d(gid, "BH_Mass", n, mbh);
	else if(dataset_exists(gid, "Masses")) read_dataset_double_1d(gid, "Masses", n, mbh);
	else for(i=0;i<n;i++) mbh[i] = mass_table[5];

	if(dataset_exists(gid, "BH_Mdot")) read_dataset_double_1d(gid, "BH_Mdot", n, mdot);
	else for(i=0;i<n;i++) mdot[i] = 0.0;

	sink = (SinkType*)Malloc(sizeof(SinkType)*n, PPTR(sink));
	for(i=0;i<n;i++){
		sink[i].x = x[i] * ram->mpcscale_l;
		sink[i].y = y[i] * ram->mpcscale_l;
		sink[i].z = z[i] * ram->mpcscale_l;
		sink[i].vx = vx[i] * ram->kmscale_v;
		sink[i].vy = vy[i] * ram->kmscale_v;
		sink[i].vz = vz[i] * ram->kmscale_v;
		sink[i].mass = mbh[i] * ram->scale_m;
		sink[i].tbirth = 0.0;
		sink[i].Jx = sink[i].Jy = sink[i].Jz = 0.0;
		sink[i].Sx = sink[i].Sy = sink[i].Sz = 0.0;
		sink[i].dMsmbh = mdot[i] * ram->scale_m;
		sink[i].dMBH_coarse = 0.0;
		sink[i].dMEd_coarse = 0.0;
		sink[i].Esave = 0.0;
		sink[i].Smag = 0.0;
		sink[i].eps = 0.0;
		sink[i].id = ids[i];
	}

	ram->sink = sink;
	ram->nsink = (int)n;

	Free(ids); Free(mdot); Free(mbh); Free(vz); Free(vy); Free(vx); Free(z); Free(y); Free(x);
	H5Gclose(gid);
	H5Fclose(file_id);
	return ram->nsink;
}

#endif
