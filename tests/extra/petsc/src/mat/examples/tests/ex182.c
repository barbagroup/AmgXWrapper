
static char help[] = "Tests using MatShift() to create a constant diagonal matrix\n\n";

#include <petscmat.h>

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **argv)
{
  Mat            A,F;
  MatFactorInfo  info;
  PetscErrorCode ierr;
  PetscInt       m = 10;
  IS             perm;
  PetscMPIInt    size;
  PetscBool      issbaij;

  PetscInitialize(&argc,&argv,(char*) 0,help);
  ierr = MPI_Comm_size(PETSC_COMM_WORLD,&size);CHKERRQ(ierr);

  ierr = MatCreate(PETSC_COMM_WORLD,&A);CHKERRQ(ierr);
  ierr = MatSetSizes(A,PETSC_DECIDE,PETSC_DECIDE,m,m);CHKERRQ(ierr);
  ierr = MatSetFromOptions(A);CHKERRQ(ierr);
  ierr = MatSetUp(A);CHKERRQ(ierr);
  ierr = MatAssemblyBegin(A,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(A,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);

  ierr = MatShift(A,1.0);CHKERRQ(ierr);

  ierr = PetscObjectTypeCompare((PetscObject)A,MATSEQSBAIJ,&issbaij);CHKERRQ(ierr);
  if (size == 1 && !issbaij) {
    ierr = MatGetFactor(A,MATSOLVERPETSC,MAT_FACTOR_LU,&F);CHKERRQ(ierr);
    ierr = MatFactorInfoInitialize(&info);CHKERRQ(ierr);
    ierr = ISCreateStride(PETSC_COMM_SELF,m,0,1,&perm);CHKERRQ(ierr);
    ierr = MatLUFactorSymbolic(F,A,perm,perm,&info);CHKERRQ(ierr);
    ierr = MatLUFactorNumeric(F,A,&info);CHKERRQ(ierr);
    ierr = MatDestroy(&F);CHKERRQ(ierr);
    ierr = ISDestroy(&perm);CHKERRQ(ierr);
  }
  ierr = MatDestroy(&A);CHKERRQ(ierr);
  ierr = PetscFinalize();
  return 0;
}

