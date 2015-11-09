#include <petsc/private/linesearchimpl.h> /*I  "petscsnes.h"  I*/
#include <petsc/private/snesimpl.h>

typedef struct {
  PetscReal norm_delta_x_prev; /* norm of previous update */
  PetscReal norm_bar_delta_x_prev; /* norm of previous bar update */
  PetscReal mu_curr; /* current local Lipschitz estimate */
  PetscReal lambda_prev; /* previous step length: for some reason SNESLineSearchGetLambda returns 1 instead of the previous step length */
} SNESLineSearch_NLEQERR;

static PetscBool NLEQERR_cited = PETSC_FALSE;
static const char NLEQERR_citation[] = "@book{deuflhard2011,\n"
                               "  title = {Newton Methods for Nonlinear Problems},\n"
                               "  author = {Peter Deuflhard},\n"
                               "  volume = 35,\n"
                               "  year = 2011,\n"
                               "  isbn = {978-3-642-23898-7},\n"
                               "  doi  = {10.1007/978-3-642-23899-4},\n"
                               "  publisher = {Springer-Verlag},\n"
                               "  address = {Berlin, Heidelberg}\n}\n";

#undef __FUNCT__
#define __FUNCT__ "SNESLineSearchReset_NLEQERR"
static PetscErrorCode SNESLineSearchReset_NLEQERR(SNESLineSearch linesearch)
{
  SNESLineSearch_NLEQERR *nleqerr;

  nleqerr   = (SNESLineSearch_NLEQERR*)linesearch->data;
  nleqerr->mu_curr    = 0.0;
  nleqerr->norm_delta_x_prev = -1.0;
  nleqerr->norm_bar_delta_x_prev = -1.0;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SNESLineSearchApply_NLEQERR"
static PetscErrorCode  SNESLineSearchApply_NLEQERR(SNESLineSearch linesearch)
{
  PetscBool         changed_y,changed_w;
  PetscErrorCode    ierr;
  Vec               X,F,Y,W,G;
  SNES              snes;
  PetscReal         fnorm, xnorm, ynorm, gnorm, wnorm;
  PetscReal         lambda, minlambda, stol;
  PetscViewer       monitor;
  PetscInt          max_its, count, snes_iteration;
  PetscReal         theta, mudash, lambdadash;
  SNESLineSearch_NLEQERR *nleqerr;
  KSPConvergedReason kspreason;

  PetscFunctionBegin;
  ierr = PetscCitationsRegister(NLEQERR_citation, &NLEQERR_cited);CHKERRQ(ierr);

  ierr = SNESLineSearchGetVecs(linesearch, &X, &F, &Y, &W, &G);CHKERRQ(ierr);
  ierr = SNESLineSearchGetNorms(linesearch, &xnorm, &fnorm, &ynorm);CHKERRQ(ierr);
  ierr = SNESLineSearchGetLambda(linesearch, &lambda);CHKERRQ(ierr);
  ierr = SNESLineSearchGetSNES(linesearch, &snes);CHKERRQ(ierr);
  ierr = SNESLineSearchGetMonitor(linesearch, &monitor);CHKERRQ(ierr);
  ierr = SNESLineSearchGetTolerances(linesearch,&minlambda,NULL,NULL,NULL,NULL,&max_its);CHKERRQ(ierr);
  ierr = SNESGetTolerances(snes,NULL,NULL,&stol,NULL,NULL);CHKERRQ(ierr);
  nleqerr   = (SNESLineSearch_NLEQERR*)linesearch->data;

  /* reset the state of the Lipschitz estimates */
  ierr = SNESGetIterationNumber(snes, &snes_iteration);CHKERRQ(ierr);
  if (snes_iteration == 0) {
    ierr = SNESLineSearchReset_NLEQERR(linesearch);CHKERRQ(ierr);
  }

  /* precheck */
  ierr = SNESLineSearchPreCheck(linesearch,X,Y,&changed_y);CHKERRQ(ierr);
  ierr = SNESLineSearchSetReason(linesearch, SNES_LINESEARCH_SUCCEEDED);CHKERRQ(ierr);

  ierr = VecNormBegin(Y, NORM_2, &ynorm);CHKERRQ(ierr);
  ierr = VecNormBegin(X, NORM_2, &xnorm);CHKERRQ(ierr);
  ierr = VecNormEnd(Y, NORM_2, &ynorm);CHKERRQ(ierr);
  ierr = VecNormEnd(X, NORM_2, &xnorm);CHKERRQ(ierr);

  /* Note: Y is *minus* the Newton step. For whatever reason PETSc doesn't solve with the minus on
     the RHS. */

  if (ynorm == 0.0) {
    if (monitor) {
      ierr = PetscViewerASCIIAddTab(monitor,((PetscObject)linesearch)->tablevel);CHKERRQ(ierr);
      ierr = PetscViewerASCIIPrintf(monitor,"    Line search: Initial direction and size is 0\n");CHKERRQ(ierr);
      ierr = PetscViewerASCIISubtractTab(monitor,((PetscObject)linesearch)->tablevel);CHKERRQ(ierr);
    }
    ierr = VecCopy(X,W);CHKERRQ(ierr);
    ierr = VecCopy(F,G);CHKERRQ(ierr);
    ierr = SNESLineSearchSetNorms(linesearch,xnorm,fnorm,ynorm);CHKERRQ(ierr);
    ierr = SNESLineSearchSetReason(linesearch, SNES_LINESEARCH_FAILED_REDUCT);CHKERRQ(ierr);
    PetscFunctionReturn(0);
  }

  /* At this point, we've solved the Newton system for delta_x, and we assume that
     its norm is greater than the solution tolerance (otherwise we wouldn't be in
     here). So let's go ahead and estimate the Lipschitz constant. 

     W contains bar_delta_x_prev at this point. */

  if (monitor) {
    ierr = PetscViewerASCIIAddTab(monitor,((PetscObject)linesearch)->tablevel);CHKERRQ(ierr);
    ierr = PetscViewerASCIIPrintf(monitor,"    Line search: norm of Newton step: %14.12e\n", (double) ynorm);CHKERRQ(ierr);
    ierr = PetscViewerASCIISubtractTab(monitor,((PetscObject)linesearch)->tablevel);CHKERRQ(ierr);
  }

  /* this needs information from a previous iteration, so can't do it on the first one */
  if (nleqerr->norm_delta_x_prev > 0 && nleqerr->norm_bar_delta_x_prev > 0) {
    ierr = VecWAXPY(G, +1.0, Y, W);CHKERRQ(ierr); /* bar_delta_x - delta_x; +1 because Y is -delta_x */
    ierr = VecNormBegin(G, NORM_2, &gnorm);CHKERRQ(ierr);
    ierr = VecNormEnd(G, NORM_2, &gnorm);CHKERRQ(ierr);

    nleqerr->mu_curr = nleqerr->lambda_prev * (nleqerr->norm_delta_x_prev * nleqerr->norm_bar_delta_x_prev) / (gnorm * ynorm);
    lambda = PetscMin(1.0, nleqerr->mu_curr);

    if (monitor) {
      ierr = PetscViewerASCIIAddTab(monitor,((PetscObject)linesearch)->tablevel);CHKERRQ(ierr);
      ierr = PetscViewerASCIIPrintf(monitor,"    Line search: Lipschitz estimate: %14.12e; lambda: %14.12e\n", (double) nleqerr->mu_curr, (double) lambda);CHKERRQ(ierr);
      ierr = PetscViewerASCIISubtractTab(monitor,((PetscObject)linesearch)->tablevel);CHKERRQ(ierr);
    }
  } else {
    lambda = linesearch->damping;
  }

  /* The main while loop of the algorithm. 
     At the end of this while loop, G should have the accepted new X in it. */

  count = 0;
  while (PETSC_TRUE) {
    if (monitor) {
      ierr = PetscViewerASCIIAddTab(monitor,((PetscObject)linesearch)->tablevel);CHKERRQ(ierr);
      ierr = PetscViewerASCIIPrintf(monitor,"    Line search: entering iteration with lambda: %14.12e\n", lambda);CHKERRQ(ierr);
      ierr = PetscViewerASCIISubtractTab(monitor,((PetscObject)linesearch)->tablevel);CHKERRQ(ierr);
    }

    /* Check that we haven't performed too many iterations */
    count += 1;
    if (count >= max_its) {
      if (monitor) {
        ierr = PetscViewerASCIIAddTab(monitor,((PetscObject)linesearch)->tablevel);CHKERRQ(ierr);
        ierr = PetscViewerASCIIPrintf(monitor,"    Line search: maximum iterations reached\n");CHKERRQ(ierr);
        ierr = PetscViewerASCIISubtractTab(monitor,((PetscObject)linesearch)->tablevel);CHKERRQ(ierr);
      }
      ierr = SNESLineSearchSetReason(linesearch, SNES_LINESEARCH_FAILED_REDUCT);CHKERRQ(ierr);
      PetscFunctionReturn(0);
    }

    /* Now comes the Regularity Test. */
    if (lambda <= minlambda) {
      /* This isn't what is suggested by Deuflhard, but it works better in my experience */
      if (monitor) {
        ierr = PetscViewerASCIIAddTab(monitor,((PetscObject)linesearch)->tablevel);CHKERRQ(ierr);
        ierr = PetscViewerASCIIPrintf(monitor,"    Line search: lambda has reached lambdamin, taking full Newton step\n");CHKERRQ(ierr);
        ierr = PetscViewerASCIISubtractTab(monitor,((PetscObject)linesearch)->tablevel);CHKERRQ(ierr);
      }
      lambda = 1.0;
      ierr = VecWAXPY(G, -lambda, Y, X);CHKERRQ(ierr);

      /* and clean up the state for next time */
      ierr = SNESLineSearchReset_NLEQERR(linesearch);CHKERRQ(ierr);
      break;
    }

    /* Compute new trial iterate */
    ierr = VecWAXPY(W, -lambda, Y, X);CHKERRQ(ierr);
    ierr = SNESComputeFunction(snes, W, G);CHKERRQ(ierr);

    /* Solve linear system for bar_delta_x_curr: old Jacobian, new RHS. Note absence of minus sign, compared to Deuflhard, in keeping with PETSc convention */
    ierr = KSPSolve(snes->ksp, G, W);CHKERRQ(ierr);
    ierr = KSPGetConvergedReason(snes->ksp, &kspreason);CHKERRQ(ierr);
    if (kspreason < 0) {
      ierr = PetscInfo(snes,"Solution for \\bar{delta x}^{k+1} failed.");CHKERRQ(ierr);
    }

    /* W now contains -bar_delta_x_curr. */

    ierr = VecNorm(W, NORM_2, &wnorm);CHKERRQ(ierr);
    if (monitor) {
      ierr = PetscViewerASCIIAddTab(monitor,((PetscObject)linesearch)->tablevel);CHKERRQ(ierr);
      ierr = PetscViewerASCIIPrintf(monitor,"    Line search: norm of simplified Newton update: %14.12e\n", (double) wnorm);CHKERRQ(ierr);
      ierr = PetscViewerASCIISubtractTab(monitor,((PetscObject)linesearch)->tablevel);CHKERRQ(ierr);
    }

    /* compute the monitoring quantities theta and mudash. */

    theta = wnorm / ynorm;

    ierr = VecWAXPY(G, -(1.0 - lambda), Y, W);CHKERRQ(ierr);
    ierr = VecNorm(G, NORM_2, &gnorm);CHKERRQ(ierr);

    mudash = (0.5 * ynorm * lambda * lambda) / gnorm;

    /* Check for termination of the linesearch */
    if (theta >= 1.0) {
      /* need to go around again with smaller lambda */
      if (monitor) {
        ierr = PetscViewerASCIIAddTab(monitor,((PetscObject)linesearch)->tablevel);CHKERRQ(ierr);
        ierr = PetscViewerASCIIPrintf(monitor,"    Line search: monotonicity check failed, ratio: %14.12e\n", (double) theta);CHKERRQ(ierr);
        ierr = PetscViewerASCIISubtractTab(monitor,((PetscObject)linesearch)->tablevel);CHKERRQ(ierr);
      }
      lambda = PetscMin(mudash, 0.5 * lambda);
      lambda = PetscMax(lambda, minlambda);
      /* continue through the loop, i.e. go back to regularity test */
    } else {
      /* linesearch terminated */
      lambdadash = PetscMin(1.0, mudash);

      if (lambdadash == 1.0 && lambda == 1.0 && wnorm <= stol) {
        /* store the updated state, X - Y - W, in G:
           I need to keep W for the next linesearch */
        ierr = VecCopy(X, G);CHKERRQ(ierr);
        ierr = VecAXPY(G, -1.0, Y);CHKERRQ(ierr);
        ierr = VecAXPY(G, -1.0, W);CHKERRQ(ierr);
        break;
      }

      /* Deuflhard suggests to add the following:
      else if (lambdadash >= 4.0 * lambda) {
        lambda = lambdadash;
      }
      to continue through the loop, i.e. go back to regularity test.
      I deliberately exclude this, as I have practical experience of this
      getting stuck in infinite loops (on e.g. an Allen--Cahn problem). */

      else {
        /* accept iterate without adding on, i.e. don't use bar_delta_x;
           again, I need to keep W for the next linesearch */
        ierr = VecWAXPY(G, -lambda, Y, X);CHKERRQ(ierr);
        break;
      }
    }
  }

  if (linesearch->ops->viproject) {
    ierr = (*linesearch->ops->viproject)(snes, G);CHKERRQ(ierr);
  }

  /* W currently contains -bar_delta_u. Scale it so that it contains bar_delta_u. */
  ierr = VecScale(W, -1.0);CHKERRQ(ierr);

  /* postcheck */
  ierr = SNESLineSearchPostCheck(linesearch,X,Y,G,&changed_y,&changed_w);CHKERRQ(ierr);
  if (changed_y || changed_w) {
    ierr = SNESLineSearchSetReason(linesearch, SNES_LINESEARCH_FAILED_USER);CHKERRQ(ierr);
    ierr = PetscInfo(snes,"Changing the search direction here doesn't make sense.\n");CHKERRQ(ierr);
    PetscFunctionReturn(0);
  }

  /* copy the solution and information from this iteration over */
  nleqerr->norm_delta_x_prev = ynorm;
  nleqerr->norm_bar_delta_x_prev = wnorm;
  nleqerr->lambda_prev = lambda;

  ierr = VecCopy(G, X);CHKERRQ(ierr);
  ierr = SNESComputeFunction(snes, X, F);CHKERRQ(ierr);
  ierr = VecNorm(X, NORM_2, &xnorm);CHKERRQ(ierr);
  ierr = VecNorm(F, NORM_2, &fnorm);CHKERRQ(ierr);
  ierr = SNESLineSearchSetLambda(linesearch, lambda);CHKERRQ(ierr);
  ierr = SNESLineSearchSetNorms(linesearch, xnorm, fnorm, ynorm);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SNESLineSearchView_NLEQERR"
PetscErrorCode SNESLineSearchView_NLEQERR(SNESLineSearch linesearch, PetscViewer viewer)
{
  PetscErrorCode          ierr;
  PetscBool               iascii;
  SNESLineSearch_NLEQERR *nleqerr;

  PetscFunctionBegin;
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERASCII,&iascii);CHKERRQ(ierr);
  nleqerr   = (SNESLineSearch_NLEQERR*)linesearch->data;
  if (iascii) {
    ierr = PetscViewerASCIIPrintf(viewer, "  NLEQ-ERR affine-covariant linesearch");CHKERRQ(ierr);
    ierr = PetscViewerASCIIPrintf(viewer, "  current local Lipschitz estimate omega=%e\n", (double)nleqerr->mu_curr);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SNESLineSearchDestroy_NLEQERR"
static PetscErrorCode SNESLineSearchDestroy_NLEQERR(SNESLineSearch linesearch)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscFree(linesearch->data);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SNESLineSearchCreate_NLEQERR"
/*MC
   SNESLINESEARCHNLEQERR - Error-oriented affine-covariant globalised Newton algorithm of Deuflhard (2011).

   This linesearch is intended for Newton-type methods which are affine covariant. Affine covariance
   means that Newton's method will give the same iterations for F(x) = 0 and AF(x) = 0 for a nonsingular
   matrix A. This is a fundamental property; the philosophy of this linesearch is that globalisations
   of Newton's method should carefully preserve it.

   For a discussion of the theory behind this algorithm, see

   @book{deuflhard2011,
     title={Newton Methods for Nonlinear Problems},
     author={Deuflhard, P.},
     volume={35},
     year={2011},
     publisher={Springer-Verlag},
     address={Berlin, Heidelberg}
   }

   Pseudocode is given on page 148.

   Options Database Keys:
+  -snes_linesearch_damping<1.0> - initial step length
-  -snes_linesearch_minlambda<1e-12> - minimum step length allowed

   Contributed by Patrick Farrell <patrick.farrell@maths.ox.ac.uk>

   Level: advanced

.keywords: SNES, SNESLineSearch, damping

.seealso: SNESLineSearchCreate(), SNESLineSearchSetType()
M*/
PETSC_EXTERN PetscErrorCode SNESLineSearchCreate_NLEQERR(SNESLineSearch linesearch)
{

  SNESLineSearch_NLEQERR *nleqerr;
  PetscErrorCode    ierr;

  PetscFunctionBegin;
  linesearch->ops->apply          = SNESLineSearchApply_NLEQERR;
  linesearch->ops->destroy        = SNESLineSearchDestroy_NLEQERR;
  linesearch->ops->setfromoptions = NULL;
  linesearch->ops->reset          = SNESLineSearchReset_NLEQERR;
  linesearch->ops->view           = SNESLineSearchView_NLEQERR;
  linesearch->ops->setup          = NULL;

  ierr = PetscNewLog(linesearch,&nleqerr);CHKERRQ(ierr);

  linesearch->data    = (void*)nleqerr;
  linesearch->max_its = 40;
  SNESLineSearchReset_NLEQERR(linesearch);
  PetscFunctionReturn(0);
}
