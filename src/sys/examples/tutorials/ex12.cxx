
static char help[] = "Demonstrates call PETSc and Chombo in the same program.\n\n";

/*T
   Concepts: introduction to PETSc; Chombo
   Processors: n
T*/

#include <petscsys.h>
#include "Box.H"

int main(int argc,char **argv)
{
  PetscErrorCode ierr;

  /*
    Every PETSc routine should begin with the PetscInitialize() routine.
    argc, argv - These command line arguments are taken to extract the options
                 supplied to PETSc and options supplied to MPI.
    help       - When PETSc executable is invoked with the option -help,
                 it prints the various options that can be applied at
                 runtime.  The user can use the "help" variable place
                 additional help messages in this printout.
  */
  ierr = PetscInitialize(&argc,&argv,(char*)0,help);CHKERRQ(ierr);
  Box::Box *nb = new Box::Box();
  delete nb;

  ierr = PetscFinalize();
  return 0;
}
