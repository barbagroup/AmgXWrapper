
static char help[] = "Scatters from a sequential vector to a parallel vector.\n\
This does case when we are merely selecting the local part of the\n\
parallel vector.\n";

#include <petscvec.h>

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **argv)
{
  PetscErrorCode ierr;
  PetscMPIInt    size,rank;
  PetscInt       n = 5,i;
  PetscScalar    value;
  Vec            x,y;
  IS             is1,is2;
  VecScatter     ctx = 0;

  ierr = PetscInitialize(&argc,&argv,(char*)0,help);CHKERRQ(ierr);

  ierr = MPI_Comm_size(PETSC_COMM_WORLD,&size);CHKERRQ(ierr);
  ierr = MPI_Comm_rank(PETSC_COMM_WORLD,&rank);CHKERRQ(ierr);

  /* create two vectors */
  ierr = VecCreate(PETSC_COMM_WORLD,&x);CHKERRQ(ierr);
  ierr = VecSetSizes(x,PETSC_DECIDE,size*n);CHKERRQ(ierr);
  ierr = VecSetFromOptions(x);CHKERRQ(ierr);
  ierr = VecCreateSeq(PETSC_COMM_SELF,n,&y);CHKERRQ(ierr);

  /* create two index sets */
  ierr = ISCreateStride(PETSC_COMM_SELF,n,n*rank,1,&is1);CHKERRQ(ierr);
  ierr = ISCreateStride(PETSC_COMM_SELF,n,0,1,&is2);CHKERRQ(ierr);

  /* each processor inserts the entire vector */
  /* this is redundant but tests assembly */
  for (i=0; i<n; i++) {
    value = (PetscScalar) (i + 10*rank);
    ierr  = VecSetValues(y,1,&i,&value,INSERT_VALUES);CHKERRQ(ierr);
  }
  ierr = VecAssemblyBegin(y);CHKERRQ(ierr);
  ierr = VecAssemblyEnd(y);CHKERRQ(ierr);

  ierr = VecScatterCreate(y,is2,x,is1,&ctx);CHKERRQ(ierr);
  ierr = VecScatterBegin(ctx,y,x,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
  ierr = VecScatterEnd(ctx,y,x,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
  ierr = VecScatterDestroy(&ctx);CHKERRQ(ierr);

  ierr = VecView(x,PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);

  ierr = VecDestroy(&x);CHKERRQ(ierr);
  ierr = VecDestroy(&y);CHKERRQ(ierr);
  ierr = ISDestroy(&is1);CHKERRQ(ierr);
  ierr = ISDestroy(&is2);CHKERRQ(ierr);

  ierr = PetscFinalize();
  return 0;
}

