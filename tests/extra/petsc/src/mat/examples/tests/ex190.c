static char help[] = "Tests MatLoad() with uneven dimensions set in program\n\n";

#include <petscmat.h>

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **args)
{
  Mat            A;
  PetscViewer    fd;
  char           file[PETSC_MAX_PATH_LEN];
  PetscErrorCode ierr;
  PetscBool      flg;
  PetscMPIInt    rank;

  PetscInitialize(&argc,&args,(char*)0,help);
  ierr = MPI_Comm_rank(PETSC_COMM_WORLD,&rank);CHKERRQ(ierr);

  /* Determine files from which we read the matrix */
  ierr = PetscOptionsGetString(NULL,NULL,"-f",file,PETSC_MAX_PATH_LEN,&flg);CHKERRQ(ierr);
  if (!flg) SETERRQ(PETSC_COMM_WORLD,PETSC_ERR_USER,"Must indicate binary file with the -f");

  /* Load matrices */
  ierr = PetscViewerBinaryOpen(PETSC_COMM_WORLD,file,FILE_MODE_READ,&fd);CHKERRQ(ierr);
  ierr = MatCreate(PETSC_COMM_WORLD,&A);CHKERRQ(ierr);
  ierr = MatSetFromOptions(A);CHKERRQ(ierr);
  ierr = MatSetBlockSize(A,2);CHKERRQ(ierr);
  if (!rank) {
    ierr = MatSetSizes(A, 4, PETSC_DETERMINE, PETSC_DETERMINE,PETSC_DETERMINE);CHKERRQ(ierr);
  } else {
    ierr = MatSetSizes(A, 8, PETSC_DETERMINE, PETSC_DETERMINE,PETSC_DETERMINE);CHKERRQ(ierr);
  }
  ierr = MatLoad(A,fd);CHKERRQ(ierr);
  ierr = PetscViewerDestroy(&fd);CHKERRQ(ierr);
  ierr = MatDestroy(&A);CHKERRQ(ierr);
  ierr = PetscFinalize();
  return 0;
}
