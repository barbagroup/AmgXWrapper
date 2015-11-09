#define PETSC_SKIP_COMPLEX
#define PETSC_SKIP_SPINLOCK

#include <petscconf.h>
#include <../src/mat/impls/aij/mpi/mpiaij.h>   /*I "petscmat.h" I*/
#include <../src/mat/impls/aij/mpi/mpicusparse/mpicusparsematimpl.h>


#undef __FUNCT__
#define __FUNCT__ "MatMPIAIJSetPreallocation_MPIAIJCUSPARSE"
PetscErrorCode  MatMPIAIJSetPreallocation_MPIAIJCUSPARSE(Mat B,PetscInt d_nz,const PetscInt d_nnz[],PetscInt o_nz,const PetscInt o_nnz[])
{
  Mat_MPIAIJ         *b               = (Mat_MPIAIJ*)B->data;
  Mat_MPIAIJCUSPARSE * cusparseStruct = (Mat_MPIAIJCUSPARSE*)b->spptr;
  PetscErrorCode     ierr;
  PetscInt           i;

  PetscFunctionBegin;
  if (d_nz == PETSC_DEFAULT || d_nz == PETSC_DECIDE) d_nz = 5;
  if (o_nz == PETSC_DEFAULT || o_nz == PETSC_DECIDE) o_nz = 2;
  if (d_nz < 0) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"d_nz cannot be less than 0: value %D",d_nz);
  if (o_nz < 0) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"o_nz cannot be less than 0: value %D",o_nz);

  ierr = PetscLayoutSetUp(B->rmap);CHKERRQ(ierr);
  ierr = PetscLayoutSetUp(B->cmap);CHKERRQ(ierr);
  if (d_nnz) {
    for (i=0; i<B->rmap->n; i++) {
      if (d_nnz[i] < 0) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"d_nnz cannot be less than 0: local row %D value %D",i,d_nnz[i]);
    }
  }
  if (o_nnz) {
    for (i=0; i<B->rmap->n; i++) {
      if (o_nnz[i] < 0) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"o_nnz cannot be less than 0: local row %D value %D",i,o_nnz[i]);
    }
  }
  if (!B->preallocated) {
    /* Explicitly create 2 MATSEQAIJCUSPARSE matrices. */
    ierr = MatCreate(PETSC_COMM_SELF,&b->A);CHKERRQ(ierr);
    ierr = MatSetSizes(b->A,B->rmap->n,B->cmap->n,B->rmap->n,B->cmap->n);CHKERRQ(ierr);
    ierr = MatSetType(b->A,MATSEQAIJCUSPARSE);CHKERRQ(ierr);
    ierr = PetscLogObjectParent((PetscObject)B,(PetscObject)b->A);CHKERRQ(ierr);
    ierr = MatCreate(PETSC_COMM_SELF,&b->B);CHKERRQ(ierr);
    ierr = MatSetSizes(b->B,B->rmap->n,B->cmap->N,B->rmap->n,B->cmap->N);CHKERRQ(ierr);
    ierr = MatSetType(b->B,MATSEQAIJCUSPARSE);CHKERRQ(ierr);
    ierr = PetscLogObjectParent((PetscObject)B,(PetscObject)b->B);CHKERRQ(ierr);
  }
  ierr = MatSeqAIJSetPreallocation(b->A,d_nz,d_nnz);CHKERRQ(ierr);
  ierr = MatSeqAIJSetPreallocation(b->B,o_nz,o_nnz);CHKERRQ(ierr);
  ierr = MatCUSPARSESetFormat(b->A,MAT_CUSPARSE_MULT,cusparseStruct->diagGPUMatFormat);CHKERRQ(ierr);
  ierr = MatCUSPARSESetFormat(b->B,MAT_CUSPARSE_MULT,cusparseStruct->offdiagGPUMatFormat);CHKERRQ(ierr);
  ierr = MatCUSPARSESetHandle(b->A,cusparseStruct->handle);CHKERRQ(ierr);
  ierr = MatCUSPARSESetHandle(b->B,cusparseStruct->handle);CHKERRQ(ierr);
  ierr = MatCUSPARSESetStream(b->A,cusparseStruct->stream);CHKERRQ(ierr);
  ierr = MatCUSPARSESetStream(b->B,cusparseStruct->stream);CHKERRQ(ierr);

  B->preallocated = PETSC_TRUE;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatCreateVecs_MPIAIJCUSPARSE"
PetscErrorCode  MatCreateVecs_MPIAIJCUSPARSE(Mat mat,Vec *right,Vec *left)
{
  PetscErrorCode ierr;
  PetscInt rbs,cbs;

  PetscFunctionBegin;
  ierr = MatGetBlockSizes(mat,&rbs,&cbs);CHKERRQ(ierr);
  if (right) {
    ierr = VecCreate(PetscObjectComm((PetscObject)mat),right);CHKERRQ(ierr);
    ierr = VecSetSizes(*right,mat->cmap->n,PETSC_DETERMINE);CHKERRQ(ierr);
    ierr = VecSetBlockSize(*right,cbs);CHKERRQ(ierr);
    ierr = VecSetType(*right,VECCUSP);CHKERRQ(ierr);
    ierr = VecSetLayout(*right,mat->cmap);CHKERRQ(ierr);
  }
  if (left) {
    ierr = VecCreate(PetscObjectComm((PetscObject)mat),left);CHKERRQ(ierr);
    ierr = VecSetSizes(*left,mat->rmap->n,PETSC_DETERMINE);CHKERRQ(ierr);
    ierr = VecSetBlockSize(*left,rbs);CHKERRQ(ierr);
    ierr = VecSetType(*left,VECCUSP);CHKERRQ(ierr);
    ierr = VecSetLayout(*left,mat->rmap);CHKERRQ(ierr);


  }
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "MatMult_MPIAIJCUSPARSE"
PetscErrorCode MatMult_MPIAIJCUSPARSE(Mat A,Vec xx,Vec yy)
{
  /* This multiplication sequence is different sequence
     than the CPU version. In particular, the diagonal block
     multiplication kernel is launched in one stream. Then,
     in a separate stream, the data transfers from DeviceToHost
     (with MPI messaging in between), then HostToDevice are
     launched. Once the data transfer stream is synchronized,
     to ensure messaging is complete, the MatMultAdd kernel
     is launched in the original (MatMult) stream to protect
     against race conditions.

     This sequence should only be called for GPU computation. */
  Mat_MPIAIJ     *a = (Mat_MPIAIJ*)A->data;
  PetscErrorCode ierr;
  PetscInt       nt;

  PetscFunctionBegin;
  ierr = VecGetLocalSize(xx,&nt);CHKERRQ(ierr);
  if (nt != A->cmap->n) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_SIZ,"Incompatible partition of A (%D) and xx (%D)",A->cmap->n,nt);
  ierr = VecScatterInitializeForGPU(a->Mvctx,xx,SCATTER_FORWARD);CHKERRQ(ierr);
  ierr = (*a->A->ops->mult)(a->A,xx,yy);CHKERRQ(ierr);
  ierr = VecScatterBegin(a->Mvctx,xx,a->lvec,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
  ierr = VecScatterEnd(a->Mvctx,xx,a->lvec,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
  ierr = (*a->B->ops->multadd)(a->B,a->lvec,yy,yy);CHKERRQ(ierr);
  ierr = VecScatterFinalizeForGPU(a->Mvctx);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatMult_MPIAIJCUSPARSE"
PetscErrorCode MatMultTranspose_MPIAIJCUSPARSE(Mat A,Vec xx,Vec yy)
{
  /* This multiplication sequence is different sequence
     than the CPU version. In particular, the diagonal block
     multiplication kernel is launched in one stream. Then,
     in a separate stream, the data transfers from DeviceToHost
     (with MPI messaging in between), then HostToDevice are
     launched. Once the data transfer stream is synchronized,
     to ensure messaging is complete, the MatMultAdd kernel
     is launched in the original (MatMult) stream to protect
     against race conditions.

     This sequence should only be called for GPU computation. */
  Mat_MPIAIJ     *a = (Mat_MPIAIJ*)A->data;
  PetscErrorCode ierr;
  PetscInt       nt;

  PetscFunctionBegin;
  ierr = VecGetLocalSize(xx,&nt);CHKERRQ(ierr);
  if (nt != A->cmap->n) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_SIZ,"Incompatible partition of A (%D) and xx (%D)",A->cmap->n,nt);
  ierr = VecScatterInitializeForGPU(a->Mvctx,xx,SCATTER_FORWARD);CHKERRQ(ierr);
  ierr = (*a->A->ops->multtranspose)(a->A,xx,yy);CHKERRQ(ierr);
  ierr = VecScatterBegin(a->Mvctx,xx,a->lvec,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
  ierr = VecScatterEnd(a->Mvctx,xx,a->lvec,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
  ierr = (*a->B->ops->multtransposeadd)(a->B,a->lvec,yy,yy);CHKERRQ(ierr);
  ierr = VecScatterFinalizeForGPU(a->Mvctx);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatCUSPARSESetFormat_MPIAIJCUSPARSE"
PetscErrorCode MatCUSPARSESetFormat_MPIAIJCUSPARSE(Mat A,MatCUSPARSEFormatOperation op,MatCUSPARSEStorageFormat format)
{
  Mat_MPIAIJ         *a               = (Mat_MPIAIJ*)A->data;
  Mat_MPIAIJCUSPARSE * cusparseStruct = (Mat_MPIAIJCUSPARSE*)a->spptr;

  PetscFunctionBegin;
  switch (op) {
  case MAT_CUSPARSE_MULT_DIAG:
    cusparseStruct->diagGPUMatFormat = format;
    break;
  case MAT_CUSPARSE_MULT_OFFDIAG:
    cusparseStruct->offdiagGPUMatFormat = format;
    break;
  case MAT_CUSPARSE_ALL:
    cusparseStruct->diagGPUMatFormat    = format;
    cusparseStruct->offdiagGPUMatFormat = format;
    break;
  default:
    SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_SUP,"unsupported operation %d for MatCUSPARSEFormatOperation. Only MAT_CUSPARSE_MULT_DIAG, MAT_CUSPARSE_MULT_DIAG, and MAT_CUSPARSE_MULT_ALL are currently supported.",op);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSetFromOptions_MPIAIJCUSPARSE"
PetscErrorCode MatSetFromOptions_MPIAIJCUSPARSE(PetscOptionItems *PetscOptionsObject,Mat A)
{
  MatCUSPARSEStorageFormat format;
  PetscErrorCode           ierr;
  PetscBool                flg;
  Mat_MPIAIJ               *a = (Mat_MPIAIJ*)A->data;
  Mat_MPIAIJCUSPARSE       *cusparseStruct = (Mat_MPIAIJCUSPARSE*)a->spptr;

  PetscFunctionBegin;
  ierr = PetscOptionsHead(PetscOptionsObject,"MPIAIJCUSPARSE options");CHKERRQ(ierr);
  ierr = PetscObjectOptionsBegin((PetscObject)A);
  if (A->factortype==MAT_FACTOR_NONE) {
    ierr = PetscOptionsEnum("-mat_cusparse_mult_diag_storage_format","sets storage format of the diagonal blocks of (mpi)aijcusparse gpu matrices for SpMV",
                            "MatCUSPARSESetFormat",MatCUSPARSEStorageFormats,(PetscEnum)cusparseStruct->diagGPUMatFormat,(PetscEnum*)&format,&flg);CHKERRQ(ierr);
    if (flg) {
      ierr = MatCUSPARSESetFormat(A,MAT_CUSPARSE_MULT_DIAG,format);CHKERRQ(ierr);
    }
    ierr = PetscOptionsEnum("-mat_cusparse_mult_offdiag_storage_format","sets storage format of the off-diagonal blocks (mpi)aijcusparse gpu matrices for SpMV",
                            "MatCUSPARSESetFormat",MatCUSPARSEStorageFormats,(PetscEnum)cusparseStruct->offdiagGPUMatFormat,(PetscEnum*)&format,&flg);CHKERRQ(ierr);
    if (flg) {
      ierr = MatCUSPARSESetFormat(A,MAT_CUSPARSE_MULT_OFFDIAG,format);CHKERRQ(ierr);
    }
    ierr = PetscOptionsEnum("-mat_cusparse_storage_format","sets storage format of the diagonal and off-diagonal blocks (mpi)aijcusparse gpu matrices for SpMV",
                            "MatCUSPARSESetFormat",MatCUSPARSEStorageFormats,(PetscEnum)cusparseStruct->diagGPUMatFormat,(PetscEnum*)&format,&flg);CHKERRQ(ierr);
    if (flg) {
      ierr = MatCUSPARSESetFormat(A,MAT_CUSPARSE_ALL,format);CHKERRQ(ierr);
    }
  }
  ierr = PetscOptionsEnd();CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatDestroy_MPIAIJCUSPARSE"
PetscErrorCode MatDestroy_MPIAIJCUSPARSE(Mat A)
{
  PetscErrorCode     ierr;
  Mat_MPIAIJ         *a              = (Mat_MPIAIJ*)A->data;
  Mat_MPIAIJCUSPARSE *cusparseStruct = (Mat_MPIAIJCUSPARSE*)a->spptr;
  cudaError_t        err;
  cusparseStatus_t   stat;

  PetscFunctionBegin;
  try {
    ierr = MatCUSPARSEClearHandle(a->A);CHKERRQ(ierr);
    ierr = MatCUSPARSEClearHandle(a->B);CHKERRQ(ierr);
    stat = cusparseDestroy(cusparseStruct->handle);CHKERRCUSP(stat);
    err = cudaStreamDestroy(cusparseStruct->stream);CHKERRCUSP(err);
    delete cusparseStruct;
  } catch(char *ex) {
    SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"Mat_MPIAIJCUSPARSE error: %s", ex);
  }
  cusparseStruct = 0;

  ierr = MatDestroy_MPIAIJ(A);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatCreate_MPIAIJCUSPARSE"
PETSC_EXTERN PetscErrorCode MatCreate_MPIAIJCUSPARSE(Mat A)
{
  PetscErrorCode     ierr;
  Mat_MPIAIJ         *a;
  Mat_MPIAIJCUSPARSE * cusparseStruct;
  cudaError_t        err;
  cusparseStatus_t   stat;

  PetscFunctionBegin;
  ierr = MatCreate_MPIAIJ(A);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)A,"MatMPIAIJSetPreallocation_C",MatMPIAIJSetPreallocation_MPIAIJCUSPARSE);CHKERRQ(ierr);
  a        = (Mat_MPIAIJ*)A->data;
  a->spptr = new Mat_MPIAIJCUSPARSE;

  cusparseStruct                      = (Mat_MPIAIJCUSPARSE*)a->spptr;
  cusparseStruct->diagGPUMatFormat    = MAT_CUSPARSE_CSR;
  cusparseStruct->offdiagGPUMatFormat = MAT_CUSPARSE_CSR;
  stat = cusparseCreate(&(cusparseStruct->handle));CHKERRCUSP(stat);
  err = cudaStreamCreate(&(cusparseStruct->stream));CHKERRCUSP(err);

  A->ops->getvecs        = MatCreateVecs_MPIAIJCUSPARSE;
  A->ops->mult           = MatMult_MPIAIJCUSPARSE;
  A->ops->multtranspose  = MatMultTranspose_MPIAIJCUSPARSE;
  A->ops->setfromoptions = MatSetFromOptions_MPIAIJCUSPARSE;
  A->ops->destroy        = MatDestroy_MPIAIJCUSPARSE;

  ierr = PetscObjectChangeTypeName((PetscObject)A,MATMPIAIJCUSPARSE);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)A,"MatCUSPARSESetFormat_C",  MatCUSPARSESetFormat_MPIAIJCUSPARSE);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/*@
   MatCreateAIJCUSPARSE - Creates a sparse matrix in AIJ (compressed row) format
   (the default parallel PETSc format).  This matrix will ultimately pushed down
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
   MatXXXXSetPreallocation() paradigm instead of this routine directly.
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

.seealso: MatCreate(), MatCreateAIJ(), MatSetValues(), MatSeqAIJSetColumnIndices(), MatCreateSeqAIJWithArrays(), MatCreateAIJ(), MATMPIAIJCUSPARSE, MATAIJCUSPARSE
@*/
#undef __FUNCT__
#define __FUNCT__ "MatCreateAIJCUSPARSE"
PetscErrorCode  MatCreateAIJCUSPARSE(MPI_Comm comm,PetscInt m,PetscInt n,PetscInt M,PetscInt N,PetscInt d_nz,const PetscInt d_nnz[],PetscInt o_nz,const PetscInt o_nnz[],Mat *A)
{
  PetscErrorCode ierr;
  PetscMPIInt    size;

  PetscFunctionBegin;
  ierr = MatCreate(comm,A);CHKERRQ(ierr);
  ierr = MatSetSizes(*A,m,n,M,N);CHKERRQ(ierr);
  ierr = MPI_Comm_size(comm,&size);CHKERRQ(ierr);
  if (size > 1) {
    ierr = MatSetType(*A,MATMPIAIJCUSPARSE);CHKERRQ(ierr);
    ierr = MatMPIAIJSetPreallocation(*A,d_nz,d_nnz,o_nz,o_nnz);CHKERRQ(ierr);
  } else {
    ierr = MatSetType(*A,MATSEQAIJCUSPARSE);CHKERRQ(ierr);
    ierr = MatSeqAIJSetPreallocation(*A,d_nz,d_nnz);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

/*M
   MATAIJCUSPARSE - MATMPIAIJCUSPARSE = "aijcusparse" = "mpiaijcusparse" - A matrix type to be used for sparse matrices.

   A matrix type type whose data resides on Nvidia GPUs. These matrices can be in either
   CSR, ELL, or Hybrid format. The ELL and HYB formats require CUDA 4.2 or later.
   All matrix calculations are performed on Nvidia GPUs using the CUSPARSE library.

   This matrix type is identical to MATSEQAIJCUSPARSE when constructed with a single process communicator,
   and MATMPIAIJCUSPARSE otherwise.  As a result, for single process communicators,
   MatSeqAIJSetPreallocation is supported, and similarly MatMPIAIJSetPreallocation is supported
   for communicators controlling multiple processes.  It is recommended that you call both of
   the above preallocation routines for simplicity.

   Options Database Keys:
+  -mat_type mpiaijcusparse - sets the matrix type to "mpiaijcusparse" during a call to MatSetFromOptions()
.  -mat_cusparse_storage_format csr - sets the storage format of diagonal and off-diagonal matrices during a call to MatSetFromOptions(). Other options include ell (ellpack) or hyb (hybrid).
.  -mat_cusparse_mult_diag_storage_format csr - sets the storage format of diagonal matrix during a call to MatSetFromOptions(). Other options include ell (ellpack) or hyb (hybrid).
-  -mat_cusparse_mult_offdiag_storage_format csr - sets the storage format of off-diagonal matrix during a call to MatSetFromOptions(). Other options include ell (ellpack) or hyb (hybrid).

  Level: beginner

 .seealso: MatCreateAIJCUSPARSE(), MATSEQAIJCUSPARSE, MatCreateSeqAIJCUSPARSE(), MatCUSPARSESetFormat(), MatCUSPARSEStorageFormat, MatCUSPARSEFormatOperation
M
M*/
