#include <petsc/private/linesearchimpl.h>     /*I  "petscsnes.h"  I*/

PETSC_EXTERN PetscErrorCode SNESLineSearchCreate_Basic(SNESLineSearch);
PETSC_EXTERN PetscErrorCode SNESLineSearchCreate_L2(SNESLineSearch);
PETSC_EXTERN PetscErrorCode SNESLineSearchCreate_CP(SNESLineSearch);
PETSC_EXTERN PetscErrorCode SNESLineSearchCreate_BT(SNESLineSearch);
PETSC_EXTERN PetscErrorCode SNESLineSearchCreate_NLEQERR(SNESLineSearch);
PETSC_EXTERN PetscErrorCode SNESLineSearchCreate_Shell(SNESLineSearch);


#undef __FUNCT__
#define __FUNCT__ "SNESLineSearchRegisterAll"
/*@C
   SNESLineSearchRegisterAll - Registers all of the nonlinear solver methods in the SNESLineSearch package.

   Not Collective

   Level: advanced

.keywords: SNESLineSearch, register, all

.seealso:  SNESLineSearchRegisterDestroy()
@*/
PetscErrorCode SNESLineSearchRegisterAll(void)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (SNESLineSearchRegisterAllCalled) PetscFunctionReturn(0);
  SNESLineSearchRegisterAllCalled = PETSC_TRUE;
  ierr = SNESLineSearchRegister(SNESLINESEARCHSHELL,   SNESLineSearchCreate_Shell);CHKERRQ(ierr);
  ierr = SNESLineSearchRegister(SNESLINESEARCHBASIC,   SNESLineSearchCreate_Basic);CHKERRQ(ierr);
  ierr = SNESLineSearchRegister(SNESLINESEARCHL2,      SNESLineSearchCreate_L2);CHKERRQ(ierr);
  ierr = SNESLineSearchRegister(SNESLINESEARCHBT,      SNESLineSearchCreate_BT);CHKERRQ(ierr);
  ierr = SNESLineSearchRegister(SNESLINESEARCHNLEQERR, SNESLineSearchCreate_NLEQERR);CHKERRQ(ierr);
  ierr = SNESLineSearchRegister(SNESLINESEARCHCP,      SNESLineSearchCreate_CP);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

