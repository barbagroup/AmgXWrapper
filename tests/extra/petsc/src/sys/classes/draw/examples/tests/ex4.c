
static char help[] = "Demonstrates use of PetscDrawZoom()\n";

#if defined(PETSC_APPLE_FRAMEWORK)
#include <PETSc/petscsys.h>
#include <PETSc/petscdraw.h>
#else
#include <petscsys.h>
#include <petscdraw.h>
#endif

#undef __FUNCT__
#define __FUNCT__ "zoomfunction"
PetscErrorCode zoomfunction(PetscDraw draw,void *dummy)
{
  int            i;
  MPI_Comm       comm = PetscObjectComm((PetscObject)draw);
  PetscMPIInt    size,rank;
  PetscErrorCode ierr;

  ierr = MPI_Comm_size(comm,&size);CHKERRQ(ierr);
  ierr = MPI_Comm_rank(comm,&rank);CHKERRQ(ierr);
  for (i=rank; i<256; i+=size) {
    PetscReal y = ((PetscReal)i)/(256-1);
    ierr = PetscDrawLine(draw,0.0,y,1.0,y,i);CHKERRQ(ierr);
  }
  return 0;
}

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **argv)
{
  int            x = 0,y = 0,width = 256,height = 256;
  PetscDraw      draw;
  PetscErrorCode ierr;

  ierr = PetscInitialize(&argc,&argv,(char*)0,help);CHKERRQ(ierr);
  ierr = PetscDrawCreate(PETSC_COMM_WORLD,NULL,"Title",x,y,width,height,&draw);CHKERRQ(ierr);
  ierr = PetscDrawSetFromOptions(draw);CHKERRQ(ierr);
  ierr = PetscDrawZoom(draw,zoomfunction,NULL);CHKERRQ(ierr);
  ierr = PetscDrawDestroy(&draw);CHKERRQ(ierr);
  ierr = PetscFinalize();
  return 0;
}


