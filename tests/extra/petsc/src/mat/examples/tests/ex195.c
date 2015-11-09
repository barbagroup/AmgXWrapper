/*
 * ex195.c
 *
 *  Created on: Aug 24, 2015
 *      Author: Fande Kong <fdkong.jd@gmail.com>
 */

static char help[] = " Demonstrate the use of MatConvert_Nest_AIJ\n";


#include <petscmat.h>


#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **args)
{
  Mat                 A1,A2,A3,A4,nest;
  Mat                 mata[4];
  Mat                 aij;
  MPI_Comm            comm;
  PetscInt            m,n,istart,iend,ii,i,J,j;
  PetscScalar         v;
  PetscMPIInt         size;
  PetscErrorCode      ierr;

  PetscInitialize(&argc,&args,(char*)0,help);
  comm = PETSC_COMM_WORLD;
  ierr = MPI_Comm_size(comm,&size);CHKERRQ(ierr);
  /*
     Assemble the matrix for the five point stencil, YET AGAIN
  */
  ierr = MatCreate(comm,&A1);CHKERRQ(ierr);
  m=2,n=2;
  ierr = MatSetSizes(A1,PETSC_DECIDE,PETSC_DECIDE,m*n,m*n);CHKERRQ(ierr);
  ierr = MatSetFromOptions(A1);CHKERRQ(ierr);
  ierr = MatSetUp(A1);CHKERRQ(ierr);
  ierr = MatGetOwnershipRange(A1,&istart,&iend);CHKERRQ(ierr);
  for (ii=istart; ii<iend; ii++) {
    v = -1.0; i = ii/n; j = ii - i*n;
    if (i>0)   {J = ii - n; ierr = MatSetValues(A1,1,&ii,1,&J,&v,INSERT_VALUES);CHKERRQ(ierr);}
    if (i<m-1) {J = ii + n; ierr = MatSetValues(A1,1,&ii,1,&J,&v,INSERT_VALUES);CHKERRQ(ierr);}
    if (j>0)   {J = ii - 1; ierr = MatSetValues(A1,1,&ii,1,&J,&v,INSERT_VALUES);CHKERRQ(ierr);}
    if (j<n-1) {J = ii + 1; ierr = MatSetValues(A1,1,&ii,1,&J,&v,INSERT_VALUES);CHKERRQ(ierr);}
    v = 4.0; ierr = MatSetValues(A1,1,&ii,1,&ii,&v,INSERT_VALUES);CHKERRQ(ierr);
  }
  ierr = MatAssemblyBegin(A1,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(A1,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatView(A1,PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);
  ierr = MatDuplicate(A1,MAT_COPY_VALUES,&A2);CHKERRQ(ierr);
  ierr = MatDuplicate(A1,MAT_COPY_VALUES,&A3);CHKERRQ(ierr);
  ierr = MatDuplicate(A1,MAT_COPY_VALUES,&A4);CHKERRQ(ierr);
  /*create a nest matrix */
  ierr = MatCreate(comm,&nest);CHKERRQ(ierr);
  ierr = MatSetType(nest,MATNEST);CHKERRQ(ierr);
  mata[0]=A1,mata[1]=A2,mata[2]=A3,mata[3]=A4;
  ierr = MatNestSetSubMats(nest,2,NULL,2,NULL,mata);CHKERRQ(ierr);
  ierr = MatSetUp(nest);CHKERRQ(ierr);
  ierr = MatConvert(nest,MATAIJ,MAT_INITIAL_MATRIX,&aij);CHKERRQ(ierr);
  ierr = MatView(aij,PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);
  ierr = MatDestroy(&A1);CHKERRQ(ierr);
  ierr = MatDestroy(&A2);CHKERRQ(ierr);
  ierr = MatDestroy(&A3);CHKERRQ(ierr);
  ierr = MatDestroy(&A4);CHKERRQ(ierr);
  ierr = PetscFinalize();
  return 0;
}
