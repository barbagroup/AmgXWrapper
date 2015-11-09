#define PETSCMAT_DLL

#include <../src/mat/impls/baij/mpi/mpibaij.h>
#include <../src/mat/impls/baij/seq/bstream/bstream.h>

extern PetscErrorCode MatMult_SeqBSTRM_4(Mat,Vec,Vec);
extern PetscErrorCode MatMult_SeqBSTRM_5(Mat,Vec,Vec);
extern PetscErrorCode MatMultAdd_SeqBSTRM_4(Mat,Vec,Vec,Vec);
extern PetscErrorCode MatMultAdd_SeqBSTRM_5(Mat,Vec,Vec,Vec);
extern PetscErrorCode MatSOR_SeqBSTRM_4(Mat,Vec,PetscReal,MatSORType,PetscReal,PetscInt,PetscInt,Vec);
extern PetscErrorCode MatSOR_SeqBSTRM_5(Mat,Vec,PetscReal,MatSORType,PetscReal,PetscInt,PetscInt,Vec);


#undef __FUNCT__
#define __FUNCT__ "MatMPIBSTRM_create_bstrm"
PetscErrorCode MatMPIBSTRM_create_bstrm(Mat A)
{
  Mat_MPIBAIJ *a   = (Mat_MPIBAIJ*)A->data;
  Mat_SeqBAIJ *Aij = (Mat_SeqBAIJ*)(a->A->data), *Bij = (Mat_SeqBAIJ*)(a->B->data);
  /* */
  Mat_SeqBSTRM *bstrmA, *bstrmB;
  PetscInt     MROW = Aij->mbs, bs = a->A->rmap->bs;
  PetscInt     *ai  = Aij->i, *bi = Bij->i;
  PetscInt     i,j,k;
  PetscScalar  *aa = Aij->a,*ba = Bij->a;

  PetscInt       bs2,  rbs, cbs, slen, blen;
  PetscErrorCode ierr;
  PetscScalar    **asp;
  PetscScalar    **bsp;

  PetscFunctionBegin;
  rbs  = cbs = bs;
  bs2  = bs*bs;
  blen = ai[MROW]-ai[0];
  slen = blen*bs;

  ierr        = PetscNewLog(a->A,&bstrmA);CHKERRQ(ierr);
  a->A->spptr = (void*) bstrmA;
  bstrmA      = (Mat_SeqBSTRM*) a->A->spptr;
  bstrmA->rbs = bstrmA->cbs = bs;
  ierr        = PetscMalloc1(bs2*blen, &bstrmA->as);CHKERRQ(ierr);

  ierr = PetscMalloc1(rbs, &asp);CHKERRQ(ierr);

  for (i=0; i<rbs; i++) asp[i] = bstrmA->as + i*slen;

  for (k=0; k<blen; k++) {
    for (j=0; j<cbs; j++) {
      for (i=0; i<rbs; i++) asp[i][k*cbs+j] = aa[k*bs2+j*rbs+i];
    }
  }

  switch (bs) {
  case 4:
    a->A->ops->mult = MatMult_SeqBSTRM_4;
    a->A->ops->sor  = MatSOR_SeqBSTRM_4;
    break;
  case 5:
    a->A->ops->mult = MatMult_SeqBSTRM_5;
    a->A->ops->sor  = MatSOR_SeqBSTRM_5;
    break;
  default:
    SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_SUP,"not supported for block size %D yet",bs);
  }
  ierr = PetscFree(asp);CHKERRQ(ierr);

/*.....*/
  blen = bi[MROW]-bi[0];
  slen = blen*bs;
  ierr = PetscNewLog(a->B,&bstrmB);CHKERRQ(ierr);

  a->B->spptr = (void*) bstrmB;
  bstrmB      = (Mat_SeqBSTRM*) a->B->spptr;
  bstrmB->rbs = bstrmB->cbs = bs;

  ierr = PetscMalloc1(bs2*blen, &bstrmB->as);CHKERRQ(ierr);
  ierr = PetscMalloc1(rbs, &bsp);CHKERRQ(ierr);

  for (i=0; i<rbs; i++) bsp[i] = bstrmB->as + i*slen;

  for (k=0; k<blen; k++) {
    for (j=0; j<cbs; j++) {
      for (i=0; i<rbs; i++) bsp[i][k*cbs+j] = ba[k*bs2+j*rbs+i];
    }
  }

  switch (bs) {
  case 4:
    a->B->ops->multadd = MatMultAdd_SeqBSTRM_4;
    break;
  case 5:
    a->B->ops->multadd = MatMultAdd_SeqBSTRM_5;
    break;
  default:
    SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_SUP,"not supported for block size %D yet",bs);
  }
  ierr = PetscFree(bsp);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

extern PetscErrorCode MatAssemblyEnd_MPIBAIJ(Mat,MatAssemblyType);

#undef __FUNCT__
#define __FUNCT__ "MatAssemblyEnd_MPIBSTRM"
PetscErrorCode MatAssemblyEnd_MPIBSTRM(Mat A, MatAssemblyType mode)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  /*
    Aij->inode.use = PETSC_FALSE;
    Bij->inode.use = PETSC_FALSE;
  */
  ierr = MatAssemblyEnd_MPIBAIJ(A,mode);CHKERRQ(ierr);
  if (mode == MAT_FLUSH_ASSEMBLY) PetscFunctionReturn(0);

  /* Now calculate the permutation and grouping information. */
  ierr = MatMPIBSTRM_create_bstrm(A);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "MatCreateMPIBSTRM"
PetscErrorCode MatCreateMPIBSTRM(MPI_Comm comm,PetscInt bs,PetscInt m,PetscInt n,PetscInt M,PetscInt N,PetscInt d_nz,const PetscInt d_nnz[],PetscInt o_nz,const PetscInt o_nnz[],Mat *A)
{
  PetscErrorCode ierr;
  PetscMPIInt    size;

  PetscFunctionBegin;
  ierr = MatCreate(comm,A);CHKERRQ(ierr);
  ierr = MatSetSizes(*A,m,n,M,N);CHKERRQ(ierr);
  ierr = MPI_Comm_size(comm,&size);CHKERRQ(ierr);
  if (size > 1) {
    ierr = MatSetType(*A,MATMPIBSTRM);CHKERRQ(ierr);
    ierr = MatMPIBAIJSetPreallocation(*A,bs,d_nz,d_nnz,o_nz,o_nnz);CHKERRQ(ierr);
  } else {
    ierr = MatSetType(*A,MATSEQBSTRM);CHKERRQ(ierr);
    ierr = MatSeqBAIJSetPreallocation(*A,bs,d_nz,d_nnz);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

PETSC_EXTERN PetscErrorCode MatConvert_SeqBAIJ_SeqBSTRM(Mat,MatType,MatReuse,Mat*);
extern PetscErrorCode MatMPIBAIJSetPreallocation_MPIBAIJ(Mat,PetscInt,PetscInt,const PetscInt *,PetscInt,const PetscInt *);

#undef __FUNCT__
#define __FUNCT__ "MatMPIBAIJSetPreallocation_MPIBSTRM"
PetscErrorCode MatMPIBAIJSetPreallocation_MPIBSTRM(Mat B,PetscInt bs,PetscInt d_nz,const PetscInt d_nnz[],PetscInt o_nz,const PetscInt o_nnz[])
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MatMPIBAIJSetPreallocation_MPIBAIJ(B,bs,d_nz,d_nnz,o_nz,o_nnz);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatConvert_MPIBAIJ_MPIBSTRM"
PETSC_EXTERN PetscErrorCode MatConvert_MPIBAIJ_MPIBSTRM(Mat A,MatType type,MatReuse reuse,Mat *newmat)
{
  PetscErrorCode ierr;
  Mat            B = *newmat;
  Mat_SeqBSTRM   *bstrm;

  PetscFunctionBegin;
  if (reuse == MAT_INITIAL_MATRIX) {
    ierr = MatDuplicate(A,MAT_COPY_VALUES,&B);CHKERRQ(ierr);
  }

  ierr     = PetscNewLog(B,&bstrm);CHKERRQ(ierr);
  B->spptr = (void*) bstrm;

  /* Set function pointers for methods that we inherit from AIJ but override.
     B->ops->duplicate   = MatDuplicate_BSTRM;
     B->ops->mult        = MatMult_BSTRM;
     B->ops->destroy     = MatDestroy_MPIBSTRM;
  */
  B->ops->assemblyend = MatAssemblyEnd_MPIBSTRM;

  /* If A has already been assembled, compute the permutation. */
  if (A->assembled) {
    ierr = MatMPIBSTRM_create_bstrm(B);CHKERRQ(ierr);
  }

  ierr = PetscObjectChangeTypeName((PetscObject) B, MATMPIBSTRM);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatMPIBAIJSetPreallocation_C",MatMPIBAIJSetPreallocation_MPIBSTRM);CHKERRQ(ierr);
  *newmat = B;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatCreate_MPIBSTRM"
PETSC_EXTERN PetscErrorCode MatCreate_MPIBSTRM(Mat A)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MatSetType(A,MATMPIBAIJ);CHKERRQ(ierr);
  ierr = MatConvert_MPIBAIJ_MPIBSTRM(A,MATMPIBSTRM,MAT_INPLACE_MATRIX,&A);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatCreate_BSTRM"
PETSC_EXTERN PetscErrorCode MatCreate_BSTRM(Mat A)
{
  PetscErrorCode ierr;
  PetscMPIInt    size;

  PetscFunctionBegin;
  ierr = MPI_Comm_size(PetscObjectComm((PetscObject)A),&size);CHKERRQ(ierr);
  if (size == 1) {
    ierr = MatSetType(A,MATSEQBSTRM);CHKERRQ(ierr);
  } else {
    ierr = MatSetType(A,MATMPIBSTRM);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

