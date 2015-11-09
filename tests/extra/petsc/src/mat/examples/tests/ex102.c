
static char help[] = "Tests MatCreateLRC()\n\n";

/*T
   Concepts: Low rank correction

   Processors: n
T*/

#include <petscmat.h>

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **args)
{
  Vec            x,b;
  Mat            A,U,V,LR;
  PetscInt       i,j,Ii,J,Istart,Iend,m = 8,n = 7,rstart,rend;
  PetscErrorCode ierr;
  PetscBool      flg;
  PetscScalar    *u,a;

  PetscInitialize(&argc,&args,(char*)0,help);
  ierr = PetscOptionsGetInt(NULL,NULL,"-m",&m,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetInt(NULL,NULL,"-n",&n,NULL);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
         Create the sparse matrix
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = MatCreate(PETSC_COMM_WORLD,&A);CHKERRQ(ierr);
  ierr = MatSetSizes(A,PETSC_DECIDE,PETSC_DECIDE,m*n,m*n);CHKERRQ(ierr);
  ierr = MatSetFromOptions(A);CHKERRQ(ierr);
  ierr = MatSetUp(A);CHKERRQ(ierr);
  ierr = MatGetOwnershipRange(A,&Istart,&Iend);CHKERRQ(ierr);
  for (Ii=Istart; Ii<Iend; Ii++) {
    a = -1.0; i = Ii/n; j = Ii - i*n;
    if (i>0)   {J = Ii - n; ierr = MatSetValues(A,1,&Ii,1,&J,&a,INSERT_VALUES);CHKERRQ(ierr);}
    if (i<m-1) {J = Ii + n; ierr = MatSetValues(A,1,&Ii,1,&J,&a,INSERT_VALUES);CHKERRQ(ierr);}
    if (j>0)   {J = Ii - 1; ierr = MatSetValues(A,1,&Ii,1,&J,&a,INSERT_VALUES);CHKERRQ(ierr);}
    if (j<n-1) {J = Ii + 1; ierr = MatSetValues(A,1,&Ii,1,&J,&a,INSERT_VALUES);CHKERRQ(ierr);}
    a = 4.0; ierr = MatSetValues(A,1,&Ii,1,&Ii,&a,INSERT_VALUES);CHKERRQ(ierr);
  }
  ierr = MatAssemblyBegin(A,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(A,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
         Create the dense matrices
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = MatCreate(PETSC_COMM_WORLD,&U);CHKERRQ(ierr);
  ierr = MatSetSizes(U,PETSC_DECIDE,PETSC_DECIDE,m*n,3);CHKERRQ(ierr);
  ierr = MatSetType(U,MATDENSE);CHKERRQ(ierr);
  ierr = MatSetUp(U);CHKERRQ(ierr);
  ierr = MatGetOwnershipRange(U,&rstart,&rend);CHKERRQ(ierr);
  ierr = MatDenseGetArray(U,&u);CHKERRQ(ierr);
  for (i=rstart; i<rend; i++) {
    u[i-rstart]          = (PetscReal)i;
    u[i+rend-2*rstart]   = (PetscReal)1000*i;
    u[i+2*rend-3*rstart] = (PetscReal)100000*i;
  }
  ierr = MatDenseRestoreArray(U,&u);CHKERRQ(ierr);
  ierr = MatAssemblyBegin(U,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(U,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);


  ierr = MatCreate(PETSC_COMM_WORLD,&V);CHKERRQ(ierr);
  ierr = MatSetSizes(V,PETSC_DECIDE,PETSC_DECIDE,m*n,3);CHKERRQ(ierr);
  ierr = MatSetType(V,MATDENSE);CHKERRQ(ierr);
  ierr = MatSetUp(V);CHKERRQ(ierr);
  ierr = MatGetOwnershipRange(U,&rstart,&rend);CHKERRQ(ierr);
  ierr = MatDenseGetArray(V,&u);CHKERRQ(ierr);
  for (i=rstart; i<rend; i++) {
    u[i-rstart]          = (PetscReal)i;
    u[i+rend-2*rstart]   = (PetscReal)1.2*i;
    u[i+2*rend-3*rstart] = (PetscReal)1.67*i+2;
  }
  ierr = MatDenseRestoreArray(V,&u);CHKERRQ(ierr);
  ierr = MatAssemblyBegin(V,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(V,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
         Create low rank created matrix
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = MatCreateLRC(A,U,V,&LR);CHKERRQ(ierr);
  ierr = MatSetUp(LR);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
         Create test vectors
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = VecCreate(PETSC_COMM_WORLD,&x);CHKERRQ(ierr);
  ierr = VecSetSizes(x,PETSC_DECIDE,m*n);CHKERRQ(ierr);
  ierr = VecSetFromOptions(x);CHKERRQ(ierr);
  ierr = VecDuplicate(x,&b);CHKERRQ(ierr);
  ierr = VecGetOwnershipRange(x,&rstart,&rend);CHKERRQ(ierr);
  ierr = VecGetArray(x,&u);CHKERRQ(ierr);
  for (i=rstart; i<rend; i++) u[i-rstart] = (PetscScalar)i;
  ierr = VecRestoreArray(x,&u);CHKERRQ(ierr);

  ierr = MatMult(LR,x,b);CHKERRQ(ierr);
  /*
     View the product if desired
  */
  ierr = PetscOptionsHasName(NULL,NULL,"-view_product",&flg);CHKERRQ(ierr);
  if (flg) {ierr = VecView(b,PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);}

  ierr = VecDestroy(&x);CHKERRQ(ierr);
  ierr = VecDestroy(&b);CHKERRQ(ierr);
  /* you can destroy the matrices in any order you like */
  ierr = MatDestroy(&A);CHKERRQ(ierr);
  ierr = MatDestroy(&U);CHKERRQ(ierr);
  ierr = MatDestroy(&V);CHKERRQ(ierr);
  ierr = MatDestroy(&LR);CHKERRQ(ierr);

  /*
     Always call PetscFinalize() before exiting a program.  This routine
       - finalizes the PETSc libraries as well as MPI
       - provides summary and diagnostic information if certain runtime
         options are chosen (e.g., -log_summary).
  */
  ierr = PetscFinalize();
  return 0;
}
