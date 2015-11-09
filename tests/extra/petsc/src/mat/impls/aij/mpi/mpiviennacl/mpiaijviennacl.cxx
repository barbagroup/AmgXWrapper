#include <petscconf.h>
#include <../src/mat/impls/aij/mpi/mpiaij.h>   /*I "petscmat.h" I*/
#include <../src/mat/impls/aij/seq/seqviennacl/viennaclmatimpl.h>

#undef __FUNCT__
#define __FUNCT__ "MatMPIAIJSetPreallocation_MPIAIJViennaCL"
PetscErrorCode  MatMPIAIJSetPreallocation_MPIAIJViennaCL(Mat B,PetscInt d_nz,const PetscInt d_nnz[],PetscInt o_nz,const PetscInt o_nnz[])
{
  Mat_MPIAIJ *b = (Mat_MPIAIJ*)B->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscLayoutSetUp(B->rmap);CHKERRQ(ierr);
  ierr = PetscLayoutSetUp(B->cmap);CHKERRQ(ierr);
  if (!B->preallocated) {
    /* Explicitly create the two MATSEQAIJVIENNACL matrices. */
    ierr = MatCreate(PETSC_COMM_SELF,&b->A);CHKERRQ(ierr);
    ierr = MatSetSizes(b->A,B->rmap->n,B->cmap->n,B->rmap->n,B->cmap->n);CHKERRQ(ierr);
    ierr = MatSetType(b->A,MATSEQAIJVIENNACL);CHKERRQ(ierr);
    ierr = PetscLogObjectParent((PetscObject)B,(PetscObject)b->A);CHKERRQ(ierr);
    ierr = MatCreate(PETSC_COMM_SELF,&b->B);CHKERRQ(ierr);
    ierr = MatSetSizes(b->B,B->rmap->n,B->cmap->N,B->rmap->n,B->cmap->N);CHKERRQ(ierr);
    ierr = MatSetType(b->B,MATSEQAIJVIENNACL);CHKERRQ(ierr);
    ierr = PetscLogObjectParent((PetscObject)B,(PetscObject)b->B);CHKERRQ(ierr);
  }
  ierr = MatSeqAIJSetPreallocation(b->A,d_nz,d_nnz);CHKERRQ(ierr);
  ierr = MatSeqAIJSetPreallocation(b->B,o_nz,o_nnz);CHKERRQ(ierr);
  B->preallocated = PETSC_TRUE;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatCreateVecs_MPIAIJViennaCL"
PetscErrorCode  MatCreateVecs_MPIAIJViennaCL(Mat mat,Vec *right,Vec *left)
{
  PetscErrorCode ierr;
  PetscInt rbs,cbs;

  PetscFunctionBegin;
  ierr = MatGetBlockSizes(mat,&rbs,&cbs);CHKERRQ(ierr);
  if (right) {
    ierr = VecCreate(PetscObjectComm((PetscObject)mat),right);CHKERRQ(ierr);
    ierr = VecSetSizes(*right,mat->cmap->n,PETSC_DETERMINE);CHKERRQ(ierr);
    ierr = VecSetBlockSize(*right,cbs);CHKERRQ(ierr);
    ierr = VecSetType(*right,VECVIENNACL);CHKERRQ(ierr);
    ierr = VecSetLayout(*right,mat->cmap);CHKERRQ(ierr);
  }
  if (left) {
    ierr = VecCreate(PetscObjectComm((PetscObject)mat),left);CHKERRQ(ierr);
    ierr = VecSetSizes(*left,mat->rmap->n,PETSC_DETERMINE);CHKERRQ(ierr);
    ierr = VecSetBlockSize(*left,rbs);CHKERRQ(ierr);
    ierr = VecSetType(*left,VECVIENNACL);CHKERRQ(ierr);
    ierr = VecSetLayout(*left,mat->rmap);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "MatDestroy_MPIAIJViennaCL"
PetscErrorCode MatDestroy_MPIAIJViennaCL(Mat A)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MatDestroy_MPIAIJ(A);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatCreate_MPIAIJViennaCL"
PETSC_EXTERN PetscErrorCode MatCreate_MPIAIJViennaCL(Mat A)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MatCreate_MPIAIJ(A);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)A,"MatMPIAIJSetPreallocation_C",MatMPIAIJSetPreallocation_MPIAIJViennaCL);CHKERRQ(ierr);
  A->ops->getvecs        = MatCreateVecs_MPIAIJViennaCL;

  ierr = PetscObjectChangeTypeName((PetscObject)A,MATMPIAIJVIENNACL);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}


/*@
   MatCreateAIJViennaCL - Creates a sparse matrix in AIJ (compressed row) format
   (the default parallel PETSc format).  This matrix will ultimately be pushed down
   to GPUs and use the ViennaCL library for calculations. For good matrix
   assembly performance the user should preallocate the matrix storage by setting
   the parameter nz (or the array nnz).  By setting these parameters accurately,
   performance during matrix assembly can be increased substantially.


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

   Level: intermediate

.seealso: MatCreate(), MatCreateAIJ(), MatCreateAIJCUSP(), MatSetValues(), MatSeqAIJSetColumnIndices(), MatCreateSeqAIJWithArrays(), MatCreateAIJ(), MATMPIAIJVIENNACL, MATAIJVIENNACL
@*/
#undef __FUNCT__
#define __FUNCT__ "MatCreateAIJViennaCL"
PetscErrorCode  MatCreateAIJViennaCL(MPI_Comm comm,PetscInt m,PetscInt n,PetscInt M,PetscInt N,PetscInt d_nz,const PetscInt d_nnz[],PetscInt o_nz,const PetscInt o_nnz[],Mat *A)
{
  PetscErrorCode ierr;
  PetscMPIInt    size;

  PetscFunctionBegin;
  ierr = MatCreate(comm,A);CHKERRQ(ierr);
  ierr = MatSetSizes(*A,m,n,M,N);CHKERRQ(ierr);
  ierr = MPI_Comm_size(comm,&size);CHKERRQ(ierr);
  if (size > 1) {
    ierr = MatSetType(*A,MATMPIAIJVIENNACL);CHKERRQ(ierr);
    ierr = MatMPIAIJSetPreallocation(*A,d_nz,d_nnz,o_nz,o_nnz);CHKERRQ(ierr);
  } else {
    ierr = MatSetType(*A,MATSEQAIJVIENNACL);CHKERRQ(ierr);
    ierr = MatSeqAIJSetPreallocation(*A,d_nz,d_nnz);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

/*M
   MATAIJVIENNACL - MATMPIAIJVIENNACL= "aijviennacl" = "mpiaijviennacl" - A matrix type to be used for sparse matrices.

   A matrix type (CSR format) whose data resides on GPUs.
   All matrix calculations are performed using the ViennaCL library.

   This matrix type is identical to MATSEQAIJVIENNACL when constructed with a single process communicator,
   and MATMPIAIJVIENNACL otherwise.  As a result, for single process communicators,
   MatSeqAIJSetPreallocation is supported, and similarly MatMPIAIJSetPreallocation is supported
   for communicators controlling multiple processes.  It is recommended that you call both of
   the above preallocation routines for simplicity.

   Options Database Keys:
+  -mat_type mpiaijviennacl - sets the matrix type to "mpiaijviennacl" during a call to MatSetFromOptions()

  Level: beginner

 .seealso: MatCreateAIJViennaCL(), MATSEQAIJVIENNACL, MatCreateSeqAIJVIENNACL()
M*/

