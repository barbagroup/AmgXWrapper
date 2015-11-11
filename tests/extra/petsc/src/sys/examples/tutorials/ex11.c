
static char help[] = "Demonstrates PetscDataTypeFromString().\n\n";

/*T
   Concepts: introduction to PETSc;
   Concepts: printing^in parallel
   Processors: n
T*/

#include <petscsys.h>
int main(int argc,char **argv)
{
  PetscErrorCode ierr;
  PetscDataType  dtype;
  PetscBool      found;

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

  ierr = PetscDataTypeFromString("Scalar",&dtype,&found);CHKERRQ(ierr);
  if (!found) SETERRQ(PETSC_COMM_WORLD,PETSC_ERR_ARG_WRONG,"Did not find scalar datatype");
  if (dtype != PETSC_SCALAR) SETERRQ(PETSC_COMM_WORLD,PETSC_ERR_ARG_WRONG,"Found wrong datatype for scalar");

  ierr = PetscDataTypeFromString("INT",&dtype,&found);CHKERRQ(ierr);
  if (!found) SETERRQ(PETSC_COMM_WORLD,PETSC_ERR_ARG_WRONG,"Did not find int datatype");
  if (dtype != PETSC_INT) SETERRQ(PETSC_COMM_WORLD,PETSC_ERR_ARG_WRONG,"Found wrong datatype for int");

  ierr = PetscDataTypeFromString("real",&dtype,&found);CHKERRQ(ierr);
  if (!found) SETERRQ(PETSC_COMM_WORLD,PETSC_ERR_ARG_WRONG,"Did not find real datatype");
  if (dtype != PETSC_REAL) SETERRQ(PETSC_COMM_WORLD,PETSC_ERR_ARG_WRONG,"Found wrong datatype for real");

  ierr = PetscDataTypeFromString("abogusdatatype",&dtype,&found);CHKERRQ(ierr);
  if (found) SETERRQ(PETSC_COMM_WORLD,PETSC_ERR_ARG_WRONG,"Found a bogus datatype");

  ierr = PetscFinalize();
  return 0;
}
