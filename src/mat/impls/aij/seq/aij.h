
#if !defined(__AIJ_H)
#define __AIJ_H

#include <petsc/private/matimpl.h>
#include <petscctable.h>

/*
    Struct header shared by SeqAIJ, SeqBAIJ and SeqSBAIJ matrix formats
*/
#define SEQAIJHEADER(datatype)        \
  PetscBool roworiented;              /* if true, row-oriented input, default */ \
  PetscInt  nonew;                    /* 1 don't add new nonzeros, -1 generate error on new */ \
  PetscInt  nounused;                 /* -1 generate error on unused space */ \
  PetscBool singlemalloc;             /* if true a, i, and j have been obtained with one big malloc */ \
  PetscInt  maxnz;                    /* allocated nonzeros */ \
  PetscInt  *imax;                    /* maximum space allocated for each row */ \
  PetscInt  *ilen;                    /* actual length of each row */ \
  PetscBool free_imax_ilen;  \
  PetscInt  reallocs;                 /* number of mallocs done during MatSetValues() \
                                        as more values are set than were prealloced */\
  PetscInt          rmax;             /* max nonzeros in any row */ \
  PetscBool         keepnonzeropattern;   /* keeps matrix structure same in calls to MatZeroRows()*/ \
  PetscBool         ignorezeroentries; \
  PetscBool         free_ij;          /* free the column indices j and row offsets i when the matrix is destroyed */ \
  PetscBool         free_a;           /* free the numerical values when matrix is destroy */ \
  Mat_CompressedRow compressedrow;    /* use compressed row format */                      \
  PetscInt          nz;               /* nonzeros */                                       \
  PetscInt          *i;               /* pointer to beginning of each row */               \
  PetscInt          *j;               /* column values: j + i[k] - 1 is start of row k */  \
  PetscInt          *diag;            /* pointers to diagonal elements */                  \
  PetscInt          nonzerorowcnt;    /* how many rows have nonzero entries */             \
  PetscBool         free_diag;         \
  datatype          *a;               /* nonzero elements */                               \
  PetscScalar       *solve_work;      /* work space used in MatSolve */                    \
  IS                row, col, icol;   /* index sets, used for reorderings */ \
  PetscBool         pivotinblocks;    /* pivot inside factorization of each diagonal block */ \
  Mat               parent             /* set if this matrix was formed with MatDuplicate(...,MAT_SHARE_NONZERO_PATTERN,....);
                                         means that this shares some data structures with the parent including diag, ilen, imax, i, j */

typedef struct {
  MatTransposeColoring matcoloring;
  Mat                  Bt_den;       /* dense matrix of B^T */
  Mat                  ABt_den;      /* dense matrix of A*B^T */
  PetscBool            usecoloring;
  PetscErrorCode (*destroy)(Mat);
} Mat_MatMatTransMult;

typedef struct { /* for MatTransposeMatMult_SeqAIJ_SeqDense() */
  Mat          mA;           /* maij matrix of A */
  Vec          bt,ct;        /* vectors to hold locally transposed arrays of B and C */
  PetscErrorCode (*destroy)(Mat);
} Mat_MatTransMatMult;

typedef struct {
  PetscInt    *api,*apj;       /* symbolic structure of A*P */
  PetscScalar *apa;            /* temporary array for storing one row of A*P */
  PetscErrorCode (*destroy)(Mat);
} Mat_PtAP;

typedef struct {
  MatTransposeColoring matcoloring;
  Mat                  Rt;    /* sparse or dense matrix of R^T */
  Mat                  RARt;  /* dense matrix of R*A*R^T */
  Mat                  ARt;   /* A*R^T used for the case -matrart_color_art */
  MatScalar            *work; /* work array to store columns of A*R^T used in MatMatMatMultNumeric_SeqAIJ_SeqAIJ_SeqDense() */
  PetscErrorCode (*destroy)(Mat);
} Mat_RARt;

typedef struct {
  Mat BC;               /* temp matrix for storing B*C */
  PetscErrorCode (*destroy)(Mat);
} Mat_MatMatMatMult;

/*
  MATSEQAIJ format - Compressed row storage (also called Yale sparse matrix
  format) or compressed sparse row (CSR).  The i[] and j[] arrays start at 0. For example,
  j[i[k]+p] is the pth column in row k.  Note that the diagonal
  matrix elements are stored with the rest of the nonzeros (not separately).
*/

/* Info about i-nodes (identical nodes) helper class for SeqAIJ */
typedef struct {
  MatScalar        *bdiag,*ibdiag,*ssor_work;        /* diagonal blocks of matrix used for MatSOR_SeqAIJ_Inode() */
  PetscInt         bdiagsize;                         /* length of bdiag and ibdiag */
  PetscBool        ibdiagvalid;                       /* do ibdiag[] and bdiag[] contain the most recent values */

  PetscBool        use;
  PetscInt         node_count;                     /* number of inodes */
  PetscInt         *size;                          /* size of each inode */
  PetscInt         limit;                          /* inode limit */
  PetscInt         max_limit;                      /* maximum supported inode limit */
  PetscBool        checked;                        /* if inodes have been checked for */
  PetscObjectState mat_nonzerostate;               /* non-zero state when inodes were checked for */
} Mat_SeqAIJ_Inode;

PETSC_INTERN PetscErrorCode MatView_SeqAIJ_Inode(Mat,PetscViewer);
PETSC_INTERN PetscErrorCode MatAssemblyEnd_SeqAIJ_Inode(Mat,MatAssemblyType);
PETSC_INTERN PetscErrorCode MatDestroy_SeqAIJ_Inode(Mat);
PETSC_INTERN PetscErrorCode MatCreate_SeqAIJ_Inode(Mat);
PETSC_INTERN PetscErrorCode MatSetOption_SeqAIJ_Inode(Mat,MatOption,PetscBool);
PETSC_INTERN PetscErrorCode MatDuplicate_SeqAIJ_Inode(Mat,MatDuplicateOption,Mat*);
PETSC_INTERN PetscErrorCode MatDuplicateNoCreate_SeqAIJ(Mat,Mat,MatDuplicateOption,PetscBool);
PETSC_INTERN PetscErrorCode MatLUFactorNumeric_SeqAIJ_Inode_inplace(Mat,Mat,const MatFactorInfo*);
PETSC_INTERN PetscErrorCode MatLUFactorNumeric_SeqAIJ_Inode(Mat,Mat,const MatFactorInfo*);

typedef struct {
  SEQAIJHEADER(MatScalar);
  Mat_SeqAIJ_Inode inode;
  MatScalar        *saved_values;             /* location for stashing nonzero values of matrix */

  PetscScalar *idiag,*mdiag,*ssor_work;       /* inverse of diagonal entries, diagonal values and workspace for Eisenstat trick */
  PetscBool   idiagvalid;                     /* current idiag[] and mdiag[] are valid */
  PetscScalar *ibdiag;                        /* inverses of block diagonals */
  PetscBool   ibdiagvalid;                    /* inverses of block diagonals are valid. */
  PetscScalar fshift,omega;                   /* last used omega and fshift */

  ISColoring coloring;                        /* set with MatADSetColoring() used by MatADSetValues() */

  PetscScalar       *matmult_abdense;    /* used by MatMatMult() */
  Mat_PtAP          *ptap;               /* used by MatPtAP() */
  Mat_MatMatMatMult *matmatmatmult;      /* used by MatMatMatMult() */
  Mat_RARt          *rart;               /* used by MatRARt() */
  Mat_MatMatTransMult *abt;              /* used by MatMatTransposeMult() */
} Mat_SeqAIJ;

/*
  Frees the a, i, and j arrays from the XAIJ (AIJ, BAIJ, and SBAIJ) matrix types
*/
#undef __FUNCT__
#define __FUNCT__ "MatSeqXAIJFreeAIJ"
PETSC_STATIC_INLINE PetscErrorCode MatSeqXAIJFreeAIJ(Mat AA,MatScalar **a,PetscInt **j,PetscInt **i)
{
  PetscErrorCode ierr;
  Mat_SeqAIJ     *A = (Mat_SeqAIJ*) AA->data;
  if (A->singlemalloc) {
    ierr = PetscFree3(*a,*j,*i);CHKERRQ(ierr);
  } else {
    if (A->free_a)  {ierr = PetscFree(*a);CHKERRQ(ierr);}
    if (A->free_ij) {ierr = PetscFree(*j);CHKERRQ(ierr);}
    if (A->free_ij) {ierr = PetscFree(*i);CHKERRQ(ierr);}
  }
  return 0;
}
/*
    Allocates larger a, i, and j arrays for the XAIJ (AIJ, BAIJ, and SBAIJ) matrix types
    This is a macro because it takes the datatype as an argument which can be either a Mat or a MatScalar
*/
#define MatSeqXAIJReallocateAIJ(Amat,AM,BS2,NROW,ROW,COL,RMAX,AA,AI,AJ,RP,AP,AIMAX,NONEW,datatype) \
  if (NROW >= RMAX) { \
    Mat_SeqAIJ *Ain = (Mat_SeqAIJ*)Amat->data; \
    /* there is no extra room in row, therefore enlarge */ \
    PetscInt CHUNKSIZE = 15,new_nz = AI[AM] + CHUNKSIZE,len,*new_i=0,*new_j=0; \
    datatype *new_a; \
 \
    if (NONEW == -2) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"New nonzero at (%D,%D) caused a malloc\nUse MatSetOption(A, MAT_NEW_NONZERO_ALLOCATION_ERR, PETSC_FALSE) to turn off this check",ROW,COL); \
    /* malloc new storage space */ \
    ierr = PetscMalloc3(BS2*new_nz,&new_a,new_nz,&new_j,AM+1,&new_i);CHKERRQ(ierr); \
 \
    /* copy over old data into new slots */ \
    for (ii=0; ii<ROW+1; ii++) {new_i[ii] = AI[ii];} \
    for (ii=ROW+1; ii<AM+1; ii++) {new_i[ii] = AI[ii]+CHUNKSIZE;} \
    ierr = PetscMemcpy(new_j,AJ,(AI[ROW]+NROW)*sizeof(PetscInt));CHKERRQ(ierr); \
    len  = (new_nz - CHUNKSIZE - AI[ROW] - NROW); \
    ierr = PetscMemcpy(new_j+AI[ROW]+NROW+CHUNKSIZE,AJ+AI[ROW]+NROW,len*sizeof(PetscInt));CHKERRQ(ierr); \
    ierr = PetscMemcpy(new_a,AA,BS2*(AI[ROW]+NROW)*sizeof(datatype));CHKERRQ(ierr); \
    ierr = PetscMemzero(new_a+BS2*(AI[ROW]+NROW),BS2*CHUNKSIZE*sizeof(datatype));CHKERRQ(ierr); \
    ierr = PetscMemcpy(new_a+BS2*(AI[ROW]+NROW+CHUNKSIZE),AA+BS2*(AI[ROW]+NROW),BS2*len*sizeof(datatype));CHKERRQ(ierr);  \
    /* free up old matrix storage */ \
    ierr              = MatSeqXAIJFreeAIJ(A,&Ain->a,&Ain->j,&Ain->i);CHKERRQ(ierr); \
    AA                = new_a; \
    Ain->a            = (MatScalar*) new_a;                   \
    AI                = Ain->i = new_i; AJ = Ain->j = new_j;  \
    Ain->singlemalloc = PETSC_TRUE; \
 \
    RP          = AJ + AI[ROW]; AP = AA + BS2*AI[ROW]; \
    RMAX        = AIMAX[ROW] = AIMAX[ROW] + CHUNKSIZE; \
    Ain->maxnz += BS2*CHUNKSIZE; \
    Ain->reallocs++; \
  } \


PETSC_EXTERN PetscErrorCode MatSeqAIJSetPreallocation_SeqAIJ(Mat,PetscInt,const PetscInt*);
PETSC_INTERN PetscErrorCode MatILUFactorSymbolic_SeqAIJ_inplace(Mat,Mat,IS,IS,const MatFactorInfo*);
PETSC_INTERN PetscErrorCode MatILUFactorSymbolic_SeqAIJ(Mat,Mat,IS,IS,const MatFactorInfo*);
PETSC_INTERN PetscErrorCode MatILUFactorSymbolic_SeqAIJ_ilu0(Mat,Mat,IS,IS,const MatFactorInfo*);

PETSC_INTERN PetscErrorCode MatICCFactorSymbolic_SeqAIJ_inplace(Mat,Mat,IS,const MatFactorInfo*);
PETSC_INTERN PetscErrorCode MatICCFactorSymbolic_SeqAIJ(Mat,Mat,IS,const MatFactorInfo*);
PETSC_INTERN PetscErrorCode MatCholeskyFactorSymbolic_SeqAIJ_inplace(Mat,Mat,IS,const MatFactorInfo*);
PETSC_INTERN PetscErrorCode MatCholeskyFactorSymbolic_SeqAIJ(Mat,Mat,IS,const MatFactorInfo*);
PETSC_INTERN PetscErrorCode MatCholeskyFactorNumeric_SeqAIJ_inplace(Mat,Mat,const MatFactorInfo*);
PETSC_INTERN PetscErrorCode MatCholeskyFactorNumeric_SeqAIJ(Mat,Mat,const MatFactorInfo*);
PETSC_INTERN PetscErrorCode MatDuplicate_SeqAIJ(Mat,MatDuplicateOption,Mat*);
PETSC_INTERN PetscErrorCode MatCopy_SeqAIJ(Mat,Mat,MatStructure);
PETSC_INTERN PetscErrorCode MatMissingDiagonal_SeqAIJ(Mat,PetscBool*,PetscInt*);
PETSC_INTERN PetscErrorCode MatMarkDiagonal_SeqAIJ(Mat);
PETSC_INTERN PetscErrorCode MatFindZeroDiagonals_SeqAIJ_Private(Mat,PetscInt*,PetscInt**);

PETSC_INTERN PetscErrorCode MatMult_SeqAIJ(Mat A,Vec,Vec);
PETSC_INTERN PetscErrorCode MatMultAdd_SeqAIJ(Mat A,Vec,Vec,Vec);
PETSC_INTERN PetscErrorCode MatMultTranspose_SeqAIJ(Mat A,Vec,Vec);
PETSC_INTERN PetscErrorCode MatMultTransposeAdd_SeqAIJ(Mat A,Vec,Vec,Vec);
PETSC_INTERN PetscErrorCode MatSOR_SeqAIJ(Mat,Vec,PetscReal,MatSORType,PetscReal,PetscInt,PetscInt,Vec);

PETSC_INTERN PetscErrorCode MatSetOption_SeqAIJ(Mat,MatOption,PetscBool);
PETSC_INTERN PetscErrorCode MatSetColoring_SeqAIJ(Mat,ISColoring);
PETSC_INTERN PetscErrorCode MatSetValuesAdifor_SeqAIJ(Mat,PetscInt,void*);

PETSC_INTERN PetscErrorCode MatGetSymbolicTranspose_SeqAIJ(Mat,PetscInt *[],PetscInt *[]);
PETSC_INTERN PetscErrorCode MatGetSymbolicTransposeReduced_SeqAIJ(Mat,PetscInt,PetscInt,PetscInt *[],PetscInt *[]);
PETSC_INTERN PetscErrorCode MatRestoreSymbolicTranspose_SeqAIJ(Mat,PetscInt *[],PetscInt *[]);
PETSC_INTERN PetscErrorCode MatTransposeSymbolic_SeqAIJ(Mat,Mat*);
PETSC_INTERN PetscErrorCode MatTranspose_SeqAIJ(Mat,MatReuse,Mat*);
PETSC_INTERN PetscErrorCode MatToSymmetricIJ_SeqAIJ(PetscInt,PetscInt*,PetscInt*,PetscInt,PetscInt,PetscInt**,PetscInt**);
PETSC_INTERN PetscErrorCode MatLUFactorSymbolic_SeqAIJ_inplace(Mat,Mat,IS,IS,const MatFactorInfo*);
PETSC_INTERN PetscErrorCode MatLUFactorSymbolic_SeqAIJ(Mat,Mat,IS,IS,const MatFactorInfo*);
PETSC_INTERN PetscErrorCode MatLUFactorNumeric_SeqAIJ_inplace(Mat,Mat,const MatFactorInfo*);
PETSC_INTERN PetscErrorCode MatLUFactorNumeric_SeqAIJ(Mat,Mat,const MatFactorInfo*);
PETSC_INTERN PetscErrorCode MatLUFactorNumeric_SeqAIJ_InplaceWithPerm(Mat,Mat,const MatFactorInfo*);
PETSC_INTERN PetscErrorCode MatLUFactor_SeqAIJ(Mat,IS,IS,const MatFactorInfo*);
PETSC_INTERN PetscErrorCode MatSolve_SeqAIJ_inplace(Mat,Vec,Vec);
PETSC_INTERN PetscErrorCode MatSolve_SeqAIJ(Mat,Vec,Vec);
PETSC_INTERN PetscErrorCode MatSolve_SeqAIJ_Inode_inplace(Mat,Vec,Vec);
PETSC_INTERN PetscErrorCode MatSolve_SeqAIJ_Inode(Mat,Vec,Vec);
PETSC_INTERN PetscErrorCode MatSolve_SeqAIJ_NaturalOrdering_inplace(Mat,Vec,Vec);
PETSC_INTERN PetscErrorCode MatSolve_SeqAIJ_NaturalOrdering(Mat,Vec,Vec);
PETSC_INTERN PetscErrorCode MatSolve_SeqAIJ_InplaceWithPerm(Mat,Vec,Vec);
PETSC_INTERN PetscErrorCode MatSolveAdd_SeqAIJ_inplace(Mat,Vec,Vec,Vec);
PETSC_INTERN PetscErrorCode MatSolveAdd_SeqAIJ(Mat,Vec,Vec,Vec);
PETSC_INTERN PetscErrorCode MatSolveTranspose_SeqAIJ_inplace(Mat,Vec,Vec);
PETSC_INTERN PetscErrorCode MatSolveTranspose_SeqAIJ(Mat,Vec,Vec);
PETSC_INTERN PetscErrorCode MatSolveTransposeAdd_SeqAIJ_inplace(Mat,Vec,Vec,Vec);
PETSC_INTERN PetscErrorCode MatSolveTransposeAdd_SeqAIJ(Mat,Vec,Vec,Vec);
PETSC_INTERN PetscErrorCode MatMatSolve_SeqAIJ_inplace(Mat,Mat,Mat);
PETSC_INTERN PetscErrorCode MatMatSolve_SeqAIJ(Mat,Mat,Mat);
PETSC_INTERN PetscErrorCode MatEqual_SeqAIJ(Mat,Mat,PetscBool*);
PETSC_INTERN PetscErrorCode MatFDColoringCreate_SeqXAIJ(Mat,ISColoring,MatFDColoring);
PETSC_INTERN PetscErrorCode MatFDColoringSetUp_SeqXAIJ(Mat,ISColoring,MatFDColoring);
PETSC_INTERN PetscErrorCode MatFDColoringSetUpBlocked_AIJ_Private(Mat,MatFDColoring,PetscInt);
PETSC_INTERN PetscErrorCode MatFDColoringApply_AIJ(Mat,MatFDColoring,Vec,void*);
PETSC_INTERN PetscErrorCode MatLoad_SeqAIJ(Mat,PetscViewer);
PETSC_INTERN PetscErrorCode RegisterApplyPtAPRoutines_Private(Mat);

PETSC_INTERN PetscErrorCode MatMatMult_SeqAIJ_SeqAIJ(Mat,Mat,MatReuse,PetscReal,Mat*);
PETSC_INTERN PetscErrorCode MatMatMultSymbolic_SeqAIJ_SeqAIJ(Mat,Mat,PetscReal,Mat*);
PETSC_INTERN PetscErrorCode MatMatMultSymbolic_SeqAIJ_SeqAIJ_Scalable(Mat,Mat,PetscReal,Mat*);
PETSC_INTERN PetscErrorCode MatMatMultSymbolic_SeqAIJ_SeqAIJ_Scalable_fast(Mat,Mat,PetscReal,Mat*);
PETSC_INTERN PetscErrorCode MatMatMultSymbolic_SeqAIJ_SeqAIJ_Heap(Mat,Mat,PetscReal,Mat*);
PETSC_INTERN PetscErrorCode MatMatMultSymbolic_SeqAIJ_SeqAIJ_BTHeap(Mat,Mat,PetscReal,Mat*);
PETSC_INTERN PetscErrorCode MatMatMultNumeric_SeqAIJ_SeqAIJ(Mat,Mat,Mat);
PETSC_INTERN PetscErrorCode MatMatMultNumeric_SeqAIJ_SeqAIJ_Scalable(Mat,Mat,Mat);

PETSC_INTERN PetscErrorCode MatPtAP_SeqAIJ_SeqAIJ(Mat,Mat,MatReuse,PetscReal,Mat*);
PETSC_INTERN PetscErrorCode MatPtAPSymbolic_SeqAIJ_SeqAIJ_DenseAxpy(Mat,Mat,PetscReal,Mat*);
PETSC_INTERN PetscErrorCode MatPtAPSymbolic_SeqAIJ_SeqAIJ_SparseAxpy(Mat,Mat,PetscReal,Mat*);
PETSC_INTERN PetscErrorCode MatPtAPNumeric_SeqAIJ_SeqAIJ(Mat,Mat,Mat);
PETSC_INTERN PetscErrorCode MatPtAPNumeric_SeqAIJ_SeqAIJ_SparseAxpy(Mat,Mat,Mat);

PETSC_INTERN PetscErrorCode MatRARtSymbolic_SeqAIJ_SeqAIJ(Mat,Mat,PetscReal,Mat*);
PETSC_INTERN PetscErrorCode MatRARtSymbolic_SeqAIJ_SeqAIJ_matmattransposemult(Mat,Mat,PetscReal,Mat*);
PETSC_INTERN PetscErrorCode MatRARtSymbolic_SeqAIJ_SeqAIJ_colorrart(Mat,Mat,PetscReal,Mat*);
PETSC_INTERN PetscErrorCode MatRARtNumeric_SeqAIJ_SeqAIJ(Mat,Mat,Mat);
PETSC_INTERN PetscErrorCode MatRARtNumeric_SeqAIJ_SeqAIJ_matmattransposemult(Mat,Mat,Mat);
PETSC_INTERN PetscErrorCode MatRARtNumeric_SeqAIJ_SeqAIJ_colorrart(Mat,Mat,Mat);

PETSC_INTERN PetscErrorCode MatTransposeMatMult_SeqAIJ_SeqAIJ(Mat,Mat,MatReuse,PetscReal,Mat*);
PETSC_INTERN PetscErrorCode MatTransposeMatMultSymbolic_SeqAIJ_SeqAIJ(Mat,Mat,PetscReal,Mat*);
PETSC_INTERN PetscErrorCode MatTransposeMatMultNumeric_SeqAIJ_SeqAIJ(Mat,Mat,Mat);

PETSC_INTERN PetscErrorCode MatTransposeMatMult_SeqAIJ_SeqDense(Mat,Mat,MatReuse,PetscReal,Mat*);
PETSC_INTERN PetscErrorCode MatTransposeMatMultSymbolic_SeqAIJ_SeqDense(Mat,Mat,PetscReal,Mat*);
PETSC_INTERN PetscErrorCode MatTransposeMatMultNumeric_SeqAIJ_SeqDense(Mat,Mat,Mat);

PETSC_INTERN PetscErrorCode MatMatTransposeMult_SeqAIJ_SeqAIJ(Mat,Mat,MatReuse,PetscReal,Mat*);
PETSC_INTERN PetscErrorCode MatMatTransposeMultSymbolic_SeqAIJ_SeqAIJ(Mat,Mat,PetscReal,Mat*);
PETSC_INTERN PetscErrorCode MatMatTransposeMultNumeric_SeqAIJ_SeqAIJ(Mat,Mat,Mat);
PETSC_INTERN PetscErrorCode MatTransposeColoringCreate_SeqAIJ(Mat,ISColoring,MatTransposeColoring);
PETSC_INTERN PetscErrorCode MatTransColoringApplySpToDen_SeqAIJ(MatTransposeColoring,Mat,Mat);
PETSC_INTERN PetscErrorCode MatTransColoringApplyDenToSp_SeqAIJ(MatTransposeColoring,Mat,Mat);

PETSC_INTERN PetscErrorCode MatMatMatMult_SeqAIJ_SeqAIJ_SeqAIJ(Mat,Mat,Mat,MatReuse,PetscReal,Mat*);
PETSC_INTERN PetscErrorCode MatMatMatMultSymbolic_SeqAIJ_SeqAIJ_SeqAIJ(Mat,Mat,Mat,PetscReal,Mat*);
PETSC_INTERN PetscErrorCode MatMatMatMultNumeric_SeqAIJ_SeqAIJ_SeqAIJ(Mat,Mat,Mat,Mat);

PETSC_INTERN PetscErrorCode MatSetValues_SeqAIJ(Mat,PetscInt,const PetscInt[],PetscInt,const PetscInt[],const PetscScalar[],InsertMode);
PETSC_INTERN PetscErrorCode MatGetRow_SeqAIJ(Mat,PetscInt,PetscInt*,PetscInt**,PetscScalar**);
PETSC_INTERN PetscErrorCode MatRestoreRow_SeqAIJ(Mat,PetscInt,PetscInt*,PetscInt**,PetscScalar**);
PETSC_INTERN PetscErrorCode MatAXPY_SeqAIJ(Mat,PetscScalar,Mat,MatStructure);
PETSC_INTERN PetscErrorCode MatGetRowIJ_SeqAIJ(Mat,PetscInt,PetscBool,PetscBool,PetscInt*,const PetscInt *[],const PetscInt *[],PetscBool*);
PETSC_INTERN PetscErrorCode MatRestoreRowIJ_SeqAIJ(Mat,PetscInt,PetscBool,PetscBool,PetscInt*,const PetscInt *[],const PetscInt *[],PetscBool*);
PETSC_INTERN PetscErrorCode MatGetColumnIJ_SeqAIJ(Mat,PetscInt,PetscBool,PetscBool,PetscInt*,const PetscInt *[],const PetscInt *[],PetscBool*);
PETSC_INTERN PetscErrorCode MatRestoreColumnIJ_SeqAIJ(Mat,PetscInt,PetscBool,PetscBool,PetscInt*,const PetscInt *[],const PetscInt *[],PetscBool*);
PETSC_INTERN PetscErrorCode MatGetColumnIJ_SeqAIJ_Color(Mat,PetscInt,PetscBool,PetscBool,PetscInt*,const PetscInt *[],const PetscInt *[],PetscInt *[],PetscBool*);
PETSC_INTERN PetscErrorCode MatRestoreColumnIJ_SeqAIJ_Color(Mat,PetscInt,PetscBool,PetscBool,PetscInt*,const PetscInt *[],const PetscInt *[],PetscInt *[],PetscBool*);
PETSC_INTERN PetscErrorCode MatDestroy_SeqAIJ(Mat);
PETSC_INTERN PetscErrorCode MatSetUp_SeqAIJ(Mat);
PETSC_INTERN PetscErrorCode MatView_SeqAIJ(Mat,PetscViewer);

PETSC_INTERN PetscErrorCode MatSeqAIJInvalidateDiagonal(Mat);
PETSC_INTERN PetscErrorCode MatSeqAIJInvalidateDiagonal_Inode(Mat);
PETSC_INTERN PetscErrorCode MatSeqAIJCheckInode(Mat);
PETSC_INTERN PetscErrorCode MatSeqAIJCheckInode_FactorLU(Mat);

PETSC_INTERN PetscErrorCode MatAXPYGetPreallocation_SeqAIJ(Mat,Mat,PetscInt*);

PETSC_EXTERN PetscErrorCode MatConvert_SeqAIJ_SeqSBAIJ(Mat,MatType,MatReuse,Mat*);
PETSC_EXTERN PetscErrorCode MatConvert_SeqAIJ_SeqBAIJ(Mat,MatType,MatReuse,Mat*);
PETSC_EXTERN PetscErrorCode MatConvert_SeqAIJ_SeqAIJPERM(Mat,MatType,MatReuse,Mat*);
PETSC_INTERN PetscErrorCode MatReorderForNonzeroDiagonal_SeqAIJ(Mat,PetscReal,IS,IS);
PETSC_INTERN PetscErrorCode MatMatMult_SeqDense_SeqAIJ(Mat,Mat,MatReuse,PetscReal,Mat*);
PETSC_INTERN PetscErrorCode MatRARt_SeqAIJ_SeqAIJ(Mat,Mat,MatReuse,PetscReal,Mat*);
PETSC_EXTERN PetscErrorCode MatCreate_SeqAIJ(Mat);
PETSC_INTERN PetscErrorCode MatAssemblyEnd_SeqAIJ(Mat,MatAssemblyType);
PETSC_INTERN PetscErrorCode MatDestroy_SeqAIJ(Mat);

PETSC_INTERN PetscErrorCode MatAXPYGetPreallocation_SeqX_private(PetscInt,const PetscInt*,const PetscInt*,const PetscInt*,const PetscInt*,PetscInt*);
PETSC_INTERN PetscErrorCode MatCreateMPIMatConcatenateSeqMat_SeqAIJ(MPI_Comm,Mat,PetscInt,MatReuse,Mat*);
PETSC_INTERN PetscErrorCode MatCreateMPIMatConcatenateSeqMat_MPIAIJ(MPI_Comm,Mat,PetscInt,MatReuse,Mat*);

PETSC_INTERN PetscErrorCode MatSetSeqMat_SeqAIJ(Mat,IS,IS,MatStructure,Mat);

/*
    PetscSparseDenseMinusDot - The inner kernel of triangular solves and Gauss-Siedel smoothing. \sum_i xv[i] * r[xi[i]] for CSR storage

  Input Parameters:
+  nnz - the number of entries
.  r - the array of vector values
.  xv - the matrix values for the row
-  xi - the column indices of the nonzeros in the row

  Output Parameter:
.  sum - negative the sum of results

  PETSc compile flags:
+   PETSC_KERNEL_USE_UNROLL_4 -   don't use this; it changes nnz and hence is WRONG
-   PETSC_KERNEL_USE_UNROLL_2 -

.seealso: PetscSparseDensePlusDot()

*/
#if defined(PETSC_KERNEL_USE_UNROLL_4)
#define PetscSparseDenseMinusDot(sum,r,xv,xi,nnz) { \
    if (nnz > 0) { \
      switch (nnz & 0x3) { \
      case 3: sum -= *xv++ *r[*xi++]; \
      case 2: sum -= *xv++ *r[*xi++]; \
      case 1: sum -= *xv++ *r[*xi++]; \
        nnz       -= 4;} \
      while (nnz > 0) { \
        sum -=  xv[0] * r[xi[0]] - xv[1] * r[xi[1]] - \
               xv[2] * r[xi[2]] - xv[3] * r[xi[3]]; \
        xv += 4; xi += 4; nnz -= 4; }}}

#elif defined(PETSC_KERNEL_USE_UNROLL_2)
#define PetscSparseDenseMinusDot(sum,r,xv,xi,nnz) { \
    PetscInt __i,__i1,__i2; \
    for (__i=0; __i<nnz-1; __i+=2) {__i1 = xi[__i]; __i2=xi[__i+1]; \
                                    sum -= (xv[__i]*r[__i1] + xv[__i+1]*r[__i2]);} \
    if (nnz & 0x1) sum -= xv[__i] * r[xi[__i]];}

#else
#define PetscSparseDenseMinusDot(sum,r,xv,xi,nnz) { \
    PetscInt __i; \
    for (__i=0; __i<nnz; __i++) sum -= xv[__i] * r[xi[__i]];}
#endif



/*
    PetscSparseDensePlusDot - The inner kernel of matrix-vector product \sum_i xv[i] * r[xi[i]] for CSR storage

  Input Parameters:
+  nnz - the number of entries
.  r - the array of vector values
.  xv - the matrix values for the row
-  xi - the column indices of the nonzeros in the row

  Output Parameter:
.  sum - the sum of results

  PETSc compile flags:
+   PETSC_KERNEL_USE_UNROLL_4 -  don't use this; it changes nnz and hence is WRONG
-   PETSC_KERNEL_USE_UNROLL_2 -

.seealso: PetscSparseDenseMinusDot()

*/
#if defined(PETSC_KERNEL_USE_UNROLL_4)
#define PetscSparseDensePlusDot(sum,r,xv,xi,nnz) { \
    if (nnz > 0) { \
      switch (nnz & 0x3) { \
      case 3: sum += *xv++ *r[*xi++]; \
      case 2: sum += *xv++ *r[*xi++]; \
      case 1: sum += *xv++ *r[*xi++]; \
        nnz       -= 4;} \
      while (nnz > 0) { \
        sum +=  xv[0] * r[xi[0]] + xv[1] * r[xi[1]] + \
               xv[2] * r[xi[2]] + xv[3] * r[xi[3]]; \
        xv += 4; xi += 4; nnz -= 4; }}}

#elif defined(PETSC_KERNEL_USE_UNROLL_2)
#define PetscSparseDensePlusDot(sum,r,xv,xi,nnz) { \
    PetscInt __i,__i1,__i2; \
    for (__i=0; __i<nnz-1; __i+=2) {__i1 = xi[__i]; __i2=xi[__i+1]; \
                                    sum += (xv[__i]*r[__i1] + xv[__i+1]*r[__i2]);} \
    if (nnz & 0x1) sum += xv[__i] * r[xi[__i]];}

#else
#define PetscSparseDensePlusDot(sum,r,xv,xi,nnz) { \
    PetscInt __i; \
    for (__i=0; __i<nnz; __i++) sum += xv[__i] * r[xi[__i]];}
#endif


/*
    PetscSparseDenseMaxDot - The inner kernel of a modified matrix-vector product \max_i xv[i] * r[xi[i]] for CSR storage

  Input Parameters:
+  nnz - the number of entries
.  r - the array of vector values
.  xv - the matrix values for the row
-  xi - the column indices of the nonzeros in the row

  Output Parameter:
.  max - the max of results

.seealso: PetscSparseDensePlusDot(), PetscSparseDenseMinusDot()

*/
#define PetscSparseDenseMaxDot(max,r,xv,xi,nnz) { \
    PetscInt __i; \
    for (__i=0; __i<nnz; __i++) max = PetscMax(PetscRealPart(max), PetscRealPart(xv[__i] * r[xi[__i]]));}

/*
 Add column indices into table for counting the max nonzeros of merged rows
 */
#define MatRowMergeMax_SeqAIJ(mat,nrows,ta) {       \
    PetscInt _j,_row,_nz,*_col;                     \
    for (_row=0; _row<nrows; _row++) {\
      _nz = mat->i[_row+1] - mat->i[_row]; \
      for (_j=0; _j<_nz; _j++) {                \
        _col = _j + mat->j + mat->i[_row];       \
        PetscTableAdd(ta,*_col+1,1,INSERT_VALUES); \
      }                                                                 \
    }                                                                   \
}

/*
 Add column indices into table for counting the nonzeros of merged rows
 */
#define MatMergeRows_SeqAIJ(mat,nrows,rows,ta) {    \
  PetscInt _j,_row,_nz,*_col,_i;                      \
    for (_i=0; _i<nrows; _i++) {\
      _row = rows[_i]; \
      _nz = mat->i[_row+1] - mat->i[_row]; \
      for (_j=0; _j<_nz; _j++) {                \
        _col = _j + mat->j + mat->i[_row];       \
        PetscTableAdd(ta,*_col+1,1,INSERT_VALUES); \
      }                                                                 \
    }                                                                   \
}

#endif
