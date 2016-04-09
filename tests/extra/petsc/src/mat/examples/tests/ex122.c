static char help[] = "Test MatMatMult() for AIJ and Dense matrices.\n\n";

#include <petscmat.h>

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **argv)
{
  Mat            A,B,C,D;
  PetscInt       i,M=10,N=5;
  PetscErrorCode ierr;
  PetscRandom    r;
  PetscBool      equal=PETSC_FALSE;
  PetscReal      fill = 1.0;
  PetscInt       nza,am,an;

  PetscInitialize(&argc,&argv,(char*)0,help);
  ierr = PetscOptionsGetInt(NULL,NULL,"-M",&M,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetInt(NULL,NULL,"-N",&N,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetReal(NULL,NULL,"-fill",&fill,NULL);CHKERRQ(ierr);

  ierr = PetscRandomCreate(PETSC_COMM_WORLD,&r);CHKERRQ(ierr);
  ierr = PetscRandomSetFromOptions(r);CHKERRQ(ierr);

  /* create a aij matrix A */
  ierr = MatCreate(PETSC_COMM_WORLD,&A);CHKERRQ(ierr);
  ierr = MatSetSizes(A,PETSC_DECIDE,PETSC_DECIDE,N,M);CHKERRQ(ierr);
  ierr = MatSetType(A,MATAIJ);CHKERRQ(ierr);
  nza  = (PetscInt)(.3*M); /* num of nozeros in each row of A */
  ierr = MatSeqAIJSetPreallocation(A,nza,NULL);CHKERRQ(ierr);
  ierr = MatMPIAIJSetPreallocation(A,nza,NULL,nza,NULL);CHKERRQ(ierr);
  ierr = MatSetRandom(A,r);CHKERRQ(ierr);

  /* create a dense matrix B */
  ierr = MatGetLocalSize(A,&am,&an);CHKERRQ(ierr);
  ierr = MatCreate(PETSC_COMM_WORLD,&B);CHKERRQ(ierr);
  ierr = MatSetSizes(B,PETSC_DECIDE,am,N,PETSC_DECIDE);CHKERRQ(ierr);
  ierr = MatSetType(B,MATDENSE);CHKERRQ(ierr);
  ierr = MatSeqDenseSetPreallocation(B,NULL);CHKERRQ(ierr);
  ierr = MatMPIDenseSetPreallocation(B,NULL);CHKERRQ(ierr);
  ierr = MatSetRandom(B,r);CHKERRQ(ierr);
  ierr = PetscRandomDestroy(&r);CHKERRQ(ierr);
  ierr = MatAssemblyBegin(B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);


  /* Test MatMatMult() */
  ierr = MatMatMult(B,A,MAT_INITIAL_MATRIX,fill,&C);CHKERRQ(ierr);

  ierr = MatMatMultSymbolic(B,A,fill,&D);CHKERRQ(ierr);
  for (i=0; i<2; i++) {
    ierr = MatMatMultNumeric(B,A,D);CHKERRQ(ierr);
  }
  ierr = MatEqual(C,D,&equal);CHKERRQ(ierr);
  if (!equal) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONGSTATE,"C != D");

  ierr = MatDestroy(&D);CHKERRQ(ierr);
  ierr = MatDestroy(&C);CHKERRQ(ierr);
  ierr = MatDestroy(&B);CHKERRQ(ierr);
  ierr = MatDestroy(&A);CHKERRQ(ierr);
  PetscFinalize();
  return(0);
}
