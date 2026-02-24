#include<stdio.h>
#include<stdlib.h>
#include<stddef.h>
#include<string.h>
#include<math.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<unistd.h>
#include<fcntl.h>
#include<errno.h>

#include "ramses.h"
#include "params.h"
#include "Memory.h"

#define min(a,b) ( (a) < (b) ? (a):(b))
#define max(a,b) ( (a) > (b) ? (a):(b))

#define GROUPID(a,b) ((a)/(b))
#define RANKINGROUP(a,b) ((a)%(b))


#ifdef USE_MPI
#include<mpi.h>
/*
extern int nid, myid;
*/
#define Wait2Start(WGroupSize) do{\
	if(RANKINGROUP(myid,WGroupSize) != 0 ) {\
		int iget, src,itag=1; \
		src = myid-1;\
		MPI_Status status; \
		MPI_Recv(&iget,1,MPI_INT,src,itag,MPI_COMM_WORLD,&status);\
	}\
}while(0)

#define Wait2Go(WGroupSize) do{\
	int isend,tgt,itag = 1;\
	tgt = myid+1;\
	if(GROUPID(myid,WGroupSize) == GROUPID(tgt,WGroupSize) && tgt < nid)\
	MPI_Send(&isend,1,MPI_INT,tgt,itag,MPI_COMM_WORLD);\
} while(0)

#endif

int dmsortx(const void *aa, const void *bb){
	dptype ai, bi;
	DmType *a = (DmType *)aa;
	DmType *b = (DmType *)bb;
	ai = a->x;
	bi = b->x;
	if( ai > bi) return 1;
	else if( ai < bi) return -1;
	else return 0;

}
int starsortx(const void *aa, const void *bb){
	dptype ai, bi;
	StarType *a = (StarType *)aa;
	StarType *b = (StarType *)bb;
	ai = a->x;
	bi = b->x;
	if( ai > bi) return 1;
	else if( ai < bi) return -1;
	else return 0;

}
int agnsortid(const void *aa, const void *bb){
	idtype ai, bi;
	AGNType *a = (AGNType *)aa;
	AGNType *b = (AGNType *)bb;
	ai = a->id;
	bi = b->id;
	if( ai > bi) return 1;
	else if( ai < bi) return -1;
	else return 0;
}
int sinksortid(const void *aa, const void *bb){
	idtype ai, bi;
	SinkType *a = (SinkType *)aa;
	SinkType *b = (SinkType *)bb;
	ai = a->id;
	bi = b->id;
	if( ai > bi) return 1;
	else if( ai < bi) return -1;
	else return 0;
}
int sinksortx(const void *aa, const void *bb){
	dptype ai, bi;
	SinkType *a = (SinkType *)aa;
	SinkType *b = (SinkType *)bb;
	ai = a->x;
	bi = b->x;
	if( ai > bi) return 1;
	else if( ai < bi) return -1;
	else return 0;

}

int hcellsortx(const void *aa, const void *bb){
	dptype ai, bi;
	HydroCellType *a = (HydroCellType *)aa;
	HydroCellType *b = (HydroCellType *)bb;
	ai = a->x;
	bi = b->x;
	if( ai > bi) return 1;
	else if( ai < bi) return -1;
	else return 0;
}

int gassortx(const void *aa, const void *bb){
	dptype ai, bi;
	GasType *a = (GasType *)aa;
	GasType *b = (GasType *)bb;
	ai = a->x;
	bi = b->x;
	if( ai > bi) return 1;
	else if( ai < bi) return -1;
	else return 0;
}

void rek_mkdir(char *path) {
    char *sep = strrchr(path, '/');
    if(sep != NULL) {
        *sep = 0;
        rek_mkdir(path);
        *sep = '/';
    }
    if(mkdir(path, 0755) && errno != EEXIST)
        printf("error while trying to create '%s'\n%m\n", path);
}

void fopen_mkdir(char *path, char *mode) {
    char *sep = strrchr(path, '/');
    if(sep) {
        char *path0 = strdup(path);
        path0[ sep - path ] = 0;
        rek_mkdir(path0);
        free(path0);
    }
    return;
}
void mkfolder(char *dir){
	struct stat sb;
	if(stat(dir,&sb) != 0 || !S_ISDIR(sb.st_mode)){
		/*
		fopen_mkdir(dir,"rw");
		*/
		char command[190];
		sprintf(command,"mkdir -p %s",dir);
		system(command);
	}
}
/*
int mkdirr(const char *, const mode_t, const int);
void mkfolder(char *dir){
	if (-1 == mkdirr(dir, 0777, 0)) {
    perror("mkdirr() failed() %s");
  }
}
#define PATH_MAX 1000000
int mkdirr(const char * path, const mode_t mode, const int fail_on_exist) {
  int result = 0;
  char * dir = NULL;

  do
  {
    if (NULL == path)
    {
      errno = EINVAL;
      result = -1;
      break;
    }

    if ((dir = strrchr(path, '/')))
    {
      *dir = '\0';
      result = mkdirr(path, mode, fail_on_exist);
      *dir = '/';

      if (result)
      {
        break;
      }
    }

    if (strlen(path))
    {
      if ((result = mkdir(path, mode)))
      {
        char s[PATH_MAX];
        sprintf(s, "mkdir() failed for '%s'", path);
        perror(s);

        if ((EEXIST == result) && (0 == fail_on_exist))
        {
          result = 0;
        }
        else
        {
          break;
        }
      }
    }
  } while (0);

  return result;
}
*/

void SplitDump(RamsesType *ram, const void *aa, int np, int type, int istep, int icpu, int sinmul,
		int nsplit){
	int ipos[nsplit], jpos[nsplit];
	dptype xpos[nsplit+1];
	char dir[1000];

	int myid=0,nid=1;

	dptype x0, x1, step;
	x0 = 0;
	x1 = ram->boxlen_ini;
	
	step = (x1-x0)/(dptype)nsplit;

#ifdef USE_MPI
	MPI_Comm_rank(MPI_COMM_WORLD,&myid);
	MPI_Comm_size(MPI_COMM_WORLD,&nid);
#endif


	int i,j;
	for(i=0;i<nsplit;i++){
		xpos[i] = x0 + step*i;
		ipos[i] = np;
		jpos[i] = -1;
	}
	xpos[nsplit] = x1;
	if(myid==0 && type == SINK){
	}


	sprintf(dir,"./FoF_Data/NewDD.%.5d", istep);
	if(myid==0){
		struct stat sb;
		stat(dir,&sb);
		if(!S_ISDIR(sb.st_mode)) mkfolder(dir);
	}

	if(icpu==1 && type == DM){
		for(i=0;i<nsplit;i++){
			char outfile[190];
			sprintf(outfile,"%s/SN.%.5d.%.5d.info",dir,istep, i);
			FILE *wp = fopen(outfile,"w");
			ram->xmin = xpos[i];
			ram->xmax = xpos[i+1];
			ram->icpu = i;
			write_head(wp, *ram);
			fclose(wp);
		}
	}

	size_t dsize;
	char outheader[190];
	char outtail[190];
	char outfile[190];
	char outmiddle[190];

	sprintf(outheader,"%s/SN.%.5d", dir,istep);

	if(type == DM){
		DmType *dump = (DmType*)aa;
		for(i=0;i<np;i++){
			int ibin = (dump[i].x-x0)/step;
			if(ibin <0 || ibin >= nsplit) {
				ERRORPRINT("Error in ibin %d : %g %g %g\n",ibin,dump[i].x,x0,step);
				continue;
			}
			ipos[ibin] = min(ipos[ibin], i);
			jpos[ibin] = max(jpos[ibin], i);
		}
		dsize = sizeof(DmType);
		sprintf(outmiddle,"%s.DM", outheader);

	}
	else if(type == STAR){
		StarType *dump = (StarType*)aa;
		for(i=0;i<np;i++){
			int ibin = (dump[i].x-x0)/step;
			if(ibin <0 || ibin >= nsplit) {
				ERRORPRINT("Error in ibin %d : %g %g %g\n",ibin,dump[i].x,x0,step);
				continue;
			}
			ipos[ibin] = min(ipos[ibin], i);
			jpos[ibin] = max(jpos[ibin], i);
		}
		dsize = sizeof(StarType);
		sprintf(outmiddle,"%s.STAR", outheader);
	}
	else if(type == SINK){
		SinkType *dump = (SinkType*)aa;
		for(i=0;i<np;i++){
			int ibin = (dump[i].x-x0)/step;
			if(ibin <0 || ibin >= nsplit) {
				ERRORPRINT("Error in ibin %d : %g %g %g\n",ibin,dump[i].x,x0,step);
				continue;
			}
			ipos[ibin] = min(ipos[ibin], i);
			jpos[ibin] = max(jpos[ibin], i);
		}
		dsize = sizeof(SinkType);
		sprintf(outmiddle,"%s.SINK", outheader);
	}
	else if(type == HCELL){
		HydroCellType *dump = (HydroCellType*)aa;
		for(i=0;i<np;i++){
			int ibin = (dump[i].x-x0)/step;
			if(ibin <0 || ibin >= nsplit) {
				ERRORPRINT("Error in ibin %d : %g %g %g\n",ibin,dump[i].x,x0,step);
				continue;
			}
			ipos[ibin] = min(ipos[ibin], i);
			jpos[ibin] = max(jpos[ibin], i);
		}
		dsize = sizeof(HydroCellType);
		sprintf(outmiddle,"%s.HCELL", outheader);
	}
	else if(type == GAS){
		GasType *dump = (GasType*)aa;
		for(i=0;i<np;i++){
			int ibin = (dump[i].x-x0)/step;
			if(ibin <0 || ibin >= nsplit) {
				ERRORPRINT("Error in ibin %d : %g %g %g\n",ibin,dump[i].x,x0,step);
				continue;
			}
			ipos[ibin] = min(ipos[ibin], i);
			jpos[ibin] = max(jpos[ibin], i);
		}
		dsize = sizeof(GasType);
		sprintf(outmiddle,"%s.GAS", outheader);
	}

#ifdef USE_MPI
#define asize(i,j) asize[(i) + nid*(j)]
#define bsize(i,j) bsize[(i) + nid*(j)]

	if(sinmul == 1)
	{
		int *asize = (int*)Malloc(sizeof(int)*nid*nsplit, PPTR(asize));
		int *bsize = (int*)Malloc(sizeof(int)*nid*nsplit, PPTR(bsize));
		size_t *baseoffset = (size_t*)Malloc(sizeof(size_t)*nsplit, PPTR(baseoffset));
		for(i=0;i<nid*nsplit;i++) asize[i] = bsize[i] = 0;
//		for(i=0;i<nsplit;i++) asize[myid + i*nid] = jpos[i]-ipos[i]+1;
		for(i=0;i<nsplit;i++) {
			int num = jpos[i]-ipos[i]+1;
			if(jpos[i]>=0 && num >=0) asize(myid,i) = num;
			else asize(myid,i) = 0;
		}
		MPI_Reduce(asize, bsize, nid*nsplit, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
		/*
		if(type==STAR){
			printf("P%d gathers data %d %d %d %d\n",
					myid, asize(0,18), asize(1,18),bsize(0,18),bsize(1,18));
		}
		*/
		if(myid==0){
			for(j=0;j<nsplit;j++) for(i=0;i<nid;i++){
				if(i==0){
//					asize[nid*j] = 0;
					asize(i,j) = 0;
				}
				else{
//					asize[i + nid*j] = asize[i-1+nid*j] + bsize[i-1+nid*j];
					asize(i,j) = asize(i-1,j) + bsize(i-1,j);
				}
			}
		}
		MPI_Bcast(asize, nid*nsplit,MPI_INT, 0, MPI_COMM_WORLD);
		/*
		if(type==STAR){
			printf("P%d gathers data %d %d %d %d\n",
					myid, asize(0,18), asize(1,18),bsize(0,18),bsize(1,18));
		}
		*/
		/* Now aaize has an info of the number of data before the currend mpi-id and dump file */
		if(myid==0){
			for(i=0;i<nsplit;i++){
				sprintf(outfile,"%s.%.5d.dat", outmiddle, i);
				FILE *wp;
				if(icpu==1) {
					wp= fopen(outfile,"w");
					baseoffset[i] = 0;
					fclose(wp);
				}
				else {
					struct stat st;
					stat(outfile,&st);
					baseoffset[i] = st.st_size;
				}

//				off_t offset = dsize*(asize[nid-1 + nid*i] + bsize[nid-1+nid*i]) + baseoffset[i];
				off_t offset = dsize*(asize(nid-1 ,i) + bsize(nid-1,i)) + baseoffset[i];
				int filenum = open(outfile,O_RDWR);
				ftruncate(filenum, offset);
				close(filenum);
			}
		}
		MPI_Bcast(baseoffset, nsplit, MPI_SIZE_T, 0, MPI_COMM_WORLD);

		int WGroupSize = WGROUPSIZE;

		char *a= (char*)aa;
		Wait2Start(WGroupSize);
		for(i=0;i<nsplit;i++){
			if(jpos[i] >=0) {
				sprintf(outfile,"%s.%.5d.dat", outmiddle, i);
				FILE *wp;
				wp= fopen(outfile,"r+");
//				fseek(wp, baseoffset[i] + dsize*asize[nid-1+ nid*i], SEEK_SET);
				long nowoffset = dsize*asize(myid,i)  + baseoffset[i];
  				fseek(wp, nowoffset, SEEK_SET);
				/*
				if(type==STAR && i == 18){
					printf("P%d is dumping %d %d from offset= %ld with asize= %d/%d\n",myid, dsize, 
							jpos[i]-ipos[i]+1, nowoffset, asize(myid,i),asize(myid+1,i));
				}
				*/
				fwrite(a+ipos[i]*dsize, dsize, jpos[i]-ipos[i]+1, wp);
				fclose(wp);
			}
		}
		Wait2Go(WGroupSize);
		Free(asize);
		Free(bsize);
		Free(baseoffset);
	}
	else 
#undef asize
#undef bsize
#endif
	{
		char *a= (char*)aa;
		for(i=0;i<nsplit;i++){
			if(jpos[i] >=0) {
				sprintf(outfile,"%s.%.5d.dat", outmiddle, i);
				FILE *wp;
				if(icpu==1) wp= fopen(outfile,"w");
				else wp= fopen(outfile,"a");
				fwrite(a+ipos[i]*dsize, dsize, jpos[i]-ipos[i]+1, wp);
				fclose(wp);
			}
			else if( icpu==1){
				sprintf(outfile,"%s.%.5d.dat", outmiddle, i);
				FILE *wp;
				wp = fopen(outfile,"w");
				fclose(wp);
			}
		}
	}
}
