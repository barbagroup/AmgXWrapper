#include <../src/snes/impls/richardson/snesrichardsonimpl.h>


#undef __FUNCT__
#define __FUNCT__ "SNESReset_NRichardson"
PetscErrorCode SNESReset_NRichardson(SNES snes)
{
  PetscFunctionBegin;
  PetscFunctionReturn(0);
}

/*
  SNESDestroy_NRichardson - Destroys the private SNES_NRichardson context that was created with SNESCreate_NRichardson().

  Input Parameter:
. snes - the SNES context

  Application Interface Routine: SNESDestroy()
*/
#undef __FUNCT__
#define __FUNCT__ "SNESDestroy_NRichardson"
PetscErrorCode SNESDestroy_NRichardson(SNES snes)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = SNESReset_NRichardson(snes);CHKERRQ(ierr);
  ierr = PetscFree(snes->data);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/*
   SNESSetUp_NRichardson - Sets up the internal data structures for the later use
   of the SNESNRICHARDSON nonlinear solver.

   Input Parameters:
+  snes - the SNES context
-  x - the solution vector

   Application Interface Routine: SNESSetUp()
 */
#undef __FUNCT__
#define __FUNCT__ "SNESSetUp_NRichardson"
PetscErrorCode SNESSetUp_NRichardson(SNES snes)
{
  PetscFunctionBegin;
  if (snes->pcside == PC_RIGHT) {SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONGSTATE,"NRichardson only supports left preconditioning");}
  if (snes->functype == SNES_FUNCTION_DEFAULT) snes->functype = SNES_FUNCTION_UNPRECONDITIONED;
  PetscFunctionReturn(0);
}

/*
  SNESSetFromOptions_NRichardson - Sets various parameters for the SNESNEWTONLS method.

  Input Parameter:
. snes - the SNES context

  Application Interface Routine: SNESSetFromOptions()
*/
#undef __FUNCT__
#define __FUNCT__ "SNESSetFromOptions_NRichardson"
static PetscErrorCode SNESSetFromOptions_NRichardson(PetscOptionItems *PetscOptionsObject,SNES snes)
{
  PetscErrorCode ierr;
  SNESLineSearch linesearch;

  PetscFunctionBegin;
  ierr = PetscOptionsHead(PetscOptionsObject,"SNES Richardson options");CHKERRQ(ierr);
  ierr = PetscOptionsTail();CHKERRQ(ierr);
  if (!snes->linesearch) {
    ierr = SNESGetLineSearch(snes, &linesearch);CHKERRQ(ierr);
    ierr = SNESLineSearchSetType(linesearch, SNESLINESEARCHL2);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

/*
  SNESView_NRichardson - Prints info from the SNESRichardson data structure.

  Input Parameters:
+ SNES - the SNES context
- viewer - visualization context

  Application Interface Routine: SNESView()
*/
#undef __FUNCT__
#define __FUNCT__ "SNESView_NRichardson"
static PetscErrorCode SNESView_NRichardson(SNES snes, PetscViewer viewer)
{
  PetscBool      iascii;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscObjectTypeCompare((PetscObject) viewer, PETSCVIEWERASCII, &iascii);CHKERRQ(ierr);
  if (iascii) {
  }
  PetscFunctionReturn(0);
}

/*
  SNESSolve_NRichardson - Solves a nonlinear system with the Richardson method.

  Input Parameters:
. snes - the SNES context

  Output Parameter:
. outits - number of iterations until termination

  Application Interface Routine: SNESSolve()
*/
#undef __FUNCT__
#define __FUNCT__ "SNESSolve_NRichardson"
PetscErrorCode SNESSolve_NRichardson(SNES snes)
{
  Vec                  X, Y, F;
  PetscReal            xnorm, fnorm, ynorm;
  PetscInt             maxits, i;
  PetscErrorCode       ierr;
  SNESLineSearchReason lsresult;
  SNESConvergedReason  reason;

  PetscFunctionBegin;
  if (snes->xl || snes->xu || snes->ops->computevariablebounds) SETERRQ1(PetscObjectComm((PetscObject)snes),PETSC_ERR_ARG_WRONGSTATE, "SNES solver %s does not support bounds", ((PetscObject)snes)->type_name);

  snes->reason = SNES_CONVERGED_ITERATING;

  maxits = snes->max_its;        /* maximum number of iterations */
  X      = snes->vec_sol;        /* X^n */
  Y      = snes->vec_sol_update; /* \tilde X */
  F      = snes->vec_func;       /* residual vector */

  ierr       = PetscObjectSAWsTakeAccess((PetscObject)snes);CHKERRQ(ierr);
  snes->iter = 0;
  snes->norm = 0.;
  ierr       = PetscObjectSAWsGrantAccess((PetscObject)snes);CHKERRQ(ierr);

  if (snes->pc && snes->functype == SNES_FUNCTION_PRECONDITIONED) {
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
  if (snes->pc && snes->functype == SNES_FUNCTION_UNPRECONDITIONED) {
      ierr = SNESApplyNPC(snes,X,F,Y);CHKERRQ(ierr);
      ierr = SNESGetConvergedReason(snes->pc,&reason);CHKERRQ(ierr);
      if (reason < 0  && reason != SNES_DIVERGED_MAX_IT) {
        snes->reason = SNES_DIVERGED_INNER;
        PetscFunctionReturn(0);
      }
  } else {
    ierr = VecCopy(F,Y);CHKERRQ(ierr);
  }

  ierr       = PetscObjectSAWsTakeAccess((PetscObject)snes);CHKERRQ(ierr);
  snes->norm = fnorm;
  ierr       = PetscObjectSAWsGrantAccess((PetscObject)snes);CHKERRQ(ierr);
  ierr       = SNESLogConvergenceHistory(snes,fnorm,0);CHKERRQ(ierr);
  ierr       = SNESMonitor(snes,0,fnorm);CHKERRQ(ierr);

  /* test convergence */
  ierr = (*snes->ops->converged)(snes,0,0.0,0.0,fnorm,&snes->reason,snes->cnvP);CHKERRQ(ierr);
  if (snes->reason) PetscFunctionReturn(0);

  /* Call general purpose update function */
  if (snes->ops->update) {
    ierr = (*snes->ops->update)(snes, snes->iter);CHKERRQ(ierr);
  }

  /* set parameter for default relative tolerance convergence test */
  snes->ttol = fnorm*snes->rtol;
  /* test convergence */
  ierr = (*snes->ops->converged)(snes,0,0.0,0.0,fnorm,&snes->reason,snes->cnvP);CHKERRQ(ierr);
  if (snes->reason) PetscFunctionReturn(0);

  for (i = 1; i < maxits+1; i++) {
    ierr = SNESLineSearchApply(snes->linesearch, X, F, &fnorm, Y);CHKERRQ(ierr);
    ierr = SNESLineSearchGetReason(snes->linesearch, &lsresult);CHKERRQ(ierr);
    ierr = SNESLineSearchGetNorms(snes->linesearch, &xnorm, &fnorm, &ynorm);CHKERRQ(ierr);
    if (lsresult) {
      if (++snes->numFailures >= snes->maxFailures) {
        snes->reason = SNES_DIVERGED_LINE_SEARCH;
        break;
      }
    }
    if (snes->nfuncs >= snes->max_funcs) {
      snes->reason = SNES_DIVERGED_FUNCTION_COUNT;
      break;
    }

    /* Monitor convergence */
    ierr       = PetscObjectSAWsTakeAccess((PetscObject)snes);CHKERRQ(ierr);
    snes->iter = i;
    snes->norm = fnorm;
    ierr       = PetscObjectSAWsGrantAccess((PetscObject)snes);CHKERRQ(ierr);
    ierr       = SNESLogConvergenceHistory(snes,snes->norm,0);CHKERRQ(ierr);
    ierr       = SNESMonitor(snes,snes->iter,snes->norm);CHKERRQ(ierr);
    /* Test for convergence */
    ierr = (*snes->ops->converged)(snes,snes->iter,xnorm,ynorm,fnorm,&snes->reason,snes->cnvP);CHKERRQ(ierr);
    if (snes->reason) break;

    /* Call general purpose update function */
    if (snes->ops->update) {
      ierr = (*snes->ops->update)(snes, snes->iter);CHKERRQ(ierr);
    }

    if (snes->pc) {
      if (snes->functype == SNES_FUNCTION_PRECONDITIONED) {
        ierr = SNESApplyNPC(snes,X,NULL,Y);CHKERRQ(ierr);
        ierr = VecNorm(F,NORM_2,&fnorm);CHKERRQ(ierr);
        ierr = VecCopy(Y,F);CHKERRQ(ierr);
      } else {
        ierr = SNESApplyNPC(snes,X,F,Y);CHKERRQ(ierr);
      }
      ierr = SNESGetConvergedReason(snes->pc,&reason);CHKERRQ(ierr);
      if (reason < 0  && reason != SNES_DIVERGED_MAX_IT) {
        snes->reason = SNES_DIVERGED_INNER;
        PetscFunctionReturn(0);
      }
    } else {
      ierr = VecCopy(F,Y);CHKERRQ(ierr);
    }
  }
  if (i == maxits+1) {
    ierr = PetscInfo1(snes, "Maximum number of iterations has been reached: %D\n", maxits);CHKERRQ(ierr);
    if (!snes->reason) snes->reason = SNES_DIVERGED_MAX_IT;
  }
  PetscFunctionReturn(0);
}

/*MC
  SNESNRICHARDSON - Richardson nonlinear solver that uses successive substitutions, also sometimes known as Picard iteration.

  Level: beginner

  Options Database:
+   -snes_linesearch_type <l2,cp,basic> Line search type.
-   -snes_linesearch_damping<1.0> Damping for the line search.

  Notes: If no inner nonlinear preconditioner is provided then solves F(x) - b = 0 using x^{n+1} = x^{n} - lambda
            (F(x^n) - b) where lambda is obtained either SNESLineSearchSetDamping(), -snes_damping or a line search.  If
            an inner nonlinear preconditioner is provided (either with -npc_snes_type or SNESSetNPC()) then the inner
            solver is called an initial solution x^n and the nonlinear Richardson uses x^{n+1} = x^{n} + lambda d^{n}
            where d^{n} = \hat{x}^{n} - x^{n} where \hat{x}^{n} is the solution returned from the inner solver.

            The update, especially without inner nonlinear preconditioner, may be ill-scaled.  If using the basic
            linesearch, one may have to scale the update with -snes_linesearch_damping

     This uses no derivative information thus will be much slower then Newton's method obtained with -snes_type ls

.seealso:  SNESCreate(), SNES, SNESSetType(), SNESNEWTONLS, SNESNEWTONTR, SNESNGMRES, SNESQN, SNESNCG
M*/
#undef __FUNCT__
#define __FUNCT__ "SNESCreate_NRichardson"
PETSC_EXTERN PetscErrorCode SNESCreate_NRichardson(SNES snes)
{
  PetscErrorCode   ierr;
  SNES_NRichardson *neP;

  PetscFunctionBegin;
  snes->ops->destroy        = SNESDestroy_NRichardson;
  snes->ops->setup          = SNESSetUp_NRichardson;
  snes->ops->setfromoptions = SNESSetFromOptions_NRichardson;
  snes->ops->view           = SNESView_NRichardson;
  snes->ops->solve          = SNESSolve_NRichardson;
  snes->ops->reset          = SNESReset_NRichardson;

  snes->usesksp = PETSC_FALSE;
  snes->usespc  = PETSC_TRUE;

  snes->pcside = PC_LEFT;

  ierr       = PetscNewLog(snes,&neP);CHKERRQ(ierr);
  snes->data = (void*) neP;

  if (!snes->tolerancesset) {
    snes->max_funcs = 30000;
    snes->max_its   = 10000;
    snes->stol      = 1e-20;
  }
  PetscFunctionReturn(0);
}
