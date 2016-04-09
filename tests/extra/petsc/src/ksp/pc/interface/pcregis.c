
#include <petsc/private/pcimpl.h>          /*I   "petscpc.h"   I*/

PETSC_EXTERN PetscErrorCode PCCreate_Jacobi(PC);
PETSC_EXTERN PetscErrorCode PCCreate_BJacobi(PC);
PETSC_EXTERN PetscErrorCode PCCreate_PBJacobi(PC);
PETSC_EXTERN PetscErrorCode PCCreate_ILU(PC);
PETSC_EXTERN PetscErrorCode PCCreate_None(PC);
PETSC_EXTERN PetscErrorCode PCCreate_LU(PC);
PETSC_EXTERN PetscErrorCode PCCreate_SOR(PC);
PETSC_EXTERN PetscErrorCode PCCreate_Shell(PC);
PETSC_EXTERN PetscErrorCode PCCreate_MG(PC);
PETSC_EXTERN PetscErrorCode PCCreate_Eisenstat(PC);
PETSC_EXTERN PetscErrorCode PCCreate_ICC(PC);
PETSC_EXTERN PetscErrorCode PCCreate_ASM(PC);
PETSC_EXTERN PetscErrorCode PCCreate_GASM(PC);
PETSC_EXTERN PetscErrorCode PCCreate_KSP(PC);
PETSC_EXTERN PetscErrorCode PCCreate_Composite(PC);
PETSC_EXTERN PetscErrorCode PCCreate_Redundant(PC);
PETSC_EXTERN PetscErrorCode PCCreate_NN(PC);
PETSC_EXTERN PetscErrorCode PCCreate_Cholesky(PC);
PETSC_EXTERN PetscErrorCode PCCreate_FieldSplit(PC);
PETSC_EXTERN PetscErrorCode PCCreate_Galerkin(PC);
PETSC_EXTERN PetscErrorCode PCCreate_Exotic(PC);
PETSC_EXTERN PetscErrorCode PCCreate_CP(PC);
PETSC_EXTERN PetscErrorCode PCCreate_LSC(PC);
PETSC_EXTERN PetscErrorCode PCCreate_Redistribute(PC);
PETSC_EXTERN PetscErrorCode PCCreate_SVD(PC);
PETSC_EXTERN PetscErrorCode PCCreate_GAMG(PC);
PETSC_EXTERN PetscErrorCode PCCreate_Kaczmarz(PC);
PETSC_EXTERN PetscErrorCode PCCreate_Telescope(PC);

#if defined(PETSC_HAVE_ML)
PETSC_EXTERN PetscErrorCode PCCreate_ML(PC);
#endif
#if defined(PETSC_HAVE_SPAI)
PETSC_EXTERN PetscErrorCode PCCreate_SPAI(PC);
#endif
PETSC_EXTERN PetscErrorCode PCCreate_Mat(PC);
#if defined(PETSC_HAVE_HYPRE)
PETSC_EXTERN PetscErrorCode PCCreate_HYPRE(PC);
PETSC_EXTERN PetscErrorCode PCCreate_PFMG(PC);
PETSC_EXTERN PetscErrorCode PCCreate_SysPFMG(PC);
#endif
#if !defined(PETSC_USE_COMPLEX)
PETSC_EXTERN PetscErrorCode PCCreate_TFS(PC);
#endif
#if defined(PETSC_HAVE_CUSP)
PETSC_EXTERN PetscErrorCode PCCreate_SACUSP(PC);
PETSC_EXTERN PetscErrorCode PCCreate_SACUSPPoly(PC);
PETSC_EXTERN PetscErrorCode PCCreate_BiCGStabCUSP(PC);
PETSC_EXTERN PetscErrorCode PCCreate_AINVCUSP(PC);
#endif
#if defined(PETSC_HAVE_PARMS)
PETSC_EXTERN PetscErrorCode PCCreate_PARMS(PC);
#endif
PETSC_EXTERN PetscErrorCode PCCreate_BDDC(PC);

#undef __FUNCT__
#define __FUNCT__ "PCRegisterAll"
/*@C
   PCRegisterAll - Registers all of the preconditioners in the PC package.

   Not Collective

   Input Parameter:
.  path - the library where the routines are to be found (optional)

   Level: advanced

.keywords: PC, register, all

.seealso: PCRegister(), PCRegisterDestroy()
@*/
PetscErrorCode  PCRegisterAll(void)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (PCRegisterAllCalled) PetscFunctionReturn(0);
  PCRegisterAllCalled = PETSC_TRUE;

  ierr = PCRegister(PCNONE         ,PCCreate_None);CHKERRQ(ierr);
  ierr = PCRegister(PCJACOBI       ,PCCreate_Jacobi);CHKERRQ(ierr);
  ierr = PCRegister(PCPBJACOBI     ,PCCreate_PBJacobi);CHKERRQ(ierr);
  ierr = PCRegister(PCBJACOBI      ,PCCreate_BJacobi);CHKERRQ(ierr);
  ierr = PCRegister(PCSOR          ,PCCreate_SOR);CHKERRQ(ierr);
  ierr = PCRegister(PCLU           ,PCCreate_LU);CHKERRQ(ierr);
  ierr = PCRegister(PCSHELL        ,PCCreate_Shell);CHKERRQ(ierr);
  ierr = PCRegister(PCMG           ,PCCreate_MG);CHKERRQ(ierr);
  ierr = PCRegister(PCEISENSTAT    ,PCCreate_Eisenstat);CHKERRQ(ierr);
  ierr = PCRegister(PCILU          ,PCCreate_ILU);CHKERRQ(ierr);
  ierr = PCRegister(PCICC          ,PCCreate_ICC);CHKERRQ(ierr);
  ierr = PCRegister(PCCHOLESKY     ,PCCreate_Cholesky);CHKERRQ(ierr);
  ierr = PCRegister(PCASM          ,PCCreate_ASM);CHKERRQ(ierr);
  ierr = PCRegister(PCGASM         ,PCCreate_GASM);CHKERRQ(ierr);
  ierr = PCRegister(PCKSP          ,PCCreate_KSP);CHKERRQ(ierr);
  ierr = PCRegister(PCCOMPOSITE    ,PCCreate_Composite);CHKERRQ(ierr);
  ierr = PCRegister(PCREDUNDANT    ,PCCreate_Redundant);CHKERRQ(ierr);
  ierr = PCRegister(PCNN           ,PCCreate_NN);CHKERRQ(ierr);
  ierr = PCRegister(PCMAT          ,PCCreate_Mat);CHKERRQ(ierr);
  ierr = PCRegister(PCFIELDSPLIT   ,PCCreate_FieldSplit);CHKERRQ(ierr);
  ierr = PCRegister(PCGALERKIN     ,PCCreate_Galerkin);CHKERRQ(ierr);
  ierr = PCRegister(PCEXOTIC       ,PCCreate_Exotic);CHKERRQ(ierr);
  ierr = PCRegister(PCCP           ,PCCreate_CP);CHKERRQ(ierr);
  ierr = PCRegister(PCLSC          ,PCCreate_LSC);CHKERRQ(ierr);
  ierr = PCRegister(PCREDISTRIBUTE ,PCCreate_Redistribute);CHKERRQ(ierr);
  ierr = PCRegister(PCSVD          ,PCCreate_SVD);CHKERRQ(ierr);
  ierr = PCRegister(PCGAMG         ,PCCreate_GAMG);CHKERRQ(ierr);
  ierr = PCRegister(PCKACZMARZ     ,PCCreate_Kaczmarz);CHKERRQ(ierr);
  ierr = PCRegister(PCTELESCOPE,PCCreate_Telescope);CHKERRQ(ierr);
#if defined(PETSC_HAVE_ML)
  ierr = PCRegister(PCML           ,PCCreate_ML);CHKERRQ(ierr);
#endif
#if defined(PETSC_HAVE_SPAI)
  ierr = PCRegister(PCSPAI         ,PCCreate_SPAI);CHKERRQ(ierr);
#endif
#if defined(PETSC_HAVE_HYPRE)
  ierr = PCRegister(PCHYPRE        ,PCCreate_HYPRE);CHKERRQ(ierr);
  ierr = PCRegister(PCPFMG         ,PCCreate_PFMG);CHKERRQ(ierr);
  ierr = PCRegister(PCSYSPFMG      ,PCCreate_SysPFMG);CHKERRQ(ierr);
#endif
#if !defined(PETSC_USE_COMPLEX)
  ierr = PCRegister(PCTFS          ,PCCreate_TFS);CHKERRQ(ierr);
#endif
#if defined(PETSC_HAVE_CUSP)
  ierr = PCRegister(PCSACUSP       ,PCCreate_SACUSP);CHKERRQ(ierr);
  ierr = PCRegister(PCAINVCUSP     ,PCCreate_AINVCUSP);CHKERRQ(ierr);
  ierr = PCRegister(PCBICGSTABCUSP ,PCCreate_BiCGStabCUSP);CHKERRQ(ierr);
  ierr = PCRegister(PCSACUSPPOLY   ,PCCreate_SACUSPPoly);CHKERRQ(ierr);
#endif
#if defined(PETSC_HAVE_PARMS)
  ierr = PCRegister(PCPARMS        ,PCCreate_PARMS);CHKERRQ(ierr);
#endif
  ierr = PCRegister(PCBDDC         ,PCCreate_BDDC);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
