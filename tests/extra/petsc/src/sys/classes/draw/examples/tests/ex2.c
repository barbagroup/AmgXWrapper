
static char help[] = "Demonstrates use of color map\n";

#include <petscsys.h>
#include <petscdraw.h>

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **argv)
{
  PetscDraw      draw;
  PetscMPIInt    size,rank;
  PetscErrorCode ierr;
  int            x = 0,y = 0,width = 256,height = 256,i;

  ierr = PetscInitialize(&argc,&argv,(char*)0,help);CHKERRQ(ierr);
  ierr = PetscDrawCreate(PETSC_COMM_WORLD,0,"Title",x,y,width,height,&draw);CHKERRQ(ierr);
  ierr = PetscDrawSetFromOptions(draw);CHKERRQ(ierr);
  ierr = MPI_Comm_size(PETSC_COMM_WORLD,&size);CHKERRQ(ierr);
  ierr = MPI_Comm_rank(PETSC_COMM_WORLD,&rank);CHKERRQ(ierr);
  for (i=rank; i<height; i+=size) {
    PetscReal y = ((PetscReal)i)/(height-1);
    ierr = PetscDrawLine(draw,0.0,y,1.0,y,i%256);CHKERRQ(ierr);
  }
  ierr = PetscDrawSynchronizedFlush(draw);CHKERRQ(ierr);
  ierr = PetscDrawPause(draw);CHKERRQ(ierr);
  ierr = PetscDrawDestroy(&draw);CHKERRQ(ierr);
  ierr = PetscFinalize();
  return 0;
}

