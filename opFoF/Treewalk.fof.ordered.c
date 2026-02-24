/* This makes Tree structure and walks along tree structures.
 *
 * */
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<math.h>
#include "../ramses.h"
#include "pmheader.h"
#include "mpi.h"
#include "fof.h"
#define IMOD(A,B) ((A) - ((A)/(B))*(B))
#define MIN(A,B) ((A)<(B) ? (A):(B))
#define MAX(A,B) ((A)>(B) ? (A):(B))
POSTYPE Lx2, Ly2,Lz2;
POSTYPE Lx, Ly,Lz;

#define mpi_nchunk 500000000

int BIG_MPI_Send(const void *a, size_t ncount, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm){
	size_t dsize;
	if(datatype == MPI_CHAR){
		dsize = sizeof(char);
	}
	else if(datatype == MPI_BYTE){
		dsize = sizeof(char);
	}
	else if(datatype == MPI_INT){
		dsize = sizeof(int);
	}
	else if(datatype == MPI_FLOAT){
		dsize = sizeof(float);
	}
	else if(datatype == MPI_LONG){
		dsize = sizeof(long);
	}
	else if(datatype == MPI_DOUBLE){
		dsize = sizeof(double);
	}
	else if(datatype == MPI_LONG_LONG){
		dsize = sizeof(long long);
	}
	else if(datatype == MPI_LONG_DOUBLE){
		dsize = sizeof(long double);
	}
	else {
		fprintf(stderr,"Unknown MPI datatype in BIG_MPI_Send\n");
		dsize = 1;
	}
//	printf("sending the Big-size Communication: ncount= %ld dest= %d \n", ncount, dest);
	size_t worktodo = ncount;
	size_t nchunk;
	void *b = (void*)a;
	while(worktodo >0){
		nchunk = MIN(mpi_nchunk, worktodo);
		MPI_Send(b, nchunk, datatype, dest, tag, comm);
		worktodo = worktodo - nchunk;
		b = (void*)((char*)b + nchunk*dsize);
	}
}

int BIG_MPI_Recv(void *a, size_t ncount, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm,
		MPI_Status *status){
	size_t dsize;
	if(datatype == MPI_CHAR){
		dsize = sizeof(char);
	}
	else if(datatype == MPI_BYTE){
		dsize = sizeof(char);
	}
	else if(datatype == MPI_INT){
		dsize = sizeof(int);
	}
	else if(datatype == MPI_FLOAT){
		dsize = sizeof(float);
	}
	else if(datatype == MPI_LONG){
		dsize = sizeof(long);
	}
	else if(datatype == MPI_DOUBLE){
		dsize = sizeof(double);
	}
	else if(datatype == MPI_LONG_LONG){
		dsize = sizeof(long long);
	}
	else if(datatype == MPI_LONG_DOUBLE){
		dsize = sizeof(long double);
	}
	else {
		fprintf(stderr,"Unknown MPI datatype in BIG_MPI_Recv\n");
		dsize = 1;
	}
//	printf("receiving the Big-size Communication: ncount= %ld from= %d \n", ncount, dest);
	size_t worktodo = ncount;
	size_t nchunk;
	void *b = (void*)a;
	while(worktodo >0){
		nchunk = MIN(mpi_nchunk, worktodo);
		MPI_Recv(b, nchunk, datatype, dest, tag, comm, status);
		worktodo = worktodo - nchunk;
		b = (void*)((char*)b + nchunk*dsize);
	}
}

void FWRITE(particle *p,size_t size, size_t np, FILE *wp ){
	DmType *dm;
	GasType *gas;
	SinkType *sink;
	StarType *star;
	void *a = (void *)malloc(size*np);
	size_t i,mp;
	particle *pp;
	dm = (DmType*)a; mp = 0;pp = p;
	for(i=0;i<np;i++){
		if(pp->type==TYPE_DM) dm[mp++] = pp->p.dm;
		pp++;
	}

	gas = (GasType*)(dm+mp); mp = 0;pp = p;
	for(i=0;i<np;i++){
		if(pp->type==TYPE_GAS) gas[mp++] = pp->p.gas;
		pp++;
	}

	sink = (SinkType*)(gas+mp); mp = 0;pp = p;
	for(i=0;i<np;i++){
		if(pp->type==TYPE_SINK) sink[mp++] = pp->p.sink;
		pp++;
	}

	star = (StarType*)(sink+mp); mp = 0;pp = p;
	for(i=0;i<np;i++){
		if(pp->type==TYPE_STAR) star[mp++] = pp->p.star;
		pp++;
	}
	size_t nnp = (char*)(star+mp) - (char*)a;
	fwrite(a, sizeof(char), nnp, wp);
	free(a);
}

void FWRITE2(particle *p,size_t size, size_t np, FILE *wp ){
	DmType *dm;
	GasType *gas;
	SinkType *sink;
	StarType *star;
	void *a = (void *)malloc(size*np);
	size_t i,mp;
	particle *pp;
	dm = (DmType*)a; mp = 0;pp = p;
	for(i=0;i<np;i++){
		if(pp->type==TYPE_DM) dm[mp++] = pp->p.dm;
		pp++;
	}
	fwrite(dm, sizeof(DmType), mp, wp);fflush(wp);

	gas = (GasType*)a; mp = 0;pp = p;
	for(i=0;i<np;i++){
		if(pp->type==TYPE_GAS) gas[mp++] = pp->p.gas;
		pp++;
	}
	fwrite(gas, sizeof(GasType), mp, wp);fflush(wp);

	sink = (SinkType*)a; mp = 0;pp = p;
	for(i=0;i<np;i++){
		if(pp->type==TYPE_SINK) sink[mp++] = pp->p.sink;
		pp++;
	}
	fwrite(sink, sizeof(SinkType), mp, wp);fflush(wp);

	star = (StarType*)a; mp = 0;pp = p;
	for(i=0;i<np;i++){
		if(pp->type==TYPE_STAR) star[mp++] = pp->p.star;
		pp++;
	}
	fwrite(star, sizeof(StarType), mp, wp);fflush(wp);

	free(a);
}
/* open node in periodic boundary conditions */
enum boolean pfof_open(particle p,FoFTStruct *tree, POSTYPE fof_link){
	POSTYPE tmpx,tmpy,tmpz; 
	POSTYPE dist2,dist,r,diff; 
	POSTYPE ratio, link02; 
	tmpx = fabs(p.x-tree->monox);
	tmpy = fabs(p.y-tree->monoy);
	tmpz = fabs(p.z-tree->monoz);
	if(tmpx > Lx2) tmpx = Lx-tmpx;
	if(tmpy > Ly2) tmpy = Ly-tmpy;
	if(tmpz > Lz2) tmpz = Lz-tmpz;
	r = tree->dist;
	dist2 = tmpx*tmpx+tmpy*tmpy+tmpz*tmpz; 
	dist = sqrt(dist2);
	diff = dist - r;
	link02 = 0.5*(p.link02 + tree->maxlink02);
	/*
	link02 = p.link02;
	*/
	if(diff <= link02) return YES;
	else return NO;
}
enum boolean fof_open(particle p,FoFTStruct *tree, POSTYPE fof_link){
	POSTYPE tmpx,tmpy,tmpz; 
	POSTYPE dist2,dist,r,diff; 
	POSTYPE ratio, link02; 
	tmpx = fabs(p.x-tree->monox);
	tmpy = fabs(p.y-tree->monoy);
	tmpz = fabs(p.z-tree->monoz);
	r = tree->dist;
	dist2 = tmpx*tmpx+tmpy*tmpy+tmpz*tmpz; 
	dist = sqrt(dist2);
	diff = dist - r;
	link02 = 0.5*(p.link02 + tree->maxlink02);
	if(diff <= link02) return YES;
	else return NO;
}
static int ntmp;   
static POSTYPE tmpx,tmpy,tmpz,dist2;   
static POSTYPE xx,yy,zz,xy,xz,yz,tmpxx; 
static POSTYPE qxx1,qxx2,qxx3,qxy,tmpdist;   
static POSTYPE fplmf1,fplmf2,fplmf3,fplmf4,tmptmp;   
static POSTYPE fplmf;  
/*
static TStruct *pointer;   
static TPtlStruct *ppointer;   
*/
static POSTYPE ptlmass;
static POSTYPE idist2,sqrtdist2,isqrtdist2; 

/* 
 * *p is the position at which you want to calculate force using tree
 * theta2 is the opening angle for tree walk
 * *tree is the tree structure.
 */
/* cross? is needed to check whether the nearby particle searching crosses 
 * the boundary face. */
enum boolean crossx,crossy,crossz;
#ifdef OLD
void WriteIsolatedHalo(size_t nhalo, HaloBound *halobound,FoFTPtlStruct *ptl,
		particle *linked, char *halofile , char *memparticlefile){
	int i;
	size_t j;
	size_t np;
	HaloQ haloq;
	FILE *hfp,*pfp;
	FoFTPtlStruct *ptr;
	int myid,nid;
	MPI_Comm_rank(MPI_COMM_WORLD,&myid);
	MPI_Comm_size(MPI_COMM_WORLD,&nid);
	fflush(stdout);
	int mid;
#ifdef USE_MASTER
	mid = nid -1;
#else
	mid = nid;
#endif
	for(i=0;i<mid;i++){
		if(i==myid){
			printf("P%d is preparing to write isolated %ld halo data ",myid,nhalo);fflush(stdout);
			hfp=fopen(halofile,"a");
			pfp=fopen(memparticlefile,"a");
			for(j=0;j<nhalo;j++){
				if(halobound[j].boundflag ==0){
					ptr = halobound[j].sibling;
					np = 0;
					while(ptr){
						/*
						linked[np].type = ptr->type;
						linked[np].x = xofP(ptr);
						linked[np].y = yofP(ptr);
						linked[np].z = zofP(ptr);
						linked[np].mass = ptr->mass;
						linked[np].vx = ptr->vx;
						linked[np].vy = ptr->vy;
						linked[np].vz = ptr->vz;
						linked[np].indx = ptr->indx;
						*/
						linked[np] = *ptr;

						ptr = ptr->sibling;
						np++;
					}
					if(np >= MinNumMem){
						haloq=haloproperty(linked,np);
						fwrite(&haloq,sizeof(HaloQ),1,hfp);
						FWRITE(linked,sizeof(particle),np,pfp);
					}
				}
			}
			printf("P%d have written isolated halo data\n",myid);fflush(stdout);
			fclose(hfp);fclose(pfp);
		}
		MPI_Barrier(MPI_COMM_WORLD);
	}
}
#elif USE_MASTER
void WriteIsolatedHalo(size_t nhalo, HaloBound *halobound,FoFTPtlStruct *ptl,
		particle *linked, char *halofile , char *memparticlefile){
	int i;
	size_t j;
	size_t np;
	HaloQ haloq;
	FILE *hfp,*pfp;
	FoFTPtlStruct *ptr;
	int rktag=99,sktag=987;
	int myid,nid;
	int masterid;
	MPI_Comm_rank(MPI_COMM_WORLD,&myid);
	MPI_Comm_size(MPI_COMM_WORLD,&nid);
	masterid = nid -1;
	fflush(stdout);
	if(myid==masterid){
		printf("P%d is preparing to write isolated %ld halo data ",myid,nhalo);fflush(stdout);
		hfp=fopen(halofile,"a");
		pfp=fopen(memparticlefile,"a");
		int ncount = 1;
		while(ncount != nid){
			int ok = 1;
			int src;
			MPI_Status rstatus;
			MPI_Probe(MPI_ANY_SOURCE, sktag,MPI_COMM_WORLD,&rstatus);
			src = rstatus.MPI_SOURCE;
			MPI_Recv(&ok,1,MPI_INT,src,sktag,MPI_COMM_WORLD,&rstatus);
			MPI_Recv(&haloq,sizeof(HaloQ),MPI_BYTE,src,rktag,MPI_COMM_WORLD,&rstatus);
			while(haloq.np !=0){
				BIG_MPI_Recv(linked,haloq.np*sizeof(particle),MPI_BYTE,src,rktag,MPI_COMM_WORLD,&rstatus);
//				MPI_Recv(linked,haloq.np*sizeof(particle),MPI_BYTE,src,rktag,MPI_COMM_WORLD,&rstatus);
				fwrite(&haloq,sizeof(HaloQ),1,hfp);
				FWRITE(linked,sizeof(particle),haloq.np,pfp);
				MPI_Recv(&haloq,sizeof(HaloQ),MPI_BYTE,src,rktag,MPI_COMM_WORLD,&rstatus);
			}
			ncount ++;
		}
		fclose(hfp);
		fclose(pfp);
		printf("P%d have written isolated halo data\n",myid);fflush(stdout);
	}
	else {
		int ok = 1;
		MPI_Send(&ok,1,MPI_INT,0,sktag,MPI_COMM_WORLD);
		for(j=0;j<nhalo;j++){
			if(halobound[j].boundflag ==0){
				ptr = halobound[j].sibling;
				np = 0;
				while(ptr){
					/*
					linked[np].type = ptr->type;
					linked[np].x = xofP(ptr);
					linked[np].y = yofP(ptr);
					linked[np].z = zofP(ptr);
					linked[np].mass = ptr->mass;
					linked[np].vx = ptr->vx;
					linked[np].vy = ptr->vy;
					linked[np].vz = ptr->vz;
					linked[np].indx = ptr->indx;
					*/
					linked[np] = *ptr;
					ptr = ptr->sibling;
					np++;
				}
				if(np >= MinNumMem){
					haloq=haloproperty(linked,np);
					MPI_Send(&haloq,sizeof(HaloQ),MPI_BYTE,0,rktag,MPI_COMM_WORLD);
					BIG_MPI_Send(linked,haloq.np*sizeof(particle),MPI_BYTE,0,rktag,MPI_COMM_WORLD);
//					MPI_Send(linked,haloq.np*sizeof(particle),MPI_BYTE,0,rktag,MPI_COMM_WORLD);
				}
			}
		}
		haloq.np = 0;
		MPI_Send(&haloq,sizeof(HaloQ),MPI_BYTE,0,rktag,MPI_COMM_WORLD);
		printf("P%d have sent all isolated halo data to P0\n",myid);fflush(stdout);
	}
}
#else 
void WriteIsolatedHalo(size_t nhalo, HaloBound *halobound,FoFTPtlStruct *ptl,
		particle *alinked, char *halofile , char *memparticlefile){
	int i;
	size_t j;
	size_t np;
	HaloQ haloq;
	FILE *hfp,*pfp;
	FoFTPtlStruct *ptr;
	int rktag=99,sktag=987;
	int myid,nid;
	MPI_Comm_rank(MPI_COMM_WORLD,&myid);
	MPI_Comm_size(MPI_COMM_WORLD,&nid);
	fflush(stdout);
	size_t nplinked = MaxLinkedParticles;
	particle *linked = (particle*)malloc(sizeof(particle)*nplinked);

	if(myid==0){
		printf("P%d is preparing to write isolated %ld halo data ",myid,nhalo);fflush(stdout);
		hfp=fopen(halofile,"a");
		pfp=fopen(memparticlefile,"a");
		{
			for(j=0;j<nhalo;j++){
				if(halobound[j].boundflag ==0){
					ptr = halobound[j].sibling;
					np = 0;
					while(ptr){
						linked[np] = *ptr;
						ptr = ptr->sibling;
						np++;
						if(np +10 > nplinked){
							nplinked += 1000000;
							linked=(particle*)realloc(linked, sizeof(particle)*nplinked);
						}
					}
					if(np >= MinNumMem){
						haloq=haloproperty(linked,np);
						fwrite(&haloq,sizeof(HaloQ),1,hfp);
						FWRITE(linked,sizeof(particle),np,pfp);
					}
				}
			}
		}

		int ncount = 1;
		while(ncount != nid){
			int ok = 1;
			int src;
			MPI_Status rstatus;
			MPI_Probe(MPI_ANY_SOURCE, sktag,MPI_COMM_WORLD,&rstatus);
			src = rstatus.MPI_SOURCE;
			MPI_Recv(&ok,1,MPI_INT,src,sktag,MPI_COMM_WORLD,&rstatus);
			MPI_Recv(&haloq,sizeof(HaloQ),MPI_BYTE,src,rktag,MPI_COMM_WORLD,&rstatus);
			while(haloq.np !=0){
				if(haloq.np > nplinked) {
					nplinked = haloq.np;
					linked=(particle*)realloc(linked, sizeof(particle)*nplinked);
				}
				BIG_MPI_Recv(linked,haloq.np*sizeof(particle),MPI_BYTE,src,rktag,MPI_COMM_WORLD,&rstatus);
//				MPI_Recv(linked,haloq.np*sizeof(particle),MPI_BYTE,src,rktag,MPI_COMM_WORLD,&rstatus);
				fwrite(&haloq,sizeof(HaloQ),1,hfp);
				FWRITE(linked,sizeof(particle),haloq.np,pfp);
				MPI_Recv(&haloq,sizeof(HaloQ),MPI_BYTE,src,rktag,MPI_COMM_WORLD,&rstatus);
			}
			ncount ++;
		}
		fclose(hfp);
		fclose(pfp);
		printf("P%d have written all the isolated halo data\n",myid);fflush(stdout);
	}
	else {
		int ok = 1;
		MPI_Send(&ok,1,MPI_INT,0,sktag,MPI_COMM_WORLD);
		for(j=0;j<nhalo;j++){
			if(halobound[j].boundflag ==0){
				ptr = halobound[j].sibling;
				np = 0;
				while(ptr){
					linked[np] = *ptr;
					ptr = ptr->sibling;
					np++;
					if(np +10 > nplinked){
						nplinked += 1000000;
						linked=(particle*)realloc(linked, sizeof(particle)*nplinked);
					}
				}
				if(np >= MinNumMem){
					haloq=haloproperty(linked,np);
					MPI_Send(&haloq,sizeof(HaloQ),MPI_BYTE,0,rktag,MPI_COMM_WORLD);
					BIG_MPI_Send(linked,haloq.np*sizeof(particle),MPI_BYTE,0,rktag,MPI_COMM_WORLD);
//					MPI_Send(linked,haloq.np*sizeof(particle),MPI_BYTE,0,rktag,MPI_COMM_WORLD);
				}
			}
		}
		haloq.np = 0;
		MPI_Send(&haloq,sizeof(HaloQ),MPI_BYTE,0,rktag,MPI_COMM_WORLD);
		printf("P%d have sent all isolated halo data to P0\n",myid);fflush(stdout);
	}
	free(linked);
}

#endif
void WriteFinalHalo(size_t nhalo, HaloBound *halobound,FoFTPtlStruct *ptl,
		particle *alinked, char *halofile , char *memparticlefile){
	int i;
	size_t j;
	size_t np;
	HaloQ haloq;
	FILE *hfp,*pfp;
	int myid,nid,mid;
	size_t nplinked = MaxLinkedParticles;

	particle *linked = (particle*)malloc(sizeof(particle)*nplinked);
	FoFTPtlStruct *ptr;
	MPI_Comm_rank(MPI_COMM_WORLD,&myid);
	MPI_Comm_size(MPI_COMM_WORLD,&nid);
#ifdef USE_MASTER
	mid = nid -1;
#else
	mid = nid;
#endif
	for(i=0;i<mid;i++){
		if(i==myid){
			hfp=fopen(halofile,"a");
			pfp=fopen(memparticlefile,"a");
			for(j=0;j<nhalo;j++){
				ptr = halobound[j].sibling;
				np = 0;
				while(ptr){
					/*
					linked[np].type = ptr->type;
					linked[np].x = xofP(ptr);
					linked[np].y = yofP(ptr);
					linked[np].z = zofP(ptr);
					linked[np].mass = ptr->mass;
					linked[np].vx = ptr->vx;
					linked[np].vy = ptr->vy;
					linked[np].vz = ptr->vz;
					linked[np].indx = ptr->indx;
					*/
					linked[np] = *ptr;
					ptr = ptr->sibling;
					np++;
					if(np +10 > nplinked){
						nplinked += 1000000;
						linked=(particle*)realloc(linked, sizeof(particle)*nplinked);
					}

				}
				if(np >= MinNumMem){
					haloq=haloproperty(linked,np);
					fwrite(&haloq,sizeof(HaloQ),1,hfp);
					FWRITE(linked,sizeof(particle),np,pfp);
				}
			}
			printf("written\n");fflush(stdout);
			fclose(hfp);fclose(pfp);
		}
		MPI_Barrier(MPI_COMM_WORLD);
	}
	free(linked);
}

void ReadBottomFaceContact(FoFTPtlStruct *p,size_t npread, particle *alinked,int src,
		int nstep, int nz){
	FoFTPtlStruct *tmp;
	size_t i;
	FILE *fp;
	char infile[190];
	size_t nread;
	int myid,nid;
	int j;
	particle *linked = malloc(sizeof(particle)*MaxLinkedParticles);

	MPI_Comm_rank(MPI_COMM_WORLD,&myid);
	MPI_Comm_size(MPI_COMM_WORLD,&nid);

	int mid;
//#ifdef USE_MASTER
//	mid = nid-1;
//#else
	mid = nid;
//#endif

	sprintf(infile,"./FoF_Garbage/Garb.%.5d/BottomFaceContactHalo.%.5d.%.5d.dat",nstep,src,nstep);
	fp=fopen(infile,"rb");
	if(fp == NULL){
		fp=fopen(infile,"wb");
		if(fp != NULL) fclose(fp);
		fp=fopen(infile,"rb");
	}
	if(fp == NULL){
		fprintf(stderr,"error opening %s\n",infile);
		free(linked);
		return;
	}
//#ifdef OLD
	for(j=0;j<mid;j++){
		if(myid==j)
//#endif
		{
			POSTYPE zoffset;
			if(myid==mid-1) zoffset = nz;
			else zoffset = 0;
			tmp = p;
			while((nread=fread(linked,sizeof(particle),MaxLinkedParticles,fp)) > 0 ){
				for(i=0;i<nread;i++){
					/*
					tmp->type = linked[i].type;
					tmp->x = linked[i].x;
					tmp->y = linked[i].y;
					tmp->z = linked[i].z + zoffset;
					tmp->mass = linked[i].mass;
					tmp->vx = linked[i].vx;
					tmp->vy = linked[i].vy;
					tmp->vz = linked[i].vz;
					tmp->indx = linked[i].indx;
					*/
					*tmp = linked[i];
					tmp++;
				}
			}
			printf("Now read %ld particles from %ld particles\n",tmp-p,npread);
		}
//#ifdef OLD
		MPI_Barrier(MPI_COMM_WORLD);
	}
//#endif
	fclose(fp);
	free(linked);
}

size_t WriteBottomFaceContact(size_t nhalo, HaloBound *halobound,
		FoFTPtlStruct *ptl, particle *alinked,int nowfile,int nstep){
	int i;
	size_t j;
	size_t np;
	HaloQ haloq;
	FILE *hfp,*pfp;
	char bottomboxhalo[80];
	size_t npwrite;
	FoFTPtlStruct *ptr;
	int myid,nid;
	size_t nplinked = MaxLinkedParticles;
	particle *linked = (particle*)malloc(sizeof(particle)*nplinked);
	
	MPI_Comm_rank(MPI_COMM_WORLD,&myid);
	MPI_Comm_size(MPI_COMM_WORLD,&nid);

#ifdef OLD
	int mid;
#ifdef USE_MASTER
	mid = nid -1;
#else
	mid = nid;
#endif
	for(i=0;i<mid;i++){
		if(i==myid)
#endif
		{
			sprintf(bottomboxhalo,"./FoF_Garbage/Garb.%.5d/BottomFaceContactHalo.%.5d.%.5d.dat",nstep,myid,nstep);
			if(nowfile == 0) pfp=fopen(bottomboxhalo,"w");
			else  pfp=fopen(bottomboxhalo,"a");
			npwrite = 0;
			for(j=0;j<nhalo;j++){
				if(halobound[j].boundflag ==1){
					ptr = halobound[j].sibling;
					np = 0;
					while(ptr){
						linked[np] = *ptr;
						ptr = ptr->sibling;
						np++;
						if(np +10 > nplinked){
							nplinked += 1000000;
							linked=(particle*)realloc(linked, sizeof(particle)*nplinked);
						}
					}
					fwrite(linked,sizeof(particle),np,pfp);
					npwrite += np;
				}
			}
			fclose(pfp);
			printf("P%d writing total %ld bottom FoF particles\n",myid,npwrite);
		}
#ifdef OLD
		MPI_Barrier(MPI_COMM_WORLD);
	}
#endif
	free(linked);
	return (npwrite);
}

void WriteAllHalo(size_t nhalo, HaloBound *halobound,FoFTPtlStruct *ptl,
		size_t np, particle *alinked, char *halofile , char *memparticlefile){
	int i;
	size_t j;
	size_t mh,mp;
	HaloQ haloq;
	FILE *hfp,*pfp;
	FoFTPtlStruct *ptr;
	size_t nplinked = MaxLinkedParticles;
	particle *linked = (particle*)malloc(sizeof(particle)*nplinked);
	for(j=0;j<nhalo;j++){
		halobound[j].sibling = NULL;
	}
	for(j=0;j<np;j++){
		mh = ptl[j].haloindx;
		ptr = halobound[mh].sibling;
		halobound[mh].sibling = &(ptl[j]);
		ptl[j].sibling = ptr;
	}
			hfp=fopen(halofile,"a");
			pfp=fopen(memparticlefile,"a");
			for(j=0;j<nhalo;j++){
				ptr = halobound[j].sibling;
				mp = 0;
				while(ptr){
					/*
					linked[mp].type = ptr->type;
					linked[mp].x = xofP(ptr);
					linked[mp].y = yofP(ptr);
					linked[mp].z = zofP(ptr);
					linked[mp].mass = ptr->mass;
					linked[mp].vx = ptr->vx;
					linked[mp].vy = ptr->vy;
					linked[mp].vz = ptr->vz;
					linked[mp].indx = ptr->indx;
					*/
					linked[mp] = *ptr;
					ptr = ptr->sibling;
					mp++;
					if(mp +10 > nplinked){
						nplinked += 1000000;
						linked=(particle*)realloc(linked, sizeof(particle)*nplinked);
					}
				}
				if(mp >= MinNumMem){
					haloq=haloproperty(linked,mp);
					fwrite(&haloq,sizeof(HaloQ),1,hfp);
					FWRITE(linked,sizeof(particle),mp,pfp);
				}
			}
			fclose(hfp);fclose(pfp);
			free(linked);
}
size_t StackUpContactParticleLeftWard(size_t nhalo,HaloBound *halobound,
		FoFTPtlStruct *ptl,size_t np){
	size_t nowp,i;
	FoFTPtlStruct *ptr;
	size_t mh;
	nowp = 0;
	for(i=0;i<np;i++){
		if(halobound[ptl[i].haloindx].boundflag ==2 ||
				halobound[ptl[i].haloindx].boundflag ==3){
			ptl[nowp] = ptl[i];
			nowp ++;
		}
	}
	return (nowp);
}

void CheckHaloBound(size_t nhalo,HaloBound *halo,FoFTPtlStruct *ptl,size_t np,
		POSTYPE fof_link,POSTYPE zdown,POSTYPE zup, POSTYPE zminlocal){
	size_t i,mh;
	FoFTPtlStruct *tmp;
	size_t maxnmem;

	for(i=0;i<nhalo;i++){
		halo[i].zmin = 2.E23;
		halo[i].zmax = -2.E23;
		halo[i].boundflag = 0;
		halo[i].sibling = NULL;
		halo[i].nmem = 0;
	}
	for(i=0;i<np;i++){
		mh = ptl[i].haloindx;
		halo[mh].nmem++;
		tmp = halo[mh].sibling;
		halo[mh].sibling = &(ptl[i]);
		ptl[i].sibling = tmp;
		halo[mh].zmin = MIN(halo[mh].zmin,ptl[i].z);
		halo[mh].zmax = MAX(halo[mh].zmax,ptl[i].z);
	}
	maxnmem = 0;
	for(i=0;i<nhalo;i++){
		if(halo[i].zmin <= zminlocal+fof_link) halo[i].boundflag ++;
		if(halo[i].zmax >= zup-fof_link)  halo[i].boundflag += 2;
		maxnmem = MAX(maxnmem,halo[i].nmem);
	}
	printf("Maximum FoF member of particles %ld\n",maxnmem);
	{
		size_t num1,num2,num3,num0;
		num1=num2=num3=num0=0;
		for(i=0;i<nhalo;i++){
			if(halo[i].boundflag == 0) num0 ++;
			else if(halo[i].boundflag ==1) num1 ++;
			else if(halo[i].boundflag ==2) num2 ++;
			else if(halo[i].boundflag ==3) num3 ++;
		}
		printf("num0=%ld num1=%ld num2=%ld num3=%ld\n",num0,num1,num2,num3);
	}
}
/* lx,ly, lz are the limits of the box */
size_t pnew_fof_link(particle *p,POSTYPE fof_link,FoFTStruct *TREE,
		FoFTPtlStruct *ptl,particle *linked,size_t nhalo,
		POSTYPE lx,POSTYPE ly, POSTYPE lz){
	size_t ncount, now;
	void *ptr,*optr,*nptr;
	POSTYPE tmpx,tmpy,tmpz;
	POSTYPE fof_link2,dist2;
	POSTYPE Lpx,Lpy,Lpz;
	particle point;
	particle *alinked;
	Lx = lx; Ly = ly; Lz = lz;
	Lx2 = Lx*0.5;
	Ly2 = Ly*0.5;
	Lz2 = Lz*0.5;
	fof_link2 = fof_link*fof_link;
	ncount = now = 0;
	point.x = p->x;
	point.y = p->y;
	point.z = p->z;
	point.link02 = p->link02;
	if(0){
		int kkk =1 ;
		while(kkk == 1) {
			kkk = 1;
		}
	}
	FoFTStruct *pnode;
	FoFTPtlStruct *pptl;

	/*
	size_t maxlinked = MaxLinkedParticles;
	linked = (particle*)malloc(sizeof(particle)*maxlinked);
	*/
	do {
		optr = (void *) TREE;
		ptr = (void*) TREE;
		while(ptr != NULL){
			switch(((TYPE*)ptr)->type){
				case TYPE_TREE:
					pnode = (FoFTStruct*)ptr;
					if(pnode->sibling == pnode->daughter){
  						EraseFromTree(optr,ptr,pnode->sibling);
						ptr = pnode->sibling;
					}
					else
					switch(pfof_open(point,ptr,fof_link)){
						case YES:
							optr = ptr;
							ptr = (void *)(pnode->daughter);
							break;
						default :
							optr = ptr;
							ptr = (void *)(pnode->sibling);
					}
					break;
				default :
					pptl = (FoFTPtlStruct*)ptr;;
					if(pptl->included == YES){
//						fprintf(stderr,"Double counting ?\n");
						nptr = pptl->sibling;
  						EraseFromTree(optr,ptr,nptr);
						ptr = nptr;
					}
					else {
						tmpx = fabs(point.x - pptl->x);
						if(tmpx>Lx2) tmpx = Lx-tmpx;

						tmpy = fabs(point.y - pptl->y);
						if(tmpy>Ly2) tmpy = Ly-tmpy;

						tmpz = fabs(point.z - pptl->z);
						if(tmpz>Lz2) tmpz = Lz-tmpz;

						dist2 = tmpx*tmpx + tmpy*tmpy + tmpz*tmpz;
						dist2 = sqrt(dist2);
						if(dist2 <= 0.5*(point.link02+pptl->link02)){
							linked[ncount] = *pptl;
							ncount ++;
							pptl->haloindx = nhalo;
							pptl->included = YES;
							nptr = pptl->sibling;
  							EraseFromTree(optr,ptr,nptr);
						}
						else optr = ptr;

						ptr = (void*)(pptl->sibling);
					}
			}
		}
		point = linked[now];
		/* for periodic boundary conditions in x,y, and z*/
		now ++;
	} while( now <= ncount);
//	free(linked);
	return (ncount);
}
size_t new_fof_link(particle *p,POSTYPE fof_link,FoFTStruct *tree,
		FoFTPtlStruct *ptl,particle *linked,size_t nhalo){
	size_t ncount, now;
	void *ptr,*optr,*nptr;
	POSTYPE tmpx,tmpy,tmpz;
	POSTYPE fof_link2,dist2;
	POSTYPE Lpx,Lpy,Lpz;
	particle point;
	particle *alinked;
	fof_link2 = fof_link*fof_link;
	ncount = now = 0;
	point.x = p->x;
	point.y = p->y;
	point.z = p->z;
	point.link02 = p->link02;
	/*
	size_t maxlinked = MaxLinkedParticles;
	linked = (particle*)malloc(sizeof(particle)*maxlinked);
	*/
	do {
		optr = (void *) tree;
		ptr = (void*) tree;
		while(ptr != NULL){
			switch(((TYPE*)ptr)->type){
				case TYPE_TREE:
					if(((FoFTStruct *)ptr)->sibling ==
							((FoFTStruct *)ptr)->daughter){
						EraseFromTree(optr,ptr,((FoFTStruct *)ptr)->sibling);
						ptr = ((FoFTStruct *)ptr)->sibling;
					}
					else
					switch(fof_open(point,ptr,fof_link)){
						case YES:
							optr = ptr;
							ptr = (void *)(((FoFTStruct*)ptr)->daughter);
							break;
						default :
							optr = ptr;
							ptr = (void *)(((FoFTStruct*)ptr)->sibling);
					}
					break;
				default :
					if(((FoFTPtlStruct*)ptr)->included == YES){
						nptr = ((FoFTPtlStruct *)ptr)->sibling;
						EraseFromTree(optr,ptr,nptr);
						ptr = nptr;
					}
					else {
						tmpx = fabs(point.x - ((FoFTPtlStruct*)ptr)->x);
						tmpy = fabs(point.y - ((FoFTPtlStruct*)ptr)->y);
						tmpz = fabs(point.z - ((FoFTPtlStruct*)ptr)->z);

						dist2 = tmpx*tmpx + tmpy*tmpy + tmpz*tmpz;
						dist2 = sqrt(dist2);
						if(dist2 <= 0.5*(point.link02+((FoFTPtlStruct*)ptr)->link02)){
							linked[ncount] = *(FoFTPtlStruct*)ptr;
							((FoFTPtlStruct*)ptr)->haloindx = nhalo;
							((FoFTPtlStruct*)ptr)->included = YES;
							ncount ++;
							/*
							if(ncount+10 >= maxlinked) {
								maxlinked += 1000000;
								linked = (particle*)realloc(linked, sizeof(particle)*maxlinked);
							}
							*/
							nptr = ((FoFTPtlStruct *)ptr)->sibling;
							EraseFromTree(optr,ptr,nptr);
						}
						else optr = ptr;
						ptr = (void*)(((FoFTPtlStruct*)ptr)->sibling);
					}
			}
		}
		point = linked[now];
		/* for periodic boundary conditions in x,y, and z*/
		now ++;
	} while( now <= ncount);
//	free(linked);
	return (ncount);
}
void FoF_Make_Tree(FoFTStruct *TREE_START,FoFTPtlStruct *ptl,size_t np,Box box){
	FoFBeginEndTree beginend;
	FoFTStruct *NewTree;
	FoFTPtlStruct *ptr;
	ptr = ptl;
	while(ptr != NULL){
		ptr->included = NO;
		ptr = ptr->sibling;
	}
	TREE_START->sibling = NULL;
	beginend = FoF_divide_node(TREE_START,TREE_START+1,ptl,box,TREE_START);
}
FoFBeginEndTree FoF_divide_node(FoFTStruct *TREE_START,FoFTStruct *NewTree, 
		FoFTPtlStruct *ptl, Box box,FoFTStruct *ThisTree){ 
	FoFBeginEndTree beginend;
	FoFTStruct *p2tree,tmpnode[8];
	FoFTStruct *NowCal;
	void *from_sibling;
	FoFTPtlStruct *p2ptl,*tmpptr,*tmpptr2;
	Box tmpbox[8];
	int i,j,k,mnode,mx,my,mz;
	POSTYPE x0,y0,z0,inv_halfw,halfw;
	POSTYPE tmpx,tmpy,tmpz,tmpdist2,distmax;
	POSTYPE ptlmass;
	int count;
	ThisTree->type = TYPE_TREE;
	/*
	ThisTree->sibling = NULL;
	*/
	ThisTree->daughter = NULL;
	/*
	ThisTree->L = box.width;
	ThisTree->x0 = box.x;
	ThisTree->y0 = box.y;
	ThisTree->z0 = box.z;
	*/
	ThisTree->Nparticle = 0;
	ThisTree->monox= ThisTree->monoy= ThisTree->monoz= 0.;

	p2ptl = ptl;
	while(p2ptl != NULL){
		ThisTree->Nparticle ++;
		ThisTree->monox += p2ptl->x;
		ThisTree->monoy += p2ptl->y;
		ThisTree->monoz += p2ptl->z;
		p2ptl = p2ptl->sibling;
	}
	ThisTree->monox /= ThisTree->Nparticle;
	ThisTree->monoy /= ThisTree->Nparticle;
	ThisTree->monoz /= ThisTree->Nparticle;
	distmax = -1.E20;
	p2ptl = ptl;
	ThisTree->maxlink02 = 0;
	while(p2ptl != NULL){
		tmpx = p2ptl->x - ThisTree->monox;
		tmpy = p2ptl->y - ThisTree->monoy;
		tmpz = p2ptl->z - ThisTree->monoz;
		tmpdist2 = tmpx*tmpx + tmpy*tmpy + tmpz*tmpz;
		distmax = MAX(distmax,tmpdist2);
		ThisTree->maxlink02 = MAX(ThisTree->maxlink02,p2ptl->link02);
		p2ptl = p2ptl->sibling;
	}
	/*
	ThisTree->dist2 = distmax;
	*/
	ThisTree->dist = sqrt(distmax);
	if(1){
		x0 = box.x;
		y0 = box.y;
		z0 = box.z;
		halfw = box.width*0.5L;
	}
	else {
		x0 = ThisTree->monox - ThisTree->dist;
		y0 = ThisTree->monoy - ThisTree->dist;
		z0 = ThisTree->monoz - ThisTree->dist;
		halfw = ThisTree->dist;
	}


	inv_halfw = 1.L/halfw;
	/* initialize temporary tree array */
	for(i=0;i<8;i++) {
		tmpnode[i].sibling = tmpnode[i].daughter = NULL;
		tmpnode[i].Nparticle = 0;
		tmpbox[i].width = halfw;
		tmpbox[i].x = x0+(i%2)*halfw;
		tmpbox[i].y = y0+((i%4)/2)*halfw;
		tmpbox[i].z = z0+(i/4)*halfw;
	}
	p2ptl = ptl;
	while(p2ptl != NULL){
		mx = my = mz = 0;
		if(1){
			if(p2ptl->x >= x0+halfw) mx = 1;
			if(p2ptl->y >= y0+halfw) my = 1;
			if(p2ptl->z >= z0+halfw) mz = 1;
		}
		else {
			if(p2ptl->x >= ThisTree->monox) mx = 1;
			if(p2ptl->y >= ThisTree->monoy) my = 1;
			if(p2ptl->z >= ThisTree->monoz) mz = 1;
		}
	
		mnode = mx + 2*my + 4*mz;
		tmpnode[mnode].Nparticle ++; 
		tmpptr = tmpnode[mnode].daughter;
		tmpptr2 = p2ptl->sibling;
		tmpnode[mnode].daughter = p2ptl;
		p2ptl->sibling = tmpptr;
		p2ptl = tmpptr2;
	}
	/* Making a link from the Mother Node */
	for(i=0;i<8;i++){
		if(tmpnode[i].Nparticle > 0) break;
	}
	if(tmpnode[i].Nparticle >= NODE_HAVE_PARTICLE && ThisTree->dist > MINCELLSIZE){
		ThisTree->daughter = (void *)(NewTree);
	}
	else {
		ThisTree->daughter = (void *) tmpnode[i].daughter ;
	}
	count = 0;
	NowCal = NewTree;
	from_sibling = NULL;
	for(i=0;i<8;i++){
		if(tmpnode[i].Nparticle >= NODE_HAVE_PARTICLE && ThisTree->dist > MINCELLSIZE){
			NewTree->daughter = tmpnode[i].daughter;
			if(from_sibling != NULL) 
				((GENERAL_TPtl_POINTER*)from_sibling)->sibling = NewTree;
			from_sibling = NewTree;
			NewTree++;
			count ++;
		}
		else if(tmpnode[i].Nparticle > 0 ){
			tmpptr = tmpnode[i].daughter;
			if(from_sibling != NULL) 
				((GENERAL_TPtl_POINTER*)from_sibling)->sibling = tmpptr;
			while(tmpptr != NULL){
				from_sibling = tmpptr;
				tmpptr = tmpptr->sibling;
			}
		}
	}
	((GENERAL_TPtl_POINTER*)from_sibling)->sibling = ThisTree->sibling;
	for(i=0;i<8;i++){
		if(tmpnode[i].Nparticle >= NODE_HAVE_PARTICLE && ThisTree->dist > MINCELLSIZE){
			beginend = FoF_divide_node(TREE_START,NewTree,
					tmpnode[i].daughter, tmpbox[i],NowCal);
			NewTree = beginend.start;
			NowCal ++;
		}
	}
	beginend.start = NewTree;
	return beginend;
}
void FoF_Make_Tree2D(FoFTStruct *TREE_START,FoFTPtlStruct *ptl,size_t np,Box box){
	FoFBeginEndTree beginend;
	FoFTStruct *NewTree;
	FoFTPtlStruct *ptr;
	ptr = ptl;
	while(ptr != NULL){
		ptr->included = NO;
		ptr = ptr->sibling;
	}
	TREE_START->sibling = NULL;
	beginend = FoF_divide_node2D(TREE_START,TREE_START+1,ptl,box,TREE_START);
	printf("Total number of tree cell: %ld\n", beginend.start-TREE_START);
}
FoFBeginEndTree FoF_divide_node2D(FoFTStruct *TREE_START,FoFTStruct *NewTree, 
		FoFTPtlStruct *ptl, Box box,FoFTStruct *ThisTree){ 
	FoFBeginEndTree beginend;
	FoFTStruct *p2tree,tmpnode[2];
	FoFTStruct *NowCal;
	void *from_sibling;
	FoFTPtlStruct *p2ptl,*tmpptr,*tmpptr2;
	Box tmpbox[2];
	int i,j,k,mnode,mx,my,mz;
	POSTYPE x0,y0,z0,inv_halfw,halfw;
	POSTYPE tmpx,tmpy,tmpz,tmpdist2,distmax;
	POSTYPE ptlmass;
	int count;
	ThisTree->type = TYPE_TREE;
	ThisTree->daughter = NULL;
	ThisTree->Nparticle = 0;
	ThisTree->monox= ThisTree->monoy= ThisTree->monoz= 0.;

	p2ptl = ptl;
	while(p2ptl != NULL){
		ThisTree->Nparticle ++;
		ThisTree->monox += p2ptl->x;
		ThisTree->monoy += p2ptl->y;
		ThisTree->monoz += p2ptl->z;
		p2ptl = p2ptl->sibling;
	}
	ThisTree->monox /= ThisTree->Nparticle;
	ThisTree->monoy /= ThisTree->Nparticle;
	ThisTree->monoz /= ThisTree->Nparticle;
	distmax = -1.E20;
	p2ptl = ptl;
	ThisTree->maxlink02 = 0;
	POSTYPE xw,yw,zw;
	while(p2ptl != NULL){
		tmpx = p2ptl->x - ThisTree->monox;
		tmpy = p2ptl->y - ThisTree->monoy;
		tmpz = p2ptl->z - ThisTree->monoz;
		tmpdist2 = tmpx*tmpx + tmpy*tmpy + tmpz*tmpz;
		xw = MAX(tmpx*tmpx,xw);
		yw = MAX(tmpy*tmpy,yw);
		zw = MAX(tmpz*tmpz,zw);
		distmax = MAX(distmax,tmpdist2);
		ThisTree->maxlink02 = MAX(ThisTree->maxlink02,p2ptl->link02);
		p2ptl = p2ptl->sibling;
	}
	xw = sqrt(xw);
	yw = sqrt(yw);
	zw = sqrt(zw);
	ThisTree->dist = sqrt(distmax);

	x0 = ThisTree->monox - ThisTree->dist;
	y0 = ThisTree->monoy - ThisTree->dist;
	z0 = ThisTree->monoz - ThisTree->dist;
	halfw = ThisTree->dist;


	inv_halfw = 1.L/halfw;
	/* initialize temporary tree array */
	for(i=0;i<2;i++) {
		tmpnode[i].sibling = tmpnode[i].daughter = NULL;
		tmpnode[i].Nparticle = 0;
		tmpbox[i].width = halfw;
		tmpbox[i].x = x0;
		tmpbox[i].y = y0;
		tmpbox[i].z = z0;
	}
	if(xw >= yw && xw>=zw) {
		tmpbox[1].x = x0+xw*0.5L;
		p2ptl = ptl;
		while(p2ptl != NULL){
			mnode = 0;
			if(p2ptl->x >= ThisTree->monox) mnode = 1;
			tmpnode[mnode].Nparticle ++; 
			tmpptr = tmpnode[mnode].daughter;
			tmpptr2 = p2ptl->sibling;
			tmpnode[mnode].daughter = p2ptl;
			p2ptl->sibling = tmpptr;
			p2ptl = tmpptr2;
		}
	}
	else if(yw >=xw && yw >=zw){
		tmpbox[1].y = y0+yw*0.5L;
		p2ptl = ptl;
		while(p2ptl != NULL){
			mnode = 0;
			if(p2ptl->y >= ThisTree->monoy) mnode = 1;
			tmpnode[mnode].Nparticle ++; 
			tmpptr = tmpnode[mnode].daughter;
			tmpptr2 = p2ptl->sibling;
			tmpnode[mnode].daughter = p2ptl;
			p2ptl->sibling = tmpptr;
			p2ptl = tmpptr2;
		}
	}
	else {
		tmpbox[1].z = z0+zw*0.5L;
		p2ptl = ptl;
		while(p2ptl != NULL){
			mnode = 0;
			if(p2ptl->z >= ThisTree->monoz) mnode = 1;
			tmpnode[mnode].Nparticle ++; 
			tmpptr = tmpnode[mnode].daughter;
			tmpptr2 = p2ptl->sibling;
			tmpnode[mnode].daughter = p2ptl;
			p2ptl->sibling = tmpptr;
			p2ptl = tmpptr2;
		}
	}
	/* Making a link from the Mother Node */
	for(i=0;i<2;i++){
		if(tmpnode[i].Nparticle > 0) break;
	}
	if(tmpnode[i].Nparticle >= NODE_HAVE_PARTICLE && ThisTree->dist > MINCELLSIZE){
		ThisTree->daughter = (void *)(NewTree);
	}
	else {
		ThisTree->daughter = (void *) tmpnode[i].daughter ;
	}
	count = 0;
	NowCal = NewTree;
	from_sibling = NULL;
	for(i=0;i<2;i++){
		if(tmpnode[i].Nparticle >= NODE_HAVE_PARTICLE && ThisTree->dist > MINCELLSIZE){
			NewTree->daughter = tmpnode[i].daughter;
			if(from_sibling != NULL) 
				((GENERAL_TPtl_POINTER*)from_sibling)->sibling = NewTree;
			from_sibling = NewTree;
			NewTree++;
			count ++;
		}
		else if(tmpnode[i].Nparticle > 0 ){
			tmpptr = tmpnode[i].daughter;
			if(from_sibling != NULL) 
				((GENERAL_TPtl_POINTER*)from_sibling)->sibling = tmpptr;
			while(tmpptr != NULL){
				from_sibling = tmpptr;
				tmpptr = tmpptr->sibling;
			}
		}
	}
	((GENERAL_TPtl_POINTER*)from_sibling)->sibling = ThisTree->sibling;
	for(i=0;i<2;i++){
		if(tmpnode[i].Nparticle >= NODE_HAVE_PARTICLE && ThisTree->dist > MINCELLSIZE){
			beginend = FoF_divide_node2D(TREE_START,NewTree,
					tmpnode[i].daughter, tmpbox[i],NowCal);
			NewTree = beginend.start;
			NowCal ++;
		}
	}
	beginend.start = NewTree;
	return beginend;
}
