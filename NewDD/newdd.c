#include<stdio.h>
#include<stdlib.h>
#include<stddef.h>
#include<string.h>
#include<math.h>

#include "ramses.h"
#include "params.h"
#include "Memory.h"
#include "newdd.h"


int nid=1, myid=0;

#ifdef USE_MPI
#include<mpi.h>

#define GROUPID(a,b) ((a)/(b))
#define RANKINGROUP(a,b) ((a)%(b))

#define Wait2Start(WGroupSize) do{\
	if(RANKINGROUP(myid,WGroupSize) != 0 ) {\
		int iget, src,itag=1; \
		src = myid-1;\
		MPI_Status status; \
		MPI_Recv(&iget,1,MPI_INT,src,itag,MPI_COMM_WORLD,&status);\
	}\
}while(0)

#define Wait2Go(WGroupSize) do{\
	int isend=0,tgt,itag = 1;\
	tgt = myid+1;\
	if(GROUPID(myid,WGroupSize) == GROUPID(tgt,WGroupSize) && tgt < nid) {\
		MPI_Send(&isend,1,MPI_INT,tgt,itag,MPI_COMM_WORLD);\
	}\
} while(0)
#define MIN(a,b) ( (a)<(b)? (a):(b) )

#else
#define Wait2Start(WGroupSize) do{}while(0)
#define Wait2Go(WGroupSize) do{}while(0)

#endif



int main(int argc, char **argv){
	size_t i;
	char infile[190];
	char basename[256];
	RamsesType ram;
	int istep, icpu;
	int WGroupSize = WGROUPSIZE;
	int sinmul,nsplit;
	int ileaf =0;
	int ifile;

	if(argc < 3){
		fprintf(stderr, "Usage: %s <istep> <nsplit>\n", argv[0]);
		return 1;
	}

	istep = atoi(argv[1]);
	nsplit = atoi(argv[2]);
	(void) Make_Total_Memory();
	memset(&ram, 0, sizeof(RamsesType));

#ifdef GADGET_HDF5
	int gadget_found = 1;

#ifdef USE_MPI
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD,&myid);
	MPI_Comm_size(MPI_COMM_WORLD,&nid);
#endif

	if(myid == 0){
		sprintf(basename,"./snapdir_%03d/snapshot_%03d", istep, istep);
		if(rd_gadget_info(&ram, basename, 0) != 0){
			sprintf(basename,"./snapshot_%03d", istep);
			if(rd_gadget_info(&ram, basename, 0) != 0){
				sprintf(basename,"./snap_%04d", istep);
				if(rd_gadget_info(&ram, basename, 0) != 0){
					fprintf(stderr, "Failed to locate snapshot base for step %d\n", istep);
					gadget_found = 0;
				}
			}
		}
	}

#ifdef USE_MPI
	MPI_Bcast(&gadget_found, 1, MPI_INT, 0, MPI_COMM_WORLD);
	if(!gadget_found){
		MPI_Finalize();
		return 2;
	}
	MPI_Bcast(basename, 256, MPI_CHAR, 0, MPI_COMM_WORLD);
	MPI_Bcast(&ram.nfiles, 1, MPI_INT, 0, MPI_COMM_WORLD);
	if(myid != 0){
		MPI_Finalize();
		return 0;
	}
#else
	if(!gadget_found) return 2;
#endif

	sinmul = 0;
	printf("GADGET mode: reading base '%s' with %d file(s)\n", basename, ram.nfiles);
	for(ifile=0;ifile<ram.nfiles;ifile++){
#ifdef DEBUG
		printf("P%d: Reading snapshot file index %d\n", myid, ifile); fflush(stdout);
#endif
		/* Allocations in order: particle, gas, sink */
		rd_gadget_particles(&ram, basename, ifile);
#ifdef DEBUG
		printf("P%d: particles done npart=%d\n", myid, ram.npart); fflush(stdout);
#endif
		rd_gadget_gas(&ram, basename, ifile);
#ifdef DEBUG
		printf("P%d: gas done ngas=%d\n", myid, ram.ngas); fflush(stdout);
#endif
		rd_gadget_bh(&ram, basename, ifile);
#ifdef DEBUG
		printf("P%d: bh done nsink=%d\n", myid, ram.nsink); fflush(stdout);
#endif

		/* SplitDump output — order of writing doesn't matter */
		if(ram.ngas > 0 && ram.gas != NULL){
#ifdef DEBUG
			printf("P%d: qsort GAS %d\n", myid, ram.ngas); fflush(stdout);
#endif
			qsort(ram.gas, ram.ngas, sizeof(GasType), gassortx);
#ifdef DEBUG
			printf("P%d: SplitDump GAS\n", myid); fflush(stdout);
#endif
			SplitDump(&ram, ram.gas, ram.ngas, GAS, istep, ifile+1, sinmul, nsplit);
#ifdef DEBUG
			printf("P%d: GAS done\n", myid); fflush(stdout);
#endif
		}

#ifdef READ_SINK
		if(ram.nsink > 0 && ram.sink != NULL){
#ifdef DEBUG
			printf("P%d: qsort SINK %d\n", myid, ram.nsink); fflush(stdout);
#endif
			qsort(ram.sink, ram.nsink, sizeof(SinkType), sinksortx);
#ifdef DEBUG
			printf("P%d: SplitDump SINK\n", myid); fflush(stdout);
#endif
			SplitDump(&ram, ram.sink, ram.nsink, SINK, istep, ifile+1, sinmul, nsplit);
#ifdef DEBUG
			printf("P%d: SINK done\n", myid); fflush(stdout);
#endif
		}
#endif

		if(ram.npart > 0 && ram.particle != NULL){
			DmType *dm;
			size_t ndm = 0;
			size_t nstar = 0;
			StarType *star;

			/* dm is allocated on top of sink/gas/particle — freed first below */
#ifdef DEBUG
			printf("P%d: Malloc dm npart=%d\n", myid, ram.npart); fflush(stdout);
#endif
			dm = (DmType*)Malloc(sizeof(DmType)*ram.npart, PPTR(dm));
			for(i=0;i<(size_t)ram.npart;i++){
				if((ram.particle)[i].family ==1) {
					dm[ndm] = ((DmType*)(ram.particle))[i];
					ndm ++;
				}
			}
			if(ndm > 0){
#ifdef DEBUG
				printf("P%d: SplitDump DM %zu\n", myid, ndm); fflush(stdout);
#endif
				qsort(dm, ndm, sizeof(DmType), dmsortx);
				SplitDump(&ram, dm, (int)ndm, DM, istep, ifile+1, sinmul, nsplit);
#ifdef DEBUG
				printf("P%d: DM done\n", myid); fflush(stdout);
#endif
			}

			star = (StarType*)dm;
			for(i=0;i<(size_t)ram.npart;i++){
				if((ram.particle)[i].family == 2) {
					star[nstar] = ((StarType*)(ram.particle))[i];
					nstar ++;
				}
			}
			if(nstar > 0){
#ifdef DEBUG
				printf("P%d: SplitDump STAR %zu\n", myid, nstar); fflush(stdout);
#endif
				qsort(star, nstar, sizeof(StarType), starsortx);
				SplitDump(&ram, star, (int)nstar, STAR, istep, ifile+1, sinmul, nsplit);
#ifdef DEBUG
				printf("P%d: STAR done\n", myid); fflush(stdout);
#endif
			}

#ifdef DEBUG
			printf("P%d: Free dm\n", myid); fflush(stdout);
#endif
			Free(dm);  /* dm is on top of the stack — free first */
		}

		/* Free in reverse allocation order (LIFO): sink → gas → particle */
#ifdef DEBUG
		printf("P%d: Free sink/gas/particle\n", myid); fflush(stdout);
#endif
		if(ram.sink != NULL)     { Free(ram.sink);     ram.sink     = NULL; ram.nsink = 0; }
		if(ram.gas != NULL)      { Free(ram.gas);       ram.gas      = NULL; ram.ngas  = 0; }
		if(ram.particle != NULL) { Free(ram.particle);  ram.particle = NULL; ram.npart = 0; }

		printf("Current memory stack %lld\n", CurMemStack());fflush(stdout);
	}

#ifdef USE_MPI
	MPI_Finalize();
#endif
	return 0;

#else
	sprintf(infile,"./output_%.5d/info_%.5d.txt", istep,istep);

#ifdef USE_MPI
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD,&myid);
	MPI_Comm_size(MPI_COMM_WORLD,&nid);
	if(0){
		int kkk = 1;
		while(kkk) {
			kkk = 1;
		}
	}

	if(myid==0) printf("%s is being read\n", infile);


	sinmul = 1;
	if(myid==0) rd_info(&ram, infile);
	MPI_Bcast(&ram, sizeof(RamsesType), MPI_BYTE, 0, MPI_COMM_WORLD); 
	int mystart, myfinal; 
	int nstep = (ram.ncpu+nid-1)/nid; 
	if(ram.ncpu%nid !=0){ 
		printf("Error %d %d\n", nid, ram.ncpu); 
		MPI_Finalize(); 
		exit(999); 
	} 
	mystart = myid*nstep; 
	myfinal = (myid+1)*nstep; 
	mystart = MIN(ram.ncpu, mystart)+1; 
	myfinal = MIN(ram.ncpu, myfinal)+1;
	printf("P%d has ranges %d : %d\n", myid, mystart, myfinal);
	for(icpu=mystart;icpu<myfinal;icpu++)
#else
	sinmul = 0;
	rd_info(&ram, infile);
	for(icpu=1;icpu<=ram.ncpu;icpu++)
#endif
	{
		sprintf(infile,"./output_%.5d/amr_%.5d.out%.5d", istep, istep, icpu);
		Wait2Start(WGroupSize);
		printf("P%d: Opening %s\n", myid, infile);fflush(stdout);
		rd_amr(&ram, infile, NO);
		Wait2Go(WGroupSize);

		printf("P%d stage 1 done \n",myid);

		sprintf(infile,"./output_%.5d/part_%.5d.out%.5d", istep, istep, icpu);
		Wait2Start(WGroupSize);
		printf("P%d: Opening %s\n", myid, infile);fflush(stdout);
		rd_part(&ram, infile);
		Wait2Go(WGroupSize);

		printf("P%d stage 2 done \n",myid);
		
		ram.nleafcell= ileaf;
#ifndef NBODY
		sprintf(infile,"./output_%.5d/hydro_%.5d.out%.5d", istep, istep, icpu);
		Wait2Start(WGroupSize);
		printf("P%d: Opening %s\n", myid, infile);fflush(stdout);
		rd_hydro(&ram, infile);
		Wait2Go(WGroupSize);

		printf("P%d stage 3 done \n",myid);


		printf("P%d stage 4 done \n",myid);

		printf("P%d has scale_d %g\n", myid, ram.scale_d);

		sprintf(infile,"./output_%.5d/LeafCells_%.5d.out%.5d", istep, istep, icpu);
		find_leaf_gas(&ram, icpu, infile);
		qsort(ram.gas, ram.ngas, sizeof(GasType), gassortx);
		SplitDump(&ram, ram.gas, ram.ngas,GAS, istep, icpu,sinmul, nsplit);

		printf("P%d stage 5 done \n",myid);
#ifdef READ_SINK

		if(icpu==1){
			sprintf(infile,"./output_%.5d/sink_%.5d.out00001", istep, istep);
			rd_sink(&ram, infile);
			qsort(ram.sink, ram.nsink, sizeof(SinkType), sinksortx);
			SplitDump(&ram, ram.sink, ram.nsink, SINK, istep, icpu,0, nsplit);
			Free(ram.sink);
		}
#endif

		printf("P%d stage 5-1 done \n",myid);
#endif

		printf("P%d stage 6 done \n",myid);
		{
			DmType *dm = (DmType*)Malloc(sizeof(DmType)*ram.npart, PPTR(dm));
			size_t ndm = 0;
			size_t nstar = 0;
			StarType *star;
			for(i=0;i<(size_t)ram.npart;i++){
				if((ram.particle)[i].family ==1) {
					dm[ndm] = ((DmType*)(ram.particle))[i];
					ndm ++;
				}
			}
			
			qsort(dm, ndm, sizeof(DmType), dmsortx);
			printf("P%d End of sorting %zu dm\n", myid, ndm);
			SplitDump(&ram, dm, (int)ndm, DM, istep, icpu,sinmul, nsplit);

			printf("P%d stage 7 done \n",myid);


			star = (StarType*)dm;
			for(i=0;i<(size_t)ram.npart;i++){
				if((ram.particle)[i].family == 2) {
					star[nstar] = ((StarType*)(ram.particle))[i];
					nstar ++;
				}
			}

			printf("P%d stage 8 done \n",myid);
			qsort(star, nstar, sizeof(StarType), starsortx);
			printf("P%d End of sorting %zu star\n", myid, nstar);
			SplitDump(&ram, star, (int)nstar,STAR, istep, icpu,sinmul, nsplit);
			Free(dm);
	
		}

		printf("P%d stage 9 done \n",myid);
		if(0){
			FILE *wp = fopen("Test.out","w");
			write_head(wp, ram);
			fclose(wp);
		}
		cleanup_ramses(&ram);
		printf("Current memory stack %lld\n", CurMemStack());fflush(stdout);
	}
#ifdef USE_MPI
	MPI_Finalize();
#endif
	return 0;
#endif
}
