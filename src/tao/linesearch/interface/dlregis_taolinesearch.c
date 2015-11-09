#define TAOLINESEARCH_DLL
#include <petsc/private/taolinesearchimpl.h>


PETSC_EXTERN PetscErrorCode TaoLineSearchCreate_Unit(TaoLineSearch);
PETSC_EXTERN PetscErrorCode TaoLineSearchCreate_MT(TaoLineSearch);
PETSC_EXTERN PetscErrorCode TaoLineSearchCreate_GPCG(TaoLineSearch);
PETSC_EXTERN PetscErrorCode TaoLineSearchCreate_Armijo(TaoLineSearch);
PETSC_EXTERN PetscErrorCode TaoLineSearchCreate_OWArmijo(TaoLineSearch);
static PetscBool TaoLineSearchPackageInitialized = PETSC_FALSE;

#undef __FUNCT__
#define __FUNCT__ "TaoLineSearchFinalizePackage"
/*@C
  TaoLineSearchFinalizePackage - This function destroys everything in the PETSc/TAO
  interface to the TaoLineSearch package. It is called from PetscFinalize().

  Level: developer
@*/
PetscErrorCode TaoLineSearchFinalizePackage(void)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscFunctionListDestroy(&TaoLineSearchList);CHKERRQ(ierr);
  TaoLineSearchPackageInitialized = PETSC_FALSE;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoLineSearchInitializePackage"
/*@C
  TaoLineSearchInitializePackage - This function registers the line-search
  algorithms in TAO.
  When using static libraries, this function is called from the
  first entry to TaoCreate(); when using shared libraries, it is called
  from PetscDLLibraryRegister()

  Level: developer

.seealso: TaoLineSearchCreate()
@*/
PetscErrorCode TaoLineSearchInitializePackage(void)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (TaoLineSearchPackageInitialized) PetscFunctionReturn(0);
  TaoLineSearchPackageInitialized=PETSC_TRUE;
#if !defined(PETSC_USE_COMPLEX)
  ierr = PetscClassIdRegister("TaoLineSearch",&TAOLINESEARCH_CLASSID);CHKERRQ(ierr);
  ierr = TaoLineSearchRegister("unit",TaoLineSearchCreate_Unit);CHKERRQ(ierr);
  ierr = TaoLineSearchRegister("more-thuente",TaoLineSearchCreate_MT);CHKERRQ(ierr);
  ierr = TaoLineSearchRegister("gpcg",TaoLineSearchCreate_GPCG);CHKERRQ(ierr);
  ierr = TaoLineSearchRegister("armijo",TaoLineSearchCreate_Armijo);CHKERRQ(ierr);
  ierr = TaoLineSearchRegister("owarmijo",TaoLineSearchCreate_OWArmijo);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("TaoLineSearchApply",TAOLINESEARCH_CLASSID,&TaoLineSearch_ApplyEvent);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("TaoLineSearchComputeObjective[Gradient]",TAOLINESEARCH_CLASSID,&TaoLineSearch_EvalEvent);CHKERRQ(ierr);
#endif
  ierr = PetscRegisterFinalize(TaoLineSearchFinalizePackage);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}



