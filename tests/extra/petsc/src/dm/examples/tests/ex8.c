
static char help[] = "Demonstrates generating a slice from a DMDA Vector.\n\n";

#include <petscdmda.h>
#include <petscao.h>

#undef __FUNCT__
#define __FUNCT__ "GenerateSliceScatter"
/*
    Given a DMDA generates a VecScatter context that will deliver a slice
  of the global vector to each processor. In this example, each processor
  receives the values i=*, j=*, k=rank, i.e. one z plane.

  Note: This code is written assuming only one degree of freedom per node.
  For multiple degrees of freedom per node use ISCreateBlock()
  instead of ISCreateGeneral().
*/
PetscErrorCode GenerateSliceScatter(DM da,VecScatter *scatter,Vec *vslice)
{
  AO             ao;
  PetscInt       M,N,P,nslice,*sliceindices,count,i,j;
  PetscMPIInt    rank;
  PetscErrorCode ierr;
  MPI_Comm       comm;
  Vec            vglobal;
  IS             isfrom,isto;

  ierr = PetscObjectGetComm((PetscObject)da,&comm);CHKERRQ(ierr);
  ierr = MPI_Comm_rank(comm,&rank);CHKERRQ(ierr);

  ierr = DMDAGetAO(da,&ao);CHKERRQ(ierr);
  ierr = DMDAGetInfo(da,0,&M,&N,&P,0,0,0,0,0,0,0,0,0);CHKERRQ(ierr);

  /*
     nslice is number of degrees of freedom in this processors slice
   if there are more processors then z plans the extra processors get 0
   elements in their slice.
  */
  if (rank < P) nslice = M*N;
  else nslice = 0;

  /*
     Generate the local vector to hold this processors slice
  */
  ierr = VecCreateSeq(PETSC_COMM_SELF,nslice,vslice);CHKERRQ(ierr);
  ierr = DMCreateGlobalVector(da,&vglobal);CHKERRQ(ierr);

  /*
       Generate the indices for the slice in the "natural" global ordering
     Note: this is just an example, one could select any subset of nodes
    on each processor. Just list them in the global natural ordering.

  */
  ierr = PetscMalloc1(nslice+1,&sliceindices);CHKERRQ(ierr);
  count = 0;
  if (rank < P) {
    for (j=0; j<N; j++) {
      for (i=0; i<M; i++) {
         sliceindices[count++] = rank*M*N + j*M + i;
      }
    }
  }
  /*
      Convert the indices to the "PETSc" global ordering
  */
  ierr = AOApplicationToPetsc(ao,nslice,sliceindices);CHKERRQ(ierr);

  /* Create the "from" and "to" index set */
  /* This is to scatter from the global vector */
  ierr = ISCreateGeneral(PETSC_COMM_SELF,nslice,sliceindices,PETSC_OWN_POINTER,&isfrom);CHKERRQ(ierr);
  /* This is to gather into the local vector */
  ierr = ISCreateStride(PETSC_COMM_SELF,nslice,0,1,&isto);CHKERRQ(ierr);
  ierr = VecScatterCreate(vglobal,isfrom,*vslice,isto,scatter);CHKERRQ(ierr);
  ierr = ISDestroy(&isfrom);CHKERRQ(ierr);
  ierr = ISDestroy(&isto);CHKERRQ(ierr);
  return 0;
}


#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **argv)
{
  PetscMPIInt      rank;
  PetscInt         m   = PETSC_DECIDE,n = PETSC_DECIDE,p = PETSC_DECIDE,M = 3,N = 5,P=3,s=1;
  PetscInt         *lx = NULL,*ly = NULL,*lz = NULL;
  PetscErrorCode   ierr;
  PetscBool        flg = PETSC_FALSE;
  DM               da;
  Vec              local,global,vslice;
  PetscScalar      value;
  DMBoundaryType   wrap         = DM_XYPERIODIC;
  DMDAStencilType  stencil_type = DMDA_STENCIL_BOX;
  VecScatter       scatter;

  ierr = PetscInitialize(&argc,&argv,(char*)0,help);CHKERRQ(ierr);
  ierr = MPI_Comm_rank(PETSC_COMM_WORLD,&rank);CHKERRQ(ierr);

  /* Read options */
  ierr = PetscOptionsGetInt(NULL,NULL,"-M",&M,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetInt(NULL,NULL,"-N",&N,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetInt(NULL,NULL,"-P",&P,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetInt(NULL,NULL,"-m",&m,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetInt(NULL,NULL,"-n",&n,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetInt(NULL,NULL,"-p",&p,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetInt(NULL,NULL,"-s",&s,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetBool(NULL,NULL,"-star",&flg,NULL);CHKERRQ(ierr);
  if (flg) stencil_type =  DMDA_STENCIL_STAR;

  /* Create distributed array and get vectors */
  ierr = DMDACreate3d(PETSC_COMM_WORLD,wrap,stencil_type,M,N,P,m,n,p,1,s,
                      lx,ly,lz,&da);CHKERRQ(ierr);
  ierr = DMView(da,PETSC_VIEWER_DRAW_WORLD);CHKERRQ(ierr);
  ierr = DMCreateGlobalVector(da,&global);CHKERRQ(ierr);
  ierr = DMCreateLocalVector(da,&local);CHKERRQ(ierr);

  ierr = GenerateSliceScatter(da,&scatter,&vslice);CHKERRQ(ierr);

  /* Put the value rank+1 into all locations of vslice and transfer back to global vector */
  value = 1.0 + rank;
  ierr  = VecSet(vslice,value);CHKERRQ(ierr);
  ierr  = VecScatterBegin(scatter,vslice,global,INSERT_VALUES,SCATTER_REVERSE);CHKERRQ(ierr);
  ierr  = VecScatterEnd(scatter,vslice,global,INSERT_VALUES,SCATTER_REVERSE);CHKERRQ(ierr);

  ierr = VecView(global,PETSC_VIEWER_DRAW_WORLD);CHKERRQ(ierr);

  ierr = VecDestroy(&local);CHKERRQ(ierr);
  ierr = VecDestroy(&global);CHKERRQ(ierr);
  ierr = DMDestroy(&da);CHKERRQ(ierr);
  ierr = PetscFinalize();
  return 0;
}
