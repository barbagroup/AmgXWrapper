#include <../src/tao/complementarity/impls/ssls/ssls.h>

#undef __FUNCT__
#define __FUNCT__ "TaoSetUp_SSILS"
PetscErrorCode TaoSetUp_SSILS(Tao tao)
{
  TAO_SSLS       *ssls = (TAO_SSLS *)tao->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = VecDuplicate(tao->solution,&tao->gradient);CHKERRQ(ierr);
  ierr = VecDuplicate(tao->solution,&tao->stepdirection);CHKERRQ(ierr);
  ierr = VecDuplicate(tao->solution,&ssls->ff);CHKERRQ(ierr);
  ierr = VecDuplicate(tao->solution,&ssls->dpsi);CHKERRQ(ierr);
  ierr = VecDuplicate(tao->solution,&ssls->da);CHKERRQ(ierr);
  ierr = VecDuplicate(tao->solution,&ssls->db);CHKERRQ(ierr);
  ierr = VecDuplicate(tao->solution,&ssls->t1);CHKERRQ(ierr);
  ierr = VecDuplicate(tao->solution,&ssls->t2);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoDestroy_SSILS"
PetscErrorCode TaoDestroy_SSILS(Tao tao)
{
  TAO_SSLS       *ssls = (TAO_SSLS *)tao->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = VecDestroy(&ssls->ff);CHKERRQ(ierr);
  ierr = VecDestroy(&ssls->dpsi);CHKERRQ(ierr);
  ierr = VecDestroy(&ssls->da);CHKERRQ(ierr);
  ierr = VecDestroy(&ssls->db);CHKERRQ(ierr);
  ierr = VecDestroy(&ssls->t1);CHKERRQ(ierr);
  ierr = VecDestroy(&ssls->t2);CHKERRQ(ierr);
  ierr = PetscFree(tao->data);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoSolve_SSILS"
static PetscErrorCode TaoSolve_SSILS(Tao tao)
{
  TAO_SSLS                     *ssls = (TAO_SSLS *)tao->data;
  PetscReal                    psi, ndpsi, normd, innerd, t=0;
  PetscReal                    delta, rho;
  TaoConvergedReason           reason;
  TaoLineSearchConvergedReason ls_reason;
  PetscErrorCode               ierr;

  PetscFunctionBegin;
  /* Assume that Setup has been called!
     Set the structure for the Jacobian and create a linear solver. */
  delta = ssls->delta;
  rho = ssls->rho;

  ierr = TaoComputeVariableBounds(tao);CHKERRQ(ierr);
  ierr = VecMedian(tao->XL,tao->solution,tao->XU,tao->solution);CHKERRQ(ierr);
  ierr = TaoLineSearchSetObjectiveAndGradientRoutine(tao->linesearch,Tao_SSLS_FunctionGradient,tao);CHKERRQ(ierr);
  ierr = TaoLineSearchSetObjectiveRoutine(tao->linesearch,Tao_SSLS_Function,tao);CHKERRQ(ierr);

  /* Calculate the function value and fischer function value at the
     current iterate */
  ierr = TaoLineSearchComputeObjectiveAndGradient(tao->linesearch,tao->solution,&psi,ssls->dpsi);CHKERRQ(ierr);
  ierr = VecNorm(ssls->dpsi,NORM_2,&ndpsi);CHKERRQ(ierr);

  while (PETSC_TRUE) {
    ierr=PetscInfo3(tao, "iter: %D, merit: %g, ndpsi: %g\n",tao->niter, (double)ssls->merit, (double)ndpsi);CHKERRQ(ierr);
    /* Check the termination criteria */
    ierr = TaoMonitor(tao,tao->niter,ssls->merit,ndpsi,0.0,t,&reason);CHKERRQ(ierr);
    if (reason!=TAO_CONTINUE_ITERATING) break;
    tao->niter++;

    /* Calculate direction.  (Really negative of newton direction.  Therefore,
       rest of the code uses -d.) */
    ierr = KSPSetOperators(tao->ksp,tao->jacobian,tao->jacobian_pre);CHKERRQ(ierr);
    ierr = KSPSolve(tao->ksp,ssls->ff,tao->stepdirection);CHKERRQ(ierr);
    ierr = KSPGetIterationNumber(tao->ksp,&tao->ksp_its);CHKERRQ(ierr);
    tao->ksp_tot_its+=tao->ksp_its;
    ierr = VecNorm(tao->stepdirection,NORM_2,&normd);CHKERRQ(ierr);
    ierr = VecDot(tao->stepdirection,ssls->dpsi,&innerd);CHKERRQ(ierr);

    /* Make sure that we have a descent direction */
    if (innerd <= delta*pow(normd, rho)) {
      ierr = PetscInfo(tao, "newton direction not descent\n");CHKERRQ(ierr);
      ierr = VecCopy(ssls->dpsi,tao->stepdirection);CHKERRQ(ierr);
      ierr = VecDot(tao->stepdirection,ssls->dpsi,&innerd);CHKERRQ(ierr);
    }

    ierr = VecScale(tao->stepdirection, -1.0);CHKERRQ(ierr);
    innerd = -innerd;

    ierr = TaoLineSearchSetInitialStepLength(tao->linesearch,1.0);CHKERRQ(ierr);
    ierr = TaoLineSearchApply(tao->linesearch,tao->solution,&psi,ssls->dpsi,tao->stepdirection,&t,&ls_reason);CHKERRQ(ierr);
    ierr = VecNorm(ssls->dpsi,NORM_2,&ndpsi);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

/* ---------------------------------------------------------- */
/*MC
   TAOSSILS - semi-smooth infeasible linesearch algorithm for solving
       complementarity constraints

   Options Database Keys:
+ -tao_ssls_delta - descent test fraction
- -tao_ssls_rho - descent test power

   Level: beginner
M*/
#undef __FUNCT__
#define __FUNCT__ "TaoCreate_SSILS"
PETSC_EXTERN PetscErrorCode TaoCreate_SSILS(Tao tao)
{
  TAO_SSLS       *ssls;
  PetscErrorCode ierr;
  const char     *armijo_type = TAOLINESEARCHARMIJO;

  PetscFunctionBegin;
  ierr = PetscNewLog(tao,&ssls);CHKERRQ(ierr);
  tao->data = (void*)ssls;
  tao->ops->solve=TaoSolve_SSILS;
  tao->ops->setup=TaoSetUp_SSILS;
  tao->ops->view=TaoView_SSLS;
  tao->ops->setfromoptions=TaoSetFromOptions_SSLS;
  tao->ops->destroy = TaoDestroy_SSILS;

  ssls->delta = 1e-10;
  ssls->rho = 2.1;

  ierr = TaoLineSearchCreate(((PetscObject)tao)->comm,&tao->linesearch);CHKERRQ(ierr);
  ierr = TaoLineSearchSetType(tao->linesearch,armijo_type);CHKERRQ(ierr);
  ierr = TaoLineSearchSetOptionsPrefix(tao->linesearch,tao->hdr.prefix);CHKERRQ(ierr);
  ierr = TaoLineSearchSetFromOptions(tao->linesearch);CHKERRQ(ierr);
  /* Note: linesearch objective and objectivegradient routines are set in solve routine */
  ierr = KSPCreate(((PetscObject)tao)->comm,&tao->ksp);CHKERRQ(ierr);
  ierr = KSPSetOptionsPrefix(tao->ksp,tao->hdr.prefix);CHKERRQ(ierr);

  /* Override default settings (unless already changed) */
  if (!tao->max_it_changed) tao->max_it = 2000;
  if (!tao->max_funcs_changed) tao->max_funcs = 4000;
  if (!tao->gttol_changed) tao->gttol = 0;
  if (!tao->grtol_changed) tao->grtol = 0;
#if defined(PETSC_USE_REAL_SINGLE)
  if (!tao->gatol_changed) tao->gatol = 1.0e-6;
  if (!tao->fmin_changed)  tao->fmin = 1.0e-4;
#else
  if (!tao->gatol_changed) tao->gatol = 1.0e-16;
  if (!tao->fmin_changed)  tao->fmin = 1.0e-8;
#endif
  PetscFunctionReturn(0);
}


