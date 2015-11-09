static char help[] = "Test MatMatMult(), MatTranspose(), MatTransposeMatMult() for Dense and Elemental matrices.\n\n";
/*
 Example:
   mpiexec -n <np> ./ex104 -mat_type elemental
*/

#include <petscmat.h>

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **argv)
{
  Mat            A,B,C,D;
  PetscInt       i,M=10,N=5,j,nrows,ncols,am,an,rstart,rend;
  PetscErrorCode ierr;
  PetscRandom    r;
  PetscBool      equal,iselemental;
  PetscReal      fill = 1.0;
  IS             isrows,iscols;
  const PetscInt *rows,*cols;
  PetscScalar    *v,rval;
#if defined(PETSC_HAVE_ELEMENTAL)
  PetscBool      Test_MatMatMult=PETSC_TRUE;
#else
  PetscBool      Test_MatMatMult=PETSC_FALSE;
#endif
  PetscMPIInt    size;

  PetscInitialize(&argc,&argv,(char*)0,help);
  ierr = MPI_Comm_size(PETSC_COMM_WORLD,&size);CHKERRQ(ierr);

  ierr = PetscOptionsGetInt(NULL,NULL,"-M",&M,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetInt(NULL,NULL,"-N",&N,NULL);CHKERRQ(ierr);
  ierr = MatCreate(PETSC_COMM_WORLD,&A);CHKERRQ(ierr);
  ierr = MatSetSizes(A,PETSC_DECIDE,PETSC_DECIDE,M,N);CHKERRQ(ierr);
  ierr = MatSetType(A,MATDENSE);CHKERRQ(ierr);
  ierr = MatSetFromOptions(A);CHKERRQ(ierr);
  ierr = MatSetUp(A);CHKERRQ(ierr);
  ierr = PetscRandomCreate(PETSC_COMM_WORLD,&r);CHKERRQ(ierr);
  ierr = PetscRandomSetFromOptions(r);CHKERRQ(ierr);

  /* Set local matrix entries */
  ierr = MatGetOwnershipIS(A,&isrows,&iscols);CHKERRQ(ierr);
  ierr = ISGetLocalSize(isrows,&nrows);CHKERRQ(ierr);
  ierr = ISGetIndices(isrows,&rows);CHKERRQ(ierr);
  ierr = ISGetLocalSize(iscols,&ncols);CHKERRQ(ierr);
  ierr = ISGetIndices(iscols,&cols);CHKERRQ(ierr);
  ierr = PetscMalloc1(nrows*ncols,&v);CHKERRQ(ierr);
  for (i=0; i<nrows; i++) {
    for (j=0; j<ncols; j++) {
      ierr         = PetscRandomGetValue(r,&rval);CHKERRQ(ierr);
      v[i*ncols+j] = rval; 
    }
  }
  ierr = MatSetValues(A,nrows,rows,ncols,cols,v,INSERT_VALUES);CHKERRQ(ierr);
  ierr = MatAssemblyBegin(A,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(A,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = ISRestoreIndices(isrows,&rows);CHKERRQ(ierr);
  ierr = ISRestoreIndices(iscols,&cols);CHKERRQ(ierr);
  ierr = ISDestroy(&isrows);CHKERRQ(ierr);
  ierr = ISDestroy(&iscols);CHKERRQ(ierr);
  ierr = PetscRandomDestroy(&r);CHKERRQ(ierr);

  /* Test MatMatMult() */
  if (Test_MatMatMult) { 
#if !defined(PETSC_HAVE_ELEMENTAL)
    if (size > 1) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SUP,"This test requires ELEMENTAL");
#endif
    ierr = MatTranspose(A,MAT_INITIAL_MATRIX,&B);CHKERRQ(ierr); /* B = A^T */
    ierr = MatMatMult(B,A,MAT_INITIAL_MATRIX,fill,&C);CHKERRQ(ierr); /* C = B*A = A^T*A */
    ierr = MatMatMult(B,A,MAT_REUSE_MATRIX,fill,&C);CHKERRQ(ierr);

    /* Test B*A*x = C*x for n random vector x */
    ierr = MatMatMultEqual(B,A,C,10,&equal);CHKERRQ(ierr);
    if (!equal) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_PLIB,"B*A*x != C*x");
    ierr = MatDestroy(&C);CHKERRQ(ierr);

    ierr = MatMatMultSymbolic(B,A,fill,&C);CHKERRQ(ierr); 
    for (i=0; i<2; i++) {
      /* Repeat the numeric product to test reuse of the previous symbolic product */
      ierr = MatMatMultNumeric(B,A,C);CHKERRQ(ierr);
   
      ierr = MatMatMultEqual(B,A,C,10,&equal);CHKERRQ(ierr);
      if (!equal) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_PLIB,"B*A*x != C*x");
    }
    ierr = MatDestroy(&C);CHKERRQ(ierr);
    ierr = MatDestroy(&B);CHKERRQ(ierr);
  }

  /* Test MatTransposeMatMult() */
  ierr = PetscObjectTypeCompare((PetscObject)A,MATELEMENTAL,&iselemental);CHKERRQ(ierr);
  if (!iselemental) {
    ierr = MatTransposeMatMult(A,A,MAT_INITIAL_MATRIX,fill,&D);CHKERRQ(ierr); /* D = A^T*A */
    ierr = MatTransposeMatMult(A,A,MAT_REUSE_MATRIX,fill,&D);CHKERRQ(ierr);
    /* ierr = MatView(D,PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr); */
    ierr = MatTransposeMatMultEqual(A,A,D,10,&equal);CHKERRQ(ierr);
    if (!equal) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_PLIB,"D*x != A^T*A*x");
    ierr = MatDestroy(&D);CHKERRQ(ierr);

    /* Test D*x = A^T*C*A*x, where C is in AIJ format */
    ierr = MatGetLocalSize(A,&am,&an);CHKERRQ(ierr);
    ierr = MatCreate(PETSC_COMM_WORLD,&C);CHKERRQ(ierr);
    if (size == 1) {
      ierr = MatSetSizes(C,PETSC_DECIDE,PETSC_DECIDE,am,am);CHKERRQ(ierr);
    } else {
      ierr = MatSetSizes(C,am,am,PETSC_DECIDE,PETSC_DECIDE);CHKERRQ(ierr);
    }
    ierr = MatSetFromOptions(C);CHKERRQ(ierr);
    ierr = MatSetUp(C);CHKERRQ(ierr);
    ierr = MatGetOwnershipRange(C,&rstart,&rend);CHKERRQ(ierr);
    v[0] = 1.0;
    for (i=rstart; i<rend; i++) {
      ierr = MatSetValues(C,1,&i,1,&i,v,INSERT_VALUES);CHKERRQ(ierr);
    }
    ierr = MatAssemblyBegin(C,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
    ierr = MatAssemblyEnd(C,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);

    /* B = C*A, D = A^T*B */
    ierr = MatMatMult(C,A,MAT_INITIAL_MATRIX,1.0,&B);CHKERRQ(ierr);
    ierr = MatTransposeMatMult(A,B,MAT_INITIAL_MATRIX,fill,&D);CHKERRQ(ierr);
    ierr = MatTransposeMatMultEqual(A,B,D,10,&equal);CHKERRQ(ierr);
    if (!equal) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_PLIB,"D*x != A^T*B*x");

    ierr = MatDestroy(&D);CHKERRQ(ierr);
    ierr = MatDestroy(&C);CHKERRQ(ierr);
    ierr = MatDestroy(&B);CHKERRQ(ierr);
  }

  ierr = MatDestroy(&A);CHKERRQ(ierr);
  ierr = PetscFree(v);CHKERRQ(ierr);
  PetscFinalize();
  return(0);
}
