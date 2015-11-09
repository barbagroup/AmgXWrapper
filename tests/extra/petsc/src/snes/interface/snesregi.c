
#include <petsc/private/snesimpl.h>     /*I  "petscsnes.h"  I*/

PETSC_EXTERN PetscErrorCode SNESCreate_NEWTONLS(SNES);
PETSC_EXTERN PetscErrorCode SNESCreate_NEWTONTR(SNES);
PETSC_EXTERN PetscErrorCode SNESCreate_Test(SNES);
PETSC_EXTERN PetscErrorCode SNESCreate_NRichardson(SNES);
PETSC_EXTERN PetscErrorCode SNESCreate_KSPONLY(SNES);
PETSC_EXTERN PetscErrorCode SNESCreate_VINEWTONRSLS(SNES);
PETSC_EXTERN PetscErrorCode SNESCreate_VINEWTONSSLS(SNES);
PETSC_EXTERN PetscErrorCode SNESCreate_NGMRES(SNES);
PETSC_EXTERN PetscErrorCode SNESCreate_QN(SNES);
PETSC_EXTERN PetscErrorCode SNESCreate_Shell(SNES);
PETSC_EXTERN PetscErrorCode SNESCreate_NGS(SNES);
PETSC_EXTERN PetscErrorCode SNESCreate_NCG(SNES);
PETSC_EXTERN PetscErrorCode SNESCreate_FAS(SNES);
PETSC_EXTERN PetscErrorCode SNESCreate_MS(SNES);
PETSC_EXTERN PetscErrorCode SNESCreate_NASM(SNES);
PETSC_EXTERN PetscErrorCode SNESCreate_Anderson(SNES);
PETSC_EXTERN PetscErrorCode SNESCreate_ASPIN(SNES);
PETSC_EXTERN PetscErrorCode SNESCreate_Composite(SNES);

const char *SNESConvergedReasons_Shifted[] = {" "," ","DIVERGED_LOCAL_MIN","DIVERGED_INNER","DIVERGED_LINE_SEARCH","DIVERGED_MAX_IT",
                                              "DIVERGED_FNORM_NAN","DIVERGED_LINEAR_SOLVE","DIVERGED_FUNCTION_COUNT","DIVERGED_FUNCTION_DOMAIN",
                                              "CONVERGED_ITERATING"," ","CONVERGED_FNORM_ABS","CONVERGED_FNORM_RELATIVE",
                                              "CONVERGED_SNORM_RELATIVE","CONVERGED_ITS"," ","CONVERGED_TR_DELTA","SNESConvergedReason","",0};
const char *const *SNESConvergedReasons = SNESConvergedReasons_Shifted + 10;

const char *SNESNormSchedules_Shifted[]    = {"DEFAULT","NONE","ALWAYS","INITIALONLY","FINALONLY","INITIALFINALONLY","SNESNormSchedule","SNES_NORM_",0};
const char *const *const SNESNormSchedules = SNESNormSchedules_Shifted + 1;

const char *SNESFunctionTypes_Shifted[]    = {"DEFAULT","UNPRECONDITIONED","PRECONDITIONED","SNESFunctionType","SNES_FUNCTION_",0};
const char *const *const SNESFunctionTypes = SNESFunctionTypes_Shifted + 1;

/*
      This is used by SNESSetType() to make sure that at least one
    SNESRegisterAll() is called. In general, if there is more than one
    DLL then SNESRegisterAll() may be called several times.
*/
extern PetscBool SNESRegisterAllCalled;

#undef __FUNCT__
#define __FUNCT__ "SNESRegisterAll"
/*@C
   SNESRegisterAll - Registers all of the nonlinear solver methods in the SNES package.

   Not Collective

   Level: advanced

.keywords: SNES, register, all

.seealso:  SNESRegisterDestroy()
@*/
PetscErrorCode  SNESRegisterAll(void)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (SNESRegisterAllCalled) PetscFunctionReturn(0);
  SNESRegisterAllCalled = PETSC_TRUE;

  ierr = SNESRegister(SNESNEWTONLS,     SNESCreate_NEWTONLS);CHKERRQ(ierr);
  ierr = SNESRegister(SNESNEWTONTR,     SNESCreate_NEWTONTR);CHKERRQ(ierr);
  ierr = SNESRegister(SNESTEST,         SNESCreate_Test);CHKERRQ(ierr);
  ierr = SNESRegister(SNESNRICHARDSON,  SNESCreate_NRichardson);CHKERRQ(ierr);
  ierr = SNESRegister(SNESKSPONLY,      SNESCreate_KSPONLY);CHKERRQ(ierr);
  ierr = SNESRegister(SNESVINEWTONRSLS, SNESCreate_VINEWTONRSLS);CHKERRQ(ierr);
  ierr = SNESRegister(SNESVINEWTONSSLS, SNESCreate_VINEWTONSSLS);CHKERRQ(ierr);
  ierr = SNESRegister(SNESNGMRES,       SNESCreate_NGMRES);CHKERRQ(ierr);
  ierr = SNESRegister(SNESQN,           SNESCreate_QN);CHKERRQ(ierr);
  ierr = SNESRegister(SNESSHELL,        SNESCreate_Shell);CHKERRQ(ierr);
  ierr = SNESRegister(SNESNGS,          SNESCreate_NGS);CHKERRQ(ierr);
  ierr = SNESRegister(SNESNCG,          SNESCreate_NCG);CHKERRQ(ierr);
  ierr = SNESRegister(SNESFAS,          SNESCreate_FAS);CHKERRQ(ierr);
  ierr = SNESRegister(SNESMS,           SNESCreate_MS);CHKERRQ(ierr);
  ierr = SNESRegister(SNESNASM,         SNESCreate_NASM);CHKERRQ(ierr);
  ierr = SNESRegister(SNESANDERSON,     SNESCreate_Anderson);CHKERRQ(ierr);
  ierr = SNESRegister(SNESASPIN,        SNESCreate_ASPIN);CHKERRQ(ierr);
  ierr = SNESRegister(SNESCOMPOSITE,    SNESCreate_Composite);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
