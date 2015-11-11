
static char help[] = "Demonstrates scattering with strided index sets.\n\n";

#include <petscvec.h>

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **argv)
{
  PetscErrorCode ierr;
  PetscInt       n   = 6,loc[6] = {0,1,2,3,4,5};
  PetscScalar    two = 2.0,vals[6] = {10,11,12,13,14,15};
  Vec            x,y;
  IS             is1,is2;
  VecScatter     ctx = 0;

  PetscInitialize(&argc,&argv,(char*)0,help);

  /* create two vectors */
  ierr = VecCreateSeq(PETSC_COMM_SELF,n,&x);CHKERRQ(ierr);
  ierr = VecDuplicate(x,&y);CHKERRQ(ierr);

  /* create two index sets */
  ierr = ISCreateStride(PETSC_COMM_SELF,3,0,2,&is1);CHKERRQ(ierr);
  ierr = ISCreateStride(PETSC_COMM_SELF,3,1,2,&is2);CHKERRQ(ierr);

  ierr = VecSetValues(x,6,loc,vals,INSERT_VALUES);CHKERRQ(ierr);
  ierr = VecView(x,PETSC_VIEWER_STDOUT_SELF);CHKERRQ(ierr);
  ierr = PetscPrintf(PETSC_COMM_SELF,"----\n");CHKERRQ(ierr);
  ierr = VecSet(y,two);CHKERRQ(ierr);
  ierr = VecScatterCreate(x,is1,y,is2,&ctx);CHKERRQ(ierr);
  ierr = VecScatterBegin(ctx,x,y,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
  ierr = VecScatterEnd(ctx,x,y,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
  ierr = VecScatterDestroy(&ctx);CHKERRQ(ierr);

  ierr = VecView(y,PETSC_VIEWER_STDOUT_SELF);CHKERRQ(ierr);

  ierr = ISDestroy(&is1);CHKERRQ(ierr);
  ierr = ISDestroy(&is2);CHKERRQ(ierr);
  ierr = VecDestroy(&x);CHKERRQ(ierr);
  ierr = VecDestroy(&y);CHKERRQ(ierr);

  ierr = PetscFinalize();
  return 0;
}

