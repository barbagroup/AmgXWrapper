static char help[] = "Test MatMatMult() and MatPtAP() for AIJ matrices.\n\n";

#include <petscmat.h>

extern PetscErrorCode testPTAPRectangular(void);

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **argv)
{
  Mat            A,B,C,D;
  PetscScalar    a[] ={1.,1.,0.,0.,1.,1.,0.,0.,1.};
  PetscInt       ij[]={0,1,2};
  PetscScalar    none=-1.;
  PetscErrorCode ierr;
  PetscReal      fill=4;
  PetscReal      norm;
  PetscMPIInt    size,rank;

  PetscInitialize(&argc,&argv,(char*)0,help);
  ierr = MPI_Comm_size(PETSC_COMM_WORLD,&size);CHKERRQ(ierr);
  ierr = MPI_Comm_rank(PETSC_COMM_WORLD,&rank);CHKERRQ(ierr);

  ierr = MatCreate(PETSC_COMM_WORLD,&A);CHKERRQ(ierr);
  ierr = MatSetSizes(A,PETSC_DECIDE,PETSC_DECIDE,3,3);CHKERRQ(ierr);
  ierr = MatSetType(A,MATAIJ);CHKERRQ(ierr);
  ierr = MatSetFromOptions(A);CHKERRQ(ierr);
  ierr = MatSetUp(A);CHKERRQ(ierr);
  ierr = MatSetOption(A,MAT_IGNORE_ZERO_ENTRIES,PETSC_TRUE);CHKERRQ(ierr);

  if (!rank) {
    ierr = MatSetValues(A,3,ij,3,ij,a,ADD_VALUES);CHKERRQ(ierr);
  }
  ierr = MatAssemblyBegin(A,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(A,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatSetOptionsPrefix(A,"A_");CHKERRQ(ierr);
  ierr = MatView(A,PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);
  ierr = PetscPrintf(PETSC_COMM_WORLD,"\n");CHKERRQ(ierr);

  /* Test MatMatMult() */
  ierr = MatTranspose(A,MAT_INITIAL_MATRIX,&B);CHKERRQ(ierr);      /* B = A^T */
  ierr = MatMatMult(B,A,MAT_INITIAL_MATRIX,fill,&C);CHKERRQ(ierr); /* C = B*A */
  ierr = MatMatMultNumeric(B,A,C);CHKERRQ(ierr);                   /* recompute C=B*A */
  ierr = MatMatMult(B,A,MAT_REUSE_MATRIX,fill,&C);CHKERRQ(ierr);   /* recompute C=B*A */
  ierr = MatSetOptionsPrefix(C,"C_");CHKERRQ(ierr);
  ierr = MatView(C,PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);
  if (!rank) {ierr = PetscPrintf(PETSC_COMM_SELF,"\n");CHKERRQ(ierr);}

  ierr = MatMatMultSymbolic(C,A,fill,&D);CHKERRQ(ierr);
  ierr = MatMatMultNumeric(C,A,D);CHKERRQ(ierr);  /* D = C*A = (A^T*A)*A */
  ierr = MatSetOptionsPrefix(D,"D_");CHKERRQ(ierr);
  ierr = MatView(D,PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);
  if (!rank) {ierr = PetscPrintf(PETSC_COMM_SELF,"\n");CHKERRQ(ierr);}

  /* Repeat the numeric product to test reuse of the previous symbolic product */
  ierr = MatMatMultNumeric(C,A,D);CHKERRQ(ierr);
  ierr = MatView(D,PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);
  if (!rank) {ierr = PetscPrintf(PETSC_COMM_SELF,"\n");CHKERRQ(ierr);}

  ierr = MatDestroy(&B);CHKERRQ(ierr);
  ierr = MatDestroy(&C);CHKERRQ(ierr);

  /* Test PtAP routine. */
  ierr = MatDuplicate(A,MAT_COPY_VALUES,&B);CHKERRQ(ierr);      /* B = A */
  ierr = MatPtAP(A,B,MAT_INITIAL_MATRIX,fill,&C);CHKERRQ(ierr); /* C = B^T*A*B */
  ierr = MatAXPY(D,none,C,DIFFERENT_NONZERO_PATTERN);CHKERRQ(ierr);
  ierr = MatNorm(D,NORM_FROBENIUS,&norm);
  if (norm > 1.e-15 && !rank) {
    ierr = PetscPrintf(PETSC_COMM_SELF,"Error in MatPtAP: %g\n",norm);
  }
  ierr = MatDestroy(&C);CHKERRQ(ierr);
  ierr = MatDestroy(&D);CHKERRQ(ierr);

  /* Repeat PtAP to test symbolic/numeric separation for reuse of the symbolic product */
  ierr = MatPtAP(A,B,MAT_INITIAL_MATRIX,fill,&C);CHKERRQ(ierr);
  ierr = MatSetOptionsPrefix(C,"C=BtAB_");CHKERRQ(ierr);
  ierr = MatView(C,PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);
  ierr = PetscPrintf(PETSC_COMM_WORLD,"\n");CHKERRQ(ierr);

  ierr = MatPtAPSymbolic(A,B,fill,&D);CHKERRQ(ierr);
  ierr = MatPtAPNumeric(A,B,D);CHKERRQ(ierr);
  ierr = MatSetOptionsPrefix(D,"D=BtAB_");CHKERRQ(ierr);
  ierr = MatView(D,PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);
  ierr = PetscPrintf(PETSC_COMM_WORLD,"\n");CHKERRQ(ierr);

  /* Repeat numeric product to test reuse of the previous symbolic product */
  ierr = MatPtAPNumeric(A,B,D);CHKERRQ(ierr);
  ierr = MatAXPY(D,none,C,DIFFERENT_NONZERO_PATTERN);CHKERRQ(ierr);
  ierr = MatNorm(D,NORM_FROBENIUS,&norm);CHKERRQ(ierr);
  if (norm > 1.e-15) {
    ierr = PetscPrintf(PETSC_COMM_WORLD,"Error in symbolic/numeric MatPtAP: %g\n",norm);
  }
  ierr = MatDestroy(&B);CHKERRQ(ierr);
  ierr = MatDestroy(&C);CHKERRQ(ierr);
  ierr = MatDestroy(&D);CHKERRQ(ierr);

  if (size == 1) {
    /* A test contributed by Tobias Neckel <neckel@in.tum.de> */
    ierr = testPTAPRectangular();CHKERRQ(ierr);

    /* test MatMatTransposeMult(): A*B^T */
    ierr = MatMatTransposeMult(A,A,MAT_INITIAL_MATRIX,fill,&D);CHKERRQ(ierr); /* D = A*A^T */
    ierr = MatSetOptionsPrefix(D,"D=A*A^T_");CHKERRQ(ierr);
    ierr = MatView(D,PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);
    ierr = PetscPrintf(PETSC_COMM_WORLD,"\n");CHKERRQ(ierr);

    ierr = MatTranspose(A,MAT_INITIAL_MATRIX,&B);CHKERRQ(ierr); /* B = A^T */
    ierr = MatMatMult(A,B,MAT_INITIAL_MATRIX,fill,&C);CHKERRQ(ierr); /* C=A*B */
    ierr = MatSetOptionsPrefix(C,"D=A*B=A*A^T_");CHKERRQ(ierr);
    ierr = MatView(C,PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);
    ierr = PetscPrintf(PETSC_COMM_WORLD,"\n");CHKERRQ(ierr);
  }

  ierr = MatDestroy(&A);CHKERRQ(ierr);
  ierr = MatDestroy(&B);CHKERRQ(ierr);
  ierr = MatDestroy(&C);CHKERRQ(ierr);
  ierr = MatDestroy(&D);CHKERRQ(ierr);
  PetscFinalize();
  return(0);
}

/* a test contributed by Tobias Neckel <neckel@in.tum.de>, 02 Jul 2008 */
#define PETSc_CHKERRQ CHKERRQ
#undef __FUNCT__
#define __FUNCT__ "testPTAPRectangular"
PetscErrorCode testPTAPRectangular(void)
{

  const int      rows = 3;
  const int      cols = 5;
  PetscErrorCode _ierr;
  int            i;
  Mat            A;
  Mat            P;
  Mat            C;

  PetscFunctionBegin;
  /* set up A  */
  _ierr = MatCreateSeqAIJ(PETSC_COMM_WORLD, rows, rows,1, NULL, &A);
  PETSc_CHKERRQ(_ierr);
  for (i=0; i<rows; i++) {
    _ierr = MatSetValue(A, i, i, 1.0, INSERT_VALUES);
    PETSc_CHKERRQ(_ierr);
  }
  _ierr = MatAssemblyBegin(A,MAT_FINAL_ASSEMBLY);
  PETSc_CHKERRQ(_ierr);
  _ierr = MatAssemblyEnd(A,MAT_FINAL_ASSEMBLY);
  PETSc_CHKERRQ(_ierr);

  /* set up P */
  _ierr = MatCreateSeqAIJ(PETSC_COMM_WORLD, rows, cols,5, NULL, &P);
  PETSc_CHKERRQ(_ierr);
  _ierr = MatSetValue(P, 0, 0,  1.0, INSERT_VALUES); PETSc_CHKERRQ(_ierr);
  _ierr = MatSetValue(P, 0, 1,  2.0, INSERT_VALUES); PETSc_CHKERRQ(_ierr);
  _ierr = MatSetValue(P, 0, 2,  0.0, INSERT_VALUES); PETSc_CHKERRQ(_ierr);

  _ierr = MatSetValue(P, 0, 3, -1.0, INSERT_VALUES); PETSc_CHKERRQ(_ierr);

  _ierr = MatSetValue(P, 1, 0,  0.0, INSERT_VALUES); PETSc_CHKERRQ(_ierr);
  _ierr = MatSetValue(P, 1, 1, -1.0, INSERT_VALUES); PETSc_CHKERRQ(_ierr);
  _ierr = MatSetValue(P, 1, 2,  1.0, INSERT_VALUES); PETSc_CHKERRQ(_ierr);

  _ierr = MatSetValue(P, 2, 0,  3.0, INSERT_VALUES); PETSc_CHKERRQ(_ierr);
  _ierr = MatSetValue(P, 2, 1,  0.0, INSERT_VALUES); PETSc_CHKERRQ(_ierr);
  _ierr = MatSetValue(P, 2, 2, -3.0, INSERT_VALUES); PETSc_CHKERRQ(_ierr);

  _ierr = MatAssemblyBegin(P,MAT_FINAL_ASSEMBLY);
  PETSc_CHKERRQ(_ierr);
  _ierr = MatAssemblyEnd(P,MAT_FINAL_ASSEMBLY);
  PETSc_CHKERRQ(_ierr);

  /* compute C */
  _ierr = MatPtAP(A, P, MAT_INITIAL_MATRIX, 1.0, &C);
  PETSc_CHKERRQ(_ierr);

  _ierr = MatAssemblyBegin(C,MAT_FINAL_ASSEMBLY);
  PETSc_CHKERRQ(_ierr);
  _ierr = MatAssemblyEnd(C,MAT_FINAL_ASSEMBLY);
  PETSc_CHKERRQ(_ierr);

  /* compare results */
  /*
  printf("C:\n");
  _ierr = MatView(C,PETSC_VIEWER_STDOUT_WORLD);PETSc_CHKERRQ(_ierr);

  blitz::Array<double,2> actualC(cols, cols);
  actualC = 0.0;
  for (int i=0; i<cols; i++) {
    for (int j=0; j<cols; j++) {
      _ierr = MatGetValues(C, 1, &i, 1, &j, &actualC(i,j));
      PETSc_CHKERRQ(_ierr); ;
    }
  }
  blitz::Array<double,2> expectedC(cols, cols);
  expectedC = 0.0;

  expectedC(0,0) = 10.0;
  expectedC(0,1) =  2.0;
  expectedC(0,2) = -9.0;
  expectedC(0,3) = -1.0;
  expectedC(1,0) =  2.0;
  expectedC(1,1) =  5.0;
  expectedC(1,2) = -1.0;
  expectedC(1,3) = -2.0;
  expectedC(2,0) = -9.0;
  expectedC(2,1) = -1.0;
  expectedC(2,2) = 10.0;
  expectedC(2,3) =  0.0;
  expectedC(3,0) = -1.0;
  expectedC(3,1) = -2.0;
  expectedC(3,2) =  0.0;
  expectedC(3,3) =  1.0;

  int check = areBlitzArrays2NumericallyEqual(actualC,expectedC);
  validateEqualsWithParams3(check, -1 , "testPTAPRectangular()", check, actualC(check), expectedC(check));
  */

  _ierr = MatDestroy(&A);
  PETSc_CHKERRQ(_ierr);
  _ierr = MatDestroy(&P);
  PETSc_CHKERRQ(_ierr);
  _ierr = MatDestroy(&C);
  PETSc_CHKERRQ(_ierr);
  PetscFunctionReturn(0);
}


