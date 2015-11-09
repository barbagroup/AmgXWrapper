
static char help[] = "Tests external direct solvers. Simplified from ex125.c\n\
Example: mpiexec -n <np> ./ex130 -f <matrix binary file> -mat_solver_package 1 -mat_superlu_equil \n\n";

#include <petscmat.h>

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **args)
{
  Mat            A,F;
  Vec            u,x,b;
  PetscErrorCode ierr;
  PetscMPIInt    rank,size;
  PetscInt       m,n,nfact,ipack=0;
  PetscReal      norm,tol=1.e-12,Anorm;
  IS             perm,iperm;
  MatFactorInfo  info;
  PetscBool      flg,testMatSolve=PETSC_TRUE;
  PetscViewer    fd;              /* viewer */
  char           file[PETSC_MAX_PATH_LEN]; /* input file name */

  PetscInitialize(&argc,&args,(char*)0,help);
  ierr = MPI_Comm_rank(PETSC_COMM_WORLD, &rank);CHKERRQ(ierr);
  ierr = MPI_Comm_size(PETSC_COMM_WORLD, &size);CHKERRQ(ierr);

  /* Determine file from which we read the matrix A */
  ierr = PetscOptionsGetString(NULL,NULL,"-f",file,PETSC_MAX_PATH_LEN,&flg);CHKERRQ(ierr);
  if (!flg) SETERRQ(PETSC_COMM_WORLD,1,"Must indicate binary file with the -f option");

  /* Load matrix A */
  ierr = PetscViewerBinaryOpen(PETSC_COMM_WORLD,file,FILE_MODE_READ,&fd);CHKERRQ(ierr);
  ierr = MatCreate(PETSC_COMM_WORLD,&A);CHKERRQ(ierr);
  ierr = MatLoad(A,fd);CHKERRQ(ierr);
  ierr = VecCreate(PETSC_COMM_WORLD,&b);CHKERRQ(ierr);
  ierr = VecLoad(b,fd);CHKERRQ(ierr);
  ierr = PetscViewerDestroy(&fd);CHKERRQ(ierr);
  ierr = MatGetLocalSize(A,&m,&n);CHKERRQ(ierr);
  if (m != n) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_SIZ, "This example is not intended for rectangular matrices (%d, %d)", m, n);
  ierr = MatNorm(A,NORM_INFINITY,&Anorm);CHKERRQ(ierr);

  /* Create vectors */
  ierr = VecDuplicate(b,&x);CHKERRQ(ierr);
  ierr = VecDuplicate(x,&u);CHKERRQ(ierr); /* save the true solution */

  /* Test LU Factorization */
  ierr = MatGetOrdering(A,MATORDERINGNATURAL,&perm,&iperm);CHKERRQ(ierr);

  ierr = PetscOptionsGetInt(NULL,NULL,"-mat_solver_package",&ipack,NULL);CHKERRQ(ierr);
  switch (ipack) {
  case 1:
#if defined(PETSC_HAVE_SUPERLU)
    if (!rank) printf(" SUPERLU LU:\n");
    ierr = MatGetFactor(A,MATSOLVERSUPERLU,MAT_FACTOR_LU,&F);CHKERRQ(ierr);
    break;
#endif
  case 2:
#if defined(PETSC_HAVE_MUMPS)
    if (!rank) printf(" MUMPS LU:\n");
    ierr = MatGetFactor(A,MATSOLVERMUMPS,MAT_FACTOR_LU,&F);CHKERRQ(ierr);
    {
      /* test mumps options */
      PetscInt icntl_7 = 5;
      ierr = MatMumpsSetIcntl(F,7,icntl_7);CHKERRQ(ierr);
    }
    break;
#endif
  default:
    if (!rank) printf(" PETSC LU:\n");
    ierr = MatGetFactor(A,MATSOLVERPETSC,MAT_FACTOR_LU,&F);CHKERRQ(ierr);
  }

  info.fill = 5.0;
  ierr      = MatLUFactorSymbolic(F,A,perm,iperm,&info);CHKERRQ(ierr);

  for (nfact = 0; nfact < 1; nfact++) {
    if (!rank) printf(" %d-the LU numfactorization \n",nfact);
    ierr = MatLUFactorNumeric(F,A,&info);CHKERRQ(ierr);

    /* Test MatSolve() */
    if (testMatSolve) {
      ierr = MatSolve(F,b,x);CHKERRQ(ierr);

      /* Check the residual */
      ierr = MatMult(A,x,u);CHKERRQ(ierr);
      ierr = VecAXPY(u,-1.0,b);CHKERRQ(ierr);
      ierr = VecNorm(u,NORM_INFINITY,&norm);CHKERRQ(ierr);
      if (norm > tol) {
        if (!rank) {
          ierr = PetscPrintf(PETSC_COMM_SELF,"MatSolve: rel residual %g/%g = %g, LU numfact %d\n",norm,Anorm,norm/Anorm,nfact);CHKERRQ(ierr);
        }
      }
    }
  }

  /* Free data structures */
  ierr = MatDestroy(&A);CHKERRQ(ierr);
  ierr = MatDestroy(&F);CHKERRQ(ierr);
  ierr = ISDestroy(&perm);CHKERRQ(ierr);
  ierr = ISDestroy(&iperm);CHKERRQ(ierr);
  ierr = VecDestroy(&x);CHKERRQ(ierr);
  ierr = VecDestroy(&b);CHKERRQ(ierr);
  ierr = VecDestroy(&u);CHKERRQ(ierr);
  ierr = PetscFinalize();
  return 0;
}
