
#if !defined(__MPIAIJ_H)
#define __MPIAIJ_H

#include <../src/mat/impls/aij/seq/aij.h>

typedef struct { /* used by MatCreateMPIAIJSumSeqAIJ for reusing the merged matrix */
  PetscLayout rowmap;
  PetscInt    **buf_ri,**buf_rj;
  PetscMPIInt *len_s,*len_r,*id_r;    /* array of length of comm->size, store send/recv matrix values */
  PetscMPIInt nsend,nrecv;
  PetscInt    *bi,*bj;    /* i and j array of the local portion of mpi C (matrix product) - rename to ci, cj! */
  PetscInt    *owners_co,*coi,*coj;    /* i and j array of (p->B)^T*A*P - used in the communication */
  PetscErrorCode (*destroy)(Mat);
  PetscErrorCode (*duplicate)(Mat,MatDuplicateOption,Mat*);
} Mat_Merge_SeqsToMPI;

typedef struct { /* used by MatPtAP_MPIAIJ_MPIAIJ() and MatMatMult_MPIAIJ_MPIAIJ() */
  PetscInt    *startsj_s,*startsj_r;    /* used by MatGetBrowsOfAoCols_MPIAIJ */
  PetscScalar *bufa;                    /* used by MatGetBrowsOfAoCols_MPIAIJ */
  Mat         P_loc,P_oth;     /* partial B_seq -- intend to replace B_seq */
  PetscInt    *api,*apj;       /* symbolic i and j arrays of the local product A_loc*B_seq */
  PetscScalar *apv;
  MatReuse    reuse;           /* flag to skip MatGetBrowsOfAoCols_MPIAIJ() and MatMPIAIJGetLocalMat() in 1st call of MatPtAPNumeric_MPIAIJ_MPIAIJ() */
  PetscScalar *apa;            /* tmp array for store a row of A*P used in MatMatMult() */
  Mat         A_loc;           /* used by MatTransposeMatMult(), contains api and apj */
  Mat         Pt;              /* used by MatTransposeMatMult(), Pt = P^T */
  PetscBool   scalable;        /* flag determines scalable or non-scalable implementation */
  Mat         Rd,Ro,AP_loc,C_loc,C_oth;

  Mat_Merge_SeqsToMPI *merge;
  PetscErrorCode (*destroy)(Mat);
  PetscErrorCode (*duplicate)(Mat,MatDuplicateOption,Mat*);
} Mat_PtAPMPI;

typedef struct {
  Mat A,B;                             /* local submatrices: A (diag part),
                                           B (off-diag part) */
  PetscMPIInt size;                     /* size of communicator */
  PetscMPIInt rank;                     /* rank of proc in communicator */

  /* The following variables are used for matrix assembly */
  PetscBool   donotstash;               /* PETSC_TRUE if off processor entries dropped */
  MPI_Request *send_waits;              /* array of send requests */
  MPI_Request *recv_waits;              /* array of receive requests */
  PetscInt    nsends,nrecvs;           /* numbers of sends and receives */
  PetscScalar *svalues,*rvalues;       /* sending and receiving data */
  PetscInt    rmax;                     /* maximum message length */
#if defined(PETSC_USE_CTABLE)
  PetscTable colmap;
#else
  PetscInt *colmap;                     /* local col number of off-diag col */
#endif
  PetscInt *garray;                     /* global index of all off-processor columns */

  /* The following variables are used for matrix-vector products */
  Vec        lvec;                 /* local vector */
  Vec        diag;
  VecScatter Mvctx;                /* scatter context for vector */
  PetscBool  roworiented;          /* if true, row-oriented input, default true */

  /* The following variables are for MatGetRow() */
  PetscInt    *rowindices;         /* column indices for row */
  PetscScalar *rowvalues;          /* nonzero values in row */
  PetscBool   getrowactive;        /* indicates MatGetRow(), not restored */

  /* Used by MatDistribute_MPIAIJ() to allow reuse of previous matrix allocation  and nonzero pattern */
  PetscInt *ld;                    /* number of entries per row left of diagona block */

  /* Used by MatMatMult() and MatPtAP() */
  Mat_PtAPMPI *ptap;

  /* used by MatMatMatMult() */
  Mat_MatMatMatMult *matmatmatmult;

  /* Used by MPICUSP and MPICUSPARSE classes */
  void * spptr;

} Mat_MPIAIJ;

PETSC_EXTERN PetscErrorCode MatCreate_MPIAIJ(Mat);

PETSC_INTERN PetscErrorCode MatSetColoring_MPIAIJ(Mat,ISColoring);
PETSC_INTERN PetscErrorCode MatSetValuesAdifor_MPIAIJ(Mat,PetscInt,void*);
PETSC_INTERN PetscErrorCode MatSetUpMultiply_MPIAIJ(Mat);
PETSC_INTERN PetscErrorCode MatDisAssemble_MPIAIJ(Mat);
PETSC_INTERN PetscErrorCode MatDuplicate_MPIAIJ(Mat,MatDuplicateOption,Mat*);
PETSC_INTERN PetscErrorCode MatIncreaseOverlap_MPIAIJ(Mat,PetscInt,IS [],PetscInt);
PETSC_INTERN PetscErrorCode MatIncreaseOverlap_MPIAIJ_Scalable(Mat,PetscInt,IS [],PetscInt);
PETSC_INTERN PetscErrorCode MatFDColoringCreate_MPIXAIJ(Mat,ISColoring,MatFDColoring);
PETSC_INTERN PetscErrorCode MatFDColoringSetUp_MPIXAIJ(Mat,ISColoring,MatFDColoring);
PETSC_INTERN PetscErrorCode MatGetSubMatrices_MPIAIJ (Mat,PetscInt,const IS[],const IS[],MatReuse,Mat *[]);
PETSC_INTERN PetscErrorCode MatGetSubMatricesMPI_MPIAIJ (Mat,PetscInt,const IS[],const IS[],MatReuse,Mat *[]);
PETSC_INTERN PetscErrorCode MatGetSubMatrix_MPIAIJ_All(Mat,MatGetSubMatrixOption,MatReuse,Mat *[]);


PETSC_INTERN PetscErrorCode MatGetSubMatrix_MPIAIJ(Mat,IS,IS,MatReuse,Mat*);
PETSC_INTERN PetscErrorCode MatGetSubMatrix_MPIAIJ_Private (Mat,IS,IS,PetscInt,MatReuse,Mat*);
PETSC_INTERN PetscErrorCode MatGetMultiProcBlock_MPIAIJ(Mat,MPI_Comm,MatReuse,Mat*);

PETSC_INTERN PetscErrorCode MatLoad_MPIAIJ(Mat,PetscViewer);
PETSC_INTERN PetscErrorCode MatCreateColmap_MPIAIJ_Private(Mat);
PETSC_INTERN PetscErrorCode MatMatMult_MPIDense_MPIAIJ(Mat,Mat,MatReuse,PetscReal,Mat*);
PETSC_INTERN PetscErrorCode MatMatMult_MPIAIJ_MPIAIJ(Mat,Mat,MatReuse,PetscReal,Mat*);
PETSC_INTERN PetscErrorCode MatMatMultSymbolic_MPIAIJ_MPIAIJ_nonscalable(Mat,Mat,PetscReal,Mat*);
PETSC_INTERN PetscErrorCode MatMatMultSymbolic_MPIAIJ_MPIAIJ(Mat,Mat,PetscReal,Mat*);
PETSC_INTERN PetscErrorCode MatMatMultNumeric_MPIAIJ_MPIAIJ(Mat,Mat,Mat);
PETSC_INTERN PetscErrorCode MatMatMultNumeric_MPIAIJ_MPIAIJ_nonscalable(Mat,Mat,Mat);

PETSC_INTERN PetscErrorCode MatMatMatMult_MPIAIJ_MPIAIJ_MPIAIJ(Mat,Mat,Mat,MatReuse,PetscReal,Mat*);
PETSC_INTERN PetscErrorCode MatMatMatMultSymbolic_MPIAIJ_MPIAIJ_MPIAIJ(Mat,Mat,Mat,PetscReal,Mat*);
PETSC_INTERN PetscErrorCode MatMatMatMultNumeric_MPIAIJ_MPIAIJ_MPIAIJ(Mat,Mat,Mat,Mat);

PETSC_INTERN PetscErrorCode MatPtAP_MPIAIJ_MPIAIJ(Mat,Mat,MatReuse,PetscReal,Mat*);
PETSC_INTERN PetscErrorCode MatPtAPSymbolic_MPIAIJ_MPIAIJ(Mat,Mat,PetscReal,Mat*);
PETSC_INTERN PetscErrorCode MatPtAPNumeric_MPIAIJ_MPIAIJ(Mat,Mat,Mat);

PETSC_INTERN PetscErrorCode MatPtAPSymbolic_MPIAIJ_MPIAIJ_ptap(Mat,Mat,PetscReal,Mat*);
PETSC_INTERN PetscErrorCode MatPtAPNumeric_MPIAIJ_MPIAIJ_ptap(Mat,Mat,Mat);

PETSC_INTERN PetscErrorCode MatDestroy_MPIAIJ_PtAP(Mat);
PETSC_INTERN PetscErrorCode MatDestroy_MPIAIJ(Mat);

PETSC_INTERN PetscErrorCode MatGetBrowsOfAoCols_MPIAIJ(Mat,Mat,MatReuse,PetscInt**,PetscInt**,MatScalar**,Mat*);
PETSC_INTERN PetscErrorCode MatSetValues_MPIAIJ(Mat,PetscInt,const PetscInt[],PetscInt,const PetscInt[],const PetscScalar [],InsertMode);
PETSC_INTERN PetscErrorCode MatDestroy_MPIAIJ_MatMatMult(Mat);
PETSC_INTERN PetscErrorCode PetscContainerDestroy_Mat_MatMatMultMPI(void*);
PETSC_INTERN PetscErrorCode MatSetOption_MPIAIJ(Mat,MatOption,PetscBool);

PETSC_INTERN PetscErrorCode MatTransposeMatMult_MPIAIJ_MPIAIJ(Mat,Mat,MatReuse,PetscReal,Mat*);
PETSC_INTERN PetscErrorCode MatTransposeMatMultSymbolic_MPIAIJ_MPIAIJ_nonscalable(Mat,Mat,PetscReal,Mat*);
PETSC_INTERN PetscErrorCode MatTransposeMatMultSymbolic_MPIAIJ_MPIAIJ(Mat,Mat,PetscReal,Mat*);
PETSC_INTERN PetscErrorCode MatTransposeMatMultNumeric_MPIAIJ_MPIAIJ(Mat,Mat,Mat);
PETSC_INTERN PetscErrorCode MatTransposeMatMultNumeric_MPIAIJ_MPIAIJ_nonscalable(Mat,Mat,Mat);
PETSC_INTERN PetscErrorCode MatTransposeMatMultNumeric_MPIAIJ_MPIAIJ_matmatmult(Mat,Mat,Mat);
PETSC_INTERN PetscErrorCode MatTransposeMatMult_MPIAIJ_MPIDense(Mat,Mat,MatReuse,PetscReal,Mat*);
PETSC_INTERN PetscErrorCode MatTransposeMatMultSymbolic_MPIAIJ_MPIDense(Mat,Mat,PetscReal,Mat*);
PETSC_INTERN PetscErrorCode MatTransposeMatMultNumeric_MPIAIJ_MPIDense(Mat,Mat,Mat);
PETSC_INTERN PetscErrorCode MatGetSeqNonzeroStructure_MPIAIJ(Mat,Mat*);

PETSC_INTERN PetscErrorCode MatSetFromOptions_MPIAIJ(PetscOptionItems*,Mat);
PETSC_INTERN PetscErrorCode MatMPIAIJSetPreallocation_MPIAIJ(Mat,PetscInt,const PetscInt[],PetscInt,const PetscInt[]);

#if !defined(PETSC_USE_COMPLEX) && !defined(PETSC_USE_REAL_SINGLE) && !defined(PETSC_USE_REAL___FLOAT128)
PETSC_INTERN PetscErrorCode MatLUFactorSymbolic_MPIAIJ_TFS(Mat,IS,IS,const MatFactorInfo*,Mat*);
#endif
PETSC_INTERN PetscErrorCode MatSolve_MPIAIJ(Mat,Vec,Vec);
PETSC_INTERN PetscErrorCode MatILUFactor_MPIAIJ(Mat,IS,IS,const MatFactorInfo*);

PETSC_INTERN PetscErrorCode MatAXPYGetPreallocation_MPIX_private(PetscInt,const PetscInt*,const PetscInt*,const PetscInt*,const PetscInt*,const PetscInt*,const PetscInt*,PetscInt*);

extern PetscErrorCode MatGetDiagonalBlock_MPIAIJ(Mat,Mat*);
extern PetscErrorCode MatDiagonalScaleLocal_MPIAIJ(Mat,Vec);

PETSC_INTERN PetscErrorCode MatGetSeqMats_MPIAIJ(Mat,Mat*,Mat*);
PETSC_INTERN PetscErrorCode MatSetSeqMats_MPIAIJ(Mat,IS,IS,IS,MatStructure,Mat,Mat);

/* compute apa = A[i,:]*P = Ad[i,:]*P_loc + Ao*[i,:]*P_oth */
#define AProw_nonscalable(i,ad,ao,p_loc,p_oth,apa) \
{\
  PetscInt    _anz,_pnz,_j,_k,*_ai,*_aj,_row,*_pi,*_pj;      \
  PetscScalar *_aa,_valtmp,*_pa;                             \
  /* diagonal portion of A */\
  _ai  = ad->i;\
  _anz = _ai[i+1] - _ai[i];\
  _aj  = ad->j + _ai[i];\
  _aa  = ad->a + _ai[i];\
  for (_j=0; _j<_anz; _j++) {\
    _row = _aj[_j]; \
    _pi  = p_loc->i;                                 \
    _pnz = _pi[_row+1] - _pi[_row];         \
    _pj  = p_loc->j + _pi[_row];                 \
    _pa  = p_loc->a + _pi[_row];                 \
    /* perform dense axpy */                    \
    _valtmp = _aa[_j];                           \
    for (_k=0; _k<_pnz; _k++) {                    \
      apa[_pj[_k]] += _valtmp*_pa[_k];               \
    }                                           \
    PetscLogFlops(2.0*_pnz);                    \
  }                                             \
  /* off-diagonal portion of A */               \
  _ai  = ao->i;\
  _anz = _ai[i+1] - _ai[i];                     \
  _aj  = ao->j + _ai[i];                         \
  _aa  = ao->a + _ai[i];                         \
  for (_j=0; _j<_anz; _j++) {                      \
    _row = _aj[_j];    \
    _pi  = p_oth->i;                         \
    _pnz = _pi[_row+1] - _pi[_row];          \
    _pj  = p_oth->j + _pi[_row];                  \
    _pa  = p_oth->a + _pi[_row];                  \
    /* perform dense axpy */                     \
    _valtmp = _aa[_j];                             \
    for (_k=0; _k<_pnz; _k++) {                     \
      apa[_pj[_k]] += _valtmp*_pa[_k];                \
    }                                            \
    PetscLogFlops(2.0*_pnz);                     \
  } \
}
#endif
