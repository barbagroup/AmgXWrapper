
#include <petsc/private/matimpl.h>

#undef __FUNCT__
#define __FUNCT__ "MatConvert_Basic"
/*
  MatConvert_Basic - Converts from any input format to another format. For
  parallel formats, the new matrix distribution is determined by PETSc.

  Does not do preallocation so in general will be slow
 */
PetscErrorCode MatConvert_Basic(Mat mat, MatType newtype,MatReuse reuse,Mat *newmat)
{
  Mat               M;
  const PetscScalar *vwork;
  PetscErrorCode    ierr;
  PetscInt          nz,i,m,n,rstart,rend,lm,ln;
  const PetscInt    *cwork;
  PetscBool         isSBAIJ;

  PetscFunctionBegin;
  ierr = PetscObjectTypeCompare((PetscObject)mat,MATSEQSBAIJ,&isSBAIJ);CHKERRQ(ierr);
  if (!isSBAIJ) {
    ierr = PetscObjectTypeCompare((PetscObject)mat,MATMPISBAIJ,&isSBAIJ);CHKERRQ(ierr);
  }
  if (isSBAIJ) SETERRQ(PetscObjectComm((PetscObject)mat),PETSC_ERR_SUP,"Cannot convert from SBAIJ matrix since cannot obtain entire rows of matrix");


  ierr = MatGetSize(mat,&m,&n);CHKERRQ(ierr);
  ierr = MatGetLocalSize(mat,&lm,&ln);CHKERRQ(ierr);

  if (ln == n) ln = PETSC_DECIDE; /* try to preserve column ownership */

  ierr = MatCreate(PetscObjectComm((PetscObject)mat),&M);CHKERRQ(ierr);
  ierr = MatSetSizes(M,lm,ln,m,n);CHKERRQ(ierr);
  ierr = MatSetBlockSizesFromMats(M,mat,mat);CHKERRQ(ierr);
  ierr = MatSetType(M,newtype);CHKERRQ(ierr);

  ierr = MatSeqDenseSetPreallocation(M,NULL);CHKERRQ(ierr);
  ierr = MatMPIDenseSetPreallocation(M,NULL);CHKERRQ(ierr);
  ierr = MatSetUp(M);CHKERRQ(ierr);
  ierr = MatSetOption(M,MAT_NEW_NONZERO_LOCATION_ERR,PETSC_FALSE);CHKERRQ(ierr);
  ierr = MatSetOption(M,MAT_NEW_NONZERO_ALLOCATION_ERR,PETSC_FALSE);CHKERRQ(ierr);

    ierr = PetscObjectTypeCompare((PetscObject)M,MATSEQSBAIJ,&isSBAIJ);CHKERRQ(ierr);
  if (!isSBAIJ) {
    ierr = PetscObjectTypeCompare((PetscObject)M,MATMPISBAIJ,&isSBAIJ);CHKERRQ(ierr);
  }
  if (isSBAIJ) {
    ierr = MatSetOption(M,MAT_IGNORE_LOWER_TRIANGULAR,PETSC_TRUE);CHKERRQ(ierr);
  }

  ierr = MatGetOwnershipRange(mat,&rstart,&rend);CHKERRQ(ierr);
  for (i=rstart; i<rend; i++) {
    ierr = MatGetRow(mat,i,&nz,&cwork,&vwork);CHKERRQ(ierr);
    ierr = MatSetValues(M,1,&i,nz,cwork,vwork,INSERT_VALUES);CHKERRQ(ierr);
    ierr = MatRestoreRow(mat,i,&nz,&cwork,&vwork);CHKERRQ(ierr);
  }
  ierr = MatAssemblyBegin(M,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(M,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);

  if (reuse == MAT_INPLACE_MATRIX) {
    ierr = MatHeaderReplace(mat,&M);CHKERRQ(ierr);
  } else {
    *newmat = M;
  }
  PetscFunctionReturn(0);
}
