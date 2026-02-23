#define MinNumMem 20
//#define MINCELLSIZE 1.e-2
#define MINCELLSIZE 2.e-3

#define MaxLinkedParticles 5000000
#define sqr(A) ((A)*(A))
#define NODE 0
#define PARTICLE 1
//#define NODE_HAVE_PARTICLE 40
#define NODE_HAVE_PARTICLE 8
#define EPSILON (0.1)
#define INDXTYPE long long
#ifdef XYZDBL
#	define POSTYPE double
#else
#	define POSTYPE float
#endif
enum boolean {NO=0, YES=1};
typedef struct Box{
	POSTYPE x,y,z;
	POSTYPE width;
} Box;
typedef struct HaloQ{
	size_t np,npstar,npgas,npdm,npsink;
	POSTYPE x,y,z;
	double mass, mstar,mgas,mdm,msink;
	float vx,vy,vz;
}HaloQ;
/*      */
/*
#define GENERAL_TYPE unsigned int type: 1
#define GENERALTPtlPOINTER GENERAL_TYPE;void *sibling
#define GENERAL_TYPE unsigned int type
*/
/*      */
enum {TYPE_TREE = 0,TYPE_STAR = 1, TYPE_DM=2, TYPE_GAS=3, TYPE_SINK=4, TYPE_AGN=4, TYPE_PTL=5};
typedef struct TYPE {
	unsigned int type;
} TYPE;
typedef struct GENERAL_TPtl_POINTER {
	unsigned int type;
	void *sibling;
} GENERAL_TPtl_POINTER;
/* for Fof */
typedef struct FoFTPtlStruct{
	unsigned int type;
	void *sibling;
	enum boolean included;
	POSTYPE x,y,z;
	POSTYPE  link02;
	union {
		DmType dm;
		StarType star;
		SinkType sink;
		GasType gas;
	}p;
	/*
	TPtlStruct *pointer;
	*/
	size_t haloindx;
} FoFTPtlStruct;
typedef struct HaloBound{
	size_t nmem;
	POSTYPE zmin,zmax;
	int boundflag;
	FoFTPtlStruct *sibling;
} HaloBound;
typedef struct FoFTStruct{
	unsigned int type;
	void *sibling;
	POSTYPE dist;
	POSTYPE monox,monoy,monoz;
	void *daughter;
	int Nparticle;
	POSTYPE  maxlink02;
} FoFTStruct;
typedef struct FoFBeginEndTree{
	FoFTStruct *start;
	void *EndTree;
	void *EndPtl;
} FoFBeginEndTree;



/*
typedef struct TPtlStruct{
	unsigned int type;
	void *sibling;
	POSTYPE x,y,z;
	float mass;
	INDXTYPE indx;
} TPtlStruct;
*/

/*
typedef struct particle {
	unsigned int type;
	POSTYPE x,y,z;
	float link02,mass;
	float vx,vy,vz;
	INDXTYPE indx;
} particle;
*/
typedef struct FoFTPtlStruct particle;

typedef struct readtype{
	float x,y,z;
//	float vx,vy,vz;
	INDXTYPE indx;
}READTYPE;

#define EraseFromTree(optr,ptr,nptr) {\
	switch(((TYPE*)optr)->type) {\
		case TYPE_TREE:\
			if(((FoFTStruct*)optr)->daughter == ptr) \
				((FoFTStruct*)optr)->daughter = nptr;\
			else ((FoFTStruct*)optr)->sibling = nptr;\
			break;\
		default :\
			((FoFTPtlStruct*)optr)->sibling = nptr;\
	}\
}\
	
FoFBeginEndTree FoF_divide_node2D(FoFTStruct *,FoFTStruct *, FoFTPtlStruct *, Box ,FoFTStruct *); 
void FoF_Make_Tree2D(FoFTStruct *,FoFTPtlStruct *,size_t ,Box );
FoFBeginEndTree FoF_divide_node(FoFTStruct *,FoFTStruct *, FoFTPtlStruct *, Box ,FoFTStruct *); 
void FoF_Make_Tree(FoFTStruct *,FoFTPtlStruct *,size_t ,Box );
size_t pnew_fof_link(particle*,POSTYPE, FoFTStruct *, FoFTPtlStruct *,particle *, size_t nhalo, POSTYPE,POSTYPE,POSTYPE);
size_t new_fof_link(particle*,POSTYPE, FoFTStruct *, FoFTPtlStruct *,particle *, size_t nhalo);
void CheckHaloBound(size_t ,HaloBound *,FoFTPtlStruct *,size_t , POSTYPE ,
		POSTYPE , POSTYPE, POSTYPE);
HaloQ haloproperty(particle *,size_t);
void WriteIsolatedHalo(size_t , HaloBound *,FoFTPtlStruct *, particle *, char *,
		char *);
void WriteFinalHalo(size_t , HaloBound *,FoFTPtlStruct *, particle *, char *,
		char *);
void WriteAllHalo(size_t , HaloBound *,FoFTPtlStruct *, size_t, 
		particle *, char *, char *);

size_t StackUpContactParticleLeftWard(size_t ,HaloBound *, FoFTPtlStruct *,
		size_t );
void ReadBottomFaceContact(FoFTPtlStruct *,size_t,particle *,int ,int,int);
size_t WriteBottomFaceContact(size_t , HaloBound *, FoFTPtlStruct *, particle *, int,int);


