
#include <petsc/private/kspimpl.h>             /*I "petscksp.h" I*/
#include <../src/ksp/ksp/impls/cg/nash/nashimpl.h>

#define NASH_PRECONDITIONED_DIRECTION   0
#define NASH_UNPRECONDITIONED_DIRECTION 1
#define NASH_DIRECTION_TYPES            2

static const char *DType_Table[64] = {  "preconditioned", "unpreconditioned"};

#undef __FUNCT__
#define __FUNCT__ "KSPNASHSetRadius"
/*@
    KSPNASHSetRadius - Sets the radius of the trust region.

    Logically Collective on KSP

    Input Parameters:
+   ksp    - the iterative context
-   radius - the trust region radius (Infinity is the default)

    Options Database Key:
.   -ksp_nash_radius <r>

    Level: advanced

.keywords: KSP, NASH, set, trust region radius
@*/
PetscErrorCode  KSPNASHSetRadius(KSP ksp, PetscReal radius)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ksp, KSP_CLASSID, 1);
  if (radius < 0.0) SETERRQ(PetscObjectComm((PetscObject)ksp),PETSC_ERR_ARG_OUTOFRANGE, "Radius negative");
  PetscValidLogicalCollectiveReal(ksp,radius,2);
  ierr = PetscTryMethod(ksp,"KSPNASHSetRadius_C",(KSP,PetscReal),(ksp,radius));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "KSPNASHGetNormD"
/*@
    KSPNASHGetNormD - Got norm of the direction.

    Collective on KSP

    Input Parameters:
+   ksp    - the iterative context
-   norm_d - the norm of the direction

    Level: advanced

.keywords: KSP, NASH, get, norm direction
@*/
PetscErrorCode  KSPNASHGetNormD(KSP ksp, PetscReal *norm_d)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ksp, KSP_CLASSID, 1);
  ierr = PetscUseMethod(ksp,"KSPNASHGetNormD_C",(KSP,PetscReal*),(ksp,norm_d));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "KSPNASHGetObjFcn"
/*@
    KSPNASHGetObjFcn - Get objective function value.

    Collective on KSP

    Input Parameters:
+   ksp   - the iterative context
-   o_fcn - the objective function value

    Level: advanced

.keywords: KSP, NASH, get, objective function
@*/
PetscErrorCode  KSPNASHGetObjFcn(KSP ksp, PetscReal *o_fcn)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ksp, KSP_CLASSID, 1);
  ierr = PetscUseMethod(ksp,"KSPNASHGetObjFcn_C",(KSP,PetscReal*),(ksp,o_fcn));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "KSPSolve_NASH"

PetscErrorCode KSPSolve_NASH(KSP ksp)
{
#if defined(PETSC_USE_COMPLEX)
  SETERRQ(PetscObjectComm((PetscObject)ksp),PETSC_ERR_SUP, "NASH is not available for complex systems");
#else
  KSP_NASH       *cg = (KSP_NASH*)ksp->data;
  PetscErrorCode ierr;
  Mat            Qmat, Mmat;
  Vec            r, z, p, d;
  PC             pc;

  PetscReal norm_r, norm_d, norm_dp1, norm_p, dMp;
  PetscReal alpha, beta, kappa, rz, rzm1;
  PetscReal rr, r2, step;

  PetscInt max_cg_its;

  PetscBool diagonalscale;

  PetscFunctionBegin;
  /***************************************************************************/
  /* Check the arguments and parameters.                                     */
  /***************************************************************************/

  ierr = PCGetDiagonalScale(ksp->pc, &diagonalscale);CHKERRQ(ierr);
  if (diagonalscale) SETERRQ1(PetscObjectComm((PetscObject)ksp),PETSC_ERR_SUP, "Krylov method %s does not support diagonal scaling", ((PetscObject)ksp)->type_name);
  if (cg->radius < 0.0) SETERRQ(PetscObjectComm((PetscObject)ksp),PETSC_ERR_ARG_OUTOFRANGE, "Input error: radius < 0");

  /***************************************************************************/
  /* Get the workspace vectors and initialize variables                      */
  /***************************************************************************/

  r2 = cg->radius * cg->radius;
  r  = ksp->work[0];
  z  = ksp->work[1];
  p  = ksp->work[2];
  d  = ksp->vec_sol;
  pc = ksp->pc;

  ierr = PCGetOperators(pc, &Qmat, &Mmat);CHKERRQ(ierr);

  ierr       = VecGetSize(d, &max_cg_its);CHKERRQ(ierr);
  max_cg_its = PetscMin(max_cg_its, ksp->max_it);
  ksp->its   = 0;

  /***************************************************************************/
  /* Initialize objective function and direction.                            */
  /***************************************************************************/

  cg->o_fcn = 0.0;

  ierr       = VecSet(d, 0.0);CHKERRQ(ierr);            /* d = 0             */
  cg->norm_d = 0.0;

  /***************************************************************************/
  /* Begin the conjugate gradient method.  Check the right-hand side for     */
  /* numerical problems.  The check for not-a-number and infinite values     */
  /* need be performed only once.                                            */
  /***************************************************************************/

  ierr = VecCopy(ksp->vec_rhs, r);CHKERRQ(ierr);        /* r = -grad         */
  ierr = VecDot(r, r, &rr);CHKERRQ(ierr);               /* rr = r^T r        */
  KSPCheckDot(ksp,rr);

  /***************************************************************************/
  /* Check the preconditioner for numerical problems and for positive        */
  /* definiteness.  The check for not-a-number and infinite values need be   */
  /* performed only once.                                                    */
  /***************************************************************************/

  ierr = KSP_PCApply(ksp, r, z);CHKERRQ(ierr);          /* z = inv(M) r      */
  ierr = VecDot(r, z, &rz);CHKERRQ(ierr);               /* rz = r^T inv(M) r */
  if (PetscIsInfOrNanScalar(rz)) {
    /*************************************************************************/
    /* The preconditioner contains not-a-number or an infinite value.        */
    /* Return the gradient direction intersected with the trust region.      */
    /*************************************************************************/

    ksp->reason = KSP_DIVERGED_NANORINF;
    ierr        = PetscInfo1(ksp, "KSPSolve_NASH: bad preconditioner: rz=%g\n", (double)rz);CHKERRQ(ierr);

    if (cg->radius) {
      if (r2 >= rr) {
        alpha      = 1.0;
        cg->norm_d = PetscSqrtReal(rr);
      } else {
        alpha      = PetscSqrtReal(r2 / rr);
        cg->norm_d = cg->radius;
      }

      ierr = VecAXPY(d, alpha, r);CHKERRQ(ierr);        /* d = d + alpha r   */

      /***********************************************************************/
      /* Compute objective function.                                         */
      /***********************************************************************/

      ierr      = KSP_MatMult(ksp, Qmat, d, z);CHKERRQ(ierr);
      ierr      = VecAYPX(z, -0.5, ksp->vec_rhs);CHKERRQ(ierr);
      ierr      = VecDot(d, z, &cg->o_fcn);CHKERRQ(ierr);
      cg->o_fcn = -cg->o_fcn;
      ++ksp->its;
    }
    PetscFunctionReturn(0);
  }

  if (rz < 0.0) {
    /*************************************************************************/
    /* The preconditioner is indefinite.  Because this is the first          */
    /* and we do not have a direction yet, we use the gradient step.  Note   */
    /* that we cannot use the preconditioned norm when computing the step    */
    /* because the matrix is indefinite.                                     */
    /*************************************************************************/

    ksp->reason = KSP_DIVERGED_INDEFINITE_PC;
    ierr        = PetscInfo1(ksp, "KSPSolve_NASH: indefinite preconditioner: rz=%g\n", (double)rz);CHKERRQ(ierr);

    if (cg->radius) {
      if (r2 >= rr) {
        alpha      = 1.0;
        cg->norm_d = PetscSqrtReal(rr);
      } else {
        alpha      = PetscSqrtReal(r2 / rr);
        cg->norm_d = cg->radius;
      }

      ierr = VecAXPY(d, alpha, r);CHKERRQ(ierr);        /* d = d + alpha r   */

      /***********************************************************************/
      /* Compute objective function.                                         */
      /***********************************************************************/

      ierr      = KSP_MatMult(ksp, Qmat, d, z);CHKERRQ(ierr);
      ierr      = VecAYPX(z, -0.5, ksp->vec_rhs);CHKERRQ(ierr);
      ierr      = VecDot(d, z, &cg->o_fcn);CHKERRQ(ierr);
      cg->o_fcn = -cg->o_fcn;
      ++ksp->its;
    }
    PetscFunctionReturn(0);
  }

  /***************************************************************************/
  /* As far as we know, the preconditioner is positive semidefinite.         */
  /* Compute and log the residual.  Check convergence because this           */
  /* initializes things, but do not terminate until at least one conjugate   */
  /* gradient iteration has been performed.                                  */
  /***************************************************************************/

  switch (ksp->normtype) {
  case KSP_NORM_PRECONDITIONED:
    ierr = VecNorm(z, NORM_2, &norm_r);CHKERRQ(ierr);   /* norm_r = |z|      */
    break;

  case KSP_NORM_UNPRECONDITIONED:
    norm_r = PetscSqrtReal(rr);                                 /* norm_r = |r|      */
    break;

  case KSP_NORM_NATURAL:
    norm_r = PetscSqrtReal(rz);                                 /* norm_r = |r|_M    */
    break;

  default:
    norm_r = 0.0;
    break;
  }

  ierr       = KSPLogResidualHistory(ksp, norm_r);CHKERRQ(ierr);
  ierr       = KSPMonitor(ksp, ksp->its, norm_r);CHKERRQ(ierr);
  ksp->rnorm = norm_r;

  ierr = (*ksp->converged)(ksp, ksp->its, norm_r, &ksp->reason, ksp->cnvP);CHKERRQ(ierr);

  /***************************************************************************/
  /* Compute the first direction and update the iteration.                   */
  /***************************************************************************/

  ierr = VecCopy(z, p);CHKERRQ(ierr);                   /* p = z             */
  ierr = KSP_MatMult(ksp, Qmat, p, z);CHKERRQ(ierr);    /* z = Q * p         */
  ++ksp->its;

  /***************************************************************************/
  /* Check the matrix for numerical problems.                                */
  /***************************************************************************/

  ierr = VecDot(p, z, &kappa);CHKERRQ(ierr);            /* kappa = p^T Q p   */
  if (PetscIsInfOrNanScalar(kappa)) {
    /*************************************************************************/
    /* The matrix produced not-a-number or an infinite value.  In this case, */
    /* we must stop and use the gradient direction.  This condition need     */
    /* only be checked once.                                                 */
    /*************************************************************************/

    ksp->reason = KSP_DIVERGED_NANORINF;
    ierr        = PetscInfo1(ksp, "KSPSolve_NASH: bad matrix: kappa=%g\n", (double)kappa);CHKERRQ(ierr);

    if (cg->radius) {
      if (r2 >= rr) {
        alpha      = 1.0;
        cg->norm_d = PetscSqrtReal(rr);
      } else {
        alpha      = PetscSqrtReal(r2 / rr);
        cg->norm_d = cg->radius;
      }

      ierr = VecAXPY(d, alpha, r);CHKERRQ(ierr);        /* d = d + alpha r   */

      /***********************************************************************/
      /* Compute objective function.                                         */
      /***********************************************************************/

      ierr      = KSP_MatMult(ksp, Qmat, d, z);CHKERRQ(ierr);
      ierr      = VecAYPX(z, -0.5, ksp->vec_rhs);CHKERRQ(ierr);
      ierr      = VecDot(d, z, &cg->o_fcn);CHKERRQ(ierr);
      cg->o_fcn = -cg->o_fcn;
      ++ksp->its;
    }
    PetscFunctionReturn(0);
  }

  /***************************************************************************/
  /* Initialize variables for calculating the norm of the direction.         */
  /***************************************************************************/

  dMp    = 0.0;
  norm_d = 0.0;
  switch (cg->dtype) {
  case NASH_PRECONDITIONED_DIRECTION:
    norm_p = rz;
    break;

  default:
    ierr = VecDot(p, p, &norm_p);CHKERRQ(ierr);
    break;
  }

  /***************************************************************************/
  /* Check for negative curvature.                                           */
  /***************************************************************************/

  if (kappa <= 0.0) {
    /*************************************************************************/
    /* In this case, the matrix is indefinite and we have encountered a      */
    /* direction of negative curvature.  Because negative curvature occurs   */
    /* during the first step, we must follow a direction.                    */
    /*************************************************************************/

    ksp->reason = KSP_CONVERGED_CG_NEG_CURVE;
    ierr        = PetscInfo1(ksp, "KSPSolve_NASH: negative curvature: kappa=%g\n", (double)kappa);CHKERRQ(ierr);

    if (cg->radius && norm_p > 0.0) {
      /***********************************************************************/
      /* Follow direction of negative curvature to the boundary of the       */
      /* trust region.                                                       */
      /***********************************************************************/

      step       = PetscSqrtReal(r2 / norm_p);
      cg->norm_d = cg->radius;

      ierr = VecAXPY(d, step, p);CHKERRQ(ierr); /* d = d + step p    */

      /***********************************************************************/
      /* Update objective function.                                          */
      /***********************************************************************/

      cg->o_fcn += step * (0.5 * step * kappa - rz);
    } else if (cg->radius) {
      /***********************************************************************/
      /* The norm of the preconditioned direction is zero; use the gradient  */
      /* step.                                                               */
      /***********************************************************************/

      if (r2 >= rr) {
        alpha      = 1.0;
        cg->norm_d = PetscSqrtReal(rr);
      } else {
        alpha      = PetscSqrtReal(r2 / rr);
        cg->norm_d = cg->radius;
      }

      ierr = VecAXPY(d, alpha, r);CHKERRQ(ierr);        /* d = d + alpha r   */

      /***********************************************************************/
      /* Compute objective function.                                         */
      /***********************************************************************/

      ierr      = KSP_MatMult(ksp, Qmat, d, z);CHKERRQ(ierr);
      ierr      = VecAYPX(z, -0.5, ksp->vec_rhs);CHKERRQ(ierr);
      ierr      = VecDot(d, z, &cg->o_fcn);CHKERRQ(ierr);
      cg->o_fcn = -cg->o_fcn;
      ++ksp->its;
    }
    PetscFunctionReturn(0);
  }

  /***************************************************************************/
  /* Run the conjugate gradient method until either the problem is solved,   */
  /* we encounter the boundary of the trust region, or the conjugate         */
  /* gradient method breaks down.                                            */
  /***************************************************************************/

  while (1) {
    /*************************************************************************/
    /* Know that kappa is nonzero, because we have not broken down, so we    */
    /* can compute the steplength.                                           */
    /*************************************************************************/

    alpha = rz / kappa;

    /*************************************************************************/
    /* Compute the steplength and check for intersection with the trust      */
    /* region.                                                               */
    /*************************************************************************/

    norm_dp1 = norm_d + alpha*(2.0*dMp + alpha*norm_p);
    if (cg->radius && norm_dp1 >= r2) {
      /***********************************************************************/
      /* In this case, the matrix is positive definite as far as we know.    */
      /* However, the full step goes beyond the trust region.                */
      /***********************************************************************/

      ksp->reason = KSP_CONVERGED_CG_CONSTRAINED;
      ierr        = PetscInfo1(ksp, "KSPSolve_NASH: constrained step: radius=%g\n", (double)cg->radius);CHKERRQ(ierr);

      if (norm_p > 0.0) {
        /*********************************************************************/
        /* Follow the direction to the boundary of the trust region.         */
        /*********************************************************************/

        step       = (PetscSqrtReal(dMp*dMp+norm_p*(r2-norm_d))-dMp)/norm_p;
        cg->norm_d = cg->radius;

        ierr = VecAXPY(d, step, p);CHKERRQ(ierr);       /* d = d + step p    */

        /*********************************************************************/
        /* Update objective function.                                        */
        /*********************************************************************/

        cg->o_fcn += step * (0.5 * step * kappa - rz);
      } else {
        /*********************************************************************/
        /* The norm of the direction is zero; there is nothing to follow.    */
        /*********************************************************************/
      }
      break;
    }

    /*************************************************************************/
    /* Now we can update the direction and residual.                         */
    /*************************************************************************/

    ierr = VecAXPY(d, alpha, p);CHKERRQ(ierr);          /* d = d + alpha p   */
    ierr = VecAXPY(r, -alpha, z);CHKERRQ(ierr);         /* r = r - alpha Q p */
    ierr = KSP_PCApply(ksp, r, z);CHKERRQ(ierr);        /* z = inv(M) r      */

    switch (cg->dtype) {
    case NASH_PRECONDITIONED_DIRECTION:
      norm_d = norm_dp1;
      break;

    default:
      ierr = VecDot(d, d, &norm_d);CHKERRQ(ierr);
      break;
    }
    cg->norm_d = PetscSqrtReal(norm_d);

    /*************************************************************************/
    /* Update objective function.                                            */
    /*************************************************************************/

    cg->o_fcn -= 0.5 * alpha * rz;

    /*************************************************************************/
    /* Check that the preconditioner appears positive semidefinite.          */
    /*************************************************************************/

    rzm1 = rz;
    ierr = VecDot(r, z, &rz);CHKERRQ(ierr);             /* rz = r^T z        */
    if (rz < 0.0) {
      /***********************************************************************/
      /* The preconditioner is indefinite.                                   */
      /***********************************************************************/

      ksp->reason = KSP_DIVERGED_INDEFINITE_PC;
      ierr        = PetscInfo1(ksp, "KSPSolve_NASH: cg indefinite preconditioner: rz=%g\n", (double)rz);CHKERRQ(ierr);
      break;
    }

    /*************************************************************************/
    /* As far as we know, the preconditioner is positive semidefinite.       */
    /* Compute the residual and check for convergence.                       */
    /*************************************************************************/

    switch (ksp->normtype) {
    case KSP_NORM_PRECONDITIONED:
      ierr = VecNorm(z, NORM_2, &norm_r);CHKERRQ(ierr); /* norm_r = |z|      */
      break;

    case KSP_NORM_UNPRECONDITIONED:
      ierr = VecNorm(r, NORM_2, &norm_r);CHKERRQ(ierr); /* norm_r = |r|      */
      break;

    case KSP_NORM_NATURAL:
      norm_r = PetscSqrtReal(rz);                               /* norm_r = |r|_M    */
      break;

    default:
      norm_r = 0.;
      break;
    }

    ierr       = KSPLogResidualHistory(ksp, norm_r);CHKERRQ(ierr);
    ierr       = KSPMonitor(ksp, ksp->its, norm_r);CHKERRQ(ierr);
    ksp->rnorm = norm_r;

    ierr = (*ksp->converged)(ksp, ksp->its, norm_r, &ksp->reason, ksp->cnvP);CHKERRQ(ierr);
    if (ksp->reason) {
      /***********************************************************************/
      /* The method has converged.                                           */
      /***********************************************************************/

      ierr = PetscInfo2(ksp, "KSPSolve_NASH: truncated step: rnorm=%g, radius=%g\n", (double)norm_r, (double)cg->radius);CHKERRQ(ierr);
      break;
    }

    /*************************************************************************/
    /* We have not converged yet.  Check for breakdown.                      */
    /*************************************************************************/

    beta = rz / rzm1;
    if (PetscAbsReal(beta) <= 0.0) {
      /***********************************************************************/
      /* Conjugate gradients has broken down.                                */
      /***********************************************************************/

      ksp->reason = KSP_DIVERGED_BREAKDOWN;
      ierr        = PetscInfo1(ksp, "KSPSolve_NASH: breakdown: beta=%g\n", (double)beta);CHKERRQ(ierr);
      break;
    }

    /*************************************************************************/
    /* Check iteration limit.                                                */
    /*************************************************************************/

    if (ksp->its >= max_cg_its) {
      ksp->reason = KSP_DIVERGED_ITS;
      ierr        = PetscInfo1(ksp, "KSPSolve_NASH: iterlim: its=%D\n", ksp->its);CHKERRQ(ierr);
      break;
    }

    /*************************************************************************/
    /* Update p and the norms.                                               */
    /*************************************************************************/

    ierr = VecAYPX(p, beta, z);CHKERRQ(ierr);          /* p = z + beta p    */

    switch (cg->dtype) {
    case NASH_PRECONDITIONED_DIRECTION:
      dMp    = beta*(dMp + alpha*norm_p);
      norm_p = beta*(rzm1 + beta*norm_p);
      break;

    default:
      ierr = VecDot(d, p, &dMp);CHKERRQ(ierr);
      ierr = VecDot(p, p, &norm_p);CHKERRQ(ierr);
      break;
    }

    /*************************************************************************/
    /* Compute the new direction and update the iteration.                   */
    /*************************************************************************/

    ierr = KSP_MatMult(ksp, Qmat, p, z);CHKERRQ(ierr);  /* z = Q * p         */
    ierr = VecDot(p, z, &kappa);CHKERRQ(ierr);          /* kappa = p^T Q p   */
    ++ksp->its;

    /*************************************************************************/
    /* Check for negative curvature.                                         */
    /*************************************************************************/

    if (kappa <= 0.0) {
      /***********************************************************************/
      /* In this case, the matrix is indefinite and we have encountered      */
      /* a direction of negative curvature.  Stop at the base.               */
      /***********************************************************************/

      ksp->reason = KSP_CONVERGED_CG_NEG_CURVE;
      ierr        = PetscInfo1(ksp, "KSPSolve_NASH: negative curvature: kappa=%g\n", (double)kappa);CHKERRQ(ierr);
      break;
    }
  }
  PetscFunctionReturn(0);
#endif
}

#undef __FUNCT__
#define __FUNCT__ "KSPSetUp_NASH"
PetscErrorCode KSPSetUp_NASH(KSP ksp)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  /***************************************************************************/
  /* Set work vectors needed by conjugate gradient method and allocate       */
  /***************************************************************************/

  ierr = KSPSetWorkVecs(ksp, 3);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "KSPDestroy_NASH"
PetscErrorCode KSPDestroy_NASH(KSP ksp)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  /***************************************************************************/
  /* Clear composed functions                                                */
  /***************************************************************************/

  ierr = PetscObjectComposeFunction((PetscObject)ksp,"KSPNASHSetRadius_C",NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)ksp,"KSPNASHGetNormD_C",NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)ksp,"KSPNASHGetObjFcn_C",NULL);CHKERRQ(ierr);

  /***************************************************************************/
  /* Destroy KSP object.                                                     */
  /***************************************************************************/

  ierr = KSPDestroyDefault(ksp);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "KSPNASHSetRadius_NASH"
static PetscErrorCode  KSPNASHSetRadius_NASH(KSP ksp, PetscReal radius)
{
  KSP_NASH *cg = (KSP_NASH*)ksp->data;

  PetscFunctionBegin;
  cg->radius = radius;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "KSPNASHGetNormD_NASH"
static PetscErrorCode  KSPNASHGetNormD_NASH(KSP ksp, PetscReal *norm_d)
{
  KSP_NASH *cg = (KSP_NASH*)ksp->data;

  PetscFunctionBegin;
  *norm_d = cg->norm_d;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "KSPNASHGetObjFcn_NASH"
static PetscErrorCode  KSPNASHGetObjFcn_NASH(KSP ksp, PetscReal *o_fcn)
{
  KSP_NASH *cg = (KSP_NASH*)ksp->data;

  PetscFunctionBegin;
  *o_fcn = cg->o_fcn;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "KSPSetFromOptions_NASH"
PetscErrorCode KSPSetFromOptions_NASH(PetscOptionItems *PetscOptionsObject,KSP ksp)
{
  PetscErrorCode ierr;
  KSP_NASH       *cg = (KSP_NASH*)ksp->data;

  PetscFunctionBegin;
  ierr = PetscOptionsHead(PetscOptionsObject,"KSP NASH options");CHKERRQ(ierr);

  ierr = PetscOptionsReal("-ksp_nash_radius", "Trust Region Radius", "KSPNASHSetRadius", cg->radius, &cg->radius, NULL);CHKERRQ(ierr);

  ierr = PetscOptionsEList("-ksp_nash_dtype", "Norm used for direction", "", DType_Table, NASH_DIRECTION_TYPES, DType_Table[cg->dtype], &cg->dtype, NULL);CHKERRQ(ierr);

  ierr = PetscOptionsTail();CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/*MC
     KSPNASH -   Code to run conjugate gradient method subject to a constraint
         on the solution norm. This is used in Trust Region methods for
         nonlinear equations, SNESNEWTONTR

   Options Database Keys:
.      -ksp_nash_radius <r> - Trust Region Radius

   Notes: This is rarely used directly

   Level: developer

  Use preconditioned conjugate gradient to compute
  an approximate minimizer of the quadratic function

            q(s) = g^T * s + 0.5 * s^T * H * s

   subject to the trust region constraint

            || s || <= delta,

   where

     delta is the trust region radius,
     g is the gradient vector,
     H is the Hessian approximation, and
     M is the positive definite preconditioner matrix.

   KSPConvergedReason may be
$  KSP_CONVERGED_CG_NEG_CURVE if convergence is reached along a negative curvature direction,
$  KSP_CONVERGED_CG_CONSTRAINED if convergence is reached along a constrained step,
$  other KSP converged/diverged reasons

  Notes:
  The preconditioner supplied should be symmetric and positive definite.

.seealso:  KSPCreate(), KSPSetType(), KSPType (for list of available types), KSP, KSPNASHSetRadius(), KSPNASHGetNormD(), KSPNASHGetObjFcn()
M*/

#undef __FUNCT__
#define __FUNCT__ "KSPCreate_NASH"
PETSC_EXTERN PetscErrorCode KSPCreate_NASH(KSP ksp)
{
  PetscErrorCode ierr;
  KSP_NASH       *cg;

  PetscFunctionBegin;
  ierr       = PetscNewLog(ksp,&cg);CHKERRQ(ierr);
  cg->radius = 0.0;
  cg->dtype  = NASH_UNPRECONDITIONED_DIRECTION;

  ksp->data = (void*) cg;
  ierr      = KSPSetSupportedNorm(ksp,KSP_NORM_UNPRECONDITIONED,PC_LEFT,3);CHKERRQ(ierr);
  ierr      = KSPSetSupportedNorm(ksp,KSP_NORM_PRECONDITIONED,PC_LEFT,2);CHKERRQ(ierr);
  ierr      = KSPSetSupportedNorm(ksp,KSP_NORM_NATURAL,PC_LEFT,2);CHKERRQ(ierr);

  /***************************************************************************/
  /* Sets the functions that are associated with this data structure         */
  /* (in C++ this is the same as defining virtual functions).                */
  /***************************************************************************/

  ksp->ops->setup          = KSPSetUp_NASH;
  ksp->ops->solve          = KSPSolve_NASH;
  ksp->ops->destroy        = KSPDestroy_NASH;
  ksp->ops->setfromoptions = KSPSetFromOptions_NASH;
  ksp->ops->buildsolution  = KSPBuildSolutionDefault;
  ksp->ops->buildresidual  = KSPBuildResidualDefault;
  ksp->ops->view           = 0;

  ierr = PetscObjectComposeFunction((PetscObject)ksp,"KSPNASHSetRadius_C",KSPNASHSetRadius_NASH);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)ksp,"KSPNASHGetNormD_C",KSPNASHGetNormD_NASH);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)ksp,"KSPNASHGetObjFcn_C",KSPNASHGetObjFcn_NASH);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
