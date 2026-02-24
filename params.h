void write_head(FILE *, RamsesType);
RamsesType read_head(FILE*);
#define MAX_LINE_LENGTH 300
#define S_FLOAT   "20f"
#define S_DOUBLE   "20lf"
#define S_INT     "20d"
#define S_LONG     "20ld"
#define S_STRING  "s"
#define SET     "define "

/* Required cosmology for SWIFT simulation family */
#define REQUIRED_OMEGA_M  0.3f
#define REQUIRED_OMEGA_B  0.049f
#define REQUIRED_OMEGA_L  0.7f
#define REQUIRED_H0       67.11f

#define P_Starting  "#Start of the Ascii Header of the RAMSES Simulation\n"
#define P_np        SET"nparticle           = %"S_INT"\n"
#define P_nsink     SET"nsink               = %"S_INT"\n"
#define P_nleafcell SET"nleafcell           = %"S_INT"\n"
#define P_ncpu      SET"ncpu                = %"S_INT"\n"
#define P_myid      SET"myid                = %"S_INT"\n"
#define P_ndim      SET"ndim                = %"S_INT"\n"
#define P_Omep0     SET"OmegaMatter0        = %"S_DOUBLE"\n"
#define P_Omepb0    SET"OmegaBaryon0        = %"S_DOUBLE"\n"
#define P_Omeplamb0 SET"OmegaLambda0        = %"S_DOUBLE"\n"
#define P_Omepk0    SET"OmegaCurvature0     = %"S_DOUBLE"\n"
#define P_nstartot  SET"nstar_tot           = %"S_INT"\n"
#define P_mstartot  SET"mstar_tot           = %"S_DOUBLE"\n"
#define P_mstarlost SET"mstar_lost          = %"S_DOUBLE"\n"
#define P_levelmin  SET"levelmin            = %"S_INT"\n"
#define P_nlevelmax SET"nlevelmax           = %"S_INT"\n"
#define P_nstep_c   SET"nstep_coarse        = %"S_INT"\n"
#define P_nstep_f   SET"nstep_fine          = %"S_INT"\n"
#define P_ngridmax  SET"ngridmax            = %"S_LONG"\n"
#define P_boxsize   SET"BoxSize             = %"S_DOUBLE"\n"
#define P_anow      SET"a(exp. now)         = %"S_DOUBLE"\n"
#define P_ai        SET"a(exp. ini)         = %"S_DOUBLE"\n"
#define P_H0        SET"H0                  = %"S_DOUBLE"\n"
#define P_U_len     SET"Unit(length)        = %"S_DOUBLE"\n"
#define P_U_den     SET"Unit(density)       = %"S_DOUBLE"\n"
#define P_U_tim     SET"Unit(time)          = %"S_DOUBLE"\n"
#define P_nx        SET"nx                  = %"S_INT"\n"
#define P_ny        SET"ny                  = %"S_INT"\n"
#define P_nz        SET"nz                  = %"S_INT"\n"
#define P_TYPESIZE  SET"SIZEOF(RamsesType)  = %"S_INT"\n"
#define P_mass_tot0 SET"mass_tot_0          = %"S_DOUBLE"\n"
#define P_rho_tot   SET"rho_tot             = %"S_DOUBLE"\n"
#define P_S_len     SET"SclFact(cm)         = %"S_DOUBLE"\n"
#define P_S_den     SET"SclFact(gr/cm^3)    = %"S_DOUBLE"\n"
#define P_S_tim     SET"SclFact(second)     = %"S_DOUBLE"\n"
#define P_S_vel     SET"SclFact(cm/second)  = %"S_DOUBLE"\n"
#define P_S_nH      SET"SclFact(nH)         = %"S_DOUBLE"\n"
#define P_S_Tem     SET"SclFact(Kelvin/mu)  = %"S_DOUBLE"\n"
#define P_S_Mpch    SET"SclFact(cMpc/h)     = %"S_DOUBLE"\n"
#define P_S_kms     SET"SclFact(km/second)  = %"S_DOUBLE"\n"
#define P_S_mass    SET"SclFact(Msun/h)     = %"S_DOUBLE"\n"
#define P_X_min     SET"X minimum           = %"S_DOUBLE"\n"
#define P_X_max     SET"X maximum           = %"S_DOUBLE"\n"
#define P_nvar      SET"nvar                = %"S_INT"\n"
#define P_nener     SET"nener               = %"S_INT"\n"
#define P_imetal    SET"imetal              = %"S_INT"\n"
#define P_ichem     SET"ichem               = %"S_INT"\n"
#define P_idust     SET"idust               = %"S_INT"\n"
#define P_inener    SET"inener              = %"S_INT"\n"
#define P_nelt      SET"nelt                = %"S_INT"\n"

#define P_Closing   "#End of Ascii Header\n"
#define P_NULL      "##################################################\n"

#define MPI_DATA(frw,wp,sp,simpar){\
	ncnt += frw(wp,P_ncpu,sp simpar.ncpu);\
 	ncnt += frw(wp,P_myid,sp simpar.icpu);\
}
#define CORE(frw,wp,sp,simpar){\
	ncnt += frw(wp,P_NULL);\
	ncnt += frw(wp,P_Omep0,sp (simpar.omega_m));\
	ncnt += frw(wp,P_Omepb0,sp simpar.omega_b);\
	ncnt += frw(wp,P_Omeplamb0,sp simpar.omega_l);\
	ncnt += frw(wp,P_Omepk0,sp simpar.omega_k);\
	ncnt += frw(wp,P_NULL);\
	ncnt += frw(wp,"### Hubble parameter is in km/sec/Mpc.\n");\
	ncnt += frw(wp,"### Boxsize should be in unit of cMpc/h.\n");\
	ncnt += frw(wp,P_H0,sp simpar.H0);\
	ncnt += frw(wp,P_boxsize,sp simpar.boxlen_ini);\
	ncnt += frw(wp,P_NULL);\
	ncnt += frw(wp,P_anow,sp simpar.aexp);\
	ncnt += frw(wp,P_NULL);\
	ncnt += frw(wp,"### Nx, Ny, and Nz should be same & nspace should be int.\n");\
	ncnt += frw(wp,P_nx,sp simpar.nx);\
	ncnt += frw(wp,P_ny,sp simpar.ny);\
	ncnt += frw(wp,P_nz,sp simpar.nz);\
	ncnt += frw(wp,P_NULL);\
	ncnt += frw(wp,P_nstep_c,sp simpar.nstep_coarse);\
	ncnt += frw(wp,P_nstep_f,sp simpar.nstep);\
	ncnt += frw(wp,P_NULL);\
	ncnt += frw(wp,P_S_len,sp simpar.scale_l);\
	ncnt += frw(wp,P_S_den,sp simpar.scale_d);\
	ncnt += frw(wp,P_S_tim,sp simpar.scale_t);\
	ncnt += frw(wp,P_S_vel,sp simpar.scale_v);\
	ncnt += frw(wp,P_S_nH ,sp simpar.scale_nH);\
	ncnt += frw(wp,P_S_Tem,sp simpar.scale_T2);\
	ncnt += frw(wp,P_S_mass,sp simpar.scale_m);\
	ncnt += frw(wp,P_S_Mpch,sp simpar.mpcscale_l);\
	ncnt += frw(wp,P_S_kms,sp simpar.kmscale_v);\
	ncnt += frw(wp,P_NULL);\
	ncnt += frw(wp,P_nvar,sp simpar.nvar);\
	ncnt += frw(wp,P_nener,sp simpar.nener);\
	ncnt += frw(wp,P_imetal,sp simpar.imetal);\
	ncnt += frw(wp,P_ichem,sp simpar.ichem);\
	ncnt += frw(wp,P_idust,sp simpar.idust);\
	ncnt += frw(wp,P_inener,sp simpar.inener);\
	ncnt += frw(wp,P_NULL);\
	ncnt += frw(wp,P_X_min,sp simpar.xmin);\
	ncnt += frw(wp,P_X_max,sp simpar.xmax);\
	ncnt += frw(wp,P_NULL);\
}
#define DEFAULT_PARAMS(frw,wp,sp,simpar){\
	ncnt += frw(wp,P_Starting);\
	CORE(frw,wp,sp,simpar);\
	ncnt += frw(wp,P_Closing);\
}


#define PARAMS(frw,wp,sp,simpar){\
	ncnt += frw(wp,P_Starting);\
	CORE(frw,wp,sp,simpar);\
	ncnt += frw(wp,P_Closing);\
}

#define FILE_HEADER(frw,wp,sp,simpar) {\
	ncnt += frw(wp,P_Starting);\
	ncnt += frw(wp,P_NULL);\
	MPI_DATA(frw,wp,sp,simpar);\
	ncnt += frw(wp,P_NULL);\
	CORE(frw,wp,sp,simpar);\
	ncnt += frw(wp,P_np,sp simpar.npart);\
	ncnt += frw(wp,P_nsink,sp simpar.nsink);\
	ncnt += frw(wp,P_nleafcell,sp simpar.nleafcell);\
	ncnt += frw(wp,P_NULL);\
	ncnt += frw(wp,P_Closing);\
}
