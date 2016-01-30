

#if !defined(_DMIMPL_H)
#define _DMIMPL_H

#include <petscdm.h>
#include <petsc/private/petscimpl.h>

PETSC_EXTERN PetscBool DMRegisterAllCalled;
PETSC_EXTERN PetscErrorCode DMRegisterAll(void);
typedef PetscErrorCode (*NullSpaceFunc)(DM dm, PetscInt field, MatNullSpace *nullSpace);

typedef struct _DMOps *DMOps;
struct _DMOps {
  PetscErrorCode (*view)(DM,PetscViewer);
  PetscErrorCode (*load)(DM,PetscViewer);
  PetscErrorCode (*clone)(DM,DM*);
  PetscErrorCode (*setfromoptions)(PetscOptionItems*,DM);
  PetscErrorCode (*setup)(DM);
  PetscErrorCode (*createdefaultsection)(DM);
  PetscErrorCode (*createdefaultconstraints)(DM);
  PetscErrorCode (*createglobalvector)(DM,Vec*);
  PetscErrorCode (*createlocalvector)(DM,Vec*);
  PetscErrorCode (*getlocaltoglobalmapping)(DM);
  PetscErrorCode (*createfieldis)(DM,PetscInt*,char***,IS**);
  PetscErrorCode (*createcoordinatedm)(DM,DM*);

  PetscErrorCode (*getcoloring)(DM,ISColoringType,ISColoring*);
  PetscErrorCode (*creatematrix)(DM, Mat*);
  PetscErrorCode (*createinterpolation)(DM,DM,Mat*,Vec*);
  PetscErrorCode (*getaggregates)(DM,DM,Mat*);
  PetscErrorCode (*getinjection)(DM,DM,Mat*);

  PetscErrorCode (*refine)(DM,MPI_Comm,DM*);
  PetscErrorCode (*coarsen)(DM,MPI_Comm,DM*);
  PetscErrorCode (*refinehierarchy)(DM,PetscInt,DM*);
  PetscErrorCode (*coarsenhierarchy)(DM,PetscInt,DM*);

  PetscErrorCode (*globaltolocalbegin)(DM,Vec,InsertMode,Vec);
  PetscErrorCode (*globaltolocalend)(DM,Vec,InsertMode,Vec);
  PetscErrorCode (*localtoglobalbegin)(DM,Vec,InsertMode,Vec);
  PetscErrorCode (*localtoglobalend)(DM,Vec,InsertMode,Vec);
  PetscErrorCode (*localtolocalbegin)(DM,Vec,InsertMode,Vec);
  PetscErrorCode (*localtolocalend)(DM,Vec,InsertMode,Vec);

  PetscErrorCode (*destroy)(DM);

  PetscErrorCode (*computevariablebounds)(DM,Vec,Vec);

  PetscErrorCode (*createsubdm)(DM,PetscInt,PetscInt*,IS*,DM*);
  PetscErrorCode (*createfielddecomposition)(DM,PetscInt*,char***,IS**,DM**);
  PetscErrorCode (*createdomaindecomposition)(DM,PetscInt*,char***,IS**,IS**,DM**);
  PetscErrorCode (*createddscatters)(DM,PetscInt,DM*,VecScatter**,VecScatter**,VecScatter**);

  PetscErrorCode (*getdimpoints)(DM,PetscInt,PetscInt*,PetscInt*);
  PetscErrorCode (*locatepoints)(DM,Vec,IS*);
};

typedef struct _DMCoarsenHookLink *DMCoarsenHookLink;
struct _DMCoarsenHookLink {
  PetscErrorCode (*coarsenhook)(DM,DM,void*);              /* Run once, when coarse DM is created */
  PetscErrorCode (*restricthook)(DM,Mat,Vec,Mat,DM,void*); /* Run each time a new problem is restricted to a coarse grid */
  void *ctx;
  DMCoarsenHookLink next;
};

typedef struct _DMRefineHookLink *DMRefineHookLink;
struct _DMRefineHookLink {
  PetscErrorCode (*refinehook)(DM,DM,void*);     /* Run once, when a fine DM is created */
  PetscErrorCode (*interphook)(DM,Mat,DM,void*); /* Run each time a new problem is interpolated to a fine grid */
  void *ctx;
  DMRefineHookLink next;
};

typedef struct _DMSubDomainHookLink *DMSubDomainHookLink;
struct _DMSubDomainHookLink {
  PetscErrorCode (*ddhook)(DM,DM,void*);
  PetscErrorCode (*restricthook)(DM,VecScatter,VecScatter,DM,void*);
  void *ctx;
  DMSubDomainHookLink next;
};

typedef struct _DMGlobalToLocalHookLink *DMGlobalToLocalHookLink;
struct _DMGlobalToLocalHookLink {
  PetscErrorCode (*beginhook)(DM,Vec,InsertMode,Vec,void*);
  PetscErrorCode (*endhook)(DM,Vec,InsertMode,Vec,void*);
  void *ctx;
  DMGlobalToLocalHookLink next;
};

typedef struct _DMLocalToGlobalHookLink *DMLocalToGlobalHookLink;
struct _DMLocalToGlobalHookLink {
  PetscErrorCode (*beginhook)(DM,Vec,InsertMode,Vec,void*);
  PetscErrorCode (*endhook)(DM,Vec,InsertMode,Vec,void*);
  void *ctx;
  DMLocalToGlobalHookLink next;
};

typedef enum {DMVEC_STATUS_IN,DMVEC_STATUS_OUT} DMVecStatus;
typedef struct _DMNamedVecLink *DMNamedVecLink;
struct _DMNamedVecLink {
  Vec X;
  char *name;
  DMVecStatus status;
  DMNamedVecLink next;
};

typedef struct _DMWorkLink *DMWorkLink;
struct _DMWorkLink {
  size_t     bytes;
  void       *mem;
  DMWorkLink next;
};

#define DM_MAX_WORK_VECTORS 100 /* work vectors available to users  via DMGetGlobalVector(), DMGetLocalVector() */

struct _n_DMLabelLink {
  DMLabel              label;
  PetscBool            output;
  struct _n_DMLabelLink *next;
};
typedef struct _n_DMLabelLink *DMLabelLink;

struct _n_DMLabelLinkList {
  PetscInt refct;
  DMLabelLink next;
};
typedef struct _n_DMLabelLinkList *DMLabelLinkList;


struct _p_DM {
  PETSCHEADER(struct _DMOps);
  Vec                     localin[DM_MAX_WORK_VECTORS],localout[DM_MAX_WORK_VECTORS];
  Vec                     globalin[DM_MAX_WORK_VECTORS],globalout[DM_MAX_WORK_VECTORS];
  DMNamedVecLink          namedglobal;
  DMNamedVecLink          namedlocal;
  DMWorkLink              workin,workout;
  DMLabelLinkList         labels;            /* Linked list of labels */
  DMLabel                 depthLabel;        /* Optimized access to depth label */
  void                    *ctx;    /* a user context */
  PetscErrorCode          (*ctxdestroy)(void**);
  Vec                     x;       /* location at which the functions/Jacobian are computed */
  ISColoringType          coloringtype;
  MatFDColoring           fd;
  VecType                 vectype;  /* type of vector created with DMCreateLocalVector() and DMCreateGlobalVector() */
  MatType                 mattype;  /* type of matrix created with DMCreateMatrix() */
  PetscInt                bs;
  ISLocalToGlobalMapping  ltogmap;
  PetscBool               prealloc_only; /* Flag indicating the DMCreateMatrix() should only preallocate, not fill the matrix */
  PetscInt                levelup,leveldown;  /* if the DM has been obtained by refining (or coarsening) this indicates how many times that process has been used to generate this DM */
  PetscBool               setupcalled;        /* Indicates that the DM has been set up, methods that modify a DM such that a fresh setup is required should reset this flag */
  void                    *data;
  DMCoarsenHookLink       coarsenhook; /* For transfering auxiliary problem data to coarser grids */
  DMRefineHookLink        refinehook;
  DMSubDomainHookLink     subdomainhook;
  DMGlobalToLocalHookLink gtolhook;
  DMLocalToGlobalHookLink ltoghook;
  /* Topology */
  PetscInt                dim;                  /* The topological dimension */
  /* Flexible communication */
  PetscSF                 sf;                   /* SF for parallel point overlap */
  PetscSF                 defaultSF;            /* SF for parallel dof overlap using default section */
  PetscSF                 sfNatural;            /* SF mapping to the "natural" ordering */
  PetscBool               useNatural;           /* Create the natural SF */
  /* Allows a non-standard data layout */
  PetscSection            defaultSection;       /* Layout for local vectors */
  PetscSection            defaultGlobalSection; /* Layout for global vectors */
  PetscLayout             map;
  /* Constraints */
  PetscSection            defaultConstraintSection;
  Mat                     defaultConstraintMat;
  /* Coordinates */
  PetscInt                dimEmbed;             /* The dimension of the embedding space */
  DM                      coordinateDM;         /* Layout for coordinates (default section) */
  Vec                     coordinates;          /* Coordinate values in global vector */
  Vec                     coordinatesLocal;     /* Coordinate values in local  vector */
  PetscReal              *L, *maxCell;          /* Size of periodic box and max cell size for determining periodicity */
  DMBoundaryType         *bdtype;               /* Indicates type of topological boundary */
  /* Null spaces -- of course I should make this have a variable number of fields */
  /*   I now believe this might not be the right way: see below */
  NullSpaceFunc           nullspaceConstructors[10];
  /* Fields are represented by objects */
  PetscDS                 prob;
  /* Output structures */
  DM                      dmBC;                 /* The DM with boundary conditions in the global DM */
  PetscInt                outputSequenceNum;    /* The current sequence number for output */
  PetscReal               outputSequenceVal;    /* The current sequence value for output */

  PetscObject             dmksp,dmsnes,dmts;
};

PETSC_EXTERN PetscLogEvent DM_Convert, DM_GlobalToLocal, DM_LocalToGlobal, DM_LocatePoints, DM_Coarsen, DM_CreateInterpolation;

PETSC_EXTERN PetscErrorCode DMCreateGlobalVector_Section_Private(DM,Vec*);
PETSC_EXTERN PetscErrorCode DMCreateLocalVector_Section_Private(DM,Vec*);
PETSC_EXTERN PetscErrorCode DMCreateSubDM_Section_Private(DM,PetscInt,PetscInt[],IS*,DM*);

/*

          Composite Vectors

      Single global representation
      Individual global representations
      Single local representation
      Individual local representations

      Subsets of individual as a single????? Do we handle this by having DMComposite inside composite??????

       DM da_u, da_v, da_p

       DM dm_velocities

       DM dm

       DMDACreate(,&da_u);
       DMDACreate(,&da_v);
       DMCompositeCreate(,&dm_velocities);
       DMCompositeAddDM(dm_velocities,(DM)du);
       DMCompositeAddDM(dm_velocities,(DM)dv);

       DMDACreate(,&da_p);
       DMCompositeCreate(,&dm_velocities);
       DMCompositeAddDM(dm,(DM)dm_velocities);
       DMCompositeAddDM(dm,(DM)dm_p);


    Access parts of composite vectors (Composite only)
    ---------------------------------
      DMCompositeGetAccess  - access the global vector as subvectors and array (for redundant arrays)
      ADD for local vector -

    Element access
    --------------
      From global vectors
         -DAVecGetArray   - for DMDA
         -VecGetArray - for DMSliced
         ADD for DMComposite???  maybe

      From individual vector
          -DAVecGetArray - for DMDA
          -VecGetArray -for sliced
         ADD for DMComposite??? maybe

      From single local vector
          ADD         * single local vector as arrays?

   Communication
   -------------
      DMGlobalToLocal - global vector to single local vector

      DMCompositeScatter/Gather - direct to individual local vectors and arrays   CHANGE name closer to GlobalToLocal?

   Obtaining vectors
   -----------------
      DMCreateGlobal/Local
      DMGetGlobal/Local
      DMCompositeGetLocalVectors   - gives individual local work vectors and arrays


?????   individual global vectors   ????

*/

#if defined(PETSC_HAVE_HDF5)
PETSC_EXTERN PetscErrorCode DMSequenceLoad_HDF5(DM, const char *, PetscInt, PetscScalar *, PetscViewer);
#endif

#endif
