#include <../src/snes/impls/ngmres/snesngmres.h> /*I "petscsnes.h" I*/

extern PetscErrorCode SNESDestroy_NGMRES(SNES);
extern PetscErrorCode SNESReset_NGMRES(SNES);
extern PetscErrorCode SNESSetUp_NGMRES(SNES);
extern PetscErrorCode SNESView_NGMRES(SNES,PetscViewer);

PETSC_EXTERN const char *const SNESNGMRESRestartTypes[];

#undef __FUNCT__
#define __FUNCT__ "SNESSetFromOptions_Anderson"
PetscErrorCode SNESSetFromOptions_Anderson(PetscOptionItems *PetscOptionsObject,SNES snes)
{
  SNES_NGMRES    *ngmres = (SNES_NGMRES*) snes->data;
  PetscErrorCode ierr;
  PetscBool      debug;
  SNESLineSearch linesearch;

  PetscFunctionBegin;
  ierr = PetscOptionsHead(PetscOptionsObject,"SNES NGMRES options");CHKERRQ(ierr);
  ierr = PetscOptionsInt("-snes_anderson_m",            "Number of directions","SNES",ngmres->msize,&ngmres->msize,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsReal("-snes_anderson_beta",        "Mixing parameter","SNES",ngmres->andersonBeta,&ngmres->andersonBeta,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsBool("-snes_anderson_monitor",     "Monitor steps of Anderson Mixing","SNES",ngmres->monitor ? PETSC_TRUE : PETSC_FALSE,&debug,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsInt("-snes_anderson_restart",      "Iterations before forced restart", "SNES",ngmres->restart_periodic,&ngmres->restart_periodic,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsInt("-snes_anderson_restart_it",   "Tolerance iterations before restart","SNES",ngmres->restart_it,&ngmres->restart_it,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsEnum("-snes_anderson_restart_type","Restart type","SNESNGMRESSetRestartType",SNESNGMRESRestartTypes,
                          (PetscEnum)ngmres->restart_type,(PetscEnum*)&ngmres->restart_type,NULL);CHKERRQ(ierr);
  if (debug) {
    ngmres->monitor = PETSC_VIEWER_STDOUT_(PetscObjectComm((PetscObject)snes));CHKERRQ(ierr);
  }
  ierr = PetscOptionsTail();CHKERRQ(ierr);
  /* set the default type of the line search if the user hasn't already. */
  if (!snes->linesearch) {
    ierr = SNESGetLineSearch(snes,&linesearch);CHKERRQ(ierr);
    ierr = SNESLineSearchSetType(linesearch,SNESLINESEARCHBASIC);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SNESSolve_Anderson"
PetscErrorCode SNESSolve_Anderson(SNES snes)
{
  SNES_NGMRES         *ngmres = (SNES_NGMRES*) snes->data;
  /* present solution, residual, and preconditioned residual */
  Vec                 X,F,B,D;
  /* candidate linear combination answers */
  Vec                 XA,FA,XM,FM;

  /* coefficients and RHS to the minimization problem */
  PetscReal           fnorm,fMnorm,fAnorm;
  PetscReal           xnorm,ynorm;
  PetscReal           dnorm,dminnorm=0.0,fminnorm;
  PetscInt            restart_count=0;
  PetscInt            k,k_restart,l,ivec;
  PetscBool           selectRestart;
  SNESConvergedReason reason;
  PetscErrorCode      ierr;

  PetscFunctionBegin;
  if (snes->xl || snes->xu || snes->ops->computevariablebounds) SETERRQ1(PetscObjectComm((PetscObject)snes),PETSC_ERR_ARG_WRONGSTATE, "SNES solver %s does not support bounds", ((PetscObject)snes)->type_name);

  ierr = PetscCitationsRegister(SNESCitation,&SNEScite);CHKERRQ(ierr);
  /* variable initialization */
  snes->reason = SNES_CONVERGED_ITERATING;
  X            = snes->vec_sol;
  F            = snes->vec_func;
  B            = snes->vec_rhs;
  XA           = snes->vec_sol_update;
  FA           = snes->work[0];
  D            = snes->work[1];

  /* work for the line search */
  XM = snes->work[3];
  FM = snes->work[4];

  ierr       = PetscObjectSAWsTakeAccess((PetscObject)snes);CHKERRQ(ierr);
  snes->iter = 0;
  snes->norm = 0.;
  ierr       = PetscObjectSAWsGrantAccess((PetscObject)snes);CHKERRQ(ierr);

  /* initialization */

  /* r = F(x) */

  if (snes->pc && snes->pcside == PC_LEFT) {
    ierr = SNESApplyNPC(snes,X,NULL,F);CHKERRQ(ierr);
    ierr = SNESGetConvergedReason(snes->pc,&reason);CHKERRQ(ierr);
    if (reason < 0  && reason != SNES_DIVERGED_MAX_IT) {
      snes->reason = SNES_DIVERGED_INNER;
      PetscFunctionReturn(0);
    }
    ierr = VecNorm(F,NORM_2,&fnorm);CHKERRQ(ierr);
  } else {
    if (!snes->vec_func_init_set) {
      ierr = SNESComputeFunction(snes,X,F);CHKERRQ(ierr);
    } else snes->vec_func_init_set = PETSC_FALSE;

    ierr = VecNorm(F,NORM_2,&fnorm);CHKERRQ(ierr);
    SNESCheckFunctionNorm(snes,fnorm);
  }
  fminnorm = fnorm;

  ierr       = PetscObjectSAWsTakeAccess((PetscObject)snes);CHKERRQ(ierr);
  snes->norm = fnorm;
  ierr       = PetscObjectSAWsGrantAccess((PetscObject)snes);CHKERRQ(ierr);
  ierr       = SNESLogConvergenceHistory(snes,fnorm,0);CHKERRQ(ierr);
  ierr       = SNESMonitor(snes,0,fnorm);CHKERRQ(ierr);
  ierr       = (*snes->ops->converged)(snes,0,0.0,0.0,fnorm,&snes->reason,snes->cnvP);CHKERRQ(ierr);
  if (snes->reason) PetscFunctionReturn(0);

  k_restart = 0;
  l         = 0;
  ivec      = 0;
  for (k=1; k < snes->max_its+1; k++) {
    /* select which vector of the stored subspace will be updated */
    if (snes->pc && snes->pcside == PC_RIGHT) {
      ierr = VecCopy(X,XM);CHKERRQ(ierr);
      ierr = SNESSetInitialFunction(snes->pc,F);CHKERRQ(ierr);

      ierr = PetscLogEventBegin(SNES_NPCSolve,snes->pc,XM,B,0);CHKERRQ(ierr);
      ierr = SNESSolve(snes->pc,B,XM);CHKERRQ(ierr);
      ierr = PetscLogEventEnd(SNES_NPCSolve,snes->pc,XM,B,0);CHKERRQ(ierr);

      ierr = SNESGetConvergedReason(snes->pc,&reason);CHKERRQ(ierr);
      if (reason < 0 && reason != SNES_DIVERGED_MAX_IT) {
        snes->reason = SNES_DIVERGED_INNER;
        PetscFunctionReturn(0);
      }
      ierr = SNESGetNPCFunction(snes,FM,&fMnorm);CHKERRQ(ierr);
      if (ngmres->andersonBeta != 1.0) {
        VecAXPBY(XM,(1.0 - ngmres->andersonBeta),ngmres->andersonBeta,X);CHKERRQ(ierr);
      }
    } else {
      ierr   = VecCopy(F,FM);CHKERRQ(ierr);
      ierr   = VecCopy(X,XM);CHKERRQ(ierr);
      ierr   = VecAXPY(XM,-ngmres->andersonBeta,FM);CHKERRQ(ierr);
      fMnorm = fnorm;
    }

    ierr = SNESNGMRESFormCombinedSolution_Private(snes,ivec,l,XM,FM,fMnorm,X,XA,FA);CHKERRQ(ierr);
    ivec = k_restart % ngmres->msize;
    if (ngmres->restart_type == SNES_NGMRES_RESTART_DIFFERENCE) {
      ierr = SNESNGMRESNorms_Private(snes,l,X,F,XM,FM,XA,FA,D,&dnorm,&dminnorm,NULL,NULL,NULL,&xnorm,&fAnorm,&ynorm);CHKERRQ(ierr);
      ierr = SNESNGMRESSelectRestart_Private(snes,l,fMnorm,fnorm,dnorm,fminnorm,dminnorm,&selectRestart);CHKERRQ(ierr);
      /* if the restart conditions persist for more than restart_it iterations, restart. */
      if (selectRestart) restart_count++;
      else restart_count = 0;
    } else if (ngmres->restart_type == SNES_NGMRES_RESTART_PERIODIC) {
      ierr = SNESNGMRESNorms_Private(snes,l,X,F,XM,FM,XA,FA,D,NULL,NULL,NULL,NULL,NULL,&xnorm,&fAnorm,&ynorm);CHKERRQ(ierr);
      if (k_restart > ngmres->restart_periodic) {
        if (ngmres->monitor) ierr = PetscViewerASCIIPrintf(ngmres->monitor,"periodic restart after %D iterations\n",k_restart);CHKERRQ(ierr);
        restart_count = ngmres->restart_it;
      }
    } else {
      ierr = SNESNGMRESNorms_Private(snes,l,X,F,XM,FM,XA,FA,D,NULL,NULL,NULL,NULL,NULL,&xnorm,&fAnorm,&ynorm);CHKERRQ(ierr);
    }
    /* restart after restart conditions have persisted for a fixed number of iterations */
    if (restart_count >= ngmres->restart_it) {
      if (ngmres->monitor) {
        ierr = PetscViewerASCIIPrintf(ngmres->monitor,"Restarted at iteration %d\n",k_restart);CHKERRQ(ierr);
      }
      restart_count = 0;
      k_restart     = 0;
      l             = 0;
      ivec          = 0;
    } else {
      if (l < ngmres->msize) l++;
      k_restart++;
      ierr = SNESNGMRESUpdateSubspace_Private(snes,ivec,l,FM,fnorm,XM);CHKERRQ(ierr);
    }

    fnorm = fAnorm;
    if (fminnorm > fnorm) fminnorm = fnorm;

    ierr = VecCopy(XA,X);CHKERRQ(ierr);
    ierr = VecCopy(FA,F);CHKERRQ(ierr);

    ierr       = PetscObjectSAWsTakeAccess((PetscObject)snes);CHKERRQ(ierr);
    snes->iter = k;
    snes->norm = fnorm;
    ierr       = PetscObjectSAWsGrantAccess((PetscObject)snes);CHKERRQ(ierr);
    ierr       = SNESLogConvergenceHistory(snes,snes->norm,snes->iter);CHKERRQ(ierr);
    ierr       = SNESMonitor(snes,snes->iter,snes->norm);CHKERRQ(ierr);
    ierr       = (*snes->ops->converged)(snes,snes->iter,xnorm,ynorm,fnorm,&snes->reason,snes->cnvP);CHKERRQ(ierr);
    if (snes->reason) PetscFunctionReturn(0);
  }
  snes->reason = SNES_DIVERGED_MAX_IT;
  PetscFunctionReturn(0);
}

/*MC
  SNESANDERSON - Anderson Mixing method.

   Level: beginner

   Options Database:
+  -snes_anderson_m                - Number of stored previous solutions and residuals
.  -snes_anderson_beta             - Relaxation parameter; X_{update} = X + \beta F
.  -snes_anderson_restart_type     - Type of restart (see SNESNGMRES)
.  -snes_anderson_restart_it       - Number of iterations of restart conditions before restart
.  -snes_anderson_restart          - Number of iterations before periodic restart
-  -snes_anderson_monitor          - Prints relevant information about the ngmres iteration

   Notes:

   The Anderson Mixing method combines m previous solutions into a minimum-residual solution by solving a small linearized
   optimization problem at each iteration.

   Very similar to the SNESNGMRES algorithm.

   References:
+  1. -  D. G. Anderson. Iterative procedures for nonlinear integral equations.
    J. Assoc. Comput. Mach., 12, 1965."
-  2. - Peter R. Brune, Matthew G. Knepley, Barry F. Smith, and Xuemin Tu,"Composing Scalable Nonlinear Algebraic Solvers", 
   SIAM Review, 57(4), 2015


.seealso: SNESNGMRES, SNESCreate(), SNES, SNESSetType(), SNESType (for list of available types)
M*/

#undef __FUNCT__
#define __FUNCT__ "SNESCreate_Anderson"
PETSC_EXTERN PetscErrorCode SNESCreate_Anderson(SNES snes)
{
  SNES_NGMRES    *ngmres;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  snes->ops->destroy        = SNESDestroy_NGMRES;
  snes->ops->setup          = SNESSetUp_NGMRES;
  snes->ops->setfromoptions = SNESSetFromOptions_Anderson;
  snes->ops->view           = SNESView_NGMRES;
  snes->ops->solve          = SNESSolve_Anderson;
  snes->ops->reset          = SNESReset_NGMRES;

  snes->usespc  = PETSC_TRUE;
  snes->usesksp = PETSC_FALSE;
  snes->pcside  = PC_RIGHT;

  ierr          = PetscNewLog(snes,&ngmres);CHKERRQ(ierr);
  snes->data    = (void*) ngmres;
  ngmres->msize = 30;

  if (!snes->tolerancesset) {
    snes->max_funcs = 30000;
    snes->max_its   = 10000;
  }

  ngmres->additive_linesearch = NULL;
  ngmres->approxfunc       = PETSC_FALSE;
  ngmres->restart_type     = SNES_NGMRES_RESTART_NONE;
  ngmres->restart_it       = 2;
  ngmres->restart_periodic = 30;
  ngmres->gammaA           = 2.0;
  ngmres->gammaC           = 2.0;
  ngmres->deltaB           = 0.9;
  ngmres->epsilonB         = 0.1;

  ngmres->andersonBeta = 1.0;
  PetscFunctionReturn(0);
}
