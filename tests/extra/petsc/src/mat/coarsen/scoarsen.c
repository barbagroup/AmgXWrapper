
#include <petscmat.h>
#include <petsc/private/matimpl.h>

PETSC_EXTERN PetscErrorCode MatCoarsenCreate_MIS(MatCoarsen);
PETSC_EXTERN PetscErrorCode MatCoarsenCreate_HEM(MatCoarsen);

#undef __FUNCT__
#define __FUNCT__ "MatCoarsenRegisterAll"
/*@C
  MatCoarsenRegisterAll - Registers all of the matrix Coarsen routines in PETSc.

  Not Collective

  Level: developer

  Adding new methods:
  To add a new method to the registry. Copy this routine and
  modify it to incorporate a call to MatCoarsenRegister() for
  the new method, after the current list.

  Restricting the choices: To prevent all of the methods from being
  registered and thus save memory, copy this routine and modify it to
  register a zero, instead of the function name, for those methods you
 do not wish to register.  Make sure that the replacement routine is
  linked before libpetscmat.a.

 .keywords: matrix, Coarsen, register, all

 .seealso: MatCoarsenRegister(), MatCoarsenRegisterDestroy()
 @*/
PetscErrorCode  MatCoarsenRegisterAll(void)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (MatCoarsenRegisterAllCalled) PetscFunctionReturn(0);
  MatCoarsenRegisterAllCalled = PETSC_TRUE;

  ierr = MatCoarsenRegister(MATCOARSENMIS,MatCoarsenCreate_MIS);CHKERRQ(ierr);
  ierr = MatCoarsenRegister(MATCOARSENHEM,MatCoarsenCreate_HEM);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

