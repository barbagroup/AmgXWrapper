/*
  Defines the basic matrix operations for the AIJ (compressed row)
  matrix storage format using the CUSPARSE library,
*/
#define PETSC_SKIP_SPINLOCK

#include <petscconf.h>
#include <../src/mat/impls/aij/seq/aij.h>          /*I "petscmat.h" I*/
#include <../src/mat/impls/sbaij/seq/sbaij.h>
#include <../src/vec/vec/impls/dvecimpl.h>
#include <petsc/private/vecimpl.h>
#undef VecType
#include <../src/mat/impls/aij/seq/seqcusparse/cusparsematimpl.h>

const char *const MatCUSPARSEStorageFormats[] = {"CSR","ELL","HYB","MatCUSPARSEStorageFormat","MAT_CUSPARSE_",0};

static PetscErrorCode MatICCFactorSymbolic_SeqAIJCUSPARSE(Mat,Mat,IS,const MatFactorInfo*);
static PetscErrorCode MatCholeskyFactorSymbolic_SeqAIJCUSPARSE(Mat,Mat,IS,const MatFactorInfo*);
static PetscErrorCode MatCholeskyFactorNumeric_SeqAIJCUSPARSE(Mat,Mat,const MatFactorInfo*);

static PetscErrorCode MatILUFactorSymbolic_SeqAIJCUSPARSE(Mat,Mat,IS,IS,const MatFactorInfo*);
static PetscErrorCode MatLUFactorSymbolic_SeqAIJCUSPARSE(Mat,Mat,IS,IS,const MatFactorInfo*);
static PetscErrorCode MatLUFactorNumeric_SeqAIJCUSPARSE(Mat,Mat,const MatFactorInfo*);

static PetscErrorCode MatSolve_SeqAIJCUSPARSE(Mat,Vec,Vec);
static PetscErrorCode MatSolve_SeqAIJCUSPARSE_NaturalOrdering(Mat,Vec,Vec);
static PetscErrorCode MatSolveTranspose_SeqAIJCUSPARSE(Mat,Vec,Vec);
static PetscErrorCode MatSolveTranspose_SeqAIJCUSPARSE_NaturalOrdering(Mat,Vec,Vec);
static PetscErrorCode MatSetFromOptions_SeqAIJCUSPARSE(PetscOptionItems *PetscOptionsObject,Mat);
static PetscErrorCode MatMult_SeqAIJCUSPARSE(Mat,Vec,Vec);
static PetscErrorCode MatMultAdd_SeqAIJCUSPARSE(Mat,Vec,Vec,Vec);
static PetscErrorCode MatMultTranspose_SeqAIJCUSPARSE(Mat,Vec,Vec);
static PetscErrorCode MatMultTransposeAdd_SeqAIJCUSPARSE(Mat,Vec,Vec,Vec);

static PetscErrorCode CsrMatrix_Destroy(CsrMatrix**);
static PetscErrorCode Mat_SeqAIJCUSPARSETriFactorStruct_Destroy(Mat_SeqAIJCUSPARSETriFactorStruct**);
static PetscErrorCode Mat_SeqAIJCUSPARSEMultStruct_Destroy(Mat_SeqAIJCUSPARSEMultStruct**,MatCUSPARSEStorageFormat);
static PetscErrorCode Mat_SeqAIJCUSPARSETriFactors_Destroy(Mat_SeqAIJCUSPARSETriFactors**);
static PetscErrorCode Mat_SeqAIJCUSPARSE_Destroy(Mat_SeqAIJCUSPARSE**);

#undef __FUNCT__
#define __FUNCT__ "MatCUSPARSESetStream"
PetscErrorCode MatCUSPARSESetStream(Mat A,const cudaStream_t stream)
{
  cusparseStatus_t   stat;
  Mat_SeqAIJCUSPARSE *cusparsestruct = (Mat_SeqAIJCUSPARSE*)A->spptr;

  PetscFunctionBegin;
  cusparsestruct->stream = stream;
  stat = cusparseSetStream(cusparsestruct->handle,cusparsestruct->stream);CHKERRCUSP(stat);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatCUSPARSESetHandle"
PetscErrorCode MatCUSPARSESetHandle(Mat A,const cusparseHandle_t handle)
{
  cusparseStatus_t   stat;
  Mat_SeqAIJCUSPARSE *cusparsestruct = (Mat_SeqAIJCUSPARSE*)A->spptr;

  PetscFunctionBegin;
  if (cusparsestruct->handle)
    stat = cusparseDestroy(cusparsestruct->handle);CHKERRCUSP(stat);
  cusparsestruct->handle = handle;
  stat = cusparseSetPointerMode(cusparsestruct->handle, CUSPARSE_POINTER_MODE_DEVICE);CHKERRCUSP(stat);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatCUSPARSEClearHandle"
PetscErrorCode MatCUSPARSEClearHandle(Mat A)
{
  Mat_SeqAIJCUSPARSE *cusparsestruct = (Mat_SeqAIJCUSPARSE*)A->spptr;
  PetscFunctionBegin;
  if (cusparsestruct->handle)
    cusparsestruct->handle = 0;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatFactorGetSolverPackage_seqaij_cusparse"
PetscErrorCode MatFactorGetSolverPackage_seqaij_cusparse(Mat A,const MatSolverPackage *type)
{
  PetscFunctionBegin;
  *type = MATSOLVERCUSPARSE;
  PetscFunctionReturn(0);
}

/*MC
  MATSOLVERCUSPARSE = "cusparse" - A matrix type providing triangular solvers for seq matrices
  on a single GPU of type, seqaijcusparse, aijcusparse, or seqaijcusp, aijcusp. Currently supported
  algorithms are ILU(k) and ICC(k). Typically, deeper factorizations (larger k) results in poorer
  performance in the triangular solves. Full LU, and Cholesky decompositions can be solved through the
  CUSPARSE triangular solve algorithm. However, the performance can be quite poor and thus these
  algorithms are not recommended. This class does NOT support direct solver operations.

  Level: beginner

.seealso: PCFactorSetMatSolverPackage(), MatSolverPackage, MatCreateSeqAIJCUSPARSE(), MATAIJCUSPARSE, MatCreateAIJCUSPARSE(), MatCUSPARSESetFormat(), MatCUSPARSEStorageFormat, MatCUSPARSEFormatOperation
M*/

#undef __FUNCT__
#define __FUNCT__ "MatGetFactor_seqaijcusparse_cusparse"
PETSC_EXTERN PetscErrorCode MatGetFactor_seqaijcusparse_cusparse(Mat A,MatFactorType ftype,Mat *B)
{
  PetscErrorCode ierr;
  PetscInt       n = A->rmap->n;

  PetscFunctionBegin;
  ierr = MatCreate(PetscObjectComm((PetscObject)A),B);CHKERRQ(ierr);
  (*B)->factortype = ftype;
  ierr = MatSetSizes(*B,n,n,n,n);CHKERRQ(ierr);
  ierr = MatSetType(*B,MATSEQAIJCUSPARSE);CHKERRQ(ierr);

  if (ftype == MAT_FACTOR_LU || ftype == MAT_FACTOR_ILU || ftype == MAT_FACTOR_ILUDT) {
    ierr = MatSetBlockSizesFromMats(*B,A,A);CHKERRQ(ierr);
    (*B)->ops->ilufactorsymbolic = MatILUFactorSymbolic_SeqAIJCUSPARSE;
    (*B)->ops->lufactorsymbolic  = MatLUFactorSymbolic_SeqAIJCUSPARSE;
  } else if (ftype == MAT_FACTOR_CHOLESKY || ftype == MAT_FACTOR_ICC) {
    (*B)->ops->iccfactorsymbolic      = MatICCFactorSymbolic_SeqAIJCUSPARSE;
    (*B)->ops->choleskyfactorsymbolic = MatCholeskyFactorSymbolic_SeqAIJCUSPARSE;
  } else SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SUP,"Factor type not supported for CUSPARSE Matrix Types");

  ierr = MatSeqAIJSetPreallocation(*B,MAT_SKIP_ALLOCATION,NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)(*B),"MatFactorGetSolverPackage_C",MatFactorGetSolverPackage_seqaij_cusparse);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatCUSPARSESetFormat_SeqAIJCUSPARSE"
PETSC_INTERN PetscErrorCode MatCUSPARSESetFormat_SeqAIJCUSPARSE(Mat A,MatCUSPARSEFormatOperation op,MatCUSPARSEStorageFormat format)
{
  Mat_SeqAIJCUSPARSE *cusparsestruct = (Mat_SeqAIJCUSPARSE*)A->spptr;

  PetscFunctionBegin;
#if CUDA_VERSION>=4020
  switch (op) {
  case MAT_CUSPARSE_MULT:
    cusparsestruct->format = format;
    break;
  case MAT_CUSPARSE_ALL:
    cusparsestruct->format = format;
    break;
  default:
    SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_SUP,"unsupported operation %d for MatCUSPARSEFormatOperation. MAT_CUSPARSE_MULT and MAT_CUSPARSE_ALL are currently supported.",op);
  }
#else
  if (format==MAT_CUSPARSE_ELL || format==MAT_CUSPARSE_HYB) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SUP,"ELL (Ellpack) and HYB (Hybrid) storage format require CUDA 4.2 or later.");
#endif
  PetscFunctionReturn(0);
}

/*@
   MatCUSPARSESetFormat - Sets the storage format of CUSPARSE matrices for a particular
   operation. Only the MatMult operation can use different GPU storage formats
   for MPIAIJCUSPARSE matrices.
   Not Collective

   Input Parameters:
+  A - Matrix of type SEQAIJCUSPARSE
.  op - MatCUSPARSEFormatOperation. SEQAIJCUSPARSE matrices support MAT_CUSPARSE_MULT and MAT_CUSPARSE_ALL. MPIAIJCUSPARSE matrices support MAT_CUSPARSE_MULT_DIAG, MAT_CUSPARSE_MULT_OFFDIAG, and MAT_CUSPARSE_ALL.
-  format - MatCUSPARSEStorageFormat (one of MAT_CUSPARSE_CSR, MAT_CUSPARSE_ELL, MAT_CUSPARSE_HYB. The latter two require CUDA 4.2)

   Output Parameter:

   Level: intermediate

.seealso: MatCUSPARSEStorageFormat, MatCUSPARSEFormatOperation
@*/
#undef __FUNCT__
#define __FUNCT__ "MatCUSPARSESetFormat"
PetscErrorCode MatCUSPARSESetFormat(Mat A,MatCUSPARSEFormatOperation op,MatCUSPARSEStorageFormat format)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(A, MAT_CLASSID,1);
  ierr = PetscTryMethod(A, "MatCUSPARSESetFormat_C",(Mat,MatCUSPARSEFormatOperation,MatCUSPARSEStorageFormat),(A,op,format));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSetFromOptions_SeqAIJCUSPARSE"
static PetscErrorCode MatSetFromOptions_SeqAIJCUSPARSE(PetscOptionItems *PetscOptionsObject,Mat A)
{
  PetscErrorCode           ierr;
  MatCUSPARSEStorageFormat format;
  PetscBool                flg;
  Mat_SeqAIJCUSPARSE       *cusparsestruct = (Mat_SeqAIJCUSPARSE*)A->spptr;

  PetscFunctionBegin;
  ierr = PetscOptionsHead(PetscOptionsObject,"SeqAIJCUSPARSE options");CHKERRQ(ierr);
  ierr = PetscObjectOptionsBegin((PetscObject)A);
  if (A->factortype==MAT_FACTOR_NONE) {
    ierr = PetscOptionsEnum("-mat_cusparse_mult_storage_format","sets storage format of (seq)aijcusparse gpu matrices for SpMV",
                            "MatCUSPARSESetFormat",MatCUSPARSEStorageFormats,(PetscEnum)cusparsestruct->format,(PetscEnum*)&format,&flg);CHKERRQ(ierr);
    if (flg) {
      ierr = MatCUSPARSESetFormat(A,MAT_CUSPARSE_MULT,format);CHKERRQ(ierr);
    }
  } 
  ierr = PetscOptionsEnum("-mat_cusparse_storage_format","sets storage format of (seq)aijcusparse gpu matrices for SpMV and TriSolve",
                          "MatCUSPARSESetFormat",MatCUSPARSEStorageFormats,(PetscEnum)cusparsestruct->format,(PetscEnum*)&format,&flg);CHKERRQ(ierr);
  if (flg) {
    ierr = MatCUSPARSESetFormat(A,MAT_CUSPARSE_ALL,format);CHKERRQ(ierr);
  }
  ierr = PetscOptionsEnd();CHKERRQ(ierr);
  PetscFunctionReturn(0);

}

#undef __FUNCT__
#define __FUNCT__ "MatILUFactorSymbolic_SeqAIJCUSPARSE"
static PetscErrorCode MatILUFactorSymbolic_SeqAIJCUSPARSE(Mat B,Mat A,IS isrow,IS iscol,const MatFactorInfo *info)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MatILUFactorSymbolic_SeqAIJ(B,A,isrow,iscol,info);CHKERRQ(ierr);
  B->ops->lufactornumeric = MatLUFactorNumeric_SeqAIJCUSPARSE;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatLUFactorSymbolic_SeqAIJCUSPARSE"
static PetscErrorCode MatLUFactorSymbolic_SeqAIJCUSPARSE(Mat B,Mat A,IS isrow,IS iscol,const MatFactorInfo *info)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MatLUFactorSymbolic_SeqAIJ(B,A,isrow,iscol,info);CHKERRQ(ierr);
  B->ops->lufactornumeric = MatLUFactorNumeric_SeqAIJCUSPARSE;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatICCFactorSymbolic_SeqAIJCUSPARSE"
static PetscErrorCode MatICCFactorSymbolic_SeqAIJCUSPARSE(Mat B,Mat A,IS perm,const MatFactorInfo *info)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MatICCFactorSymbolic_SeqAIJ(B,A,perm,info);CHKERRQ(ierr);
  B->ops->choleskyfactornumeric = MatCholeskyFactorNumeric_SeqAIJCUSPARSE;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatCholeskyFactorSymbolic_SeqAIJCUSPARSE"
static PetscErrorCode MatCholeskyFactorSymbolic_SeqAIJCUSPARSE(Mat B,Mat A,IS perm,const MatFactorInfo *info)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MatCholeskyFactorSymbolic_SeqAIJ(B,A,perm,info);CHKERRQ(ierr);
  B->ops->choleskyfactornumeric = MatCholeskyFactorNumeric_SeqAIJCUSPARSE;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSeqAIJCUSPARSEBuildILULowerTriMatrix"
static PetscErrorCode MatSeqAIJCUSPARSEBuildILULowerTriMatrix(Mat A)
{
  Mat_SeqAIJ                        *a = (Mat_SeqAIJ*)A->data;
  PetscInt                          n = A->rmap->n;
  Mat_SeqAIJCUSPARSETriFactors      *cusparseTriFactors = (Mat_SeqAIJCUSPARSETriFactors*)A->spptr;
  Mat_SeqAIJCUSPARSETriFactorStruct *loTriFactor = (Mat_SeqAIJCUSPARSETriFactorStruct*)cusparseTriFactors->loTriFactorPtr;
  cusparseStatus_t                  stat;
  const PetscInt                    *ai = a->i,*aj = a->j,*vi;
  const MatScalar                   *aa = a->a,*v;
  PetscInt                          *AiLo, *AjLo;
  PetscScalar                       *AALo;
  PetscInt                          i,nz, nzLower, offset, rowOffset;
  PetscErrorCode                    ierr;

  PetscFunctionBegin;
  if (A->valid_GPU_matrix == PETSC_CUSP_UNALLOCATED || A->valid_GPU_matrix == PETSC_CUSP_CPU) {
    try {
      /* first figure out the number of nonzeros in the lower triangular matrix including 1's on the diagonal. */
      nzLower=n+ai[n]-ai[1];

      /* Allocate Space for the lower triangular matrix */
      ierr = cudaMallocHost((void**) &AiLo, (n+1)*sizeof(PetscInt));CHKERRCUSP(ierr);
      ierr = cudaMallocHost((void**) &AjLo, nzLower*sizeof(PetscInt));CHKERRCUSP(ierr);
      ierr = cudaMallocHost((void**) &AALo, nzLower*sizeof(PetscScalar));CHKERRCUSP(ierr);

      /* Fill the lower triangular matrix */
      AiLo[0]  = (PetscInt) 0;
      AiLo[n]  = nzLower;
      AjLo[0]  = (PetscInt) 0;
      AALo[0]  = (MatScalar) 1.0;
      v        = aa;
      vi       = aj;
      offset   = 1;
      rowOffset= 1;
      for (i=1; i<n; i++) {
        nz = ai[i+1] - ai[i];
        /* additional 1 for the term on the diagonal */
        AiLo[i]    = rowOffset;
        rowOffset += nz+1;

        ierr = PetscMemcpy(&(AjLo[offset]), vi, nz*sizeof(PetscInt));CHKERRQ(ierr);
        ierr = PetscMemcpy(&(AALo[offset]), v, nz*sizeof(PetscScalar));CHKERRQ(ierr);

        offset      += nz;
        AjLo[offset] = (PetscInt) i;
        AALo[offset] = (MatScalar) 1.0;
        offset      += 1;

        v  += nz;
        vi += nz;
      }

      /* allocate space for the triangular factor information */
      loTriFactor = new Mat_SeqAIJCUSPARSETriFactorStruct;

      /* Create the matrix description */
      stat = cusparseCreateMatDescr(&loTriFactor->descr);CHKERRCUSP(stat);
      stat = cusparseSetMatIndexBase(loTriFactor->descr, CUSPARSE_INDEX_BASE_ZERO);CHKERRCUSP(stat);
      stat = cusparseSetMatType(loTriFactor->descr, CUSPARSE_MATRIX_TYPE_TRIANGULAR);CHKERRCUSP(stat);
      stat = cusparseSetMatFillMode(loTriFactor->descr, CUSPARSE_FILL_MODE_LOWER);CHKERRCUSP(stat);
      stat = cusparseSetMatDiagType(loTriFactor->descr, CUSPARSE_DIAG_TYPE_UNIT);CHKERRCUSP(stat);

      /* Create the solve analysis information */
      stat = cusparseCreateSolveAnalysisInfo(&loTriFactor->solveInfo);CHKERRCUSP(stat);

      /* set the operation */
      loTriFactor->solveOp = CUSPARSE_OPERATION_NON_TRANSPOSE;

      /* set the matrix */
      loTriFactor->csrMat = new CsrMatrix;
      loTriFactor->csrMat->num_rows = n;
      loTriFactor->csrMat->num_cols = n;
      loTriFactor->csrMat->num_entries = nzLower;

      loTriFactor->csrMat->row_offsets = new THRUSTINTARRAY32(n+1);
      loTriFactor->csrMat->row_offsets->assign(AiLo, AiLo+n+1);

      loTriFactor->csrMat->column_indices = new THRUSTINTARRAY32(nzLower);
      loTriFactor->csrMat->column_indices->assign(AjLo, AjLo+nzLower);

      loTriFactor->csrMat->values = new THRUSTARRAY(nzLower);
      loTriFactor->csrMat->values->assign(AALo, AALo+nzLower);

      /* perform the solve analysis */
      stat = cusparse_analysis(cusparseTriFactors->handle, loTriFactor->solveOp,
                               loTriFactor->csrMat->num_rows, loTriFactor->csrMat->num_entries, loTriFactor->descr,
                               loTriFactor->csrMat->values->data().get(), loTriFactor->csrMat->row_offsets->data().get(),
                               loTriFactor->csrMat->column_indices->data().get(), loTriFactor->solveInfo);CHKERRCUSP(stat);

      /* assign the pointer. Is this really necessary? */
      ((Mat_SeqAIJCUSPARSETriFactors*)A->spptr)->loTriFactorPtr = loTriFactor;

      ierr = cudaFreeHost(AiLo);CHKERRCUSP(ierr);
      ierr = cudaFreeHost(AjLo);CHKERRCUSP(ierr);
      ierr = cudaFreeHost(AALo);CHKERRCUSP(ierr);
    } catch(char *ex) {
      SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"CUSPARSE error: %s", ex);
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSeqAIJCUSPARSEBuildILUUpperTriMatrix"
static PetscErrorCode MatSeqAIJCUSPARSEBuildILUUpperTriMatrix(Mat A)
{
  Mat_SeqAIJ                        *a = (Mat_SeqAIJ*)A->data;
  PetscInt                          n = A->rmap->n;
  Mat_SeqAIJCUSPARSETriFactors      *cusparseTriFactors = (Mat_SeqAIJCUSPARSETriFactors*)A->spptr;
  Mat_SeqAIJCUSPARSETriFactorStruct *upTriFactor = (Mat_SeqAIJCUSPARSETriFactorStruct*)cusparseTriFactors->upTriFactorPtr;
  cusparseStatus_t                  stat;
  const PetscInt                    *aj = a->j,*adiag = a->diag,*vi;
  const MatScalar                   *aa = a->a,*v;
  PetscInt                          *AiUp, *AjUp;
  PetscScalar                       *AAUp;
  PetscInt                          i,nz, nzUpper, offset;
  PetscErrorCode                    ierr;

  PetscFunctionBegin;
  if (A->valid_GPU_matrix == PETSC_CUSP_UNALLOCATED || A->valid_GPU_matrix == PETSC_CUSP_CPU) {
    try {
      /* next, figure out the number of nonzeros in the upper triangular matrix. */
      nzUpper = adiag[0]-adiag[n];

      /* Allocate Space for the upper triangular matrix */
      ierr = cudaMallocHost((void**) &AiUp, (n+1)*sizeof(PetscInt));CHKERRCUSP(ierr);
      ierr = cudaMallocHost((void**) &AjUp, nzUpper*sizeof(PetscInt));CHKERRCUSP(ierr);
      ierr = cudaMallocHost((void**) &AAUp, nzUpper*sizeof(PetscScalar));CHKERRCUSP(ierr);

      /* Fill the upper triangular matrix */
      AiUp[0]=(PetscInt) 0;
      AiUp[n]=nzUpper;
      offset = nzUpper;
      for (i=n-1; i>=0; i--) {
        v  = aa + adiag[i+1] + 1;
        vi = aj + adiag[i+1] + 1;

        /* number of elements NOT on the diagonal */
        nz = adiag[i] - adiag[i+1]-1;

        /* decrement the offset */
        offset -= (nz+1);

        /* first, set the diagonal elements */
        AjUp[offset] = (PetscInt) i;
        AAUp[offset] = 1./v[nz];
        AiUp[i]      = AiUp[i+1] - (nz+1);

        ierr = PetscMemcpy(&(AjUp[offset+1]), vi, nz*sizeof(PetscInt));CHKERRQ(ierr);
        ierr = PetscMemcpy(&(AAUp[offset+1]), v, nz*sizeof(PetscScalar));CHKERRQ(ierr);
      }

      /* allocate space for the triangular factor information */
      upTriFactor = new Mat_SeqAIJCUSPARSETriFactorStruct;

      /* Create the matrix description */
      stat = cusparseCreateMatDescr(&upTriFactor->descr);CHKERRCUSP(stat);
      stat = cusparseSetMatIndexBase(upTriFactor->descr, CUSPARSE_INDEX_BASE_ZERO);CHKERRCUSP(stat);
      stat = cusparseSetMatType(upTriFactor->descr, CUSPARSE_MATRIX_TYPE_TRIANGULAR);CHKERRCUSP(stat);
      stat = cusparseSetMatFillMode(upTriFactor->descr, CUSPARSE_FILL_MODE_UPPER);CHKERRCUSP(stat);
      stat = cusparseSetMatDiagType(upTriFactor->descr, CUSPARSE_DIAG_TYPE_NON_UNIT);CHKERRCUSP(stat);

      /* Create the solve analysis information */
      stat = cusparseCreateSolveAnalysisInfo(&upTriFactor->solveInfo);CHKERRCUSP(stat);

      /* set the operation */
      upTriFactor->solveOp = CUSPARSE_OPERATION_NON_TRANSPOSE;

      /* set the matrix */
      upTriFactor->csrMat = new CsrMatrix;
      upTriFactor->csrMat->num_rows = n;
      upTriFactor->csrMat->num_cols = n;
      upTriFactor->csrMat->num_entries = nzUpper;

      upTriFactor->csrMat->row_offsets = new THRUSTINTARRAY32(n+1);
      upTriFactor->csrMat->row_offsets->assign(AiUp, AiUp+n+1);

      upTriFactor->csrMat->column_indices = new THRUSTINTARRAY32(nzUpper);
      upTriFactor->csrMat->column_indices->assign(AjUp, AjUp+nzUpper);

      upTriFactor->csrMat->values = new THRUSTARRAY(nzUpper);
      upTriFactor->csrMat->values->assign(AAUp, AAUp+nzUpper);

      /* perform the solve analysis */
      stat = cusparse_analysis(cusparseTriFactors->handle, upTriFactor->solveOp,
                               upTriFactor->csrMat->num_rows, upTriFactor->csrMat->num_entries, upTriFactor->descr,
                               upTriFactor->csrMat->values->data().get(), upTriFactor->csrMat->row_offsets->data().get(),
                               upTriFactor->csrMat->column_indices->data().get(), upTriFactor->solveInfo);CHKERRCUSP(stat);

      /* assign the pointer. Is this really necessary? */
      ((Mat_SeqAIJCUSPARSETriFactors*)A->spptr)->upTriFactorPtr = upTriFactor;

      ierr = cudaFreeHost(AiUp);CHKERRCUSP(ierr);
      ierr = cudaFreeHost(AjUp);CHKERRCUSP(ierr);
      ierr = cudaFreeHost(AAUp);CHKERRCUSP(ierr);
    } catch(char *ex) {
      SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"CUSPARSE error: %s", ex);
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSeqAIJCUSPARSEILUAnalysisAndCopyToGPU"
static PetscErrorCode MatSeqAIJCUSPARSEILUAnalysisAndCopyToGPU(Mat A)
{
  PetscErrorCode               ierr;
  Mat_SeqAIJ                   *a                  = (Mat_SeqAIJ*)A->data;
  Mat_SeqAIJCUSPARSETriFactors *cusparseTriFactors = (Mat_SeqAIJCUSPARSETriFactors*)A->spptr;
  IS                           isrow = a->row,iscol = a->icol;
  PetscBool                    row_identity,col_identity;
  const PetscInt               *r,*c;
  PetscInt                     n = A->rmap->n;

  PetscFunctionBegin;
  ierr = MatSeqAIJCUSPARSEBuildILULowerTriMatrix(A);CHKERRQ(ierr);
  ierr = MatSeqAIJCUSPARSEBuildILUUpperTriMatrix(A);CHKERRQ(ierr);

  cusparseTriFactors->workVector = new THRUSTARRAY;
  cusparseTriFactors->workVector->resize(n);
  cusparseTriFactors->nnz=a->nz;

  A->valid_GPU_matrix = PETSC_CUSP_BOTH;
  /*lower triangular indices */
  ierr = ISGetIndices(isrow,&r);CHKERRQ(ierr);
  ierr = ISIdentity(isrow,&row_identity);CHKERRQ(ierr);
  if (!row_identity) {
    cusparseTriFactors->rpermIndices = new THRUSTINTARRAY(n);
    cusparseTriFactors->rpermIndices->assign(r, r+n);
  }
  ierr = ISRestoreIndices(isrow,&r);CHKERRQ(ierr);

  /*upper triangular indices */
  ierr = ISGetIndices(iscol,&c);CHKERRQ(ierr);
  ierr = ISIdentity(iscol,&col_identity);CHKERRQ(ierr);
  if (!col_identity) {
    cusparseTriFactors->cpermIndices = new THRUSTINTARRAY(n);
    cusparseTriFactors->cpermIndices->assign(c, c+n);
  }
  ierr = ISRestoreIndices(iscol,&c);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSeqAIJCUSPARSEBuildICCTriMatrices"
static PetscErrorCode MatSeqAIJCUSPARSEBuildICCTriMatrices(Mat A)
{
  Mat_SeqAIJ                        *a = (Mat_SeqAIJ*)A->data;
  Mat_SeqAIJCUSPARSETriFactors      *cusparseTriFactors = (Mat_SeqAIJCUSPARSETriFactors*)A->spptr;
  Mat_SeqAIJCUSPARSETriFactorStruct *loTriFactor = (Mat_SeqAIJCUSPARSETriFactorStruct*)cusparseTriFactors->loTriFactorPtr;
  Mat_SeqAIJCUSPARSETriFactorStruct *upTriFactor = (Mat_SeqAIJCUSPARSETriFactorStruct*)cusparseTriFactors->upTriFactorPtr;
  cusparseStatus_t                  stat;
  PetscErrorCode                    ierr;
  PetscInt                          *AiUp, *AjUp;
  PetscScalar                       *AAUp;
  PetscScalar                       *AALo;
  PetscInt                          nzUpper = a->nz,n = A->rmap->n,i,offset,nz,j;
  Mat_SeqSBAIJ                      *b = (Mat_SeqSBAIJ*)A->data;
  const PetscInt                    *ai = b->i,*aj = b->j,*vj;
  const MatScalar                   *aa = b->a,*v;

  PetscFunctionBegin;
  if (A->valid_GPU_matrix == PETSC_CUSP_UNALLOCATED || A->valid_GPU_matrix == PETSC_CUSP_CPU) {
    try {
      /* Allocate Space for the upper triangular matrix */
      ierr = cudaMallocHost((void**) &AiUp, (n+1)*sizeof(PetscInt));CHKERRCUSP(ierr);
      ierr = cudaMallocHost((void**) &AjUp, nzUpper*sizeof(PetscInt));CHKERRCUSP(ierr);
      ierr = cudaMallocHost((void**) &AAUp, nzUpper*sizeof(PetscScalar));CHKERRCUSP(ierr);
      ierr = cudaMallocHost((void**) &AALo, nzUpper*sizeof(PetscScalar));CHKERRCUSP(ierr);

      /* Fill the upper triangular matrix */
      AiUp[0]=(PetscInt) 0;
      AiUp[n]=nzUpper;
      offset = 0;
      for (i=0; i<n; i++) {
        /* set the pointers */
        v  = aa + ai[i];
        vj = aj + ai[i];
        nz = ai[i+1] - ai[i] - 1; /* exclude diag[i] */

        /* first, set the diagonal elements */
        AjUp[offset] = (PetscInt) i;
        AAUp[offset] = 1.0/v[nz];
        AiUp[i]      = offset;
        AALo[offset] = 1.0/v[nz];

        offset+=1;
        if (nz>0) {
          ierr = PetscMemcpy(&(AjUp[offset]), vj, nz*sizeof(PetscInt));CHKERRQ(ierr);
          ierr = PetscMemcpy(&(AAUp[offset]), v, nz*sizeof(PetscScalar));CHKERRQ(ierr);
          for (j=offset; j<offset+nz; j++) {
            AAUp[j] = -AAUp[j];
            AALo[j] = AAUp[j]/v[nz];
          }
          offset+=nz;
        }
      }

      /* allocate space for the triangular factor information */
      upTriFactor = new Mat_SeqAIJCUSPARSETriFactorStruct;

      /* Create the matrix description */
      stat = cusparseCreateMatDescr(&upTriFactor->descr);CHKERRCUSP(stat);
      stat = cusparseSetMatIndexBase(upTriFactor->descr, CUSPARSE_INDEX_BASE_ZERO);CHKERRCUSP(stat);
      stat = cusparseSetMatType(upTriFactor->descr, CUSPARSE_MATRIX_TYPE_TRIANGULAR);CHKERRCUSP(stat);
      stat = cusparseSetMatFillMode(upTriFactor->descr, CUSPARSE_FILL_MODE_UPPER);CHKERRCUSP(stat);
      stat = cusparseSetMatDiagType(upTriFactor->descr, CUSPARSE_DIAG_TYPE_UNIT);CHKERRCUSP(stat);

      /* Create the solve analysis information */
      stat = cusparseCreateSolveAnalysisInfo(&upTriFactor->solveInfo);CHKERRCUSP(stat);

      /* set the operation */
      upTriFactor->solveOp = CUSPARSE_OPERATION_NON_TRANSPOSE;

      /* set the matrix */
      upTriFactor->csrMat = new CsrMatrix;
      upTriFactor->csrMat->num_rows = A->rmap->n;
      upTriFactor->csrMat->num_cols = A->cmap->n;
      upTriFactor->csrMat->num_entries = a->nz;

      upTriFactor->csrMat->row_offsets = new THRUSTINTARRAY32(A->rmap->n+1);
      upTriFactor->csrMat->row_offsets->assign(AiUp, AiUp+A->rmap->n+1);

      upTriFactor->csrMat->column_indices = new THRUSTINTARRAY32(a->nz);
      upTriFactor->csrMat->column_indices->assign(AjUp, AjUp+a->nz);

      upTriFactor->csrMat->values = new THRUSTARRAY(a->nz);
      upTriFactor->csrMat->values->assign(AAUp, AAUp+a->nz);

      /* perform the solve analysis */
      stat = cusparse_analysis(cusparseTriFactors->handle, upTriFactor->solveOp,
                               upTriFactor->csrMat->num_rows, upTriFactor->csrMat->num_entries, upTriFactor->descr,
                               upTriFactor->csrMat->values->data().get(), upTriFactor->csrMat->row_offsets->data().get(),
                               upTriFactor->csrMat->column_indices->data().get(), upTriFactor->solveInfo);CHKERRCUSP(stat);

      /* assign the pointer. Is this really necessary? */
      ((Mat_SeqAIJCUSPARSETriFactors*)A->spptr)->upTriFactorPtr = upTriFactor;

      /* allocate space for the triangular factor information */
      loTriFactor = new Mat_SeqAIJCUSPARSETriFactorStruct;

      /* Create the matrix description */
      stat = cusparseCreateMatDescr(&loTriFactor->descr);CHKERRCUSP(stat);
      stat = cusparseSetMatIndexBase(loTriFactor->descr, CUSPARSE_INDEX_BASE_ZERO);CHKERRCUSP(stat);
      stat = cusparseSetMatType(loTriFactor->descr, CUSPARSE_MATRIX_TYPE_TRIANGULAR);CHKERRCUSP(stat);
      stat = cusparseSetMatFillMode(loTriFactor->descr, CUSPARSE_FILL_MODE_UPPER);CHKERRCUSP(stat);
      stat = cusparseSetMatDiagType(loTriFactor->descr, CUSPARSE_DIAG_TYPE_NON_UNIT);CHKERRCUSP(stat);

      /* Create the solve analysis information */
      stat = cusparseCreateSolveAnalysisInfo(&loTriFactor->solveInfo);CHKERRCUSP(stat);

      /* set the operation */
      loTriFactor->solveOp = CUSPARSE_OPERATION_TRANSPOSE;

      /* set the matrix */
      loTriFactor->csrMat = new CsrMatrix;
      loTriFactor->csrMat->num_rows = A->rmap->n;
      loTriFactor->csrMat->num_cols = A->cmap->n;
      loTriFactor->csrMat->num_entries = a->nz;

      loTriFactor->csrMat->row_offsets = new THRUSTINTARRAY32(A->rmap->n+1);
      loTriFactor->csrMat->row_offsets->assign(AiUp, AiUp+A->rmap->n+1);

      loTriFactor->csrMat->column_indices = new THRUSTINTARRAY32(a->nz);
      loTriFactor->csrMat->column_indices->assign(AjUp, AjUp+a->nz);

      loTriFactor->csrMat->values = new THRUSTARRAY(a->nz);
      loTriFactor->csrMat->values->assign(AALo, AALo+a->nz);

      /* perform the solve analysis */
      stat = cusparse_analysis(cusparseTriFactors->handle, loTriFactor->solveOp,
                               loTriFactor->csrMat->num_rows, loTriFactor->csrMat->num_entries, loTriFactor->descr,
                               loTriFactor->csrMat->values->data().get(), loTriFactor->csrMat->row_offsets->data().get(),
                               loTriFactor->csrMat->column_indices->data().get(), loTriFactor->solveInfo);CHKERRCUSP(stat);

      /* assign the pointer. Is this really necessary? */
      ((Mat_SeqAIJCUSPARSETriFactors*)A->spptr)->loTriFactorPtr = loTriFactor;

      A->valid_GPU_matrix = PETSC_CUSP_BOTH;
      ierr = cudaFreeHost(AiUp);CHKERRCUSP(ierr);
      ierr = cudaFreeHost(AjUp);CHKERRCUSP(ierr);
      ierr = cudaFreeHost(AAUp);CHKERRCUSP(ierr);
      ierr = cudaFreeHost(AALo);CHKERRCUSP(ierr);
    } catch(char *ex) {
      SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"CUSPARSE error: %s", ex);
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSeqAIJCUSPARSEICCAnalysisAndCopyToGPU"
static PetscErrorCode MatSeqAIJCUSPARSEICCAnalysisAndCopyToGPU(Mat A)
{
  PetscErrorCode               ierr;
  Mat_SeqAIJ                   *a                  = (Mat_SeqAIJ*)A->data;
  Mat_SeqAIJCUSPARSETriFactors *cusparseTriFactors = (Mat_SeqAIJCUSPARSETriFactors*)A->spptr;
  IS                           ip = a->row;
  const PetscInt               *rip;
  PetscBool                    perm_identity;
  PetscInt                     n = A->rmap->n;

  PetscFunctionBegin;
  ierr = MatSeqAIJCUSPARSEBuildICCTriMatrices(A);CHKERRQ(ierr);
  cusparseTriFactors->workVector = new THRUSTARRAY;
  cusparseTriFactors->workVector->resize(n);
  cusparseTriFactors->nnz=(a->nz-n)*2 + n;

  /*lower triangular indices */
  ierr = ISGetIndices(ip,&rip);CHKERRQ(ierr);
  ierr = ISIdentity(ip,&perm_identity);CHKERRQ(ierr);
  if (!perm_identity) {
    cusparseTriFactors->rpermIndices = new THRUSTINTARRAY(n);
    cusparseTriFactors->rpermIndices->assign(rip, rip+n);
    cusparseTriFactors->cpermIndices = new THRUSTINTARRAY(n);
    cusparseTriFactors->cpermIndices->assign(rip, rip+n);
  }
  ierr = ISRestoreIndices(ip,&rip);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatLUFactorNumeric_SeqAIJCUSPARSE"
static PetscErrorCode MatLUFactorNumeric_SeqAIJCUSPARSE(Mat B,Mat A,const MatFactorInfo *info)
{
  Mat_SeqAIJ     *b = (Mat_SeqAIJ*)B->data;
  IS             isrow = b->row,iscol = b->col;
  PetscBool      row_identity,col_identity;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MatLUFactorNumeric_SeqAIJ(B,A,info);CHKERRQ(ierr);
  /* determine which version of MatSolve needs to be used. */
  ierr = ISIdentity(isrow,&row_identity);CHKERRQ(ierr);
  ierr = ISIdentity(iscol,&col_identity);CHKERRQ(ierr);
  if (row_identity && col_identity) {
    B->ops->solve = MatSolve_SeqAIJCUSPARSE_NaturalOrdering;
    B->ops->solvetranspose = MatSolveTranspose_SeqAIJCUSPARSE_NaturalOrdering;
  } else {
    B->ops->solve = MatSolve_SeqAIJCUSPARSE;
    B->ops->solvetranspose = MatSolveTranspose_SeqAIJCUSPARSE;
  }

  /* get the triangular factors */
  ierr = MatSeqAIJCUSPARSEILUAnalysisAndCopyToGPU(B);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatCholeskyFactorNumeric_SeqAIJCUSPARSE"
static PetscErrorCode MatCholeskyFactorNumeric_SeqAIJCUSPARSE(Mat B,Mat A,const MatFactorInfo *info)
{
  Mat_SeqAIJ     *b = (Mat_SeqAIJ*)B->data;
  IS             ip = b->row;
  PetscBool      perm_identity;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MatCholeskyFactorNumeric_SeqAIJ(B,A,info);CHKERRQ(ierr);

  /* determine which version of MatSolve needs to be used. */
  ierr = ISIdentity(ip,&perm_identity);CHKERRQ(ierr);
  if (perm_identity) {
    B->ops->solve = MatSolve_SeqAIJCUSPARSE_NaturalOrdering;
    B->ops->solvetranspose = MatSolveTranspose_SeqAIJCUSPARSE_NaturalOrdering;
  } else {
    B->ops->solve = MatSolve_SeqAIJCUSPARSE;
    B->ops->solvetranspose = MatSolveTranspose_SeqAIJCUSPARSE;
  }

  /* get the triangular factors */
  ierr = MatSeqAIJCUSPARSEICCAnalysisAndCopyToGPU(B);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSeqAIJCUSPARSEAnalyzeTransposeForSolve"
static PetscErrorCode MatSeqAIJCUSPARSEAnalyzeTransposeForSolve(Mat A)
{
  Mat_SeqAIJCUSPARSETriFactors      *cusparseTriFactors = (Mat_SeqAIJCUSPARSETriFactors*)A->spptr;
  Mat_SeqAIJCUSPARSETriFactorStruct *loTriFactor = (Mat_SeqAIJCUSPARSETriFactorStruct*)cusparseTriFactors->loTriFactorPtr;
  Mat_SeqAIJCUSPARSETriFactorStruct *upTriFactor = (Mat_SeqAIJCUSPARSETriFactorStruct*)cusparseTriFactors->upTriFactorPtr;
  Mat_SeqAIJCUSPARSETriFactorStruct *loTriFactorT = (Mat_SeqAIJCUSPARSETriFactorStruct*)cusparseTriFactors->loTriFactorPtrTranspose;
  Mat_SeqAIJCUSPARSETriFactorStruct *upTriFactorT = (Mat_SeqAIJCUSPARSETriFactorStruct*)cusparseTriFactors->upTriFactorPtrTranspose;
  cusparseStatus_t                  stat;
  cusparseIndexBase_t               indexBase;
  cusparseMatrixType_t              matrixType;
  cusparseFillMode_t                fillMode;
  cusparseDiagType_t                diagType;

  PetscFunctionBegin;

  /*********************************************/
  /* Now the Transpose of the Lower Tri Factor */
  /*********************************************/

  /* allocate space for the transpose of the lower triangular factor */
  loTriFactorT = new Mat_SeqAIJCUSPARSETriFactorStruct;

  /* set the matrix descriptors of the lower triangular factor */
  matrixType = cusparseGetMatType(loTriFactor->descr);
  indexBase = cusparseGetMatIndexBase(loTriFactor->descr);
  fillMode = cusparseGetMatFillMode(loTriFactor->descr)==CUSPARSE_FILL_MODE_UPPER ?
    CUSPARSE_FILL_MODE_LOWER : CUSPARSE_FILL_MODE_UPPER;
  diagType = cusparseGetMatDiagType(loTriFactor->descr);

  /* Create the matrix description */
  stat = cusparseCreateMatDescr(&loTriFactorT->descr);CHKERRCUSP(stat);
  stat = cusparseSetMatIndexBase(loTriFactorT->descr, indexBase);CHKERRCUSP(stat);
  stat = cusparseSetMatType(loTriFactorT->descr, matrixType);CHKERRCUSP(stat);
  stat = cusparseSetMatFillMode(loTriFactorT->descr, fillMode);CHKERRCUSP(stat);
  stat = cusparseSetMatDiagType(loTriFactorT->descr, diagType);CHKERRCUSP(stat);

  /* Create the solve analysis information */
  stat = cusparseCreateSolveAnalysisInfo(&loTriFactorT->solveInfo);CHKERRCUSP(stat);

  /* set the operation */
  loTriFactorT->solveOp = CUSPARSE_OPERATION_NON_TRANSPOSE;

  /* allocate GPU space for the CSC of the lower triangular factor*/
  loTriFactorT->csrMat = new CsrMatrix;
  loTriFactorT->csrMat->num_rows = loTriFactor->csrMat->num_rows;
  loTriFactorT->csrMat->num_cols = loTriFactor->csrMat->num_cols;
  loTriFactorT->csrMat->num_entries = loTriFactor->csrMat->num_entries;
  loTriFactorT->csrMat->row_offsets = new THRUSTINTARRAY32(loTriFactor->csrMat->num_rows+1);
  loTriFactorT->csrMat->column_indices = new THRUSTINTARRAY32(loTriFactor->csrMat->num_entries);
  loTriFactorT->csrMat->values = new THRUSTARRAY(loTriFactor->csrMat->num_entries);

  /* compute the transpose of the lower triangular factor, i.e. the CSC */
  stat = cusparse_csr2csc(cusparseTriFactors->handle, loTriFactor->csrMat->num_rows,
                          loTriFactor->csrMat->num_cols, loTriFactor->csrMat->num_entries,
                          loTriFactor->csrMat->values->data().get(),
                          loTriFactor->csrMat->row_offsets->data().get(),
                          loTriFactor->csrMat->column_indices->data().get(),
                          loTriFactorT->csrMat->values->data().get(),
                          loTriFactorT->csrMat->column_indices->data().get(),
                          loTriFactorT->csrMat->row_offsets->data().get(),
                          CUSPARSE_ACTION_NUMERIC, indexBase);CHKERRCUSP(stat);

  /* perform the solve analysis on the transposed matrix */
  stat = cusparse_analysis(cusparseTriFactors->handle, loTriFactorT->solveOp,
                           loTriFactorT->csrMat->num_rows, loTriFactorT->csrMat->num_entries,
                           loTriFactorT->descr, loTriFactorT->csrMat->values->data().get(),
                           loTriFactorT->csrMat->row_offsets->data().get(), loTriFactorT->csrMat->column_indices->data().get(),
                           loTriFactorT->solveInfo);CHKERRCUSP(stat);

  /* assign the pointer. Is this really necessary? */
  ((Mat_SeqAIJCUSPARSETriFactors*)A->spptr)->loTriFactorPtrTranspose = loTriFactorT;

  /*********************************************/
  /* Now the Transpose of the Upper Tri Factor */
  /*********************************************/

  /* allocate space for the transpose of the upper triangular factor */
  upTriFactorT = new Mat_SeqAIJCUSPARSETriFactorStruct;

  /* set the matrix descriptors of the upper triangular factor */
  matrixType = cusparseGetMatType(upTriFactor->descr);
  indexBase = cusparseGetMatIndexBase(upTriFactor->descr);
  fillMode = cusparseGetMatFillMode(upTriFactor->descr)==CUSPARSE_FILL_MODE_UPPER ?
    CUSPARSE_FILL_MODE_LOWER : CUSPARSE_FILL_MODE_UPPER;
  diagType = cusparseGetMatDiagType(upTriFactor->descr);

  /* Create the matrix description */
  stat = cusparseCreateMatDescr(&upTriFactorT->descr);CHKERRCUSP(stat);
  stat = cusparseSetMatIndexBase(upTriFactorT->descr, indexBase);CHKERRCUSP(stat);
  stat = cusparseSetMatType(upTriFactorT->descr, matrixType);CHKERRCUSP(stat);
  stat = cusparseSetMatFillMode(upTriFactorT->descr, fillMode);CHKERRCUSP(stat);
  stat = cusparseSetMatDiagType(upTriFactorT->descr, diagType);CHKERRCUSP(stat);

  /* Create the solve analysis information */
  stat = cusparseCreateSolveAnalysisInfo(&upTriFactorT->solveInfo);CHKERRCUSP(stat);

  /* set the operation */
  upTriFactorT->solveOp = CUSPARSE_OPERATION_NON_TRANSPOSE;

  /* allocate GPU space for the CSC of the upper triangular factor*/
  upTriFactorT->csrMat = new CsrMatrix;
  upTriFactorT->csrMat->num_rows = upTriFactor->csrMat->num_rows;
  upTriFactorT->csrMat->num_cols = upTriFactor->csrMat->num_cols;
  upTriFactorT->csrMat->num_entries = upTriFactor->csrMat->num_entries;
  upTriFactorT->csrMat->row_offsets = new THRUSTINTARRAY32(upTriFactor->csrMat->num_rows+1);
  upTriFactorT->csrMat->column_indices = new THRUSTINTARRAY32(upTriFactor->csrMat->num_entries);
  upTriFactorT->csrMat->values = new THRUSTARRAY(upTriFactor->csrMat->num_entries);

  /* compute the transpose of the upper triangular factor, i.e. the CSC */
  stat = cusparse_csr2csc(cusparseTriFactors->handle, upTriFactor->csrMat->num_rows,
                          upTriFactor->csrMat->num_cols, upTriFactor->csrMat->num_entries,
                          upTriFactor->csrMat->values->data().get(),
                          upTriFactor->csrMat->row_offsets->data().get(),
                          upTriFactor->csrMat->column_indices->data().get(),
                          upTriFactorT->csrMat->values->data().get(),
                          upTriFactorT->csrMat->column_indices->data().get(),
                          upTriFactorT->csrMat->row_offsets->data().get(),
                          CUSPARSE_ACTION_NUMERIC, indexBase);CHKERRCUSP(stat);

  /* perform the solve analysis on the transposed matrix */
  stat = cusparse_analysis(cusparseTriFactors->handle, upTriFactorT->solveOp,
                           upTriFactorT->csrMat->num_rows, upTriFactorT->csrMat->num_entries,
                           upTriFactorT->descr, upTriFactorT->csrMat->values->data().get(),
                           upTriFactorT->csrMat->row_offsets->data().get(), upTriFactorT->csrMat->column_indices->data().get(),
                           upTriFactorT->solveInfo);CHKERRCUSP(stat);

  /* assign the pointer. Is this really necessary? */
  ((Mat_SeqAIJCUSPARSETriFactors*)A->spptr)->upTriFactorPtrTranspose = upTriFactorT;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSeqAIJCUSPARSEGenerateTransposeForMult"
static PetscErrorCode MatSeqAIJCUSPARSEGenerateTransposeForMult(Mat A)
{
  Mat_SeqAIJCUSPARSE           *cusparsestruct = (Mat_SeqAIJCUSPARSE*)A->spptr;
  Mat_SeqAIJCUSPARSEMultStruct *matstruct = (Mat_SeqAIJCUSPARSEMultStruct*)cusparsestruct->mat;
  Mat_SeqAIJCUSPARSEMultStruct *matstructT = (Mat_SeqAIJCUSPARSEMultStruct*)cusparsestruct->matTranspose;
  Mat_SeqAIJ                   *a = (Mat_SeqAIJ*)A->data;
  cusparseStatus_t             stat;
  cusparseIndexBase_t          indexBase;
  cudaError_t                  err;

  PetscFunctionBegin;

  /* allocate space for the triangular factor information */
  matstructT = new Mat_SeqAIJCUSPARSEMultStruct;
  stat = cusparseCreateMatDescr(&matstructT->descr);CHKERRCUSP(stat);
  indexBase = cusparseGetMatIndexBase(matstruct->descr);
  stat = cusparseSetMatIndexBase(matstructT->descr, indexBase);CHKERRCUSP(stat);
  stat = cusparseSetMatType(matstructT->descr, CUSPARSE_MATRIX_TYPE_GENERAL);CHKERRCUSP(stat);

  /* set alpha and beta */
  err = cudaMalloc((void **)&(matstructT->alpha),sizeof(PetscScalar));CHKERRCUSP(err);
  err = cudaMemcpy(matstructT->alpha,&ALPHA,sizeof(PetscScalar),cudaMemcpyHostToDevice);CHKERRCUSP(err);
  err = cudaMalloc((void **)&(matstructT->beta),sizeof(PetscScalar));CHKERRCUSP(err);
  err = cudaMemcpy(matstructT->beta,&BETA,sizeof(PetscScalar),cudaMemcpyHostToDevice);CHKERRCUSP(err);
  stat = cusparseSetPointerMode(cusparsestruct->handle, CUSPARSE_POINTER_MODE_DEVICE);CHKERRCUSP(stat);

  if (cusparsestruct->format==MAT_CUSPARSE_CSR) {
    CsrMatrix *matrix = (CsrMatrix*)matstruct->mat;
    CsrMatrix *matrixT= new CsrMatrix;
    matrixT->num_rows = A->rmap->n;
    matrixT->num_cols = A->cmap->n;
    matrixT->num_entries = a->nz;
    matrixT->row_offsets = new THRUSTINTARRAY32(A->rmap->n+1);
    matrixT->column_indices = new THRUSTINTARRAY32(a->nz);
    matrixT->values = new THRUSTARRAY(a->nz);

    /* compute the transpose of the upper triangular factor, i.e. the CSC */
    indexBase = cusparseGetMatIndexBase(matstruct->descr);
    stat = cusparse_csr2csc(cusparsestruct->handle, matrix->num_rows,
                            matrix->num_cols, matrix->num_entries,
                            matrix->values->data().get(),
                            matrix->row_offsets->data().get(),
                            matrix->column_indices->data().get(),
                            matrixT->values->data().get(),
                            matrixT->column_indices->data().get(),
                            matrixT->row_offsets->data().get(),
                            CUSPARSE_ACTION_NUMERIC, indexBase);CHKERRCUSP(stat);

    /* assign the pointer */
    matstructT->mat = matrixT;

  } else if (cusparsestruct->format==MAT_CUSPARSE_ELL || cusparsestruct->format==MAT_CUSPARSE_HYB) {
#if CUDA_VERSION>=5000
    /* First convert HYB to CSR */
    CsrMatrix *temp= new CsrMatrix;
    temp->num_rows = A->rmap->n;
    temp->num_cols = A->cmap->n;
    temp->num_entries = a->nz;
    temp->row_offsets = new THRUSTINTARRAY32(A->rmap->n+1);
    temp->column_indices = new THRUSTINTARRAY32(a->nz);
    temp->values = new THRUSTARRAY(a->nz);


    stat = cusparse_hyb2csr(cusparsestruct->handle,
                            matstruct->descr, (cusparseHybMat_t)matstruct->mat,
                            temp->values->data().get(),
                            temp->row_offsets->data().get(),
                            temp->column_indices->data().get());CHKERRCUSP(stat);

    /* Next, convert CSR to CSC (i.e. the matrix transpose) */
    CsrMatrix *tempT= new CsrMatrix;
    tempT->num_rows = A->rmap->n;
    tempT->num_cols = A->cmap->n;
    tempT->num_entries = a->nz;
    tempT->row_offsets = new THRUSTINTARRAY32(A->rmap->n+1);
    tempT->column_indices = new THRUSTINTARRAY32(a->nz);
    tempT->values = new THRUSTARRAY(a->nz);

    stat = cusparse_csr2csc(cusparsestruct->handle, temp->num_rows,
                            temp->num_cols, temp->num_entries,
                            temp->values->data().get(),
                            temp->row_offsets->data().get(),
                            temp->column_indices->data().get(),
                            tempT->values->data().get(),
                            tempT->column_indices->data().get(),
                            tempT->row_offsets->data().get(),
                            CUSPARSE_ACTION_NUMERIC, indexBase);CHKERRCUSP(stat);

    /* Last, convert CSC to HYB */
    cusparseHybMat_t hybMat;
    stat = cusparseCreateHybMat(&hybMat);CHKERRCUSP(stat);
    cusparseHybPartition_t partition = cusparsestruct->format==MAT_CUSPARSE_ELL ?
      CUSPARSE_HYB_PARTITION_MAX : CUSPARSE_HYB_PARTITION_AUTO;
    stat = cusparse_csr2hyb(cusparsestruct->handle, A->rmap->n, A->cmap->n,
                            matstructT->descr, tempT->values->data().get(),
                            tempT->row_offsets->data().get(),
                            tempT->column_indices->data().get(),
                            hybMat, 0, partition);CHKERRCUSP(stat);

    /* assign the pointer */
    matstructT->mat = hybMat;

    /* delete temporaries */
    if (tempT) {
      if (tempT->values) delete (THRUSTARRAY*) tempT->values;
      if (tempT->column_indices) delete (THRUSTINTARRAY32*) tempT->column_indices;
      if (tempT->row_offsets) delete (THRUSTINTARRAY32*) tempT->row_offsets;
      delete (CsrMatrix*) tempT;
    }
    if (temp) {
      if (temp->values) delete (THRUSTARRAY*) temp->values;
      if (temp->column_indices) delete (THRUSTINTARRAY32*) temp->column_indices;
      if (temp->row_offsets) delete (THRUSTINTARRAY32*) temp->row_offsets;
      delete (CsrMatrix*) temp;
    }
#else
    SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SUP,"ELL (Ellpack) and HYB (Hybrid) storage format for the Matrix Transpose (in MatMultTranspose) require CUDA 5.0 or later.");
#endif
  }
  /* assign the compressed row indices */
  matstructT->cprowIndices = new THRUSTINTARRAY;

  /* assign the pointer */
  ((Mat_SeqAIJCUSPARSE*)A->spptr)->matTranspose = matstructT;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSolveTranspose_SeqAIJCUSPARSE"
static PetscErrorCode MatSolveTranspose_SeqAIJCUSPARSE(Mat A,Vec bb,Vec xx)
{
  CUSPARRAY                         *xGPU, *bGPU;
  cusparseStatus_t                  stat;
  Mat_SeqAIJCUSPARSETriFactors      *cusparseTriFactors = (Mat_SeqAIJCUSPARSETriFactors*)A->spptr;
  Mat_SeqAIJCUSPARSETriFactorStruct *loTriFactorT = (Mat_SeqAIJCUSPARSETriFactorStruct*)cusparseTriFactors->loTriFactorPtrTranspose;
  Mat_SeqAIJCUSPARSETriFactorStruct *upTriFactorT = (Mat_SeqAIJCUSPARSETriFactorStruct*)cusparseTriFactors->upTriFactorPtrTranspose;
  THRUSTARRAY                       *tempGPU = (THRUSTARRAY*)cusparseTriFactors->workVector;
  PetscErrorCode                    ierr;

  PetscFunctionBegin;
  /* Analyze the matrix and create the transpose ... on the fly */
  if (!loTriFactorT && !upTriFactorT) {
    ierr = MatSeqAIJCUSPARSEAnalyzeTransposeForSolve(A);CHKERRQ(ierr);
    loTriFactorT       = (Mat_SeqAIJCUSPARSETriFactorStruct*)cusparseTriFactors->loTriFactorPtrTranspose;
    upTriFactorT       = (Mat_SeqAIJCUSPARSETriFactorStruct*)cusparseTriFactors->upTriFactorPtrTranspose;
  }

  /* Get the GPU pointers */
  ierr = VecCUSPGetArrayWrite(xx,&xGPU);CHKERRQ(ierr);
  ierr = VecCUSPGetArrayRead(bb,&bGPU);CHKERRQ(ierr);

  /* First, reorder with the row permutation */
  thrust::copy(thrust::make_permutation_iterator(bGPU->begin(), cusparseTriFactors->rpermIndices->begin()),
               thrust::make_permutation_iterator(bGPU->end(), cusparseTriFactors->rpermIndices->end()),
               xGPU->begin());

  /* First, solve U */
  stat = cusparse_solve(cusparseTriFactors->handle, upTriFactorT->solveOp,
                        upTriFactorT->csrMat->num_rows, &ALPHA, upTriFactorT->descr,
                        upTriFactorT->csrMat->values->data().get(),
                        upTriFactorT->csrMat->row_offsets->data().get(),
                        upTriFactorT->csrMat->column_indices->data().get(),
                        upTriFactorT->solveInfo,
                        xGPU->data().get(), tempGPU->data().get());CHKERRCUSP(stat);

  /* Then, solve L */
  stat = cusparse_solve(cusparseTriFactors->handle, loTriFactorT->solveOp,
                        loTriFactorT->csrMat->num_rows, &ALPHA, loTriFactorT->descr,
                        loTriFactorT->csrMat->values->data().get(),
                        loTriFactorT->csrMat->row_offsets->data().get(),
                        loTriFactorT->csrMat->column_indices->data().get(),
                        loTriFactorT->solveInfo,
                        tempGPU->data().get(), xGPU->data().get());CHKERRCUSP(stat);

  /* Last, copy the solution, xGPU, into a temporary with the column permutation ... can't be done in place. */
  thrust::copy(thrust::make_permutation_iterator(xGPU->begin(), cusparseTriFactors->cpermIndices->begin()),
               thrust::make_permutation_iterator(xGPU->end(), cusparseTriFactors->cpermIndices->end()),
               tempGPU->begin());

  /* Copy the temporary to the full solution. */
  thrust::copy(tempGPU->begin(), tempGPU->end(), xGPU->begin());

  /* restore */
  ierr = VecCUSPRestoreArrayRead(bb,&bGPU);CHKERRQ(ierr);
  ierr = VecCUSPRestoreArrayWrite(xx,&xGPU);CHKERRQ(ierr);
  ierr = WaitForGPU();CHKERRCUSP(ierr);

  ierr = PetscLogFlops(2.0*cusparseTriFactors->nnz - A->cmap->n);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSolveTranspose_SeqAIJCUSPARSE_NaturalOrdering"
static PetscErrorCode MatSolveTranspose_SeqAIJCUSPARSE_NaturalOrdering(Mat A,Vec bb,Vec xx)
{
  CUSPARRAY                         *xGPU,*bGPU;
  cusparseStatus_t                  stat;
  Mat_SeqAIJCUSPARSETriFactors      *cusparseTriFactors = (Mat_SeqAIJCUSPARSETriFactors*)A->spptr;
  Mat_SeqAIJCUSPARSETriFactorStruct *loTriFactorT = (Mat_SeqAIJCUSPARSETriFactorStruct*)cusparseTriFactors->loTriFactorPtrTranspose;
  Mat_SeqAIJCUSPARSETriFactorStruct *upTriFactorT = (Mat_SeqAIJCUSPARSETriFactorStruct*)cusparseTriFactors->upTriFactorPtrTranspose;
  THRUSTARRAY                       *tempGPU = (THRUSTARRAY*)cusparseTriFactors->workVector;
  PetscErrorCode                    ierr;

  PetscFunctionBegin;
  /* Analyze the matrix and create the transpose ... on the fly */
  if (!loTriFactorT && !upTriFactorT) {
    ierr = MatSeqAIJCUSPARSEAnalyzeTransposeForSolve(A);CHKERRQ(ierr);
    loTriFactorT       = (Mat_SeqAIJCUSPARSETriFactorStruct*)cusparseTriFactors->loTriFactorPtrTranspose;
    upTriFactorT       = (Mat_SeqAIJCUSPARSETriFactorStruct*)cusparseTriFactors->upTriFactorPtrTranspose;
  }

  /* Get the GPU pointers */
  ierr = VecCUSPGetArrayWrite(xx,&xGPU);CHKERRQ(ierr);
  ierr = VecCUSPGetArrayRead(bb,&bGPU);CHKERRQ(ierr);

  /* First, solve U */
  stat = cusparse_solve(cusparseTriFactors->handle, upTriFactorT->solveOp,
                        upTriFactorT->csrMat->num_rows, &ALPHA, upTriFactorT->descr,
                        upTriFactorT->csrMat->values->data().get(),
                        upTriFactorT->csrMat->row_offsets->data().get(),
                        upTriFactorT->csrMat->column_indices->data().get(),
                        upTriFactorT->solveInfo,
                        bGPU->data().get(), tempGPU->data().get());CHKERRCUSP(stat);

  /* Then, solve L */
  stat = cusparse_solve(cusparseTriFactors->handle, loTriFactorT->solveOp,
                        loTriFactorT->csrMat->num_rows, &ALPHA, loTriFactorT->descr,
                        loTriFactorT->csrMat->values->data().get(),
                        loTriFactorT->csrMat->row_offsets->data().get(),
                        loTriFactorT->csrMat->column_indices->data().get(),
                        loTriFactorT->solveInfo,
                        tempGPU->data().get(), xGPU->data().get());CHKERRCUSP(stat);

  /* restore */
  ierr = VecCUSPRestoreArrayRead(bb,&bGPU);CHKERRQ(ierr);
  ierr = VecCUSPRestoreArrayWrite(xx,&xGPU);CHKERRQ(ierr);
  ierr = WaitForGPU();CHKERRCUSP(ierr);
  ierr = PetscLogFlops(2.0*cusparseTriFactors->nnz - A->cmap->n);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSolve_SeqAIJCUSPARSE"
static PetscErrorCode MatSolve_SeqAIJCUSPARSE(Mat A,Vec bb,Vec xx)
{
  CUSPARRAY                         *xGPU,*bGPU;
  cusparseStatus_t                  stat;
  Mat_SeqAIJCUSPARSETriFactors      *cusparseTriFactors = (Mat_SeqAIJCUSPARSETriFactors*)A->spptr;
  Mat_SeqAIJCUSPARSETriFactorStruct *loTriFactor = (Mat_SeqAIJCUSPARSETriFactorStruct*)cusparseTriFactors->loTriFactorPtr;
  Mat_SeqAIJCUSPARSETriFactorStruct *upTriFactor = (Mat_SeqAIJCUSPARSETriFactorStruct*)cusparseTriFactors->upTriFactorPtr;
  THRUSTARRAY                       *tempGPU = (THRUSTARRAY*)cusparseTriFactors->workVector;
  PetscErrorCode                    ierr;
  VecType                           t;
  PetscBool                         flg;

  PetscFunctionBegin;
  ierr = VecGetType(bb,&t);CHKERRQ(ierr);
  ierr = PetscStrcmp(t,VECSEQCUSP,&flg);CHKERRQ(ierr);
  if (!flg) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONG,"Vector of type %s passed into MatSolve_SeqAIJCUSPARSE (Arg #2). Can only deal with %s\n.",t,VECSEQCUSP);
  ierr = VecGetType(xx,&t);CHKERRQ(ierr);
  ierr = PetscStrcmp(t,VECSEQCUSP,&flg);CHKERRQ(ierr);
  if (!flg) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONG,"Vector of type %s passed into MatSolve_SeqAIJCUSPARSE (Arg #3). Can only deal with %s\n.",t,VECSEQCUSP);

  /* Get the GPU pointers */
  ierr = VecCUSPGetArrayWrite(xx,&xGPU);CHKERRQ(ierr);
  ierr = VecCUSPGetArrayRead(bb,&bGPU);CHKERRQ(ierr);

  /* First, reorder with the row permutation */
  thrust::copy(thrust::make_permutation_iterator(bGPU->begin(), cusparseTriFactors->rpermIndices->begin()),
               thrust::make_permutation_iterator(bGPU->end(), cusparseTriFactors->rpermIndices->end()),
               xGPU->begin());

  /* Next, solve L */
  stat = cusparse_solve(cusparseTriFactors->handle, loTriFactor->solveOp,
                        loTriFactor->csrMat->num_rows, &ALPHA, loTriFactor->descr,
                        loTriFactor->csrMat->values->data().get(),
                        loTriFactor->csrMat->row_offsets->data().get(),
                        loTriFactor->csrMat->column_indices->data().get(),
                        loTriFactor->solveInfo,
                        xGPU->data().get(), tempGPU->data().get());CHKERRCUSP(stat);

  /* Then, solve U */
  stat = cusparse_solve(cusparseTriFactors->handle, upTriFactor->solveOp,
                        upTriFactor->csrMat->num_rows, &ALPHA, upTriFactor->descr,
                        upTriFactor->csrMat->values->data().get(),
                        upTriFactor->csrMat->row_offsets->data().get(),
                        upTriFactor->csrMat->column_indices->data().get(),
                        upTriFactor->solveInfo,
                        tempGPU->data().get(), xGPU->data().get());CHKERRCUSP(stat);

  /* Last, copy the solution, xGPU, into a temporary with the column permutation ... can't be done in place. */
  thrust::copy(thrust::make_permutation_iterator(xGPU->begin(), cusparseTriFactors->cpermIndices->begin()),
               thrust::make_permutation_iterator(xGPU->end(), cusparseTriFactors->cpermIndices->end()),
               tempGPU->begin());

  /* Copy the temporary to the full solution. */
  thrust::copy(tempGPU->begin(), tempGPU->end(), xGPU->begin());

  ierr = VecCUSPRestoreArrayRead(bb,&bGPU);CHKERRQ(ierr);
  ierr = VecCUSPRestoreArrayWrite(xx,&xGPU);CHKERRQ(ierr);
  ierr = WaitForGPU();CHKERRCUSP(ierr);
  ierr = PetscLogFlops(2.0*cusparseTriFactors->nnz - A->cmap->n);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSolve_SeqAIJCUSPARSE_NaturalOrdering"
static PetscErrorCode MatSolve_SeqAIJCUSPARSE_NaturalOrdering(Mat A,Vec bb,Vec xx)
{
  CUSPARRAY                         *xGPU,*bGPU;
  cusparseStatus_t                  stat;
  Mat_SeqAIJCUSPARSETriFactors      *cusparseTriFactors = (Mat_SeqAIJCUSPARSETriFactors*)A->spptr;
  Mat_SeqAIJCUSPARSETriFactorStruct *loTriFactor = (Mat_SeqAIJCUSPARSETriFactorStruct*)cusparseTriFactors->loTriFactorPtr;
  Mat_SeqAIJCUSPARSETriFactorStruct *upTriFactor = (Mat_SeqAIJCUSPARSETriFactorStruct*)cusparseTriFactors->upTriFactorPtr;
  THRUSTARRAY                       *tempGPU = (THRUSTARRAY*)cusparseTriFactors->workVector;
  PetscErrorCode                    ierr;

  PetscFunctionBegin;
  /* Get the GPU pointers */
  ierr = VecCUSPGetArrayWrite(xx,&xGPU);CHKERRQ(ierr);
  ierr = VecCUSPGetArrayRead(bb,&bGPU);CHKERRQ(ierr);

  /* First, solve L */
  stat = cusparse_solve(cusparseTriFactors->handle, loTriFactor->solveOp,
                        loTriFactor->csrMat->num_rows, &ALPHA, loTriFactor->descr,
                        loTriFactor->csrMat->values->data().get(),
                        loTriFactor->csrMat->row_offsets->data().get(),
                        loTriFactor->csrMat->column_indices->data().get(),
                        loTriFactor->solveInfo,
                        bGPU->data().get(), tempGPU->data().get());CHKERRCUSP(stat);

  /* Next, solve U */
  stat = cusparse_solve(cusparseTriFactors->handle, upTriFactor->solveOp,
                        upTriFactor->csrMat->num_rows, &ALPHA, upTriFactor->descr,
                        upTriFactor->csrMat->values->data().get(),
                        upTriFactor->csrMat->row_offsets->data().get(),
                        upTriFactor->csrMat->column_indices->data().get(),
                        upTriFactor->solveInfo,
                        tempGPU->data().get(), xGPU->data().get());CHKERRCUSP(stat);

  ierr = VecCUSPRestoreArrayRead(bb,&bGPU);CHKERRQ(ierr);
  ierr = VecCUSPRestoreArrayWrite(xx,&xGPU);CHKERRQ(ierr);
  ierr = WaitForGPU();CHKERRCUSP(ierr);
  ierr = PetscLogFlops(2.0*cusparseTriFactors->nnz - A->cmap->n);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSeqAIJCUSPARSECopyToGPU"
static PetscErrorCode MatSeqAIJCUSPARSECopyToGPU(Mat A)
{

  Mat_SeqAIJCUSPARSE           *cusparsestruct = (Mat_SeqAIJCUSPARSE*)A->spptr;
  Mat_SeqAIJCUSPARSEMultStruct *matstruct = (Mat_SeqAIJCUSPARSEMultStruct*)cusparsestruct->mat;
  Mat_SeqAIJ                   *a = (Mat_SeqAIJ*)A->data;
  PetscInt                     m = A->rmap->n,*ii,*ridx;
  PetscErrorCode               ierr;
  cusparseStatus_t             stat;
  cudaError_t                  err;

  PetscFunctionBegin;
  if (A->valid_GPU_matrix == PETSC_CUSP_UNALLOCATED || A->valid_GPU_matrix == PETSC_CUSP_CPU) {
    ierr = PetscLogEventBegin(MAT_CUSPARSECopyToGPU,A,0,0,0);CHKERRQ(ierr);
    Mat_SeqAIJCUSPARSEMultStruct_Destroy(&matstruct,cusparsestruct->format);
    try {
      cusparsestruct->nonzerorow=0;
      for (int j = 0; j<m; j++) cusparsestruct->nonzerorow += ((a->i[j+1]-a->i[j])>0);

      if (a->compressedrow.use) {
        m    = a->compressedrow.nrows;
        ii   = a->compressedrow.i;
        ridx = a->compressedrow.rindex;
      } else {
        /* Forcing compressed row on the GPU */
        int k=0;
        ierr = PetscMalloc1(cusparsestruct->nonzerorow+1, &ii);CHKERRQ(ierr);
        ierr = PetscMalloc1(cusparsestruct->nonzerorow, &ridx);CHKERRQ(ierr);
        ii[0]=0;
        for (int j = 0; j<m; j++) {
          if ((a->i[j+1]-a->i[j])>0) {
            ii[k]  = a->i[j];
            ridx[k]= j;
            k++;
          }
        }
        ii[cusparsestruct->nonzerorow] = a->nz;
        m = cusparsestruct->nonzerorow;
      }

      /* allocate space for the triangular factor information */
      matstruct = new Mat_SeqAIJCUSPARSEMultStruct;
      stat = cusparseCreateMatDescr(&matstruct->descr);CHKERRCUSP(stat);
      stat = cusparseSetMatIndexBase(matstruct->descr, CUSPARSE_INDEX_BASE_ZERO);CHKERRCUSP(stat);
      stat = cusparseSetMatType(matstruct->descr, CUSPARSE_MATRIX_TYPE_GENERAL);CHKERRCUSP(stat);

      err = cudaMalloc((void **)&(matstruct->alpha),sizeof(PetscScalar));CHKERRCUSP(err);
      err = cudaMemcpy(matstruct->alpha,&ALPHA,sizeof(PetscScalar),cudaMemcpyHostToDevice);CHKERRCUSP(err);
      err = cudaMalloc((void **)&(matstruct->beta),sizeof(PetscScalar));CHKERRCUSP(err);
      err = cudaMemcpy(matstruct->beta,&BETA,sizeof(PetscScalar),cudaMemcpyHostToDevice);CHKERRCUSP(err);
      stat = cusparseSetPointerMode(cusparsestruct->handle, CUSPARSE_POINTER_MODE_DEVICE);CHKERRCUSP(stat);

      /* Build a hybrid/ellpack matrix if this option is chosen for the storage */
      if (cusparsestruct->format==MAT_CUSPARSE_CSR) {
/* set the matrix */
        CsrMatrix *matrix= new CsrMatrix;
        matrix->num_rows = m;
        matrix->num_cols = A->cmap->n;
        matrix->num_entries = a->nz;
        matrix->row_offsets = new THRUSTINTARRAY32(m+1);
        matrix->row_offsets->assign(ii, ii + m+1);

        matrix->column_indices = new THRUSTINTARRAY32(a->nz);
        matrix->column_indices->assign(a->j, a->j+a->nz);

        matrix->values = new THRUSTARRAY(a->nz);
        matrix->values->assign(a->a, a->a+a->nz);

/* assign the pointer */
        matstruct->mat = matrix;

      } else if (cusparsestruct->format==MAT_CUSPARSE_ELL || cusparsestruct->format==MAT_CUSPARSE_HYB) {
#if CUDA_VERSION>=4020
        CsrMatrix *matrix= new CsrMatrix;
        matrix->num_rows = m;
        matrix->num_cols = A->cmap->n;
        matrix->num_entries = a->nz;
        matrix->row_offsets = new THRUSTINTARRAY32(m+1);
        matrix->row_offsets->assign(ii, ii + m+1);

        matrix->column_indices = new THRUSTINTARRAY32(a->nz);
        matrix->column_indices->assign(a->j, a->j+a->nz);

        matrix->values = new THRUSTARRAY(a->nz);
        matrix->values->assign(a->a, a->a+a->nz);

        cusparseHybMat_t hybMat;
        stat = cusparseCreateHybMat(&hybMat);CHKERRCUSP(stat);
        cusparseHybPartition_t partition = cusparsestruct->format==MAT_CUSPARSE_ELL ?
          CUSPARSE_HYB_PARTITION_MAX : CUSPARSE_HYB_PARTITION_AUTO;
        stat = cusparse_csr2hyb(cusparsestruct->handle, matrix->num_rows, matrix->num_cols,
                                matstruct->descr, matrix->values->data().get(),
                                matrix->row_offsets->data().get(),
                                matrix->column_indices->data().get(),
                                hybMat, 0, partition);CHKERRCUSP(stat);
        /* assign the pointer */
        matstruct->mat = hybMat;

        if (matrix) {
          if (matrix->values) delete (THRUSTARRAY*)matrix->values;
          if (matrix->column_indices) delete (THRUSTINTARRAY32*)matrix->column_indices;
          if (matrix->row_offsets) delete (THRUSTINTARRAY32*)matrix->row_offsets;
          delete (CsrMatrix*)matrix;
        }
#endif
      }

      /* assign the compressed row indices */
      matstruct->cprowIndices = new THRUSTINTARRAY(m);
      matstruct->cprowIndices->assign(ridx,ridx+m);

      /* assign the pointer */
      cusparsestruct->mat = matstruct;

      if (!a->compressedrow.use) {
        ierr = PetscFree(ii);CHKERRQ(ierr);
        ierr = PetscFree(ridx);CHKERRQ(ierr);
      }
      cusparsestruct->workVector = new THRUSTARRAY;
      cusparsestruct->workVector->resize(m);
    } catch(char *ex) {
      SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"CUSPARSE error: %s", ex);
    }
    ierr = WaitForGPU();CHKERRCUSP(ierr);

    A->valid_GPU_matrix = PETSC_CUSP_BOTH;

    ierr = PetscLogEventEnd(MAT_CUSPARSECopyToGPU,A,0,0,0);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatCreateVecs_SeqAIJCUSPARSE"
static PetscErrorCode MatCreateVecs_SeqAIJCUSPARSE(Mat mat, Vec *right, Vec *left)
{
  PetscErrorCode ierr;
  PetscInt rbs,cbs;

  PetscFunctionBegin;
  ierr = MatGetBlockSizes(mat,&rbs,&cbs);CHKERRQ(ierr);
  if (right) {
    ierr = VecCreate(PetscObjectComm((PetscObject)mat),right);CHKERRQ(ierr);
    ierr = VecSetSizes(*right,mat->cmap->n,PETSC_DETERMINE);CHKERRQ(ierr);
    ierr = VecSetBlockSize(*right,cbs);CHKERRQ(ierr);
    ierr = VecSetType(*right,VECSEQCUSP);CHKERRQ(ierr);
    ierr = PetscLayoutReference(mat->cmap,&(*right)->map);CHKERRQ(ierr);
  }
  if (left) {
    ierr = VecCreate(PetscObjectComm((PetscObject)mat),left);CHKERRQ(ierr);
    ierr = VecSetSizes(*left,mat->rmap->n,PETSC_DETERMINE);CHKERRQ(ierr);
    ierr = VecSetBlockSize(*left,rbs);CHKERRQ(ierr);
    ierr = VecSetType(*left,VECSEQCUSP);CHKERRQ(ierr);
    ierr = PetscLayoutReference(mat->rmap,&(*left)->map);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

struct VecCUSPPlusEquals
{
  template <typename Tuple>
  __host__ __device__
  void operator()(Tuple t)
  {
    thrust::get<1>(t) = thrust::get<1>(t) + thrust::get<0>(t);
  }
};

#undef __FUNCT__
#define __FUNCT__ "MatMult_SeqAIJCUSPARSE"
static PetscErrorCode MatMult_SeqAIJCUSPARSE(Mat A,Vec xx,Vec yy)
{
  Mat_SeqAIJ                   *a = (Mat_SeqAIJ*)A->data;
  Mat_SeqAIJCUSPARSE           *cusparsestruct = (Mat_SeqAIJCUSPARSE*)A->spptr;
  Mat_SeqAIJCUSPARSEMultStruct *matstruct = (Mat_SeqAIJCUSPARSEMultStruct*)cusparsestruct->mat;
  CUSPARRAY                    *xarray,*yarray;
  PetscErrorCode               ierr;
  cusparseStatus_t             stat;

  PetscFunctionBegin;
  /* The line below should not be necessary as it has been moved to MatAssemblyEnd_SeqAIJCUSPARSE
     ierr = MatSeqAIJCUSPARSECopyToGPU(A);CHKERRQ(ierr); */
  ierr = VecCUSPGetArrayRead(xx,&xarray);CHKERRQ(ierr);
  ierr = VecCUSPGetArrayWrite(yy,&yarray);CHKERRQ(ierr);
  if (cusparsestruct->format==MAT_CUSPARSE_CSR) {
    CsrMatrix *mat = (CsrMatrix*)matstruct->mat;
    stat = cusparse_csr_spmv(cusparsestruct->handle, CUSPARSE_OPERATION_NON_TRANSPOSE,
                             mat->num_rows, mat->num_cols, mat->num_entries,
                             matstruct->alpha, matstruct->descr, mat->values->data().get(), mat->row_offsets->data().get(),
                             mat->column_indices->data().get(), xarray->data().get(), matstruct->beta,
                             yarray->data().get());CHKERRCUSP(stat);
  } else {
#if CUDA_VERSION>=4020
    cusparseHybMat_t hybMat = (cusparseHybMat_t)matstruct->mat;
    stat = cusparse_hyb_spmv(cusparsestruct->handle, CUSPARSE_OPERATION_NON_TRANSPOSE,
                             matstruct->alpha, matstruct->descr, hybMat,
                             xarray->data().get(), matstruct->beta,
                             yarray->data().get());CHKERRCUSP(stat);
#endif
  }
  ierr = VecCUSPRestoreArrayRead(xx,&xarray);CHKERRQ(ierr);
  ierr = VecCUSPRestoreArrayWrite(yy,&yarray);CHKERRQ(ierr);
  if (!cusparsestruct->stream) {
    ierr = WaitForGPU();CHKERRCUSP(ierr);
  }
  ierr = PetscLogFlops(2.0*a->nz - cusparsestruct->nonzerorow);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatMultTranspose_SeqAIJCUSPARSE"
static PetscErrorCode MatMultTranspose_SeqAIJCUSPARSE(Mat A,Vec xx,Vec yy)
{
  Mat_SeqAIJ                   *a = (Mat_SeqAIJ*)A->data;
  Mat_SeqAIJCUSPARSE           *cusparsestruct = (Mat_SeqAIJCUSPARSE*)A->spptr;
  Mat_SeqAIJCUSPARSEMultStruct *matstructT = (Mat_SeqAIJCUSPARSEMultStruct*)cusparsestruct->matTranspose;
  CUSPARRAY                    *xarray,*yarray;
  PetscErrorCode               ierr;
  cusparseStatus_t             stat;

  PetscFunctionBegin;
  /* The line below should not be necessary as it has been moved to MatAssemblyEnd_SeqAIJCUSPARSE
     ierr = MatSeqAIJCUSPARSECopyToGPU(A);CHKERRQ(ierr); */
  if (!matstructT) {
    ierr = MatSeqAIJCUSPARSEGenerateTransposeForMult(A);CHKERRQ(ierr);
    matstructT = (Mat_SeqAIJCUSPARSEMultStruct*)cusparsestruct->matTranspose;
  }
  ierr = VecCUSPGetArrayRead(xx,&xarray);CHKERRQ(ierr);
  ierr = VecCUSPGetArrayWrite(yy,&yarray);CHKERRQ(ierr);

  if (cusparsestruct->format==MAT_CUSPARSE_CSR) {
    CsrMatrix *mat = (CsrMatrix*)matstructT->mat;
    stat = cusparse_csr_spmv(cusparsestruct->handle, CUSPARSE_OPERATION_NON_TRANSPOSE,
                             mat->num_rows, mat->num_cols,
                             mat->num_entries, matstructT->alpha, matstructT->descr,
                             mat->values->data().get(), mat->row_offsets->data().get(),
                             mat->column_indices->data().get(), xarray->data().get(), matstructT->beta,
                             yarray->data().get());CHKERRCUSP(stat);
  } else {
#if CUDA_VERSION>=4020
    cusparseHybMat_t hybMat = (cusparseHybMat_t)matstructT->mat;
    stat = cusparse_hyb_spmv(cusparsestruct->handle, CUSPARSE_OPERATION_NON_TRANSPOSE,
                             matstructT->alpha, matstructT->descr, hybMat,
                             xarray->data().get(), matstructT->beta,
                             yarray->data().get());CHKERRCUSP(stat);
#endif
  }
  ierr = VecCUSPRestoreArrayRead(xx,&xarray);CHKERRQ(ierr);
  ierr = VecCUSPRestoreArrayWrite(yy,&yarray);CHKERRQ(ierr);
  if (!cusparsestruct->stream) {
    ierr = WaitForGPU();CHKERRCUSP(ierr);
  }
  ierr = PetscLogFlops(2.0*a->nz - cusparsestruct->nonzerorow);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "MatMultAdd_SeqAIJCUSPARSE"
static PetscErrorCode MatMultAdd_SeqAIJCUSPARSE(Mat A,Vec xx,Vec yy,Vec zz)
{
  Mat_SeqAIJ                   *a = (Mat_SeqAIJ*)A->data;
  Mat_SeqAIJCUSPARSE           *cusparsestruct = (Mat_SeqAIJCUSPARSE*)A->spptr;
  Mat_SeqAIJCUSPARSEMultStruct *matstruct = (Mat_SeqAIJCUSPARSEMultStruct*)cusparsestruct->mat;
  CUSPARRAY                    *xarray,*yarray,*zarray;
  PetscErrorCode               ierr;
  cusparseStatus_t             stat;

  PetscFunctionBegin;
  /* The line below should not be necessary as it has been moved to MatAssemblyEnd_SeqAIJCUSPARSE
     ierr = MatSeqAIJCUSPARSECopyToGPU(A);CHKERRQ(ierr); */
  try {
    ierr = VecCopy_SeqCUSP(yy,zz);CHKERRQ(ierr);
    ierr = VecCUSPGetArrayRead(xx,&xarray);CHKERRQ(ierr);
    ierr = VecCUSPGetArrayRead(yy,&yarray);CHKERRQ(ierr);
    ierr = VecCUSPGetArrayWrite(zz,&zarray);CHKERRQ(ierr);

    /* multiply add */
    if (cusparsestruct->format==MAT_CUSPARSE_CSR) {
      CsrMatrix *mat = (CsrMatrix*)matstruct->mat;
    /* here we need to be careful to set the number of rows in the multiply to the
       number of compressed rows in the matrix ... which is equivalent to the
       size of the workVector */
      stat = cusparse_csr_spmv(cusparsestruct->handle, CUSPARSE_OPERATION_NON_TRANSPOSE,
                               mat->num_rows, mat->num_cols,
                               mat->num_entries, matstruct->alpha, matstruct->descr,
                               mat->values->data().get(), mat->row_offsets->data().get(),
                               mat->column_indices->data().get(), xarray->data().get(), matstruct->beta,
                               cusparsestruct->workVector->data().get());CHKERRCUSP(stat);
    } else {
#if CUDA_VERSION>=4020
      cusparseHybMat_t hybMat = (cusparseHybMat_t)matstruct->mat;
      if (cusparsestruct->workVector->size()) {
        stat = cusparse_hyb_spmv(cusparsestruct->handle, CUSPARSE_OPERATION_NON_TRANSPOSE,
            matstruct->alpha, matstruct->descr, hybMat,
            xarray->data().get(), matstruct->beta,
            cusparsestruct->workVector->data().get());CHKERRCUSP(stat);
      }
#endif
    }

    /* scatter the data from the temporary into the full vector with a += operation */
    thrust::for_each(thrust::make_zip_iterator(thrust::make_tuple(cusparsestruct->workVector->begin(), thrust::make_permutation_iterator(zarray->begin(), matstruct->cprowIndices->begin()))),
        thrust::make_zip_iterator(thrust::make_tuple(cusparsestruct->workVector->begin(), thrust::make_permutation_iterator(zarray->begin(), matstruct->cprowIndices->begin()))) + cusparsestruct->workVector->size(),
        VecCUSPPlusEquals());
    ierr = VecCUSPRestoreArrayRead(xx,&xarray);CHKERRQ(ierr);
    ierr = VecCUSPRestoreArrayRead(yy,&yarray);CHKERRQ(ierr);
    ierr = VecCUSPRestoreArrayWrite(zz,&zarray);CHKERRQ(ierr);

  } catch(char *ex) {
    SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"CUSPARSE error: %s", ex);
  }
  ierr = WaitForGPU();CHKERRCUSP(ierr);
  ierr = PetscLogFlops(2.0*a->nz);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatMultTransposeAdd_SeqAIJCUSPARSE"
static PetscErrorCode MatMultTransposeAdd_SeqAIJCUSPARSE(Mat A,Vec xx,Vec yy,Vec zz)
{
  Mat_SeqAIJ                   *a = (Mat_SeqAIJ*)A->data;
  Mat_SeqAIJCUSPARSE           *cusparsestruct = (Mat_SeqAIJCUSPARSE*)A->spptr;
  Mat_SeqAIJCUSPARSEMultStruct *matstructT = (Mat_SeqAIJCUSPARSEMultStruct*)cusparsestruct->matTranspose;
  CUSPARRAY                    *xarray,*yarray,*zarray;
  PetscErrorCode               ierr;
  cusparseStatus_t             stat;

  PetscFunctionBegin;
  /* The line below should not be necessary as it has been moved to MatAssemblyEnd_SeqAIJCUSPARSE
     ierr = MatSeqAIJCUSPARSECopyToGPU(A);CHKERRQ(ierr); */
  if (!matstructT) {
    ierr = MatSeqAIJCUSPARSEGenerateTransposeForMult(A);CHKERRQ(ierr);
    matstructT = (Mat_SeqAIJCUSPARSEMultStruct*)cusparsestruct->matTranspose;
  }

  try {
    ierr = VecCopy_SeqCUSP(yy,zz);CHKERRQ(ierr);
    ierr = VecCUSPGetArrayRead(xx,&xarray);CHKERRQ(ierr);
    ierr = VecCUSPGetArrayRead(yy,&yarray);CHKERRQ(ierr);
    ierr = VecCUSPGetArrayWrite(zz,&zarray);CHKERRQ(ierr);

    /* multiply add with matrix transpose */
    if (cusparsestruct->format==MAT_CUSPARSE_CSR) {
      CsrMatrix *mat = (CsrMatrix*)matstructT->mat;
      /* here we need to be careful to set the number of rows in the multiply to the
         number of compressed rows in the matrix ... which is equivalent to the
         size of the workVector */
      stat = cusparse_csr_spmv(cusparsestruct->handle, CUSPARSE_OPERATION_NON_TRANSPOSE,
                               mat->num_rows, mat->num_cols,
                               mat->num_entries, matstructT->alpha, matstructT->descr,
                               mat->values->data().get(), mat->row_offsets->data().get(),
                               mat->column_indices->data().get(), xarray->data().get(), matstructT->beta,
                               cusparsestruct->workVector->data().get());CHKERRCUSP(stat);
    } else {
#if CUDA_VERSION>=4020
      cusparseHybMat_t hybMat = (cusparseHybMat_t)matstructT->mat;
      if (cusparsestruct->workVector->size()) {
        stat = cusparse_hyb_spmv(cusparsestruct->handle, CUSPARSE_OPERATION_NON_TRANSPOSE,
            matstructT->alpha, matstructT->descr, hybMat,
            xarray->data().get(), matstructT->beta,
            cusparsestruct->workVector->data().get());CHKERRCUSP(stat);
      }
#endif
    }

    /* scatter the data from the temporary into the full vector with a += operation */
    thrust::for_each(thrust::make_zip_iterator(thrust::make_tuple(cusparsestruct->workVector->begin(), thrust::make_permutation_iterator(zarray->begin(), matstructT->cprowIndices->begin()))),
        thrust::make_zip_iterator(thrust::make_tuple(cusparsestruct->workVector->begin(), thrust::make_permutation_iterator(zarray->begin(), matstructT->cprowIndices->begin()))) + cusparsestruct->workVector->size(),
        VecCUSPPlusEquals());

    ierr = VecCUSPRestoreArrayRead(xx,&xarray);CHKERRQ(ierr);
    ierr = VecCUSPRestoreArrayRead(yy,&yarray);CHKERRQ(ierr);
    ierr = VecCUSPRestoreArrayWrite(zz,&zarray);CHKERRQ(ierr);

  } catch(char *ex) {
    SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"CUSPARSE error: %s", ex);
  }
  ierr = WaitForGPU();CHKERRCUSP(ierr);
  ierr = PetscLogFlops(2.0*a->nz);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatAssemblyEnd_SeqAIJCUSPARSE"
static PetscErrorCode MatAssemblyEnd_SeqAIJCUSPARSE(Mat A,MatAssemblyType mode)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MatAssemblyEnd_SeqAIJ(A,mode);CHKERRQ(ierr);
  if (A->factortype==MAT_FACTOR_NONE) {
    ierr = MatSeqAIJCUSPARSECopyToGPU(A);CHKERRQ(ierr);
  }
  if (mode == MAT_FLUSH_ASSEMBLY) PetscFunctionReturn(0);
  A->ops->mult             = MatMult_SeqAIJCUSPARSE;
  A->ops->multadd          = MatMultAdd_SeqAIJCUSPARSE;
  A->ops->multtranspose    = MatMultTranspose_SeqAIJCUSPARSE;
  A->ops->multtransposeadd = MatMultTransposeAdd_SeqAIJCUSPARSE;
  PetscFunctionReturn(0);
}

/* --------------------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "MatCreateSeqAIJCUSPARSE"
/*@
   MatCreateSeqAIJCUSPARSE - Creates a sparse matrix in AIJ (compressed row) format
   (the default parallel PETSc format). This matrix will ultimately pushed down
   to NVidia GPUs and use the CUSPARSE library for calculations. For good matrix
   assembly performance the user should preallocate the matrix storage by setting
   the parameter nz (or the array nnz).  By setting these parameters accurately,
   performance during matrix assembly can be increased by more than a factor of 50.

   Collective on MPI_Comm

   Input Parameters:
+  comm - MPI communicator, set to PETSC_COMM_SELF
.  m - number of rows
.  n - number of columns
.  nz - number of nonzeros per row (same for all rows)
-  nnz - array containing the number of nonzeros in the various rows
         (possibly different for each row) or NULL

   Output Parameter:
.  A - the matrix

   It is recommended that one use the MatCreate(), MatSetType() and/or MatSetFromOptions(),
   MatXXXXSetPreallocation() paradgm instead of this routine directly.
   [MatXXXXSetPreallocation() is, for example, MatSeqAIJSetPreallocation]

   Notes:
   If nnz is given then nz is ignored

   The AIJ format (also called the Yale sparse matrix format or
   compressed row storage), is fully compatible with standard Fortran 77
   storage.  That is, the stored row and column indices can begin at
   either one (as in Fortran) or zero.  See the users' manual for details.

   Specify the preallocated storage with either nz or nnz (not both).
   Set nz=PETSC_DEFAULT and nnz=NULL for PETSc to control dynamic memory
   allocation.  For large problems you MUST preallocate memory or you
   will get TERRIBLE performance, see the users' manual chapter on matrices.

   By default, this format uses inodes (identical nodes) when possible, to
   improve numerical efficiency of matrix-vector products and solves. We
   search for consecutive rows with the same nonzero structure, thereby
   reusing matrix information to achieve increased efficiency.

   Level: intermediate

.seealso: MatCreate(), MatCreateAIJ(), MatSetValues(), MatSeqAIJSetColumnIndices(), MatCreateSeqAIJWithArrays(), MatCreateAIJ(), MATSEQAIJCUSPARSE, MATAIJCUSPARSE
@*/
PetscErrorCode  MatCreateSeqAIJCUSPARSE(MPI_Comm comm,PetscInt m,PetscInt n,PetscInt nz,const PetscInt nnz[],Mat *A)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MatCreate(comm,A);CHKERRQ(ierr);
  ierr = MatSetSizes(*A,m,n,m,n);CHKERRQ(ierr);
  ierr = MatSetType(*A,MATSEQAIJCUSPARSE);CHKERRQ(ierr);
  ierr = MatSeqAIJSetPreallocation_SeqAIJ(*A,nz,(PetscInt*)nnz);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatDestroy_SeqAIJCUSPARSE"
static PetscErrorCode MatDestroy_SeqAIJCUSPARSE(Mat A)
{
  PetscErrorCode   ierr;

  PetscFunctionBegin;
  if (A->factortype==MAT_FACTOR_NONE) {
    if (A->valid_GPU_matrix != PETSC_CUSP_UNALLOCATED) {
      ierr = Mat_SeqAIJCUSPARSE_Destroy((Mat_SeqAIJCUSPARSE**)&A->spptr);CHKERRQ(ierr);
    }
  } else {
    ierr = Mat_SeqAIJCUSPARSETriFactors_Destroy((Mat_SeqAIJCUSPARSETriFactors**)&A->spptr);CHKERRQ(ierr);
  }
  ierr = MatDestroy_SeqAIJ(A);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatCreate_SeqAIJCUSPARSE"
PETSC_EXTERN PetscErrorCode MatCreate_SeqAIJCUSPARSE(Mat B)
{
  PetscErrorCode ierr;
  cusparseStatus_t stat;
  cusparseHandle_t handle=0;

  PetscFunctionBegin;
  ierr = MatCreate_SeqAIJ(B);CHKERRQ(ierr);
  if (B->factortype==MAT_FACTOR_NONE) {
    /* you cannot check the inode.use flag here since the matrix was just created.
       now build a GPU matrix data structure */
    B->spptr = new Mat_SeqAIJCUSPARSE;
    ((Mat_SeqAIJCUSPARSE*)B->spptr)->mat          = 0;
    ((Mat_SeqAIJCUSPARSE*)B->spptr)->matTranspose = 0;
    ((Mat_SeqAIJCUSPARSE*)B->spptr)->workVector   = 0;
    ((Mat_SeqAIJCUSPARSE*)B->spptr)->format       = MAT_CUSPARSE_CSR;
    ((Mat_SeqAIJCUSPARSE*)B->spptr)->stream       = 0;
    ((Mat_SeqAIJCUSPARSE*)B->spptr)->handle       = 0;
    stat = cusparseCreate(&handle);CHKERRCUSP(stat);
    ((Mat_SeqAIJCUSPARSE*)B->spptr)->handle       = handle;
    ((Mat_SeqAIJCUSPARSE*)B->spptr)->stream       = 0;
  } else {
    /* NEXT, set the pointers to the triangular factors */
    B->spptr = new Mat_SeqAIJCUSPARSETriFactors;
    ((Mat_SeqAIJCUSPARSETriFactors*)B->spptr)->loTriFactorPtr          = 0;
    ((Mat_SeqAIJCUSPARSETriFactors*)B->spptr)->upTriFactorPtr          = 0;
    ((Mat_SeqAIJCUSPARSETriFactors*)B->spptr)->loTriFactorPtrTranspose = 0;
    ((Mat_SeqAIJCUSPARSETriFactors*)B->spptr)->upTriFactorPtrTranspose = 0;
    ((Mat_SeqAIJCUSPARSETriFactors*)B->spptr)->rpermIndices            = 0;
    ((Mat_SeqAIJCUSPARSETriFactors*)B->spptr)->cpermIndices            = 0;
    ((Mat_SeqAIJCUSPARSETriFactors*)B->spptr)->workVector              = 0;
    ((Mat_SeqAIJCUSPARSETriFactors*)B->spptr)->handle                  = 0;
    stat = cusparseCreate(&handle);CHKERRCUSP(stat);
    ((Mat_SeqAIJCUSPARSETriFactors*)B->spptr)->handle                  = handle;
    ((Mat_SeqAIJCUSPARSETriFactors*)B->spptr)->nnz                     = 0;
  }

  B->ops->assemblyend      = MatAssemblyEnd_SeqAIJCUSPARSE;
  B->ops->destroy          = MatDestroy_SeqAIJCUSPARSE;
  B->ops->getvecs          = MatCreateVecs_SeqAIJCUSPARSE;
  B->ops->setfromoptions   = MatSetFromOptions_SeqAIJCUSPARSE;
  B->ops->mult             = MatMult_SeqAIJCUSPARSE;
  B->ops->multadd          = MatMultAdd_SeqAIJCUSPARSE;
  B->ops->multtranspose    = MatMultTranspose_SeqAIJCUSPARSE;
  B->ops->multtransposeadd = MatMultTransposeAdd_SeqAIJCUSPARSE;

  ierr = PetscObjectChangeTypeName((PetscObject)B,MATSEQAIJCUSPARSE);CHKERRQ(ierr);

  B->valid_GPU_matrix = PETSC_CUSP_UNALLOCATED;

  ierr = PetscObjectComposeFunction((PetscObject)B, "MatCUSPARSESetFormat_C", MatCUSPARSESetFormat_SeqAIJCUSPARSE);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/*M
   MATSEQAIJCUSPARSE - MATAIJCUSPARSE = "(seq)aijcusparse" - A matrix type to be used for sparse matrices.

   A matrix type type whose data resides on Nvidia GPUs. These matrices can be in either
   CSR, ELL, or Hybrid format. The ELL and HYB formats require CUDA 4.2 or later.
   All matrix calculations are performed on Nvidia GPUs using the CUSPARSE library.

   Options Database Keys:
+  -mat_type aijcusparse - sets the matrix type to "seqaijcusparse" during a call to MatSetFromOptions()
.  -mat_cusparse_storage_format csr - sets the storage format of matrices (for MatMult and factors in MatSolve) during a call to MatSetFromOptions(). Other options include ell (ellpack) or hyb (hybrid).
.  -mat_cusparse_mult_storage_format csr - sets the storage format of matrices (for MatMult) during a call to MatSetFromOptions(). Other options include ell (ellpack) or hyb (hybrid).

  Level: beginner

.seealso: MatCreateSeqAIJCUSPARSE(), MATAIJCUSPARSE, MatCreateAIJCUSPARSE(), MatCUSPARSESetFormat(), MatCUSPARSEStorageFormat, MatCUSPARSEFormatOperation
M*/

PETSC_EXTERN PetscErrorCode MatGetFactor_seqaijcusparse_cusparse(Mat,MatFactorType,Mat*);


#undef __FUNCT__
#define __FUNCT__ "MatSolverPackageRegister_CUSPARSE"
PETSC_EXTERN PetscErrorCode MatSolverPackageRegister_CUSPARSE(void)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MatSolverPackageRegister(MATSOLVERCUSPARSE,MATSEQAIJCUSPARSE,MAT_FACTOR_LU,MatGetFactor_seqaijcusparse_cusparse);CHKERRQ(ierr);
  ierr = MatSolverPackageRegister(MATSOLVERCUSPARSE,MATSEQAIJCUSPARSE,MAT_FACTOR_CHOLESKY,MatGetFactor_seqaijcusparse_cusparse);CHKERRQ(ierr);
  ierr = MatSolverPackageRegister(MATSOLVERCUSPARSE,MATSEQAIJCUSPARSE,MAT_FACTOR_ILU,MatGetFactor_seqaijcusparse_cusparse);CHKERRQ(ierr);
  ierr = MatSolverPackageRegister(MATSOLVERCUSPARSE,MATSEQAIJCUSPARSE,MAT_FACTOR_ICC,MatGetFactor_seqaijcusparse_cusparse);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "Mat_SeqAIJCUSPARSE_Destroy"
static PetscErrorCode Mat_SeqAIJCUSPARSE_Destroy(Mat_SeqAIJCUSPARSE **cusparsestruct)
{
  cusparseStatus_t stat;
  cusparseHandle_t handle;

  PetscFunctionBegin;
  if (*cusparsestruct) {
    Mat_SeqAIJCUSPARSEMultStruct_Destroy(&(*cusparsestruct)->mat,(*cusparsestruct)->format);
    Mat_SeqAIJCUSPARSEMultStruct_Destroy(&(*cusparsestruct)->matTranspose,(*cusparsestruct)->format);
    delete (*cusparsestruct)->workVector;
    if (handle = (*cusparsestruct)->handle) {
      stat = cusparseDestroy(handle);CHKERRCUSP(stat);
    }
    delete *cusparsestruct;
    *cusparsestruct = 0;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "CsrMatrix_Destroy"
static PetscErrorCode CsrMatrix_Destroy(CsrMatrix **mat)
{
  PetscFunctionBegin;
  if (*mat) {
    delete (*mat)->values;
    delete (*mat)->column_indices;
    delete (*mat)->row_offsets;
    delete *mat;
    *mat = 0;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "Mat_SeqAIJCUSPARSETriFactorStruct_Destroy"
static PetscErrorCode Mat_SeqAIJCUSPARSETriFactorStruct_Destroy(Mat_SeqAIJCUSPARSETriFactorStruct **trifactor)
{
  cusparseStatus_t stat;
  PetscErrorCode   ierr;

  PetscFunctionBegin;
  if (*trifactor) {
    if ((*trifactor)->descr) { stat = cusparseDestroyMatDescr((*trifactor)->descr);CHKERRCUSP(stat); }
    if ((*trifactor)->solveInfo) { stat = cusparseDestroySolveAnalysisInfo((*trifactor)->solveInfo);CHKERRCUSP(stat); }
    ierr = CsrMatrix_Destroy(&(*trifactor)->csrMat);CHKERRQ(ierr);
    delete *trifactor;
    *trifactor = 0;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "Mat_SeqAIJCUSPARSEMultStruct_Destroy"
static PetscErrorCode Mat_SeqAIJCUSPARSEMultStruct_Destroy(Mat_SeqAIJCUSPARSEMultStruct **matstruct,MatCUSPARSEStorageFormat format)
{
  CsrMatrix        *mat;
  cusparseStatus_t stat;
  cudaError_t      err;

  PetscFunctionBegin;
  if (*matstruct) {
    if ((*matstruct)->mat) {
      if (format==MAT_CUSPARSE_ELL || format==MAT_CUSPARSE_HYB) {
        cusparseHybMat_t hybMat = (cusparseHybMat_t)(*matstruct)->mat;
        stat = cusparseDestroyHybMat(hybMat);CHKERRCUSP(stat);
      } else {
        mat = (CsrMatrix*)(*matstruct)->mat;
        CsrMatrix_Destroy(&mat);
      }
    }
    if ((*matstruct)->descr) { stat = cusparseDestroyMatDescr((*matstruct)->descr);CHKERRCUSP(stat); }
    delete (*matstruct)->cprowIndices;
    if ((*matstruct)->alpha) { err=cudaFree((*matstruct)->alpha);CHKERRCUSP(err); }
    if ((*matstruct)->beta) { err=cudaFree((*matstruct)->beta);CHKERRCUSP(err); }
    delete *matstruct;
    *matstruct = 0;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "Mat_SeqAIJCUSPARSETriFactors_Destroy"
static PetscErrorCode Mat_SeqAIJCUSPARSETriFactors_Destroy(Mat_SeqAIJCUSPARSETriFactors** trifactors)
{
  cusparseHandle_t handle;
  cusparseStatus_t stat;

  PetscFunctionBegin;
  if (*trifactors) {
    Mat_SeqAIJCUSPARSETriFactorStruct_Destroy(&(*trifactors)->loTriFactorPtr);
    Mat_SeqAIJCUSPARSETriFactorStruct_Destroy(&(*trifactors)->upTriFactorPtr);
    Mat_SeqAIJCUSPARSETriFactorStruct_Destroy(&(*trifactors)->loTriFactorPtrTranspose);
    Mat_SeqAIJCUSPARSETriFactorStruct_Destroy(&(*trifactors)->upTriFactorPtrTranspose);
    delete (*trifactors)->rpermIndices;
    delete (*trifactors)->cpermIndices;
    delete (*trifactors)->workVector;
    if (handle = (*trifactors)->handle) {
      stat = cusparseDestroy(handle);CHKERRCUSP(stat);
    }
    delete *trifactors;
    *trifactors = 0;
  }
  PetscFunctionReturn(0);
}

