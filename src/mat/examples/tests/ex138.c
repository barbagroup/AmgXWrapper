
static char help[] = "Tests MatGetColumnNorms() for matrix read from file.";

#include <petscmat.h>

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **args)
{
  Mat            A;
  PetscErrorCode ierr;
  PetscReal      *norms;
  char           file[PETSC_MAX_PATH_LEN];
  PetscBool      flg;
  PetscViewer    fd;
  PetscInt       n;
  PetscMPIInt    rank;

  PetscInitialize(&argc,&args,(char*)0,help);
  ierr = MPI_Comm_rank(PETSC_COMM_WORLD,&rank);CHKERRQ(ierr);
  ierr = PetscOptionsGetString(NULL,NULL,"-f",file,PETSC_MAX_PATH_LEN,&flg);CHKERRQ(ierr);
  if (!flg) SETERRQ(PETSC_COMM_WORLD,PETSC_ERR_USER,"Must indicate binary file with the -f option");
  ierr = PetscViewerBinaryOpen(PETSC_COMM_WORLD,file,FILE_MODE_READ,&fd);CHKERRQ(ierr);
  ierr = MatCreate(PETSC_COMM_WORLD,&A);CHKERRQ(ierr);
  ierr = MatSetFromOptions(A);CHKERRQ(ierr);
  ierr = MatLoad(A,fd);CHKERRQ(ierr);
  ierr = PetscViewerDestroy(&fd);CHKERRQ(ierr);

  ierr = MatGetSize(A,NULL,&n);CHKERRQ(ierr);
  ierr = PetscMalloc1(n,&norms);CHKERRQ(ierr);

  ierr = MatGetColumnNorms(A,NORM_2,norms);CHKERRQ(ierr);
  if (!rank) {
    ierr = PetscPrintf(PETSC_COMM_SELF,"NORM_2:\n");CHKERRQ(ierr);
    ierr = PetscRealView(n,norms,PETSC_VIEWER_STDOUT_SELF);CHKERRQ(ierr);
  }

  ierr = MatGetColumnNorms(A,NORM_1,norms);CHKERRQ(ierr);
  if (!rank) {
    ierr = PetscPrintf(PETSC_COMM_SELF,"NORM_1:\n");CHKERRQ(ierr);
    ierr = PetscRealView(n,norms,PETSC_VIEWER_STDOUT_SELF);CHKERRQ(ierr);
  }

  ierr = MatGetColumnNorms(A,NORM_INFINITY,norms);CHKERRQ(ierr);
  if (!rank) {
    ierr = PetscPrintf(PETSC_COMM_SELF,"NORM_INFINITY:\n");CHKERRQ(ierr);
    ierr = PetscRealView(n,norms,PETSC_VIEWER_STDOUT_SELF);CHKERRQ(ierr);
  }

  ierr = PetscFree(norms);CHKERRQ(ierr);
  ierr = MatDestroy(&A);CHKERRQ(ierr);
  ierr = PetscFinalize();
  return 0;
}
