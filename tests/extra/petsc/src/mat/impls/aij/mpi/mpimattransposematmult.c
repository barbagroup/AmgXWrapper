
/*
  Defines matrix-matrix product routines for pairs of MPIAIJ matrices
          C = A^T * B
  The routines are slightly modified from MatTransposeMatMultxxx_SeqAIJ_SeqDense(). 
*/
#include <../src/mat/impls/aij/seq/aij.h> /*I "petscmat.h" I*/
#include <../src/mat/impls/aij/mpi/mpiaij.h>
#include <../src/mat/impls/dense/mpi/mpidense.h>

#undef __FUNCT__
#define __FUNCT__ "MatDestroy_MPIDense_MatTransMatMult"
PetscErrorCode MatDestroy_MPIDense_MatTransMatMult(Mat A)
{
  PetscErrorCode      ierr;
  Mat_MPIDense        *a = (Mat_MPIDense*)A->data;
  Mat_MatTransMatMult *atb = a->atb;

  PetscFunctionBegin;
  ierr = MatDestroy(&atb->mA);CHKERRQ(ierr);
  ierr = VecDestroy(&atb->bt);CHKERRQ(ierr);
  ierr = VecDestroy(&atb->ct);CHKERRQ(ierr);
  ierr = (atb->destroy)(A);CHKERRQ(ierr);
  ierr = PetscFree(atb);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatTransposeMatMult_MPIAIJ_MPIDense"
PetscErrorCode MatTransposeMatMult_MPIAIJ_MPIDense(Mat A,Mat B,MatReuse scall,PetscReal fill,Mat *C)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (scall == MAT_INITIAL_MATRIX) {
    ierr = PetscLogEventBegin(MAT_TransposeMatMultSymbolic,A,B,0,0);CHKERRQ(ierr);
    ierr = MatTransposeMatMultSymbolic_MPIAIJ_MPIDense(A,B,fill,C);CHKERRQ(ierr);
    ierr = PetscLogEventEnd(MAT_TransposeMatMultSymbolic,A,B,0,0);CHKERRQ(ierr);
  } 
  ierr = PetscLogEventBegin(MAT_TransposeMatMultNumeric,A,B,0,0);CHKERRQ(ierr);
  ierr = MatTransposeMatMultNumeric_MPIAIJ_MPIDense(A,B,*C);CHKERRQ(ierr);
  ierr = PetscLogEventEnd(MAT_TransposeMatMultNumeric,A,B,0,0);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatTransposeMatMultSymbolic_MPIAIJ_MPIDense"
PetscErrorCode MatTransposeMatMultSymbolic_MPIAIJ_MPIDense(Mat A,Mat B,PetscReal fill,Mat *C)
{
  PetscErrorCode      ierr;
  PetscInt            m=A->rmap->n,n=A->cmap->n,BN=B->cmap->N;
  Mat_MatTransMatMult *atb;
  Mat                 Cdense;
  Vec                 bt,ct;
  Mat_MPIDense        *c;
  
  PetscFunctionBegin;
  ierr = PetscNew(&atb);CHKERRQ(ierr);

  /* create output dense matrix C = A^T*B */
  ierr = MatCreate(PetscObjectComm((PetscObject)A),&Cdense);CHKERRQ(ierr);
  ierr = MatSetSizes(Cdense,n,PETSC_DECIDE,PETSC_DECIDE,BN);CHKERRQ(ierr);
  ierr = MatSetType(Cdense,MATMPIDENSE);CHKERRQ(ierr);
  ierr = MatMPIDenseSetPreallocation(Cdense,NULL);CHKERRQ(ierr);

  /* create vectors bt and ct to hold locally transposed arrays of B and C */
  ierr = VecCreate(PetscObjectComm((PetscObject)A),&bt);CHKERRQ(ierr);
  ierr = VecSetSizes(bt,m*BN,PETSC_DECIDE);CHKERRQ(ierr);
  ierr = VecSetType(bt,VECSTANDARD);CHKERRQ(ierr);
  ierr = VecCreate(PetscObjectComm((PetscObject)A),&ct);CHKERRQ(ierr);
  ierr = VecSetSizes(ct,n*BN,PETSC_DECIDE);CHKERRQ(ierr);
  ierr = VecSetType(ct,VECSTANDARD);CHKERRQ(ierr);
  atb->bt = bt;
  atb->ct = ct;

  *C = Cdense;
  c                    = (Mat_MPIDense*)Cdense->data;
  c->atb               = atb;
  atb->destroy         = Cdense->ops->destroy;
  Cdense->ops->destroy = MatDestroy_MPIDense_MatTransMatMult;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatTransposeMatMultNumeric_MPIAIJ_MPIDense"
PetscErrorCode MatTransposeMatMultNumeric_MPIAIJ_MPIDense(Mat A,Mat B,Mat C)
{
  PetscErrorCode      ierr;
  PetscInt            i,j,k,m=A->rmap->n,n=A->cmap->n,BN=B->cmap->N;
  PetscScalar         *Barray,*Carray,*btarray,*ctarray;
  Mat_MPIDense        *c=(Mat_MPIDense*)C->data;
  Mat_MatTransMatMult *atb=c->atb;
  Vec                 bt=atb->bt,ct=atb->ct;

  PetscFunctionBegin;
  /* create MAIJ matrix mA from A -- should be done in symbolic phase */
  ierr = MatDestroy(&atb->mA);CHKERRQ(ierr);
  ierr = MatCreateMAIJ(A,BN,&atb->mA);CHKERRQ(ierr);

  /* transpose local arry of B, then copy it to vector bt */
  ierr = MatDenseGetArray(B,&Barray);CHKERRQ(ierr);
  ierr = VecGetArray(bt,&btarray);CHKERRQ(ierr);

  k=0;
  for (j=0; j<BN; j++) {
    for (i=0; i<m; i++) btarray[i*BN + j] = Barray[k++]; 
  }
  ierr = VecRestoreArray(bt,&btarray);CHKERRQ(ierr);
  ierr = MatDenseRestoreArray(B,&Barray);CHKERRQ(ierr);

  /* compute ct = mA^T * cb */
  ierr = MatMultTranspose(atb->mA,bt,ct);CHKERRQ(ierr);

  /* transpose local arry of ct to matrix C */
  ierr = MatDenseGetArray(C,&Carray);CHKERRQ(ierr);
  ierr = VecGetArray(ct,&ctarray);CHKERRQ(ierr);
  k = 0;
  for (j=0; j<BN; j++) {
    for (i=0; i<n; i++) Carray[k++] = ctarray[i*BN + j];
  }
  ierr = VecRestoreArray(ct,&ctarray);CHKERRQ(ierr);
  ierr = MatDenseRestoreArray(C,&Carray);CHKERRQ(ierr);
  ierr = MatAssemblyBegin(C,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(C,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
