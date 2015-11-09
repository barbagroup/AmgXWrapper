#include <../src/tao/matrix/lmvmmat.h>
#include <../src/tao/unconstrained/impls/ntl/ntl.h>

#include <petscksp.h>
#include <petscpc.h>
#include <petsc/private/kspimpl.h>
#include <petsc/private/pcimpl.h>

#define NTL_KSP_NASH    0
#define NTL_KSP_STCG    1
#define NTL_KSP_GLTR    2
#define NTL_KSP_TYPES   3

#define NTL_PC_NONE     0
#define NTL_PC_AHESS    1
#define NTL_PC_BFGS     2
#define NTL_PC_PETSC    3
#define NTL_PC_TYPES    4

#define BFGS_SCALE_AHESS        0
#define BFGS_SCALE_BFGS         1
#define BFGS_SCALE_TYPES        2

#define NTL_INIT_CONSTANT         0
#define NTL_INIT_DIRECTION        1
#define NTL_INIT_INTERPOLATION    2
#define NTL_INIT_TYPES            3

#define NTL_UPDATE_REDUCTION      0
#define NTL_UPDATE_INTERPOLATION  1
#define NTL_UPDATE_TYPES          2

static const char *NTL_KSP[64] = {"nash", "stcg", "gltr"};

static const char *NTL_PC[64] = {"none", "ahess", "bfgs", "petsc"};

static const char *BFGS_SCALE[64] = {"ahess", "bfgs"};

static const char *NTL_INIT[64] = {"constant", "direction", "interpolation"};

static const char *NTL_UPDATE[64] = {"reduction", "interpolation"};

/* Routine for BFGS preconditioner */

#undef __FUNCT__
#define __FUNCT__ "MatLMVMSolveShell"
static PetscErrorCode MatLMVMSolveShell(PC pc, Vec b, Vec x)
{
  PetscErrorCode ierr;
  Mat            M;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(pc,PC_CLASSID,1);
  PetscValidHeaderSpecific(b,VEC_CLASSID,2);
  PetscValidHeaderSpecific(x,VEC_CLASSID,3);
  ierr = PCShellGetContext(pc,(void**)&M);CHKERRQ(ierr);
  ierr = MatLMVMSolve(M, b, x);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/* Implements Newton's Method with a trust-region, line-search approach for
   solving unconstrained minimization problems.  A More'-Thuente line search
   is used to guarantee that the bfgs preconditioner remains positive
   definite. */

#define NTL_NEWTON              0
#define NTL_BFGS                1
#define NTL_SCALED_GRADIENT     2
#define NTL_GRADIENT            3

#undef __FUNCT__
#define __FUNCT__ "TaoSolve_NTL"
static PetscErrorCode TaoSolve_NTL(Tao tao)
{
  TAO_NTL                      *tl = (TAO_NTL *)tao->data;
  PC                           pc;
  KSPConvergedReason           ksp_reason;
  TaoConvergedReason           reason;
  TaoLineSearchConvergedReason ls_reason;

  PetscReal                    fmin, ftrial, prered, actred, kappa, sigma;
  PetscReal                    tau, tau_1, tau_2, tau_max, tau_min, max_radius;
  PetscReal                    f, fold, gdx, gnorm;
  PetscReal                    step = 1.0;

  PetscReal                    delta;
  PetscReal                    norm_d = 0.0;
  PetscErrorCode               ierr;
  PetscInt                     stepType;
  PetscInt                     its;

  PetscInt                     bfgsUpdates = 0;
  PetscInt                     needH;

  PetscInt                     i_max = 5;
  PetscInt                     j_max = 1;
  PetscInt                     i, j, n, N;

  PetscInt                     tr_reject;

  PetscFunctionBegin;
  if (tao->XL || tao->XU || tao->ops->computebounds) {
    ierr = PetscPrintf(((PetscObject)tao)->comm,"WARNING: Variable bounds have been set but will be ignored by ntl algorithm\n");CHKERRQ(ierr);
  }

  /* Initialize trust-region radius */
  tao->trust = tao->trust0;

  /* Modify the radius if it is too large or small */
  tao->trust = PetscMax(tao->trust, tl->min_radius);
  tao->trust = PetscMin(tao->trust, tl->max_radius);

  if (NTL_PC_BFGS == tl->pc_type && !tl->M) {
    ierr = VecGetLocalSize(tao->solution,&n);CHKERRQ(ierr);
    ierr = VecGetSize(tao->solution,&N);CHKERRQ(ierr);
    ierr = MatCreateLMVM(((PetscObject)tao)->comm,n,N,&tl->M);CHKERRQ(ierr);
    ierr = MatLMVMAllocateVectors(tl->M,tao->solution);CHKERRQ(ierr);
  }

  /* Check convergence criteria */
  ierr = TaoComputeObjectiveAndGradient(tao, tao->solution, &f, tao->gradient);CHKERRQ(ierr);
  ierr = VecNorm(tao->gradient, NORM_2, &gnorm);CHKERRQ(ierr);
  if (PetscIsInfOrNanReal(f) || PetscIsInfOrNanReal(gnorm)) SETERRQ(PETSC_COMM_SELF,1, "User provided compute function generated Inf or NaN");
  needH = 1;

  ierr = TaoMonitor(tao, tao->niter, f, gnorm, 0.0, 1.0, &reason);CHKERRQ(ierr);
  if (reason != TAO_CONTINUE_ITERATING) PetscFunctionReturn(0);

  /* Create vectors for the limited memory preconditioner */
  if ((NTL_PC_BFGS == tl->pc_type) && (BFGS_SCALE_BFGS != tl->bfgs_scale_type)) {
    if (!tl->Diag) {
      ierr = VecDuplicate(tao->solution, &tl->Diag);CHKERRQ(ierr);
    }
  }

  /* Modify the linear solver to a conjugate gradient method */
  switch(tl->ksp_type) {
  case NTL_KSP_NASH:
    ierr = KSPSetType(tao->ksp, KSPNASH);CHKERRQ(ierr);
    ierr = KSPSetFromOptions(tao->ksp);CHKERRQ(ierr);
    break;

  case NTL_KSP_STCG:
    ierr = KSPSetType(tao->ksp, KSPSTCG);CHKERRQ(ierr);
    ierr = KSPSetFromOptions(tao->ksp);CHKERRQ(ierr);
    break;

  default:
    ierr = KSPSetType(tao->ksp, KSPGLTR);CHKERRQ(ierr);
    ierr = KSPSetFromOptions(tao->ksp);CHKERRQ(ierr);
    break;
  }

  /* Modify the preconditioner to use the bfgs approximation */
  ierr = KSPGetPC(tao->ksp, &pc);CHKERRQ(ierr);
  switch(tl->pc_type) {
  case NTL_PC_NONE:
    ierr = PCSetType(pc, PCNONE);CHKERRQ(ierr);
    ierr = PCSetFromOptions(pc);CHKERRQ(ierr);
    break;

  case NTL_PC_AHESS:
    ierr = PCSetType(pc, PCJACOBI);CHKERRQ(ierr);
    ierr = PCSetFromOptions(pc);CHKERRQ(ierr);
    ierr = PCJacobiSetUseAbs(pc,PETSC_TRUE);CHKERRQ(ierr);
    break;

  case NTL_PC_BFGS:
    ierr = PCSetType(pc, PCSHELL);CHKERRQ(ierr);
    ierr = PCSetFromOptions(pc);CHKERRQ(ierr);
    ierr = PCShellSetName(pc, "bfgs");CHKERRQ(ierr);
    ierr = PCShellSetContext(pc, tl->M);CHKERRQ(ierr);
    ierr = PCShellSetApply(pc, MatLMVMSolveShell);CHKERRQ(ierr);
    break;

  default:
    /* Use the pc method set by pc_type */
    break;
  }

  /* Initialize trust-region radius.  The initialization is only performed
     when we are using Steihaug-Toint or the Generalized Lanczos method. */
  switch(tl->init_type) {
  case NTL_INIT_CONSTANT:
    /* Use the initial radius specified */
    break;

  case NTL_INIT_INTERPOLATION:
    /* Use the initial radius specified */
    max_radius = 0.0;

    for (j = 0; j < j_max; ++j) {
      fmin = f;
      sigma = 0.0;

      if (needH) {
        ierr = TaoComputeHessian(tao,tao->solution,tao->hessian,tao->hessian_pre);CHKERRQ(ierr);
        needH = 0;
      }

      for (i = 0; i < i_max; ++i) {
        ierr = VecCopy(tao->solution, tl->W);CHKERRQ(ierr);
        ierr = VecAXPY(tl->W, -tao->trust/gnorm, tao->gradient);CHKERRQ(ierr);

        ierr = TaoComputeObjective(tao, tl->W, &ftrial);CHKERRQ(ierr);
        if (PetscIsInfOrNanReal(ftrial)) {
          tau = tl->gamma1_i;
        } else {
          if (ftrial < fmin) {
            fmin = ftrial;
            sigma = -tao->trust / gnorm;
          }

          ierr = MatMult(tao->hessian, tao->gradient, tao->stepdirection);CHKERRQ(ierr);
          ierr = VecDot(tao->gradient, tao->stepdirection, &prered);CHKERRQ(ierr);

          prered = tao->trust * (gnorm - 0.5 * tao->trust * prered / (gnorm * gnorm));
          actred = f - ftrial;
          if ((PetscAbsScalar(actred) <= tl->epsilon) && (PetscAbsScalar(prered) <= tl->epsilon)) {
            kappa = 1.0;
          } else {
            kappa = actred / prered;
          }

          tau_1 = tl->theta_i * gnorm * tao->trust / (tl->theta_i * gnorm * tao->trust + (1.0 - tl->theta_i) * prered - actred);
          tau_2 = tl->theta_i * gnorm * tao->trust / (tl->theta_i * gnorm * tao->trust - (1.0 + tl->theta_i) * prered + actred);
          tau_min = PetscMin(tau_1, tau_2);
          tau_max = PetscMax(tau_1, tau_2);

          if (PetscAbsScalar(kappa - 1.0) <= tl->mu1_i) {
            /* Great agreement */
            max_radius = PetscMax(max_radius, tao->trust);

            if (tau_max < 1.0) {
              tau = tl->gamma3_i;
            } else if (tau_max > tl->gamma4_i) {
              tau = tl->gamma4_i;
            } else if (tau_1 >= 1.0 && tau_1 <= tl->gamma4_i && tau_2 < 1.0) {
              tau = tau_1;
            } else if (tau_2 >= 1.0 && tau_2 <= tl->gamma4_i && tau_1 < 1.0) {
              tau = tau_2;
            } else {
              tau = tau_max;
            }
          } else if (PetscAbsScalar(kappa - 1.0) <= tl->mu2_i) {
            /* Good agreement */
            max_radius = PetscMax(max_radius, tao->trust);

            if (tau_max < tl->gamma2_i) {
              tau = tl->gamma2_i;
            } else if (tau_max > tl->gamma3_i) {
              tau = tl->gamma3_i;
            } else {
              tau = tau_max;
            }
          } else {
            /* Not good agreement */
            if (tau_min > 1.0) {
              tau = tl->gamma2_i;
            } else if (tau_max < tl->gamma1_i) {
              tau = tl->gamma1_i;
            } else if ((tau_min < tl->gamma1_i) && (tau_max >= 1.0)) {
              tau = tl->gamma1_i;
            } else if ((tau_1 >= tl->gamma1_i) && (tau_1 < 1.0) &&  ((tau_2 < tl->gamma1_i) || (tau_2 >= 1.0))) {
              tau = tau_1;
            } else if ((tau_2 >= tl->gamma1_i) && (tau_2 < 1.0) &&  ((tau_1 < tl->gamma1_i) || (tau_2 >= 1.0))) {
              tau = tau_2;
            } else {
              tau = tau_max;
            }
          }
        }
        tao->trust = tau * tao->trust;
      }

      if (fmin < f) {
        f = fmin;
        ierr = VecAXPY(tao->solution, sigma, tao->gradient);CHKERRQ(ierr);
        ierr = TaoComputeGradient(tao, tao->solution, tao->gradient);CHKERRQ(ierr);

        ierr = VecNorm(tao->gradient, NORM_2, &gnorm);CHKERRQ(ierr);
        if (PetscIsInfOrNanReal(f) || PetscIsInfOrNanReal(gnorm)) SETERRQ(PETSC_COMM_SELF,1, "User provided compute function generated Inf or NaN");
        needH = 1;

        ierr = TaoMonitor(tao, tao->niter, f, gnorm, 0.0, 1.0, &reason);CHKERRQ(ierr);
        if (reason != TAO_CONTINUE_ITERATING) PetscFunctionReturn(0);
      }
    }
    tao->trust = PetscMax(tao->trust, max_radius);

    /* Modify the radius if it is too large or small */
    tao->trust = PetscMax(tao->trust, tl->min_radius);
    tao->trust = PetscMin(tao->trust, tl->max_radius);
    break;

  default:
    /* Norm of the first direction will initialize radius */
    tao->trust = 0.0;
    break;
  }

  /* Set initial scaling for the BFGS preconditioner
     This step is done after computing the initial trust-region radius
     since the function value may have decreased */
  if (NTL_PC_BFGS == tl->pc_type) {
    if (f != 0.0) {
      delta = 2.0 * PetscAbsScalar(f) / (gnorm*gnorm);
    } else {
      delta = 2.0 / (gnorm*gnorm);
    }
    ierr = MatLMVMSetDelta(tl->M, delta);CHKERRQ(ierr);
  }

  /* Set counter for gradient/reset steps */
  tl->ntrust = 0;
  tl->newt = 0;
  tl->bfgs = 0;
  tl->sgrad = 0;
  tl->grad = 0;

  /* Have not converged; continue with Newton method */
  while (reason == TAO_CONTINUE_ITERATING) {
    ++tao->niter;
    tao->ksp_its=0;
    /* Compute the Hessian */
    if (needH) {
      ierr = TaoComputeHessian(tao,tao->solution,tao->hessian,tao->hessian_pre);CHKERRQ(ierr);
      needH = 0;
    }

    if (NTL_PC_BFGS == tl->pc_type) {
      if (BFGS_SCALE_AHESS == tl->bfgs_scale_type) {
        /* Obtain diagonal for the bfgs preconditioner */
        ierr = MatGetDiagonal(tao->hessian, tl->Diag);CHKERRQ(ierr);
        ierr = VecAbs(tl->Diag);CHKERRQ(ierr);
        ierr = VecReciprocal(tl->Diag);CHKERRQ(ierr);
        ierr = MatLMVMSetScale(tl->M, tl->Diag);CHKERRQ(ierr);
      }

      /* Update the limited memory preconditioner */
      ierr = MatLMVMUpdate(tl->M,tao->solution, tao->gradient);CHKERRQ(ierr);
      ++bfgsUpdates;
    }
    ierr = KSPSetOperators(tao->ksp, tao->hessian, tao->hessian_pre);CHKERRQ(ierr);
    /* Solve the Newton system of equations */
    if (NTL_KSP_NASH == tl->ksp_type) {
      ierr = KSPNASHSetRadius(tao->ksp,tl->max_radius);CHKERRQ(ierr);
      ierr = KSPSolve(tao->ksp, tao->gradient, tao->stepdirection);CHKERRQ(ierr);
      ierr = KSPGetIterationNumber(tao->ksp,&its);CHKERRQ(ierr);
      tao->ksp_its+=its;
      tao->ksp_tot_its+=its;
      ierr = KSPNASHGetNormD(tao->ksp, &norm_d);CHKERRQ(ierr);
    } else if (NTL_KSP_STCG == tl->ksp_type) {
      ierr = KSPSTCGSetRadius(tao->ksp,tl->max_radius);CHKERRQ(ierr);
      ierr = KSPSolve(tao->ksp, tao->gradient, tao->stepdirection);CHKERRQ(ierr);
      ierr = KSPGetIterationNumber(tao->ksp,&its);CHKERRQ(ierr);
      tao->ksp_its+=its;
      tao->ksp_tot_its+=its;
      ierr = KSPSTCGGetNormD(tao->ksp, &norm_d);CHKERRQ(ierr);
    } else { /* NTL_KSP_GLTR */
      ierr = KSPGLTRSetRadius(tao->ksp,tl->max_radius);CHKERRQ(ierr);
      ierr = KSPSolve(tao->ksp, tao->gradient, tao->stepdirection);CHKERRQ(ierr);
      ierr = KSPGetIterationNumber(tao->ksp,&its);CHKERRQ(ierr);
      tao->ksp_its+=its;
      tao->ksp_tot_its+=its;
      ierr = KSPGLTRGetNormD(tao->ksp, &norm_d);CHKERRQ(ierr);
    }

    if (0.0 == tao->trust) {
      /* Radius was uninitialized; use the norm of the direction */
      if (norm_d > 0.0) {
        tao->trust = norm_d;

        /* Modify the radius if it is too large or small */
        tao->trust = PetscMax(tao->trust, tl->min_radius);
        tao->trust = PetscMin(tao->trust, tl->max_radius);
      } else {
        /* The direction was bad; set radius to default value and re-solve
           the trust-region subproblem to get a direction */
        tao->trust = tao->trust0;

        /* Modify the radius if it is too large or small */
        tao->trust = PetscMax(tao->trust, tl->min_radius);
        tao->trust = PetscMin(tao->trust, tl->max_radius);

        if (NTL_KSP_NASH == tl->ksp_type) {
          ierr = KSPNASHSetRadius(tao->ksp,tl->max_radius);CHKERRQ(ierr);
          ierr = KSPSolve(tao->ksp, tao->gradient, tao->stepdirection);CHKERRQ(ierr);
          ierr = KSPGetIterationNumber(tao->ksp,&its);CHKERRQ(ierr);
          tao->ksp_its+=its;
          tao->ksp_tot_its+=its;
          ierr = KSPNASHGetNormD(tao->ksp, &norm_d);CHKERRQ(ierr);
        } else if (NTL_KSP_STCG == tl->ksp_type) {
          ierr = KSPSTCGSetRadius(tao->ksp,tl->max_radius);CHKERRQ(ierr);
          ierr = KSPSolve(tao->ksp, tao->gradient, tao->stepdirection);CHKERRQ(ierr);
          ierr = KSPGetIterationNumber(tao->ksp,&its);CHKERRQ(ierr);
          tao->ksp_its+=its;
          tao->ksp_tot_its+=its;
          ierr = KSPSTCGGetNormD(tao->ksp, &norm_d);CHKERRQ(ierr);
        } else { /* NTL_KSP_GLTR */
          ierr = KSPGLTRSetRadius(tao->ksp,tl->max_radius);CHKERRQ(ierr);
          ierr = KSPSolve(tao->ksp, tao->gradient, tao->stepdirection);CHKERRQ(ierr);
          ierr = KSPGetIterationNumber(tao->ksp,&its);CHKERRQ(ierr);
          tao->ksp_its+=its;
          tao->ksp_tot_its+=its;
          ierr = KSPGLTRGetNormD(tao->ksp, &norm_d);CHKERRQ(ierr);
        }


        if (norm_d == 0.0) SETERRQ(PETSC_COMM_SELF,1, "Initial direction zero");
      }
    }

    ierr = VecScale(tao->stepdirection, -1.0);CHKERRQ(ierr);
    ierr = KSPGetConvergedReason(tao->ksp, &ksp_reason);CHKERRQ(ierr);
    if ((KSP_DIVERGED_INDEFINITE_PC == ksp_reason) && (NTL_PC_BFGS == tl->pc_type) && (bfgsUpdates > 1)) {
      /* Preconditioner is numerically indefinite; reset the
         approximate if using BFGS preconditioning. */

      if (f != 0.0) {
        delta = 2.0 * PetscAbsScalar(f) / (gnorm*gnorm);
      } else {
        delta = 2.0 / (gnorm*gnorm);
      }
      ierr = MatLMVMSetDelta(tl->M, delta);CHKERRQ(ierr);
      ierr = MatLMVMReset(tl->M);CHKERRQ(ierr);
      ierr = MatLMVMUpdate(tl->M, tao->solution, tao->gradient);CHKERRQ(ierr);
      bfgsUpdates = 1;
    }

    /* Check trust-region reduction conditions */
    tr_reject = 0;
    if (NTL_UPDATE_REDUCTION == tl->update_type) {
      /* Get predicted reduction */
      if (NTL_KSP_NASH == tl->ksp_type) {
        ierr = KSPNASHGetObjFcn(tao->ksp,&prered);CHKERRQ(ierr);
      } else if (NTL_KSP_STCG == tl->ksp_type) {
        ierr = KSPSTCGGetObjFcn(tao->ksp,&prered);CHKERRQ(ierr);
      } else { /* gltr */
        ierr = KSPGLTRGetObjFcn(tao->ksp,&prered);CHKERRQ(ierr);
      }

      if (prered >= 0.0) {
        /* The predicted reduction has the wrong sign.  This cannot
           happen in infinite precision arithmetic.  Step should
           be rejected! */
        tao->trust = tl->alpha1 * PetscMin(tao->trust, norm_d);
        tr_reject = 1;
      } else {
        /* Compute trial step and function value */
        ierr = VecCopy(tao->solution, tl->W);CHKERRQ(ierr);
        ierr = VecAXPY(tl->W, 1.0, tao->stepdirection);CHKERRQ(ierr);
        ierr = TaoComputeObjective(tao, tl->W, &ftrial);CHKERRQ(ierr);

        if (PetscIsInfOrNanReal(ftrial)) {
          tao->trust = tl->alpha1 * PetscMin(tao->trust, norm_d);
          tr_reject = 1;
        } else {
          /* Compute and actual reduction */
          actred = f - ftrial;
          prered = -prered;
          if ((PetscAbsScalar(actred) <= tl->epsilon) &&
              (PetscAbsScalar(prered) <= tl->epsilon)) {
            kappa = 1.0;
          } else {
            kappa = actred / prered;
          }

          /* Accept of reject the step and update radius */
          if (kappa < tl->eta1) {
            /* Reject the step */
            tao->trust = tl->alpha1 * PetscMin(tao->trust, norm_d);
            tr_reject = 1;
          } else {
            /* Accept the step */
            if (kappa < tl->eta2) {
              /* Marginal bad step */
              tao->trust = tl->alpha2 * PetscMin(tao->trust, norm_d);
            } else if (kappa < tl->eta3) {
              /* Reasonable step */
              tao->trust = tl->alpha3 * tao->trust;
            } else if (kappa < tl->eta4) {
              /* Good step */
              tao->trust = PetscMax(tl->alpha4 * norm_d, tao->trust);
            } else {
              /* Very good step */
              tao->trust = PetscMax(tl->alpha5 * norm_d, tao->trust);
            }
          }
        }
      }
    } else {
      /* Get predicted reduction */
      if (NTL_KSP_NASH == tl->ksp_type) {
        ierr = KSPNASHGetObjFcn(tao->ksp,&prered);CHKERRQ(ierr);
      } else if (NTL_KSP_STCG == tl->ksp_type) {
        ierr = KSPSTCGGetObjFcn(tao->ksp,&prered);CHKERRQ(ierr);
      } else { /* gltr */
        ierr = KSPGLTRGetObjFcn(tao->ksp,&prered);CHKERRQ(ierr);
      }

      if (prered >= 0.0) {
        /* The predicted reduction has the wrong sign.  This cannot
           happen in infinite precision arithmetic.  Step should
           be rejected! */
        tao->trust = tl->gamma1 * PetscMin(tao->trust, norm_d);
        tr_reject = 1;
      } else {
        ierr = VecCopy(tao->solution, tl->W);CHKERRQ(ierr);
        ierr = VecAXPY(tl->W, 1.0, tao->stepdirection);CHKERRQ(ierr);
        ierr = TaoComputeObjective(tao, tl->W, &ftrial);CHKERRQ(ierr);
        if (PetscIsInfOrNanReal(ftrial)) {
          tao->trust = tl->gamma1 * PetscMin(tao->trust, norm_d);
          tr_reject = 1;
        } else {
          ierr = VecDot(tao->gradient, tao->stepdirection, &gdx);CHKERRQ(ierr);

          actred = f - ftrial;
          prered = -prered;
          if ((PetscAbsScalar(actred) <= tl->epsilon) &&
              (PetscAbsScalar(prered) <= tl->epsilon)) {
            kappa = 1.0;
          } else {
            kappa = actred / prered;
          }

          tau_1 = tl->theta * gdx / (tl->theta * gdx - (1.0 - tl->theta) * prered + actred);
          tau_2 = tl->theta * gdx / (tl->theta * gdx + (1.0 + tl->theta) * prered - actred);
          tau_min = PetscMin(tau_1, tau_2);
          tau_max = PetscMax(tau_1, tau_2);

          if (kappa >= 1.0 - tl->mu1) {
            /* Great agreement; accept step and update radius */
            if (tau_max < 1.0) {
              tao->trust = PetscMax(tao->trust, tl->gamma3 * norm_d);
            } else if (tau_max > tl->gamma4) {
              tao->trust = PetscMax(tao->trust, tl->gamma4 * norm_d);
            } else {
              tao->trust = PetscMax(tao->trust, tau_max * norm_d);
            }
          } else if (kappa >= 1.0 - tl->mu2) {
            /* Good agreement */

            if (tau_max < tl->gamma2) {
              tao->trust = tl->gamma2 * PetscMin(tao->trust, norm_d);
            } else if (tau_max > tl->gamma3) {
              tao->trust = PetscMax(tao->trust, tl->gamma3 * norm_d);
            } else if (tau_max < 1.0) {
              tao->trust = tau_max * PetscMin(tao->trust, norm_d);
            } else {
              tao->trust = PetscMax(tao->trust, tau_max * norm_d);
            }
          } else {
            /* Not good agreement */
            if (tau_min > 1.0) {
              tao->trust = tl->gamma2 * PetscMin(tao->trust, norm_d);
            } else if (tau_max < tl->gamma1) {
              tao->trust = tl->gamma1 * PetscMin(tao->trust, norm_d);
            } else if ((tau_min < tl->gamma1) && (tau_max >= 1.0)) {
              tao->trust = tl->gamma1 * PetscMin(tao->trust, norm_d);
            } else if ((tau_1 >= tl->gamma1) && (tau_1 < 1.0) && ((tau_2 < tl->gamma1) || (tau_2 >= 1.0))) {
              tao->trust = tau_1 * PetscMin(tao->trust, norm_d);
            } else if ((tau_2 >= tl->gamma1) && (tau_2 < 1.0) && ((tau_1 < tl->gamma1) || (tau_2 >= 1.0))) {
              tao->trust = tau_2 * PetscMin(tao->trust, norm_d);
            } else {
              tao->trust = tau_max * PetscMin(tao->trust, norm_d);
            }
            tr_reject = 1;
          }
        }
      }
    }

    if (tr_reject) {
      /* The trust-region constraints rejected the step.  Apply a linesearch.
         Check for descent direction. */
      ierr = VecDot(tao->stepdirection, tao->gradient, &gdx);CHKERRQ(ierr);
      if ((gdx >= 0.0) || PetscIsInfOrNanReal(gdx)) {
        /* Newton step is not descent or direction produced Inf or NaN */

        if (NTL_PC_BFGS != tl->pc_type) {
          /* We don't have the bfgs matrix around and updated
             Must use gradient direction in this case */
          ierr = VecCopy(tao->gradient, tao->stepdirection);CHKERRQ(ierr);
          ierr = VecScale(tao->stepdirection, -1.0);CHKERRQ(ierr);
          ++tl->grad;
          stepType = NTL_GRADIENT;
        } else {
          /* Attempt to use the BFGS direction */
          ierr = MatLMVMSolve(tl->M, tao->gradient, tao->stepdirection);CHKERRQ(ierr);
          ierr = VecScale(tao->stepdirection, -1.0);CHKERRQ(ierr);

          /* Check for success (descent direction) */
          ierr = VecDot(tao->stepdirection, tao->gradient, &gdx);CHKERRQ(ierr);
          if ((gdx >= 0) || PetscIsInfOrNanReal(gdx)) {
            /* BFGS direction is not descent or direction produced not a number
               We can assert bfgsUpdates > 1 in this case because
               the first solve produces the scaled gradient direction,
               which is guaranteed to be descent */

            /* Use steepest descent direction (scaled) */
            if (f != 0.0) {
              delta = 2.0 * PetscAbsScalar(f) / (gnorm*gnorm);
            } else {
              delta = 2.0 / (gnorm*gnorm);
            }
            ierr = MatLMVMSetDelta(tl->M, delta);CHKERRQ(ierr);
            ierr = MatLMVMReset(tl->M);CHKERRQ(ierr);
            ierr = MatLMVMUpdate(tl->M, tao->solution, tao->gradient);CHKERRQ(ierr);
            ierr = MatLMVMSolve(tl->M, tao->gradient, tao->stepdirection);CHKERRQ(ierr);
            ierr = VecScale(tao->stepdirection, -1.0);CHKERRQ(ierr);

            bfgsUpdates = 1;
            ++tl->sgrad;
            stepType = NTL_SCALED_GRADIENT;
          } else {
            if (1 == bfgsUpdates) {
              /* The first BFGS direction is always the scaled gradient */
              ++tl->sgrad;
              stepType = NTL_SCALED_GRADIENT;
            } else {
              ++tl->bfgs;
              stepType = NTL_BFGS;
            }
          }
        }
      } else {
        /* Computed Newton step is descent */
        ++tl->newt;
        stepType = NTL_NEWTON;
      }

      /* Perform the linesearch */
      fold = f;
      ierr = VecCopy(tao->solution, tl->Xold);CHKERRQ(ierr);
      ierr = VecCopy(tao->gradient, tl->Gold);CHKERRQ(ierr);

      step = 1.0;
      ierr = TaoLineSearchApply(tao->linesearch, tao->solution, &f, tao->gradient, tao->stepdirection, &step, &ls_reason);CHKERRQ(ierr);
      ierr = TaoAddLineSearchCounts(tao);CHKERRQ(ierr);

      while (ls_reason != TAOLINESEARCH_SUCCESS && ls_reason != TAOLINESEARCH_SUCCESS_USER && stepType != NTL_GRADIENT) {      /* Linesearch failed */
        /* Linesearch failed */
        f = fold;
        ierr = VecCopy(tl->Xold, tao->solution);CHKERRQ(ierr);
        ierr = VecCopy(tl->Gold, tao->gradient);CHKERRQ(ierr);

        switch(stepType) {
        case NTL_NEWTON:
          /* Failed to obtain acceptable iterate with Newton step */

          if (NTL_PC_BFGS != tl->pc_type) {
            /* We don't have the bfgs matrix around and being updated
               Must use gradient direction in this case */
            ierr = VecCopy(tao->gradient, tao->stepdirection);CHKERRQ(ierr);
            ++tl->grad;
            stepType = NTL_GRADIENT;
          } else {
            /* Attempt to use the BFGS direction */
            ierr = MatLMVMSolve(tl->M, tao->gradient, tao->stepdirection);CHKERRQ(ierr);


            /* Check for success (descent direction) */
            ierr = VecDot(tao->stepdirection, tao->gradient, &gdx);CHKERRQ(ierr);
            if ((gdx <= 0) || PetscIsInfOrNanReal(gdx)) {
              /* BFGS direction is not descent or direction produced
                 not a number.  We can assert bfgsUpdates > 1 in this case
                 Use steepest descent direction (scaled) */

              if (f != 0.0) {
                delta = 2.0 * PetscAbsScalar(f) / (gnorm*gnorm);
              } else {
                delta = 2.0 / (gnorm*gnorm);
              }
              ierr = MatLMVMSetDelta(tl->M, delta);CHKERRQ(ierr);
              ierr = MatLMVMReset(tl->M);CHKERRQ(ierr);
              ierr = MatLMVMUpdate(tl->M, tao->solution, tao->gradient);CHKERRQ(ierr);
              ierr = MatLMVMSolve(tl->M, tao->gradient, tao->stepdirection);CHKERRQ(ierr);

              bfgsUpdates = 1;
              ++tl->sgrad;
              stepType = NTL_SCALED_GRADIENT;
            } else {
              if (1 == bfgsUpdates) {
                /* The first BFGS direction is always the scaled gradient */
                ++tl->sgrad;
                stepType = NTL_SCALED_GRADIENT;
              } else {
                ++tl->bfgs;
                stepType = NTL_BFGS;
              }
            }
          }
          break;

        case NTL_BFGS:
          /* Can only enter if pc_type == NTL_PC_BFGS
             Failed to obtain acceptable iterate with BFGS step
             Attempt to use the scaled gradient direction */

          if (f != 0.0) {
            delta = 2.0 * PetscAbsScalar(f) / (gnorm*gnorm);
          } else {
            delta = 2.0 / (gnorm*gnorm);
          }
          ierr = MatLMVMSetDelta(tl->M, delta);CHKERRQ(ierr);
          ierr = MatLMVMReset(tl->M);CHKERRQ(ierr);
          ierr = MatLMVMUpdate(tl->M, tao->solution, tao->gradient);CHKERRQ(ierr);
          ierr = MatLMVMSolve(tl->M, tao->gradient, tao->stepdirection);CHKERRQ(ierr);

          bfgsUpdates = 1;
          ++tl->sgrad;
          stepType = NTL_SCALED_GRADIENT;
          break;

        case NTL_SCALED_GRADIENT:
          /* Can only enter if pc_type == NTL_PC_BFGS
             The scaled gradient step did not produce a new iterate;
             attemp to use the gradient direction.
             Need to make sure we are not using a different diagonal scaling */
          ierr = MatLMVMSetScale(tl->M, tl->Diag);CHKERRQ(ierr);
          ierr = MatLMVMSetDelta(tl->M, 1.0);CHKERRQ(ierr);
          ierr = MatLMVMReset(tl->M);CHKERRQ(ierr);
          ierr = MatLMVMUpdate(tl->M, tao->solution, tao->gradient);CHKERRQ(ierr);
          ierr = MatLMVMSolve(tl->M, tao->gradient, tao->stepdirection);CHKERRQ(ierr);

          bfgsUpdates = 1;
          ++tl->grad;
          stepType = NTL_GRADIENT;
          break;
        }
        ierr = VecScale(tao->stepdirection, -1.0);CHKERRQ(ierr);

        /* This may be incorrect; linesearch has values for stepmax and stepmin
           that should be reset. */
        step = 1.0;
        ierr = TaoLineSearchApply(tao->linesearch, tao->solution, &f, tao->gradient, tao->stepdirection, &step, &ls_reason);CHKERRQ(ierr);
        ierr = TaoAddLineSearchCounts(tao);CHKERRQ(ierr);
      }

      if (ls_reason != TAOLINESEARCH_SUCCESS && ls_reason != TAOLINESEARCH_SUCCESS_USER) {
        /* Failed to find an improving point */
        f = fold;
        ierr = VecCopy(tl->Xold, tao->solution);CHKERRQ(ierr);
        ierr = VecCopy(tl->Gold, tao->gradient);CHKERRQ(ierr);
        tao->trust = 0.0;
        step = 0.0;
        reason = TAO_DIVERGED_LS_FAILURE;
        tao->reason = TAO_DIVERGED_LS_FAILURE;
        break;
      } else if (stepType == NTL_NEWTON) {
        if (step < tl->nu1) {
          /* Very bad step taken; reduce radius */
          tao->trust = tl->omega1 * PetscMin(norm_d, tao->trust);
        } else if (step < tl->nu2) {
          /* Reasonably bad step taken; reduce radius */
          tao->trust = tl->omega2 * PetscMin(norm_d, tao->trust);
        } else if (step < tl->nu3) {
          /* Reasonable step was taken; leave radius alone */
          if (tl->omega3 < 1.0) {
            tao->trust = tl->omega3 * PetscMin(norm_d, tao->trust);
          } else if (tl->omega3 > 1.0) {
            tao->trust = PetscMax(tl->omega3 * norm_d, tao->trust);
          }
        } else if (step < tl->nu4) {
          /* Full step taken; increase the radius */
          tao->trust = PetscMax(tl->omega4 * norm_d, tao->trust);
        } else {
          /* More than full step taken; increase the radius */
          tao->trust = PetscMax(tl->omega5 * norm_d, tao->trust);
        }
      } else {
        /* Newton step was not good; reduce the radius */
        tao->trust = tl->omega1 * PetscMin(norm_d, tao->trust);
      }
    } else {
      /* Trust-region step is accepted */
      ierr = VecCopy(tl->W, tao->solution);CHKERRQ(ierr);
      f = ftrial;
      ierr = TaoComputeGradient(tao, tao->solution, tao->gradient);CHKERRQ(ierr);
      ++tl->ntrust;
    }

    /* The radius may have been increased; modify if it is too large */
    tao->trust = PetscMin(tao->trust, tl->max_radius);

    /* Check for converged */
    ierr = VecNorm(tao->gradient, NORM_2, &gnorm);CHKERRQ(ierr);
    if (PetscIsInfOrNanReal(f) || PetscIsInfOrNanReal(gnorm)) SETERRQ(PETSC_COMM_SELF,1,"User provided compute function generated Not-a-Number");
    needH = 1;

    ierr = TaoMonitor(tao, tao->niter, f, gnorm, 0.0, tao->trust, &reason);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

/* ---------------------------------------------------------- */
#undef __FUNCT__
#define __FUNCT__ "TaoSetUp_NTL"
static PetscErrorCode TaoSetUp_NTL(Tao tao)
{
  TAO_NTL        *tl = (TAO_NTL *)tao->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (!tao->gradient) {ierr = VecDuplicate(tao->solution, &tao->gradient);CHKERRQ(ierr); }
  if (!tao->stepdirection) {ierr = VecDuplicate(tao->solution, &tao->stepdirection);CHKERRQ(ierr);}
  if (!tl->W) { ierr = VecDuplicate(tao->solution, &tl->W);CHKERRQ(ierr);}
  if (!tl->Xold) { ierr = VecDuplicate(tao->solution, &tl->Xold);CHKERRQ(ierr);}
  if (!tl->Gold) { ierr = VecDuplicate(tao->solution, &tl->Gold);CHKERRQ(ierr);}
  tl->Diag = 0;
  tl->M = 0;
  PetscFunctionReturn(0);
}

/*------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "TaoDestroy_NTL"
static PetscErrorCode TaoDestroy_NTL(Tao tao)
{
  TAO_NTL        *tl = (TAO_NTL *)tao->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (tao->setupcalled) {
    ierr = VecDestroy(&tl->W);CHKERRQ(ierr);
    ierr = VecDestroy(&tl->Xold);CHKERRQ(ierr);
    ierr = VecDestroy(&tl->Gold);CHKERRQ(ierr);
  }
  ierr = VecDestroy(&tl->Diag);CHKERRQ(ierr);
  ierr = MatDestroy(&tl->M);CHKERRQ(ierr);
  ierr = PetscFree(tao->data);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/*------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "TaoSetFromOptions_NTL"
static PetscErrorCode TaoSetFromOptions_NTL(PetscOptionItems *PetscOptionsObject,Tao tao)
{
  TAO_NTL        *tl = (TAO_NTL *)tao->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscOptionsHead(PetscOptionsObject,"Newton trust region with line search method for unconstrained optimization");CHKERRQ(ierr);
  ierr = PetscOptionsEList("-tao_ntl_ksp_type", "ksp type", "", NTL_KSP, NTL_KSP_TYPES, NTL_KSP[tl->ksp_type], &tl->ksp_type,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsEList("-tao_ntl_pc_type", "pc type", "", NTL_PC, NTL_PC_TYPES, NTL_PC[tl->pc_type], &tl->pc_type,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsEList("-tao_ntl_bfgs_scale_type", "bfgs scale type", "", BFGS_SCALE, BFGS_SCALE_TYPES, BFGS_SCALE[tl->bfgs_scale_type], &tl->bfgs_scale_type,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsEList("-tao_ntl_init_type", "radius initialization type", "", NTL_INIT, NTL_INIT_TYPES, NTL_INIT[tl->init_type], &tl->init_type,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsEList("-tao_ntl_update_type", "radius update type", "", NTL_UPDATE, NTL_UPDATE_TYPES, NTL_UPDATE[tl->update_type], &tl->update_type,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsReal("-tao_ntl_eta1", "poor steplength; reduce radius", "", tl->eta1, &tl->eta1,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsReal("-tao_ntl_eta2", "reasonable steplength; leave radius alone", "", tl->eta2, &tl->eta2,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsReal("-tao_ntl_eta3", "good steplength; increase radius", "", tl->eta3, &tl->eta3,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsReal("-tao_ntl_eta4", "excellent steplength; greatly increase radius", "", tl->eta4, &tl->eta4,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsReal("-tao_ntl_alpha1", "", "", tl->alpha1, &tl->alpha1,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsReal("-tao_ntl_alpha2", "", "", tl->alpha2, &tl->alpha2,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsReal("-tao_ntl_alpha3", "", "", tl->alpha3, &tl->alpha3,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsReal("-tao_ntl_alpha4", "", "", tl->alpha4, &tl->alpha4,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsReal("-tao_ntl_alpha5", "", "", tl->alpha5, &tl->alpha5,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsReal("-tao_ntl_nu1", "poor steplength; reduce radius", "", tl->nu1, &tl->nu1,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsReal("-tao_ntl_nu2", "reasonable steplength; leave radius alone", "", tl->nu2, &tl->nu2,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsReal("-tao_ntl_nu3", "good steplength; increase radius", "", tl->nu3, &tl->nu3,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsReal("-tao_ntl_nu4", "excellent steplength; greatly increase radius", "", tl->nu4, &tl->nu4,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsReal("-tao_ntl_omega1", "", "", tl->omega1, &tl->omega1,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsReal("-tao_ntl_omega2", "", "", tl->omega2, &tl->omega2,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsReal("-tao_ntl_omega3", "", "", tl->omega3, &tl->omega3,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsReal("-tao_ntl_omega4", "", "", tl->omega4, &tl->omega4,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsReal("-tao_ntl_omega5", "", "", tl->omega5, &tl->omega5,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsReal("-tao_ntl_mu1_i", "", "", tl->mu1_i, &tl->mu1_i,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsReal("-tao_ntl_mu2_i", "", "", tl->mu2_i, &tl->mu2_i,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsReal("-tao_ntl_gamma1_i", "", "", tl->gamma1_i, &tl->gamma1_i,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsReal("-tao_ntl_gamma2_i", "", "", tl->gamma2_i, &tl->gamma2_i,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsReal("-tao_ntl_gamma3_i", "", "", tl->gamma3_i, &tl->gamma3_i,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsReal("-tao_ntl_gamma4_i", "", "", tl->gamma4_i, &tl->gamma4_i,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsReal("-tao_ntl_theta_i", "", "", tl->theta_i, &tl->theta_i,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsReal("-tao_ntl_mu1", "", "", tl->mu1, &tl->mu1,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsReal("-tao_ntl_mu2", "", "", tl->mu2, &tl->mu2,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsReal("-tao_ntl_gamma1", "", "", tl->gamma1, &tl->gamma1,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsReal("-tao_ntl_gamma2", "", "", tl->gamma2, &tl->gamma2,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsReal("-tao_ntl_gamma3", "", "", tl->gamma3, &tl->gamma3,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsReal("-tao_ntl_gamma4", "", "", tl->gamma4, &tl->gamma4,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsReal("-tao_ntl_theta", "", "", tl->theta, &tl->theta,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsReal("-tao_ntl_min_radius", "lower bound on initial radius", "", tl->min_radius, &tl->min_radius,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsReal("-tao_ntl_max_radius", "upper bound on radius", "", tl->max_radius, &tl->max_radius,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsReal("-tao_ntl_epsilon", "tolerance used when computing actual and predicted reduction", "", tl->epsilon, &tl->epsilon,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsTail();CHKERRQ(ierr);
  ierr = TaoLineSearchSetFromOptions(tao->linesearch);CHKERRQ(ierr);
  ierr = KSPSetFromOptions(tao->ksp);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/*------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "TaoView_NTL"
static PetscErrorCode TaoView_NTL(Tao tao, PetscViewer viewer)
{
  TAO_NTL        *tl = (TAO_NTL *)tao->data;
  PetscInt       nrejects;
  PetscBool      isascii;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERASCII,&isascii);CHKERRQ(ierr);
  if (isascii) {
    ierr = PetscViewerASCIIPushTab(viewer);CHKERRQ(ierr);
    if (NTL_PC_BFGS == tl->pc_type && tl->M) {
      ierr = MatLMVMGetRejects(tl->M, &nrejects);CHKERRQ(ierr);
      ierr = PetscViewerASCIIPrintf(viewer, "Rejected matrix updates: %D\n", nrejects);CHKERRQ(ierr);
    }
    ierr = PetscViewerASCIIPrintf(viewer, "Trust-region steps: %D\n", tl->ntrust);CHKERRQ(ierr);
    ierr = PetscViewerASCIIPrintf(viewer, "Newton search steps: %D\n", tl->newt);CHKERRQ(ierr);
    ierr = PetscViewerASCIIPrintf(viewer, "BFGS search steps: %D\n", tl->bfgs);CHKERRQ(ierr);
    ierr = PetscViewerASCIIPrintf(viewer, "Scaled gradient search steps: %D\n", tl->sgrad);CHKERRQ(ierr);
    ierr = PetscViewerASCIIPrintf(viewer, "Gradient search steps: %D\n", tl->grad);CHKERRQ(ierr);
    ierr = PetscViewerASCIIPopTab(viewer);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

/* ---------------------------------------------------------- */
/*MC
  TAONTR - Newton's method with trust region and linesearch
  for unconstrained minimization.
  At each iteration, the Newton trust region method solves the system for d
  and performs a line search in the d direction:

            min_d  .5 dT Hk d + gkT d,  s.t.   ||d|| < Delta_k

  Options Database Keys:
+ -tao_ntl_ksp_type - "nash","stcg","gltr"
. -tao_ntl_pc_type - "none","ahess","bfgs","petsc"
. -tao_ntl_bfgs_scale_type - type of scaling with bfgs pc, "ahess" or "bfgs"
. -tao_ntl_init_type - "constant","direction","interpolation"
. -tao_ntl_update_type - "reduction","interpolation"
. -tao_ntl_min_radius - lower bound on trust region radius
. -tao_ntl_max_radius - upper bound on trust region radius
. -tao_ntl_epsilon - tolerance for accepting actual / predicted reduction
. -tao_ntl_mu1_i - mu1 interpolation init factor
. -tao_ntl_mu2_i - mu2 interpolation init factor
. -tao_ntl_gamma1_i - gamma1 interpolation init factor
. -tao_ntl_gamma2_i - gamma2 interpolation init factor
. -tao_ntl_gamma3_i - gamma3 interpolation init factor
. -tao_ntl_gamma4_i - gamma4 interpolation init factor
. -tao_ntl_theta_i - thetha1 interpolation init factor
. -tao_ntl_eta1 - eta1 reduction update factor
. -tao_ntl_eta2 - eta2 reduction update factor
. -tao_ntl_eta3 - eta3 reduction update factor
. -tao_ntl_eta4 - eta4 reduction update factor
. -tao_ntl_alpha1 - alpha1 reduction update factor
. -tao_ntl_alpha2 - alpha2 reduction update factor
. -tao_ntl_alpha3 - alpha3 reduction update factor
. -tao_ntl_alpha4 - alpha4 reduction update factor
. -tao_ntl_alpha4 - alpha4 reduction update factor
. -tao_ntl_mu1 - mu1 interpolation update
. -tao_ntl_mu2 - mu2 interpolation update
. -tao_ntl_gamma1 - gamma1 interpolcation update
. -tao_ntl_gamma2 - gamma2 interpolcation update
. -tao_ntl_gamma3 - gamma3 interpolcation update
. -tao_ntl_gamma4 - gamma4 interpolation update
- -tao_ntl_theta - theta1 interpolation update

  Level: beginner
M*/

#undef __FUNCT__
#define __FUNCT__ "TaoCreate_NTL"
PETSC_EXTERN PetscErrorCode TaoCreate_NTL(Tao tao)
{
  TAO_NTL        *tl;
  PetscErrorCode ierr;
  const char     *morethuente_type = TAOLINESEARCHMT;

  PetscFunctionBegin;
  ierr = PetscNewLog(tao,&tl);CHKERRQ(ierr);
  tao->ops->setup = TaoSetUp_NTL;
  tao->ops->solve = TaoSolve_NTL;
  tao->ops->view = TaoView_NTL;
  tao->ops->setfromoptions = TaoSetFromOptions_NTL;
  tao->ops->destroy = TaoDestroy_NTL;

  /* Override default settings (unless already changed) */
  if (!tao->max_it_changed) tao->max_it = 50;
  if (!tao->trust0_changed) tao->trust0 = 100.0;

  tao->data = (void*)tl;

  /* Default values for trust-region radius update based on steplength */
  tl->nu1 = 0.25;
  tl->nu2 = 0.50;
  tl->nu3 = 1.00;
  tl->nu4 = 1.25;

  tl->omega1 = 0.25;
  tl->omega2 = 0.50;
  tl->omega3 = 1.00;
  tl->omega4 = 2.00;
  tl->omega5 = 4.00;

  /* Default values for trust-region radius update based on reduction */
  tl->eta1 = 1.0e-4;
  tl->eta2 = 0.25;
  tl->eta3 = 0.50;
  tl->eta4 = 0.90;

  tl->alpha1 = 0.25;
  tl->alpha2 = 0.50;
  tl->alpha3 = 1.00;
  tl->alpha4 = 2.00;
  tl->alpha5 = 4.00;

  /* Default values for trust-region radius update based on interpolation */
  tl->mu1 = 0.10;
  tl->mu2 = 0.50;

  tl->gamma1 = 0.25;
  tl->gamma2 = 0.50;
  tl->gamma3 = 2.00;
  tl->gamma4 = 4.00;

  tl->theta = 0.05;

  /* Default values for trust region initialization based on interpolation */
  tl->mu1_i = 0.35;
  tl->mu2_i = 0.50;

  tl->gamma1_i = 0.0625;
  tl->gamma2_i = 0.5;
  tl->gamma3_i = 2.0;
  tl->gamma4_i = 5.0;

  tl->theta_i = 0.25;

  /* Remaining parameters */
  tl->min_radius = 1.0e-10;
  tl->max_radius = 1.0e10;
  tl->epsilon = 1.0e-6;

  tl->ksp_type        = NTL_KSP_STCG;
  tl->pc_type         = NTL_PC_BFGS;
  tl->bfgs_scale_type = BFGS_SCALE_AHESS;
  tl->init_type       = NTL_INIT_INTERPOLATION;
  tl->update_type     = NTL_UPDATE_REDUCTION;

  ierr = TaoLineSearchCreate(((PetscObject)tao)->comm, &tao->linesearch);CHKERRQ(ierr);
  ierr = TaoLineSearchSetType(tao->linesearch, morethuente_type);CHKERRQ(ierr);
  ierr = TaoLineSearchUseTaoRoutines(tao->linesearch, tao);CHKERRQ(ierr);
  ierr = TaoLineSearchSetOptionsPrefix(tao->linesearch,tao->hdr.prefix);CHKERRQ(ierr);
  ierr = KSPCreate(((PetscObject)tao)->comm, &tao->ksp);CHKERRQ(ierr);
  ierr = KSPSetOptionsPrefix(tao->ksp, tao->hdr.prefix);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}




