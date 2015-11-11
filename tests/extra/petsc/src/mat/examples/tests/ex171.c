
static char help[] = "Tests MatDiagonalSet() on MatLoad() matrix \n\n";

#include <petscmat.h>

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **args)
{
  Mat            A;
  Vec            x;
  PetscErrorCode ierr;
  PetscViewer    fd;              /* viewer */
  char           file[PETSC_MAX_PATH_LEN]; /* input file name */
  PetscReal      norm;
  PetscBool      flg;

  PetscInitialize(&argc,&args,(char*)0,help);

  /* Determine file from which we read the matrix A */
  ierr = PetscOptionsGetString(NULL,NULL,"-f",file,PETSC_MAX_PATH_LEN,&flg);CHKERRQ(ierr);
  if (!flg) SETERRQ(PETSC_COMM_WORLD,1,"Must indicate binary file with the -f option");

  /* Load matrix A */
  ierr = PetscViewerBinaryOpen(PETSC_COMM_WORLD,file,FILE_MODE_READ,&fd);CHKERRQ(ierr);
  ierr = MatCreate(PETSC_COMM_WORLD,&A);CHKERRQ(ierr);
  ierr = MatLoad(A,fd);CHKERRQ(ierr);
  ierr = PetscViewerDestroy(&fd);CHKERRQ(ierr);
  ierr = MatCreateVecs(A,&x,NULL);CHKERRQ(ierr);
  ierr = MatGetDiagonal(A,x);CHKERRQ(ierr);
  ierr = VecScale(x,-1.0);CHKERRQ(ierr);
  ierr = MatDiagonalSet(A,x,ADD_VALUES);CHKERRQ(ierr);
  ierr = MatGetDiagonal(A,x);CHKERRQ(ierr);
  ierr = VecNorm(x,NORM_2,&norm);CHKERRQ(ierr);
  ierr = PetscPrintf(PETSC_COMM_WORLD,"Norm %g\n",(double)norm);CHKERRQ(ierr);

  /* Free data structures */
  ierr = MatDestroy(&A);CHKERRQ(ierr);
  ierr = VecDestroy(&x);CHKERRQ(ierr);
  ierr = PetscFinalize();
  return 0;
}
