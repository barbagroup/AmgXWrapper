#include <petsc/private/snesimpl.h>             /*I   "petscsnes.h"   I*/
#include <petscdm.h>

#undef __FUNCT__
#define __FUNCT__ "MatMultASPIN"
PetscErrorCode MatMultASPIN(Mat m,Vec X,Vec Y)
{
  PetscErrorCode ierr;
  void           *ctx;
  SNES           snes;
  PetscInt       n,i;
  VecScatter     *oscatter;
  SNES           *subsnes;
  PetscBool      match;
  MPI_Comm       comm;
  KSP            ksp;
  Vec            *x,*b;
  Vec            W;
  SNES           npc;
  Mat            subJ,subpJ;

  PetscFunctionBegin;
  ierr = MatShellGetContext(m,&ctx);CHKERRQ(ierr);
  snes = (SNES)ctx;
  ierr = SNESGetNPC(snes,&npc);CHKERRQ(ierr);
  ierr = SNESGetFunction(npc,&W,NULL,NULL);CHKERRQ(ierr);
  ierr = PetscObjectTypeCompare((PetscObject)npc,SNESNASM,&match);CHKERRQ(ierr);
  if (!match) {
    ierr = PetscObjectGetComm((PetscObject)snes,&comm);CHKERRQ(ierr);
    SETERRQ(comm,PETSC_ERR_ARG_WRONGSTATE,"MatMultASPIN requires that the nonlinear preconditioner be Nonlinear additive Schwarz");
  }
  ierr = SNESNASMGetSubdomains(npc,&n,&subsnes,NULL,&oscatter,NULL);CHKERRQ(ierr);
  ierr = SNESNASMGetSubdomainVecs(npc,&n,&x,&b,NULL,NULL);CHKERRQ(ierr);

  ierr = VecSet(Y,0);CHKERRQ(ierr);
  ierr = MatMult(npc->jacobian_pre,X,W);CHKERRQ(ierr);

  for (i=0;i<n;i++) {
    ierr = VecScatterBegin(oscatter[i],W,b[i],INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
  }
  for (i=0;i<n;i++) {
    ierr = VecScatterEnd(oscatter[i],W,b[i],INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
    ierr = VecSet(x[i],0.);CHKERRQ(ierr);
    ierr = SNESGetJacobian(subsnes[i],&subJ,&subpJ,NULL,NULL);CHKERRQ(ierr);
    ierr = SNESGetKSP(subsnes[i],&ksp);CHKERRQ(ierr);
    ierr = KSPSetOperators(ksp,subJ,subpJ);CHKERRQ(ierr);
    ierr = KSPSolve(ksp,b[i],x[i]);CHKERRQ(ierr);
    ierr = VecScatterBegin(oscatter[i],x[i],Y,ADD_VALUES,SCATTER_REVERSE);CHKERRQ(ierr);
  }
  for (i=0;i<n;i++) {
    ierr = VecScatterEnd(oscatter[i],x[i],Y,ADD_VALUES,SCATTER_REVERSE);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SNESDestroy_ASPIN"
PetscErrorCode SNESDestroy_ASPIN(SNES snes)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = SNESDestroy(&snes->pc);CHKERRQ(ierr);
  /* reset NEWTONLS and free the data */
  ierr = SNESReset(snes);CHKERRQ(ierr);
  ierr = PetscFree(snes->data);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SNESCreate_ASPIN"
/* -------------------------------------------------------------------------- */
/*MC
      SNESASPIN - Helper SNES type for Additive-Schwarz Preconditioned Inexact Newton

   Options Database:
+  -npc_snes_ - options prefix of the nonlinear subdomain solver (must be of type NASM)
.  -npc_sub_snes_ - options prefix of the subdomain nonlinear solves
.  -npc_sub_ksp_ - options prefix of the subdomain Krylov solver
-  -npc_sub_pc_ - options prefix of the subdomain preconditioner

    Notes: This routine sets up an instance of NETWONLS with nonlinear left preconditioning.  It differs from other
    similar functionality in SNES as it creates a linear shell matrix that corresponds to the product:

    \sum_{i=0}^{N_b}J_b({X^b_{converged}})^{-1}J(X + \sum_{i=0}^{N_b}(X^b_{converged} - X^b))

    which is the ASPIN preconditioned matrix. Similar solvers may be constructed by having matrix-free differencing of
    nonlinear solves per linear iteration, but this is far more efficient when subdomain sparse-direct preconditioner
    factorizations are reused on each application of J_b^{-1}.

   Level: intermediate

.seealso:  SNESCreate(), SNES, SNESSetType(), SNESNEWTONLS, SNESNASM, SNESGetNPC(), SNESGetNPCSide()

M*/
PETSC_EXTERN PetscErrorCode SNESCreate_ASPIN(SNES snes)
{
  PetscErrorCode ierr;
  SNES           npc;
  KSP            ksp;
  PC             pc;
  Mat            aspinmat;
  Vec            F;
  PetscInt       n;
  SNESLineSearch linesearch;

  PetscFunctionBegin;
  /* set up the solver */
  ierr = SNESSetType(snes,SNESNEWTONLS);CHKERRQ(ierr);
  ierr = SNESSetNPCSide(snes,PC_LEFT);CHKERRQ(ierr);
  ierr = SNESSetFunctionType(snes,SNES_FUNCTION_PRECONDITIONED);CHKERRQ(ierr);
  ierr = SNESGetNPC(snes,&npc);CHKERRQ(ierr);
  ierr = SNESSetType(npc,SNESNASM);CHKERRQ(ierr);
  ierr = SNESNASMSetType(npc,PC_ASM_BASIC);CHKERRQ(ierr);
  ierr = SNESNASMSetComputeFinalJacobian(npc,PETSC_TRUE);CHKERRQ(ierr);
  ierr = SNESGetKSP(snes,&ksp);CHKERRQ(ierr);
  ierr = KSPGetPC(ksp,&pc);CHKERRQ(ierr);
  ierr = PCSetType(pc,PCNONE);CHKERRQ(ierr);
  ierr = SNESGetLineSearch(snes,&linesearch);CHKERRQ(ierr);
  ierr = SNESLineSearchSetType(linesearch,SNESLINESEARCHBT);CHKERRQ(ierr);

  /* set up the shell matrix */
  ierr = SNESGetFunction(snes,&F,NULL,NULL);CHKERRQ(ierr);
  ierr = VecGetLocalSize(F,&n);CHKERRQ(ierr);
  ierr = MatCreateShell(PetscObjectComm((PetscObject)snes),n,n,PETSC_DECIDE,PETSC_DECIDE,snes,&aspinmat);CHKERRQ(ierr);
  ierr = MatSetType(aspinmat,MATSHELL);CHKERRQ(ierr);
  ierr = MatShellSetOperation(aspinmat,MATOP_MULT,(void(*)(void))MatMultASPIN);CHKERRQ(ierr);
  ierr = SNESSetJacobian(snes,aspinmat,NULL,NULL,NULL);CHKERRQ(ierr);
  ierr = MatDestroy(&aspinmat);CHKERRQ(ierr);

  snes->ops->destroy = SNESDestroy_ASPIN;

  PetscFunctionReturn(0);
}
