

/*
    Defines the basic matrix operations for the AIJ (compressed row)
  matrix storage format.
*/

#include <petscconf.h>
#include <../src/mat/impls/aij/seq/aij.h>          /*I "petscmat.h" I*/
#include <petscbt.h>
#include <../src/vec/vec/impls/dvecimpl.h>
#include <petsc/private/vecimpl.h>

#include <../src/mat/impls/aij/seq/seqviennacl/viennaclmatimpl.h>


#include <algorithm>
#include <vector>
#include <string>

#include "viennacl/linalg/prod.hpp"

#undef __FUNCT__
#define __FUNCT__ "MatViennaCLCopyToGPU"
PetscErrorCode MatViennaCLCopyToGPU(Mat A)
{

  Mat_SeqAIJViennaCL *viennaclstruct = (Mat_SeqAIJViennaCL*)A->spptr;
  Mat_SeqAIJ         *a              = (Mat_SeqAIJ*)A->data;
  PetscErrorCode     ierr;


  PetscFunctionBegin;
  if (A->rmap->n > 0 && A->cmap->n > 0) { //some OpenCL SDKs have issues with buffers of size 0
    if (A->valid_GPU_matrix == PETSC_VIENNACL_UNALLOCATED || A->valid_GPU_matrix == PETSC_VIENNACL_CPU) {
      ierr = PetscLogEventBegin(MAT_ViennaCLCopyToGPU,A,0,0,0);CHKERRQ(ierr);

      try {
        ierr = PetscObjectViennaCLSetFromOptions((PetscObject)A);CHKERRQ(ierr); /* Allows to set device type before allocating any objects */
        if (a->compressedrow.use) {
          if (!viennaclstruct->compressed_mat) viennaclstruct->compressed_mat = new ViennaCLCompressedAIJMatrix();

          // Since PetscInt is different from cl_uint, we have to convert:
          viennacl::backend::mem_handle dummy;

          viennacl::backend::typesafe_host_array<unsigned int> row_buffer; row_buffer.raw_resize(dummy, a->compressedrow.nrows+1);
          for (PetscInt i=0; i<=a->compressedrow.nrows; ++i)
            row_buffer.set(i, (a->compressedrow.i)[i]);

          viennacl::backend::typesafe_host_array<unsigned int> row_indices; row_indices.raw_resize(dummy, a->compressedrow.nrows);
          for (PetscInt i=0; i<a->compressedrow.nrows; ++i)
            row_indices.set(i, (a->compressedrow.rindex)[i]);

          viennacl::backend::typesafe_host_array<unsigned int> col_buffer; col_buffer.raw_resize(dummy, a->nz);
          for (PetscInt i=0; i<a->nz; ++i)
            col_buffer.set(i, (a->j)[i]);

          viennaclstruct->compressed_mat->set(row_buffer.get(), row_indices.get(), col_buffer.get(), a->a, A->rmap->n, A->cmap->n, a->compressedrow.nrows, a->nz);
        } else {
          if (!viennaclstruct->mat) viennaclstruct->mat = new ViennaCLAIJMatrix();

          // Since PetscInt is in general different from cl_uint, we have to convert:
          viennacl::backend::mem_handle dummy;

          viennacl::backend::typesafe_host_array<unsigned int> row_buffer; row_buffer.raw_resize(dummy, A->rmap->n+1);
          for (PetscInt i=0; i<=A->rmap->n; ++i)
            row_buffer.set(i, (a->i)[i]);

          viennacl::backend::typesafe_host_array<unsigned int> col_buffer; col_buffer.raw_resize(dummy, a->nz);
          for (PetscInt i=0; i<a->nz; ++i)
            col_buffer.set(i, (a->j)[i]);

          viennaclstruct->mat->set(row_buffer.get(), col_buffer.get(), a->a, A->rmap->n, A->cmap->n, a->nz);
        }
        ViennaCLWaitForGPU();
      } catch(std::exception const & ex) {
        SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"ViennaCL error: %s", ex.what());
      }

      // Create temporary vector for v += A*x:
      if (viennaclstruct->tempvec) {
        if (viennaclstruct->tempvec->size() != static_cast<std::size_t>(a->nz)) {
          delete (ViennaCLVector*)viennaclstruct->tempvec;
          viennaclstruct->tempvec = new ViennaCLVector(a->nz);
        } else {
          viennaclstruct->tempvec->clear();
        }
      } else {
        viennaclstruct->tempvec = new ViennaCLVector(a->nz);
      }

      A->valid_GPU_matrix = PETSC_VIENNACL_BOTH;

      ierr = PetscLogEventEnd(MAT_ViennaCLCopyToGPU,A,0,0,0);CHKERRQ(ierr);
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatViennaCLCopyFromGPU"
PetscErrorCode MatViennaCLCopyFromGPU(Mat A, const ViennaCLAIJMatrix *Agpu)
{
  Mat_SeqAIJ         *a              = (Mat_SeqAIJ*)A->data;
  PetscInt           m               = A->rmap->n;
  PetscErrorCode     ierr;


  PetscFunctionBegin;
  if (A->valid_GPU_matrix == PETSC_VIENNACL_UNALLOCATED) {
    try {
      if (a->compressedrow.use) SETERRQ(PETSC_COMM_WORLD, PETSC_ERR_ARG_WRONG, "ViennaCL: Cannot handle row compression for GPU matrices");
      else {

        if ((PetscInt)Agpu->size1() != m) SETERRQ2(PETSC_COMM_WORLD, PETSC_ERR_ARG_SIZ, "GPU matrix has %d rows, should be %d", Agpu->size1(), m);
        a->nz           = Agpu->nnz();
        a->maxnz        = a->nz; /* Since we allocate exactly the right amount */
        A->preallocated = PETSC_TRUE;
        if (a->singlemalloc) {
          if (a->a) {ierr = PetscFree3(a->a,a->j,a->i);CHKERRQ(ierr);}
        } else {
          if (a->i) {ierr = PetscFree(a->i);CHKERRQ(ierr);}
          if (a->j) {ierr = PetscFree(a->j);CHKERRQ(ierr);}
          if (a->a) {ierr = PetscFree(a->a);CHKERRQ(ierr);}
        }
        ierr = PetscMalloc3(a->nz,&a->a,a->nz,&a->j,m+1,&a->i);CHKERRQ(ierr);
        ierr = PetscLogObjectMemory((PetscObject)A, a->nz*(sizeof(PetscScalar)+sizeof(PetscInt))+(m+1)*sizeof(PetscInt));CHKERRQ(ierr);

        a->singlemalloc = PETSC_TRUE;

        /* Setup row lengths */
        if (a->imax) {ierr = PetscFree2(a->imax,a->ilen);CHKERRQ(ierr);}
        ierr = PetscMalloc2(m,&a->imax,m,&a->ilen);CHKERRQ(ierr);
        ierr = PetscLogObjectMemory((PetscObject)A, 2*m*sizeof(PetscInt));CHKERRQ(ierr);

        /* Copy data back from GPU */
        viennacl::backend::typesafe_host_array<unsigned int> row_buffer; row_buffer.raw_resize(Agpu->handle1(), Agpu->size1() + 1);

        // copy row array
        viennacl::backend::memory_read(Agpu->handle1(), 0, row_buffer.raw_size(), row_buffer.get());
        (a->i)[0] = row_buffer[0];
        for (PetscInt i = 0; i < (PetscInt)Agpu->size1(); ++i) {
          (a->i)[i+1] = row_buffer[i+1];
          a->imax[i]  = a->ilen[i] = a->i[i+1] - a->i[i];  //Set imax[] and ilen[] arrays at the same time as i[] for better cache reuse
        }

        // copy column indices
        viennacl::backend::typesafe_host_array<unsigned int> col_buffer; col_buffer.raw_resize(Agpu->handle2(), Agpu->nnz());
        viennacl::backend::memory_read(Agpu->handle2(), 0, col_buffer.raw_size(), col_buffer.get());
        for (PetscInt i=0; i < (PetscInt)Agpu->nnz(); ++i)
          (a->j)[i] = col_buffer[i];

        // copy nonzero entries directly to destination (no conversion required)
        viennacl::backend::memory_read(Agpu->handle(), 0, sizeof(PetscScalar)*Agpu->nnz(), a->a);

        ViennaCLWaitForGPU();
        /* TODO: Once a->diag is moved out of MatAssemblyEnd(), invalidate it here. */
      }
    } catch(std::exception const & ex) {
      SETERRQ1(PETSC_COMM_SELF, PETSC_ERR_LIB, "ViennaCL error: %s", ex.what());
    }

    /* This assembly prevents resetting the flag to PETSC_VIENNACL_CPU and recopying */
    ierr = MatAssemblyBegin(A, MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
    ierr = MatAssemblyEnd(A, MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);

    A->valid_GPU_matrix = PETSC_VIENNACL_BOTH;
  } else SETERRQ(PETSC_COMM_WORLD, PETSC_ERR_ARG_WRONG, "ViennaCL error: Only valid for unallocated GPU matrices");
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatCreateVecs_SeqAIJViennaCL"
PetscErrorCode MatCreateVecs_SeqAIJViennaCL(Mat mat, Vec *right, Vec *left)
{
  PetscErrorCode ierr;
  PetscInt rbs,cbs;

  PetscFunctionBegin;
  ierr = MatGetBlockSizes(mat,&rbs,&cbs);CHKERRQ(ierr);
  if (right) {
    ierr = VecCreate(PetscObjectComm((PetscObject)mat),right);CHKERRQ(ierr);
    ierr = VecSetSizes(*right,mat->cmap->n,PETSC_DETERMINE);CHKERRQ(ierr);
    ierr = VecSetBlockSize(*right,cbs);CHKERRQ(ierr);
    ierr = VecSetType(*right,VECSEQVIENNACL);CHKERRQ(ierr);
    ierr = PetscLayoutReference(mat->cmap,&(*right)->map);CHKERRQ(ierr);
  }
  if (left) {
    ierr = VecCreate(PetscObjectComm((PetscObject)mat),left);CHKERRQ(ierr);
    ierr = VecSetSizes(*left,mat->rmap->n,PETSC_DETERMINE);CHKERRQ(ierr);
    ierr = VecSetBlockSize(*left,rbs);CHKERRQ(ierr);
    ierr = VecSetType(*left,VECSEQVIENNACL);CHKERRQ(ierr);
    ierr = PetscLayoutReference(mat->rmap,&(*left)->map);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatMult_SeqAIJViennaCL"
PetscErrorCode MatMult_SeqAIJViennaCL(Mat A,Vec xx,Vec yy)
{
  Mat_SeqAIJ           *a = (Mat_SeqAIJ*)A->data;
  PetscErrorCode       ierr;
  Mat_SeqAIJViennaCL   *viennaclstruct = (Mat_SeqAIJViennaCL*)A->spptr;
  const ViennaCLVector *xgpu=NULL;
  ViennaCLVector       *ygpu=NULL;

  PetscFunctionBegin;
  if (A->rmap->n > 0 && A->cmap->n > 0) {
    ierr = VecViennaCLGetArrayRead(xx,&xgpu);CHKERRQ(ierr);
    ierr = VecViennaCLGetArrayWrite(yy,&ygpu);CHKERRQ(ierr);
    try {
      *ygpu = viennacl::linalg::prod(*viennaclstruct->mat,*xgpu);
      ViennaCLWaitForGPU();
    } catch (std::exception const & ex) {
      SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"ViennaCL error: %s", ex.what());
    }
    ierr = VecViennaCLRestoreArrayRead(xx,&xgpu);CHKERRQ(ierr);
    ierr = VecViennaCLRestoreArrayWrite(yy,&ygpu);CHKERRQ(ierr);
    ierr = PetscLogFlops(2.0*a->nz - viennaclstruct->mat->nnz());CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}



#undef __FUNCT__
#define __FUNCT__ "MatMultAdd_SeqAIJViennaCL"
PetscErrorCode MatMultAdd_SeqAIJViennaCL(Mat A,Vec xx,Vec yy,Vec zz)
{
  Mat_SeqAIJ           *a = (Mat_SeqAIJ*)A->data;
  PetscErrorCode       ierr;
  Mat_SeqAIJViennaCL   *viennaclstruct = (Mat_SeqAIJViennaCL*)A->spptr;
  const ViennaCLVector *xgpu=NULL,*ygpu=NULL;
  ViennaCLVector       *zgpu=NULL;

  PetscFunctionBegin;
  if (A->rmap->n > 0 && A->cmap->n > 0) {
    try {
      ierr = VecViennaCLGetArrayRead(xx,&xgpu);CHKERRQ(ierr);
      ierr = VecViennaCLGetArrayRead(yy,&ygpu);CHKERRQ(ierr);
      ierr = VecViennaCLGetArrayWrite(zz,&zgpu);CHKERRQ(ierr);

      if (a->compressedrow.use) {
        ViennaCLVector temp = viennacl::linalg::prod(*viennaclstruct->compressed_mat, *xgpu);
        *zgpu = *ygpu + temp;
        ViennaCLWaitForGPU();
      } else {
        if (zz == xx || zz == yy) { //temporary required
          ViennaCLVector temp = viennacl::linalg::prod(*viennaclstruct->mat, *xgpu);
          *zgpu = *ygpu;
          *zgpu += temp;
          ViennaCLWaitForGPU();
        } else {
          *viennaclstruct->tempvec = viennacl::linalg::prod(*viennaclstruct->mat, *xgpu);
          *zgpu = *ygpu + *viennaclstruct->tempvec;
          ViennaCLWaitForGPU();
        }
      }

      ierr = VecViennaCLRestoreArrayRead(xx,&xgpu);CHKERRQ(ierr);
      ierr = VecViennaCLRestoreArrayRead(yy,&ygpu);CHKERRQ(ierr);
      ierr = VecViennaCLRestoreArrayWrite(zz,&zgpu);CHKERRQ(ierr);

    } catch(std::exception const & ex) {
      SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"ViennaCL error: %s", ex.what());
    }
    ierr = PetscLogFlops(2.0*a->nz);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatAssemblyEnd_SeqAIJViennaCL"
PetscErrorCode MatAssemblyEnd_SeqAIJViennaCL(Mat A,MatAssemblyType mode)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MatAssemblyEnd_SeqAIJ(A,mode);CHKERRQ(ierr);
  ierr = MatViennaCLCopyToGPU(A);CHKERRQ(ierr);
  if (mode == MAT_FLUSH_ASSEMBLY) PetscFunctionReturn(0);
  A->ops->mult    = MatMult_SeqAIJViennaCL;
  A->ops->multadd = MatMultAdd_SeqAIJViennaCL;
  PetscFunctionReturn(0);
}

/* --------------------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "MatCreateSeqAIJViennaCL"
/*@
   MatCreateSeqAIJViennaCL - Creates a sparse matrix in AIJ (compressed row) format
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

.seealso: MatCreate(), MatCreateAIJ(), MatCreateAIJCUSP(), MatSetValues(), MatSeqAIJSetColumnIndices(), MatCreateSeqAIJWithArrays(), MatCreateAIJ()

@*/
PetscErrorCode  MatCreateSeqAIJViennaCL(MPI_Comm comm,PetscInt m,PetscInt n,PetscInt nz,const PetscInt nnz[],Mat *A)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MatCreate(comm,A);CHKERRQ(ierr);
  ierr = MatSetSizes(*A,m,n,m,n);CHKERRQ(ierr);
  ierr = MatSetType(*A,MATSEQAIJVIENNACL);CHKERRQ(ierr);
  ierr = MatSeqAIJSetPreallocation_SeqAIJ(*A,nz,(PetscInt*)nnz);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "MatDestroy_SeqAIJViennaCL"
PetscErrorCode MatDestroy_SeqAIJViennaCL(Mat A)
{
  PetscErrorCode ierr;
  Mat_SeqAIJViennaCL *viennaclcontainer = (Mat_SeqAIJViennaCL*)A->spptr;

  PetscFunctionBegin;
  try {
    if (viennaclcontainer) {
      delete viennaclcontainer->tempvec;
      delete viennaclcontainer->mat;
      delete viennaclcontainer->compressed_mat;
      delete viennaclcontainer;
    }
    A->valid_GPU_matrix = PETSC_VIENNACL_UNALLOCATED;
  } catch(std::exception const & ex) {
    SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"ViennaCL error: %s", ex.what());
  }
  /* this next line is because MatDestroy tries to PetscFree spptr if it is not zero, and PetscFree only works if the memory was allocated with PetscNew or PetscMalloc, which don't call the constructor */
  A->spptr = 0;
  ierr     = MatDestroy_SeqAIJ(A);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "MatCreate_SeqAIJViennaCL"
PETSC_EXTERN PetscErrorCode MatCreate_SeqAIJViennaCL(Mat B)
{
  PetscErrorCode ierr;
  Mat_SeqAIJ     *aij;

  PetscFunctionBegin;
  ierr            = MatCreate_SeqAIJ(B);CHKERRQ(ierr);
  aij             = (Mat_SeqAIJ*)B->data;
  aij->inode.use  = PETSC_FALSE;
  B->ops->mult    = MatMult_SeqAIJViennaCL;
  B->ops->multadd = MatMultAdd_SeqAIJViennaCL;
  B->spptr        = new Mat_SeqAIJViennaCL();

  ((Mat_SeqAIJViennaCL*)B->spptr)->tempvec        = NULL;
  ((Mat_SeqAIJViennaCL*)B->spptr)->mat            = NULL;
  ((Mat_SeqAIJViennaCL*)B->spptr)->compressed_mat = NULL;

  B->ops->assemblyend    = MatAssemblyEnd_SeqAIJViennaCL;
  B->ops->destroy        = MatDestroy_SeqAIJViennaCL;
  B->ops->getvecs        = MatCreateVecs_SeqAIJViennaCL;

  ierr = PetscObjectChangeTypeName((PetscObject)B,MATSEQAIJVIENNACL);CHKERRQ(ierr);

  B->valid_GPU_matrix = PETSC_VIENNACL_UNALLOCATED;
  PetscFunctionReturn(0);
}


/*M
   MATSEQAIJVIENNACL - MATAIJVIENNACL = "aijviennacl" = "seqaijviennacl" - A matrix type to be used for sparse matrices.

   A matrix type type whose data resides on GPUs. These matrices are in CSR format by
   default. All matrix calculations are performed using the ViennaCL library.

   Options Database Keys:
+  -mat_type aijviennacl - sets the matrix type to "seqaijviennacl" during a call to MatSetFromOptions()
.  -mat_viennacl_storage_format csr - sets the storage format of matrices for MatMult during a call to MatSetFromOptions().
-  -mat_viennacl_mult_storage_format csr - sets the storage format of matrices for MatMult during a call to MatSetFromOptions().

  Level: beginner

.seealso: MatCreateSeqAIJViennaCL(), MATAIJVIENNACL, MatCreateAIJViennaCL()
M*/

