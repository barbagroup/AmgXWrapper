static const char help[] = "Test overlapped communication on a single star forest (PetscSF)\n\n";

#include <petscvec.h>
#include <petscsf.h>
#include <petscviewer.h>

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc, char **argv)
{
  PetscInt    ierr;
  PetscSF     sf;
  Vec         A,Aout;
  Vec         B,Bout;
  PetscScalar *bufA;
  PetscScalar *bufAout;
  PetscScalar *bufB;
  PetscScalar *bufBout;
  PetscMPIInt rank, size;
  PetscInt    nroots, nleaves;
  PetscInt    i;
  PetscInt    *ilocal;
  PetscSFNode *iremote;

  PetscInitialize(&argc,&argv,NULL,help);

  ierr = MPI_Comm_rank(PETSC_COMM_WORLD,&rank);CHKERRQ(ierr);
  ierr = MPI_Comm_size(PETSC_COMM_WORLD,&size);CHKERRQ(ierr);

  if (size != 2) SETERRQ(PETSC_COMM_WORLD, PETSC_ERR_USER, "Only coded for two MPI processes\n");

  ierr = PetscSFCreate(PETSC_COMM_WORLD,&sf);CHKERRQ(ierr);
  ierr = PetscSFSetFromOptions(sf);CHKERRQ(ierr);

  nleaves = 2;
  nroots = 1;
  ierr = PetscMalloc1(nleaves,&ilocal);CHKERRQ(ierr);

  for (i = 0; i<nleaves; i++) {
    ilocal[i] = i;
  }

  ierr = PetscMalloc1(nleaves,&iremote);CHKERRQ(ierr);
  if (rank == 0) {
    iremote[0].rank = 0;
    iremote[0].index = 0;
    iremote[1].rank = 1;
    iremote[1].index = 0;
  } else {
    iremote[0].rank = 1;
    iremote[0].index = 0;
    iremote[1].rank = 0;
    iremote[1].index = 0;
  }
  ierr = PetscSFSetGraph(sf,nroots,nleaves,ilocal,PETSC_OWN_POINTER,iremote,PETSC_OWN_POINTER);CHKERRQ(ierr);
  ierr = PetscSFSetUp(sf);CHKERRQ(ierr);
  ierr = PetscSFView(sf,PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);
  ierr = VecCreate(PETSC_COMM_WORLD,&A);CHKERRQ(ierr);
  ierr = VecSetSizes(A,2,PETSC_DETERMINE);CHKERRQ(ierr);
  ierr = VecSetFromOptions(A);CHKERRQ(ierr);
  ierr = VecSetUp(A);CHKERRQ(ierr);

  ierr = VecDuplicate(A,&B);CHKERRQ(ierr);
  ierr = VecDuplicate(A,&Aout);CHKERRQ(ierr);
  ierr = VecDuplicate(A,&Bout);CHKERRQ(ierr);
  ierr = VecGetArray(A,&bufA);CHKERRQ(ierr);
  ierr = VecGetArray(B,&bufB);CHKERRQ(ierr);
  for (i=0; i<2; i++) {
    bufA[i] = (PetscScalar)rank;
    bufB[i] = (PetscScalar)(rank) + 10.0;
  }
  ierr = VecRestoreArray(A,&bufA);CHKERRQ(ierr);
  ierr = VecRestoreArray(B,&bufB);CHKERRQ(ierr);

  ierr = VecGetArrayRead(A,(const PetscScalar**)&bufA);CHKERRQ(ierr);
  ierr = VecGetArrayRead(B,(const PetscScalar**)&bufB);CHKERRQ(ierr);
  ierr = VecGetArray(Aout,&bufAout);CHKERRQ(ierr);
  ierr = VecGetArray(Bout,&bufBout);CHKERRQ(ierr);
  ierr = PetscSFBcastBegin(sf,MPIU_SCALAR,(const void*)bufA,(void *)bufAout);CHKERRQ(ierr);
  ierr = PetscSFBcastBegin(sf,MPIU_SCALAR,(const void*)bufB,(void *)bufBout);CHKERRQ(ierr);
  ierr = PetscSFBcastEnd(sf,MPIU_SCALAR,(const void*)bufA,(void *)bufAout);CHKERRQ(ierr);
  ierr = PetscSFBcastEnd(sf,MPIU_SCALAR,(const void*)bufB,(void *)bufBout);CHKERRQ(ierr);
  ierr = VecRestoreArrayRead(A,(const PetscScalar**)&bufA);CHKERRQ(ierr);
  ierr = VecRestoreArrayRead(B,(const PetscScalar**)&bufB);CHKERRQ(ierr);
  ierr = VecRestoreArray(Aout,&bufAout);CHKERRQ(ierr);
  ierr = VecRestoreArray(Bout,&bufBout);CHKERRQ(ierr);

  ierr = VecView(Aout,PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);
  ierr = VecView(Bout,PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);
  ierr = VecDestroy(&A);CHKERRQ(ierr);
  ierr = VecDestroy(&B);CHKERRQ(ierr);
  ierr = VecDestroy(&Aout);CHKERRQ(ierr);
  ierr = VecDestroy(&Bout);CHKERRQ(ierr);
  ierr = PetscSFDestroy(&sf);CHKERRQ(ierr);

  PetscFinalize();
  return 0;
}
