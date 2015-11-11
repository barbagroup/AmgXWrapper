/*
  Code for timestepping with implicit Theta method
*/
#include <petsc/private/tsimpl.h>                /*I   "petscts.h"   I*/
#include <petscsnes.h>
#include <petscdm.h>
#include <petscmat.h>

typedef struct {
  Vec          X,Xdot;                   /* Storage for one stage */
  Vec          X0;                       /* work vector to store X0 */
  Vec          affine;                   /* Affine vector needed for residual at beginning of step */
  Vec          *VecsDeltaLam;             /* Increment of the adjoint sensitivity w.r.t IC at stage*/
  Vec          *VecsDeltaMu;              /* Increment of the adjoint sensitivity w.r.t P at stage*/
  Vec          *VecsSensiTemp;            /* Vector to be timed with Jacobian transpose*/
  PetscBool    extrapolate;
  PetscBool    endpoint;
  PetscReal    Theta;
  PetscReal    stage_time;
  TSStepStatus status;
  char         *name;
  PetscInt     order;
  PetscReal    ccfl;               /* Placeholder for CFL coefficient relative to forward Euler */
  PetscBool    adapt;  /* use time-step adaptivity ? */
  PetscReal    ptime;
} TS_Theta;

#undef __FUNCT__
#define __FUNCT__ "TSThetaGetX0AndXdot"
static PetscErrorCode TSThetaGetX0AndXdot(TS ts,DM dm,Vec *X0,Vec *Xdot)
{
  TS_Theta       *th = (TS_Theta*)ts->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (X0) {
    if (dm && dm != ts->dm) {
      ierr = DMGetNamedGlobalVector(dm,"TSTheta_X0",X0);CHKERRQ(ierr);
    } else *X0 = ts->vec_sol;
  }
  if (Xdot) {
    if (dm && dm != ts->dm) {
      ierr = DMGetNamedGlobalVector(dm,"TSTheta_Xdot",Xdot);CHKERRQ(ierr);
    } else *Xdot = th->Xdot;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TSThetaRestoreX0AndXdot"
static PetscErrorCode TSThetaRestoreX0AndXdot(TS ts,DM dm,Vec *X0,Vec *Xdot)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (X0) {
    if (dm && dm != ts->dm) {
      ierr = DMRestoreNamedGlobalVector(dm,"TSTheta_X0",X0);CHKERRQ(ierr);
    }
  }
  if (Xdot) {
    if (dm && dm != ts->dm) {
      ierr = DMRestoreNamedGlobalVector(dm,"TSTheta_Xdot",Xdot);CHKERRQ(ierr);
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMCoarsenHook_TSTheta"
static PetscErrorCode DMCoarsenHook_TSTheta(DM fine,DM coarse,void *ctx)
{

  PetscFunctionBegin;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMRestrictHook_TSTheta"
static PetscErrorCode DMRestrictHook_TSTheta(DM fine,Mat restrct,Vec rscale,Mat inject,DM coarse,void *ctx)
{
  TS             ts = (TS)ctx;
  PetscErrorCode ierr;
  Vec            X0,Xdot,X0_c,Xdot_c;

  PetscFunctionBegin;
  ierr = TSThetaGetX0AndXdot(ts,fine,&X0,&Xdot);CHKERRQ(ierr);
  ierr = TSThetaGetX0AndXdot(ts,coarse,&X0_c,&Xdot_c);CHKERRQ(ierr);
  ierr = MatRestrict(restrct,X0,X0_c);CHKERRQ(ierr);
  ierr = MatRestrict(restrct,Xdot,Xdot_c);CHKERRQ(ierr);
  ierr = VecPointwiseMult(X0_c,rscale,X0_c);CHKERRQ(ierr);
  ierr = VecPointwiseMult(Xdot_c,rscale,Xdot_c);CHKERRQ(ierr);
  ierr = TSThetaRestoreX0AndXdot(ts,fine,&X0,&Xdot);CHKERRQ(ierr);
  ierr = TSThetaRestoreX0AndXdot(ts,coarse,&X0_c,&Xdot_c);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMSubDomainHook_TSTheta"
static PetscErrorCode DMSubDomainHook_TSTheta(DM dm,DM subdm,void *ctx)
{

  PetscFunctionBegin;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMSubDomainRestrictHook_TSTheta"
static PetscErrorCode DMSubDomainRestrictHook_TSTheta(DM dm,VecScatter gscat,VecScatter lscat,DM subdm,void *ctx)
{
  TS             ts = (TS)ctx;
  PetscErrorCode ierr;
  Vec            X0,Xdot,X0_sub,Xdot_sub;

  PetscFunctionBegin;
  ierr = TSThetaGetX0AndXdot(ts,dm,&X0,&Xdot);CHKERRQ(ierr);
  ierr = TSThetaGetX0AndXdot(ts,subdm,&X0_sub,&Xdot_sub);CHKERRQ(ierr);

  ierr = VecScatterBegin(gscat,X0,X0_sub,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
  ierr = VecScatterEnd(gscat,X0,X0_sub,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);

  ierr = VecScatterBegin(gscat,Xdot,Xdot_sub,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
  ierr = VecScatterEnd(gscat,Xdot,Xdot_sub,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);

  ierr = TSThetaRestoreX0AndXdot(ts,dm,&X0,&Xdot);CHKERRQ(ierr);
  ierr = TSThetaRestoreX0AndXdot(ts,subdm,&X0_sub,&Xdot_sub);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TSEvaluateStep_Theta"
static PetscErrorCode TSEvaluateStep_Theta(TS ts,PetscInt order,Vec U,PetscBool *done)
{
  PetscErrorCode ierr;
  TS_Theta       *th = (TS_Theta*)ts->data;

  PetscFunctionBegin;
  if (order == 0) SETERRQ(PetscObjectComm((PetscObject)ts),PETSC_ERR_USER,"No time-step adaptivity implemented for 1st order theta method; Run with -ts_adapt_type none");
  if (order == th->order) {
    if (th->endpoint) {
      ierr = VecCopy(th->X,U);CHKERRQ(ierr);
    } else {
      PetscReal shift = 1./(th->Theta*ts->time_step);
      ierr = VecAXPBYPCZ(th->Xdot,-shift,shift,0,U,th->X);CHKERRQ(ierr);
      ierr = VecAXPY(U,ts->time_step,th->Xdot);CHKERRQ(ierr);
    }
  } else if (order == th->order-1 && order) {
    ierr = VecWAXPY(U,ts->time_step,th->Xdot,th->X0);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TSRollBack_Theta"
static PetscErrorCode TSRollBack_Theta(TS ts)
{
  TS_Theta       *th = (TS_Theta*)ts->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = VecCopy(th->X0,ts->vec_sol);CHKERRQ(ierr);
  th->status    = TS_STEP_INCOMPLETE;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TSStep_Theta"
static PetscErrorCode TSStep_Theta(TS ts)
{
  TS_Theta       *th = (TS_Theta*)ts->data;
  PetscInt       its,lits,reject,next_scheme;
  PetscReal      next_time_step;
  TSAdapt        adapt;
  PetscBool      stageok,accept = PETSC_TRUE;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  th->status = TS_STEP_INCOMPLETE;
  ierr = VecCopy(ts->vec_sol,th->X0);CHKERRQ(ierr);
  for (reject=0; !ts->reason && th->status != TS_STEP_COMPLETE; ts->reject++) {
    PetscReal shift = 1./(th->Theta*ts->time_step);
    th->stage_time = ts->ptime + (th->endpoint ? 1. : th->Theta)*ts->time_step;
    ierr = TSPreStep(ts);CHKERRQ(ierr);
    ierr = TSPreStage(ts,th->stage_time);CHKERRQ(ierr);

    if (th->endpoint) {           /* This formulation assumes linear time-independent mass matrix */
      ierr = VecZeroEntries(th->Xdot);CHKERRQ(ierr);
      if (!th->affine) {ierr = VecDuplicate(ts->vec_sol,&th->affine);CHKERRQ(ierr);}
      ierr = TSComputeIFunction(ts,ts->ptime,ts->vec_sol,th->Xdot,th->affine,PETSC_FALSE);CHKERRQ(ierr);
      ierr = VecScale(th->affine,(th->Theta-1.)/th->Theta);CHKERRQ(ierr);
    }
    if (th->extrapolate) {
      ierr = VecWAXPY(th->X,1./shift,th->Xdot,ts->vec_sol);CHKERRQ(ierr);
    } else {
      ierr = VecCopy(ts->vec_sol,th->X);CHKERRQ(ierr);
    }
    ierr = SNESSolve(ts->snes,th->affine,th->X);CHKERRQ(ierr);
    ierr = SNESGetIterationNumber(ts->snes,&its);CHKERRQ(ierr);
    ierr = SNESGetLinearSolveIterations(ts->snes,&lits);CHKERRQ(ierr);
    ts->snes_its += its; ts->ksp_its += lits;
    ierr = TSPostStage(ts,th->stage_time,0,&(th->X));CHKERRQ(ierr);
    ierr = TSGetAdapt(ts,&adapt);CHKERRQ(ierr);
    ierr = TSAdaptCheckStage(adapt,ts,th->stage_time,th->X,&stageok);CHKERRQ(ierr);
    if (!stageok) {accept = PETSC_FALSE; goto reject_step;}

    ierr = TSEvaluateStep(ts,th->order,ts->vec_sol,NULL);CHKERRQ(ierr);
    th->status = TS_STEP_PENDING;
    /* Register only the current method as a candidate because we're not supporting multiple candidates yet. */
    ierr = TSGetAdapt(ts,&adapt);CHKERRQ(ierr);
    ierr = TSAdaptCandidatesClear(adapt);CHKERRQ(ierr);
    ierr = TSAdaptCandidateAdd(adapt,NULL,th->order,1,th->ccfl,1.0,PETSC_TRUE);CHKERRQ(ierr);
    ierr = TSAdaptChoose(adapt,ts,ts->time_step,&next_scheme,&next_time_step,&accept);CHKERRQ(ierr);
    if (!accept) {           /* Roll back the current step */
      ts->ptime += next_time_step; /* This will be undone in rollback */
      th->status = TS_STEP_INCOMPLETE;
      ierr = TSRollBack(ts);CHKERRQ(ierr);
      goto reject_step;
    }

    if (ts->vec_costintegral) {
      /* Evolve ts->vec_costintegral to compute integrals */
      if (th->endpoint) {
        ierr = TSAdjointComputeCostIntegrand(ts,ts->ptime,th->X0,ts->vec_costintegrand);CHKERRQ(ierr);
        ierr = VecAXPY(ts->vec_costintegral,ts->time_step*(1.-th->Theta),ts->vec_costintegrand);CHKERRQ(ierr);
      }
      ierr = TSAdjointComputeCostIntegrand(ts,th->stage_time,th->X,ts->vec_costintegrand);CHKERRQ(ierr);
      if (th->endpoint) {
        ierr = VecAXPY(ts->vec_costintegral,ts->time_step*th->Theta,ts->vec_costintegrand);CHKERRQ(ierr);
      }else {
        ierr = VecAXPY(ts->vec_costintegral,ts->time_step,ts->vec_costintegrand);CHKERRQ(ierr);
      }
    }

    /* ignore next_scheme for now */
    ts->ptime    += ts->time_step;
    ts->time_step = next_time_step;
    ts->steps++;
    th->status = TS_STEP_COMPLETE;
    break;

reject_step:
    if (!ts->reason && ++reject > ts->max_reject && ts->max_reject >= 0) {
      ts->reason = TS_DIVERGED_STEP_REJECTED;
      ierr = PetscInfo2(ts,"Step=%D, step rejections %D greater than current TS allowed, stopping solve\n",ts->steps,reject);CHKERRQ(ierr);
    }
    continue;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TSAdjointStep_Theta"
static PetscErrorCode TSAdjointStep_Theta(TS ts)
{
  TS_Theta            *th = (TS_Theta*)ts->data;
  Vec                 *VecsDeltaLam = th->VecsDeltaLam,*VecsDeltaMu = th->VecsDeltaMu,*VecsSensiTemp = th->VecsSensiTemp;
  PetscInt            nadj;
  PetscErrorCode      ierr;
  Mat                 J,Jp;
  KSP                 ksp;
  PetscReal           shift;

  PetscFunctionBegin;

  th->status = TS_STEP_INCOMPLETE;
  ierr = SNESGetKSP(ts->snes,&ksp);CHKERRQ(ierr);
  ierr = TSGetIJacobian(ts,&J,&Jp,NULL,NULL);CHKERRQ(ierr);

  /* If endpoint=1, th->ptime and th->X0 will be used; if endpoint=0, th->stage_time and th->X will be used. */
  th->stage_time = ts->ptime + (th->endpoint ? ts->time_step : (1.-th->Theta)*ts->time_step); /* time_step is negative*/
  th->ptime      = ts->ptime + ts->time_step;

  ierr = TSPreStep(ts);CHKERRQ(ierr);

  /* Build RHS */
  if (ts->vec_costintegral) { /* Cost function has an integral term */
    if (th->endpoint) {
      ierr = TSAdjointComputeDRDYFunction(ts,ts->ptime,ts->vec_sol,ts->vecs_drdy);CHKERRQ(ierr);
    }else {
      ierr = TSAdjointComputeDRDYFunction(ts,th->stage_time,th->X,ts->vecs_drdy);CHKERRQ(ierr);
    }
  }
  for (nadj=0; nadj<ts->numcost; nadj++) {
    ierr = VecCopy(ts->vecs_sensi[nadj],VecsSensiTemp[nadj]);CHKERRQ(ierr);
    ierr = VecScale(VecsSensiTemp[nadj],-1./(th->Theta*ts->time_step));CHKERRQ(ierr);
    if (ts->vec_costintegral) {
      ierr = VecAXPY(VecsSensiTemp[nadj],1.,ts->vecs_drdy[nadj]);CHKERRQ(ierr);
    }
  }

  /* Build LHS */
  shift = -1./(th->Theta*ts->time_step);
  if (th->endpoint) {
    ierr = TSComputeIJacobian(ts,ts->ptime,ts->vec_sol,th->Xdot,shift,J,Jp,PETSC_FALSE);CHKERRQ(ierr);
  }else {
    ierr = TSComputeIJacobian(ts,th->stage_time,th->X,th->Xdot,shift,J,Jp,PETSC_FALSE);CHKERRQ(ierr);
  }
  ierr = KSPSetOperators(ksp,J,Jp);CHKERRQ(ierr);

  /* Solve LHS X = RHS */
  for (nadj=0; nadj<ts->numcost; nadj++) {
    ierr = KSPSolveTranspose(ksp,VecsSensiTemp[nadj],VecsDeltaLam[nadj]);CHKERRQ(ierr);
  }

  /* Update sensitivities, and evaluate integrals if there is any */
  if(th->endpoint) { /* two-stage case */
    if (th->Theta!=1.) {
      shift = -1./((th->Theta-1.)*ts->time_step);
      ierr  = TSComputeIJacobian(ts,th->ptime,th->X0,th->Xdot,shift,J,Jp,PETSC_FALSE);CHKERRQ(ierr);
      if (ts->vec_costintegral) {
        ierr = TSAdjointComputeDRDYFunction(ts,th->ptime,th->X0,ts->vecs_drdy);CHKERRQ(ierr);
        if (!ts->costintegralfwd) {
          /* Evolve ts->vec_costintegral to compute integrals */
          ierr = TSAdjointComputeCostIntegrand(ts,ts->ptime,ts->vec_sol,ts->vec_costintegrand);CHKERRQ(ierr);
          ierr = VecAXPY(ts->vec_costintegral,-ts->time_step*th->Theta,ts->vec_costintegrand);CHKERRQ(ierr);
          ierr = TSAdjointComputeCostIntegrand(ts,th->ptime,th->X0,ts->vec_costintegrand);CHKERRQ(ierr);
          ierr = VecAXPY(ts->vec_costintegral,ts->time_step*(th->Theta-1.),ts->vec_costintegrand);CHKERRQ(ierr);
        }
      }
      for (nadj=0; nadj<ts->numcost; nadj++) {
        ierr = MatMultTranspose(J,VecsDeltaLam[nadj],ts->vecs_sensi[nadj]);CHKERRQ(ierr);
        if (ts->vec_costintegral) {
          ierr = VecAXPY(ts->vecs_sensi[nadj],-1.,ts->vecs_drdy[nadj]);CHKERRQ(ierr);
        }
        ierr = VecScale(ts->vecs_sensi[nadj],1./shift);CHKERRQ(ierr);
      }
    }else { /* backward Euler */
      shift = 0.0;
      ierr  = TSComputeIJacobian(ts,ts->ptime,ts->vec_sol,th->Xdot,shift,J,Jp,PETSC_FALSE);CHKERRQ(ierr); /* get -f_y */
      for (nadj=0; nadj<ts->numcost; nadj++) {
        ierr = MatMultTranspose(J,VecsDeltaLam[nadj],VecsSensiTemp[nadj]);CHKERRQ(ierr);
        ierr = VecAXPY(ts->vecs_sensi[nadj],ts->time_step,VecsSensiTemp[nadj]);CHKERRQ(ierr);
        if (ts->vec_costintegral) {
          ierr = VecAXPY(ts->vecs_sensi[nadj],-ts->time_step,ts->vecs_drdy[nadj]);CHKERRQ(ierr);
          if (!ts->costintegralfwd) {
          /* Evolve ts->vec_costintegral to compute integrals */
            ierr = TSAdjointComputeCostIntegrand(ts,ts->ptime,ts->vec_sol,ts->vec_costintegrand);CHKERRQ(ierr);
            ierr = VecAXPY(ts->vec_costintegral,-ts->time_step*th->Theta,ts->vec_costintegrand);CHKERRQ(ierr);
          }
        }
      }
    }

    if (ts->vecs_sensip) { /* sensitivities wrt parameters */
      ierr = TSAdjointComputeRHSJacobian(ts,ts->ptime,ts->vec_sol,ts->Jacp);CHKERRQ(ierr);
      for (nadj=0; nadj<ts->numcost; nadj++) {
        ierr = MatMultTranspose(ts->Jacp,VecsDeltaLam[nadj],VecsDeltaMu[nadj]);CHKERRQ(ierr);
        ierr = VecAXPY(ts->vecs_sensip[nadj],-ts->time_step*th->Theta,VecsDeltaMu[nadj]);CHKERRQ(ierr);
      }
      if (th->Theta!=1.) {
        ierr = TSAdjointComputeRHSJacobian(ts,th->ptime,th->X0,ts->Jacp);CHKERRQ(ierr);
        for (nadj=0; nadj<ts->numcost; nadj++) {
          ierr = MatMultTranspose(ts->Jacp,VecsDeltaLam[nadj],VecsDeltaMu[nadj]);CHKERRQ(ierr);
          ierr = VecAXPY(ts->vecs_sensip[nadj],-ts->time_step*(1.-th->Theta),VecsDeltaMu[nadj]);CHKERRQ(ierr);
        }
      }
      if (ts->vec_costintegral) {
        ierr = TSAdjointComputeDRDPFunction(ts,ts->ptime,ts->vec_sol,ts->vecs_drdp);CHKERRQ(ierr);
        for (nadj=0; nadj<ts->numcost; nadj++) {
          ierr = VecAXPY(ts->vecs_sensip[nadj],-ts->time_step*th->Theta,ts->vecs_drdp[nadj]);CHKERRQ(ierr);
        }
        if (th->Theta!=1.) {
          ierr = TSAdjointComputeDRDPFunction(ts,th->ptime,th->X0,ts->vecs_drdp);CHKERRQ(ierr);
          for (nadj=0; nadj<ts->numcost; nadj++) {
            ierr = VecAXPY(ts->vecs_sensip[nadj],-ts->time_step*(1.-th->Theta),ts->vecs_drdp[nadj]);CHKERRQ(ierr);
          }
        }
      }
    }
  }else { /* one-stage case */
    shift = 0.0;
    ierr  = TSComputeIJacobian(ts,th->stage_time,th->X,th->Xdot,shift,J,Jp,PETSC_FALSE);CHKERRQ(ierr); /* get -f_y */
    if (ts->vec_costintegral) {
      ierr = TSAdjointComputeDRDYFunction(ts,th->stage_time,th->X,ts->vecs_drdy);CHKERRQ(ierr);
      if (!ts->costintegralfwd) {
        /* Evolve ts->vec_costintegral to compute integrals */
        ierr = TSAdjointComputeCostIntegrand(ts,th->stage_time,th->X,ts->vec_costintegrand);CHKERRQ(ierr);
        ierr = VecAXPY(ts->vec_costintegral,-ts->time_step,ts->vec_costintegrand);CHKERRQ(ierr);
      }
    }
    for (nadj=0; nadj<ts->numcost; nadj++) {
      ierr = MatMultTranspose(J,VecsDeltaLam[nadj],VecsSensiTemp[nadj]);CHKERRQ(ierr);
      ierr = VecAXPY(ts->vecs_sensi[nadj],ts->time_step,VecsSensiTemp[nadj]);CHKERRQ(ierr);
      if (ts->vec_costintegral) {
        ierr = VecAXPY(ts->vecs_sensi[nadj],-ts->time_step,ts->vecs_drdy[nadj]);CHKERRQ(ierr);
      }
    }
    if (ts->vecs_sensip) {
      ierr = TSAdjointComputeRHSJacobian(ts,th->stage_time,th->X,ts->Jacp);CHKERRQ(ierr);
      for (nadj=0; nadj<ts->numcost; nadj++) {
        ierr = MatMultTranspose(ts->Jacp,VecsDeltaLam[nadj],VecsDeltaMu[nadj]);CHKERRQ(ierr);
        ierr = VecAXPY(ts->vecs_sensip[nadj],-ts->time_step,VecsDeltaMu[nadj]);CHKERRQ(ierr);
      }
      if (ts->vec_costintegral) {
        ierr = TSAdjointComputeDRDPFunction(ts,th->stage_time,th->X,ts->vecs_drdp);CHKERRQ(ierr);
        for (nadj=0; nadj<ts->numcost; nadj++) {
          ierr = VecAXPY(ts->vecs_sensip[nadj],-ts->time_step,ts->vecs_drdp[nadj]);CHKERRQ(ierr);
        }
      }
    }
  }

  ts->ptime += ts->time_step;
  ts->steps++;
  th->status = TS_STEP_COMPLETE;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TSInterpolate_Theta"
static PetscErrorCode TSInterpolate_Theta(TS ts,PetscReal t,Vec X)
{
  TS_Theta       *th   = (TS_Theta*)ts->data;
  PetscReal      alpha = t - ts->ptime;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = VecCopy(ts->vec_sol,th->X);CHKERRQ(ierr);
  if (th->endpoint) alpha *= th->Theta;
  ierr = VecWAXPY(X,alpha,th->Xdot,th->X);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/*------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "TSReset_Theta"
static PetscErrorCode TSReset_Theta(TS ts)
{
  TS_Theta       *th = (TS_Theta*)ts->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = VecDestroy(&th->X);CHKERRQ(ierr);
  ierr = VecDestroy(&th->Xdot);CHKERRQ(ierr);
  ierr = VecDestroy(&th->X0);CHKERRQ(ierr);
  ierr = VecDestroy(&th->affine);CHKERRQ(ierr);
  ierr = VecDestroyVecs(ts->numcost,&th->VecsDeltaLam);CHKERRQ(ierr);
  ierr = VecDestroyVecs(ts->numcost,&th->VecsDeltaMu);CHKERRQ(ierr);
  ierr = VecDestroyVecs(ts->numcost,&th->VecsSensiTemp);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TSDestroy_Theta"
static PetscErrorCode TSDestroy_Theta(TS ts)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = TSReset_Theta(ts);CHKERRQ(ierr);
  ierr = PetscFree(ts->data);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)ts,"TSThetaGetTheta_C",NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)ts,"TSThetaSetTheta_C",NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)ts,"TSThetaGetEndpoint_C",NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)ts,"TSThetaSetEndpoint_C",NULL);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/*
  This defines the nonlinear equation that is to be solved with SNES
  G(U) = F[t0+Theta*dt, U, (U-U0)*shift] = 0
*/
#undef __FUNCT__
#define __FUNCT__ "SNESTSFormFunction_Theta"
static PetscErrorCode SNESTSFormFunction_Theta(SNES snes,Vec x,Vec y,TS ts)
{
  TS_Theta       *th = (TS_Theta*)ts->data;
  PetscErrorCode ierr;
  Vec            X0,Xdot;
  DM             dm,dmsave;
  PetscReal      shift = 1./(th->Theta*ts->time_step);

  PetscFunctionBegin;
  ierr = SNESGetDM(snes,&dm);CHKERRQ(ierr);
  /* When using the endpoint variant, this is actually 1/Theta * Xdot */
  ierr = TSThetaGetX0AndXdot(ts,dm,&X0,&Xdot);CHKERRQ(ierr);
  ierr = VecAXPBYPCZ(Xdot,-shift,shift,0,X0,x);CHKERRQ(ierr);

  /* DM monkey-business allows user code to call TSGetDM() inside of functions evaluated on levels of FAS */
  dmsave = ts->dm;
  ts->dm = dm;
  ierr   = TSComputeIFunction(ts,th->stage_time,x,Xdot,y,PETSC_FALSE);CHKERRQ(ierr);
  ts->dm = dmsave;
  ierr   = TSThetaRestoreX0AndXdot(ts,dm,&X0,&Xdot);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SNESTSFormJacobian_Theta"
static PetscErrorCode SNESTSFormJacobian_Theta(SNES snes,Vec x,Mat A,Mat B,TS ts)
{
  TS_Theta       *th = (TS_Theta*)ts->data;
  PetscErrorCode ierr;
  Vec            Xdot;
  DM             dm,dmsave;
  PetscReal      shift = 1./(th->Theta*ts->time_step);

  PetscFunctionBegin;
  ierr = SNESGetDM(snes,&dm);CHKERRQ(ierr);

  /* th->Xdot has already been computed in SNESTSFormFunction_Theta (SNES guarantees this) */
  ierr = TSThetaGetX0AndXdot(ts,dm,NULL,&Xdot);CHKERRQ(ierr);

  dmsave = ts->dm;
  ts->dm = dm;
  ierr   = TSComputeIJacobian(ts,th->stage_time,x,Xdot,shift,A,B,PETSC_FALSE);CHKERRQ(ierr);
  ts->dm = dmsave;
  ierr   = TSThetaRestoreX0AndXdot(ts,dm,NULL,&Xdot);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TSSetUp_Theta"
static PetscErrorCode TSSetUp_Theta(TS ts)
{
  TS_Theta       *th = (TS_Theta*)ts->data;
  PetscErrorCode ierr;
  SNES           snes;
  TSAdapt        adapt;
  DM             dm;

  PetscFunctionBegin;
  if (!th->X) {
    ierr = VecDuplicate(ts->vec_sol,&th->X);CHKERRQ(ierr);
  }
  if (!th->Xdot) {
    ierr = VecDuplicate(ts->vec_sol,&th->Xdot);CHKERRQ(ierr);
  }
  if (!th->X0) {
    ierr = VecDuplicate(ts->vec_sol,&th->X0);CHKERRQ(ierr);
  }
  ierr = TSGetSNES(ts,&snes);CHKERRQ(ierr);
  ierr = TSGetDM(ts,&dm);CHKERRQ(ierr);
  if (dm) {
    ierr = DMCoarsenHookAdd(dm,DMCoarsenHook_TSTheta,DMRestrictHook_TSTheta,ts);CHKERRQ(ierr);
    ierr = DMSubDomainHookAdd(dm,DMSubDomainHook_TSTheta,DMSubDomainRestrictHook_TSTheta,ts);CHKERRQ(ierr);
  }
  if (th->Theta == 0.5 && th->endpoint) th->order = 2;
  else th->order = 1;

  ierr = TSGetAdapt(ts,&adapt);CHKERRQ(ierr);
  if (!th->adapt) {
    ierr = TSAdaptSetType(adapt,TSADAPTNONE);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TSSetUp_BEuler"
static PetscErrorCode TSSetUp_BEuler(TS ts)
{
  TS_Theta       *th = (TS_Theta*)ts->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (th->Theta != 1.0) SETERRQ(PetscObjectComm((PetscObject)ts),PETSC_ERR_OPT_OVERWRITE,"Can not change the default value (1) of theta when using backward Euler\n");
  ierr = TSSetUp_Theta(ts);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TSSetUp_CN"
static PetscErrorCode TSSetUp_CN(TS ts)
{
  TS_Theta       *th = (TS_Theta*)ts->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (th->Theta != 0.5) SETERRQ(PetscObjectComm((PetscObject)ts),PETSC_ERR_OPT_OVERWRITE,"Can not change the default value (0.5) of theta when using Crank-Nicolson\n");
  if (!th->endpoint) SETERRQ(PetscObjectComm((PetscObject)ts),PETSC_ERR_OPT_OVERWRITE,"Can not change to the midpoint form of the Theta methods when using Crank-Nicolson\n");
  ierr = TSSetUp_Theta(ts);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
/*------------------------------------------------------------*/

#undef __FUNCT__
#define __FUNCT__ "TSAdjointSetUp_Theta"
static PetscErrorCode TSAdjointSetUp_Theta(TS ts)
{
  TS_Theta       *th = (TS_Theta*)ts->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = VecDuplicateVecs(ts->vecs_sensi[0],ts->numcost,&th->VecsDeltaLam);CHKERRQ(ierr);
  if(ts->vecs_sensip) {
    ierr = VecDuplicateVecs(ts->vecs_sensip[0],ts->numcost,&th->VecsDeltaMu);CHKERRQ(ierr);
  }
  ierr = VecDuplicateVecs(ts->vecs_sensi[0],ts->numcost,&th->VecsSensiTemp);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
/*------------------------------------------------------------*/

#undef __FUNCT__
#define __FUNCT__ "TSSetFromOptions_Theta"
static PetscErrorCode TSSetFromOptions_Theta(PetscOptionItems *PetscOptionsObject,TS ts)
{
  TS_Theta       *th = (TS_Theta*)ts->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscOptionsHead(PetscOptionsObject,"Theta ODE solver options");CHKERRQ(ierr);
  {
    ierr = PetscOptionsReal("-ts_theta_theta","Location of stage (0<Theta<=1)","TSThetaSetTheta",th->Theta,&th->Theta,NULL);CHKERRQ(ierr);
    ierr = PetscOptionsBool("-ts_theta_extrapolate","Extrapolate stage solution from previous solution (sometimes unstable)","TSThetaSetExtrapolate",th->extrapolate,&th->extrapolate,NULL);CHKERRQ(ierr);
    ierr = PetscOptionsBool("-ts_theta_endpoint","Use the endpoint instead of midpoint form of the Theta method","TSThetaSetEndpoint",th->endpoint,&th->endpoint,NULL);CHKERRQ(ierr);
    ierr = PetscOptionsBool("-ts_theta_adapt","Use time-step adaptivity with the Theta method","",th->adapt,&th->adapt,NULL);CHKERRQ(ierr);
  }
  ierr = PetscOptionsTail();CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TSView_Theta"
static PetscErrorCode TSView_Theta(TS ts,PetscViewer viewer)
{
  TS_Theta       *th = (TS_Theta*)ts->data;
  PetscBool      iascii;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERASCII,&iascii);CHKERRQ(ierr);
  if (iascii) {
    ierr = PetscViewerASCIIPrintf(viewer,"  Theta=%g\n",(double)th->Theta);CHKERRQ(ierr);
    ierr = PetscViewerASCIIPrintf(viewer,"  Extrapolation=%s\n",th->extrapolate ? "yes" : "no");CHKERRQ(ierr);
  }
  if (ts->snes) {ierr = SNESView(ts->snes,viewer);CHKERRQ(ierr);}
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TSThetaGetTheta_Theta"
PetscErrorCode  TSThetaGetTheta_Theta(TS ts,PetscReal *theta)
{
  TS_Theta *th = (TS_Theta*)ts->data;

  PetscFunctionBegin;
  *theta = th->Theta;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TSThetaSetTheta_Theta"
PetscErrorCode  TSThetaSetTheta_Theta(TS ts,PetscReal theta)
{
  TS_Theta *th = (TS_Theta*)ts->data;

  PetscFunctionBegin;
  if (theta <= 0 || 1 < theta) SETERRQ1(PetscObjectComm((PetscObject)ts),PETSC_ERR_ARG_OUTOFRANGE,"Theta %g not in range (0,1]",(double)theta);
  th->Theta = theta;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TSThetaGetEndpoint_Theta"
PetscErrorCode  TSThetaGetEndpoint_Theta(TS ts,PetscBool *endpoint)
{
  TS_Theta *th = (TS_Theta*)ts->data;

  PetscFunctionBegin;
  *endpoint = th->endpoint;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TSThetaSetEndpoint_Theta"
PetscErrorCode  TSThetaSetEndpoint_Theta(TS ts,PetscBool flg)
{
  TS_Theta *th = (TS_Theta*)ts->data;

  PetscFunctionBegin;
  th->endpoint = flg;
  PetscFunctionReturn(0);
}

#if defined(PETSC_HAVE_COMPLEX)
#undef __FUNCT__
#define __FUNCT__ "TSComputeLinearStability_Theta"
static PetscErrorCode TSComputeLinearStability_Theta(TS ts,PetscReal xr,PetscReal xi,PetscReal *yr,PetscReal *yi)
{
  PetscComplex z   = xr + xi*PETSC_i,f;
  TS_Theta     *th = (TS_Theta*)ts->data;
  const PetscReal one = 1.0;

  PetscFunctionBegin;
  f   = (one + (one - th->Theta)*z)/(one - th->Theta*z);
  *yr = PetscRealPartComplex(f);
  *yi = PetscImaginaryPartComplex(f);
  PetscFunctionReturn(0);
}
#endif

#undef __FUNCT__
#define __FUNCT__ "TSGetStages_Theta"
static PetscErrorCode  TSGetStages_Theta(TS ts,PetscInt *ns,Vec **Y)
{
  TS_Theta     *th = (TS_Theta*)ts->data;

  PetscFunctionBegin;
  *ns = 1;
  if(Y) {
    *Y  = (th->endpoint)?&(th->X0):&(th->X);
  }
  PetscFunctionReturn(0);
}

/* ------------------------------------------------------------ */
/*MC
      TSTHETA - DAE solver using the implicit Theta method

   Level: beginner

   Options Database:
+      -ts_theta_theta <Theta> - Location of stage (0<Theta<=1)
.      -ts_theta_extrapolate <flg> - Extrapolate stage solution from previous solution (sometimes unstable)
.      -ts_theta_endpoint <flag> - Use the endpoint (like Crank-Nicholson) instead of midpoint form of the Theta method
-     -ts_theta_adapt <flg> - Use time-step adaptivity with the Theta method

   Notes:
$  -ts_type theta -ts_theta_theta 1.0 corresponds to backward Euler (TSBEULER)
$  -ts_type theta -ts_theta_theta 0.5 corresponds to the implicit midpoint rule
$  -ts_type theta -ts_theta_theta 0.5 -ts_theta_endpoint corresponds to Crank-Nicholson (TSCN)

   This method can be applied to DAE.

   This method is cast as a 1-stage implicit Runge-Kutta method.

.vb
  Theta | Theta
  -------------
        |  1
.ve

   For the default Theta=0.5, this is also known as the implicit midpoint rule.

   When the endpoint variant is chosen, the method becomes a 2-stage method with first stage explicit:

.vb
  0 | 0         0
  1 | 1-Theta   Theta
  -------------------
    | 1-Theta   Theta
.ve

   For the default Theta=0.5, this is the trapezoid rule (also known as Crank-Nicolson, see TSCN).

   To apply a diagonally implicit RK method to DAE, the stage formula

$  Y_i = X + h sum_j a_ij Y'_j

   is interpreted as a formula for Y'_i in terms of Y_i and known values (Y'_j, j<i)

.seealso:  TSCreate(), TS, TSSetType(), TSCN, TSBEULER, TSThetaSetTheta(), TSThetaSetEndpoint()

M*/
#undef __FUNCT__
#define __FUNCT__ "TSCreate_Theta"
PETSC_EXTERN PetscErrorCode TSCreate_Theta(TS ts)
{
  TS_Theta       *th;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ts->ops->reset           = TSReset_Theta;
  ts->ops->destroy         = TSDestroy_Theta;
  ts->ops->view            = TSView_Theta;
  ts->ops->setup           = TSSetUp_Theta;
  ts->ops->adjointsetup    = TSAdjointSetUp_Theta;
  ts->ops->step            = TSStep_Theta;
  ts->ops->interpolate     = TSInterpolate_Theta;
  ts->ops->evaluatestep    = TSEvaluateStep_Theta;
  ts->ops->rollback        = TSRollBack_Theta;
  ts->ops->setfromoptions  = TSSetFromOptions_Theta;
  ts->ops->snesfunction    = SNESTSFormFunction_Theta;
  ts->ops->snesjacobian    = SNESTSFormJacobian_Theta;
#if defined(PETSC_HAVE_COMPLEX)
  ts->ops->linearstability = TSComputeLinearStability_Theta;
#endif
  ts->ops->getstages       = TSGetStages_Theta;
  ts->ops->adjointstep     = TSAdjointStep_Theta;

  ierr = PetscNewLog(ts,&th);CHKERRQ(ierr);
  ts->data = (void*)th;

  th->extrapolate = PETSC_FALSE;
  th->Theta       = 0.5;
  th->ccfl        = 1.0;
  th->adapt       = PETSC_FALSE;
  ierr = PetscObjectComposeFunction((PetscObject)ts,"TSThetaGetTheta_C",TSThetaGetTheta_Theta);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)ts,"TSThetaSetTheta_C",TSThetaSetTheta_Theta);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)ts,"TSThetaGetEndpoint_C",TSThetaGetEndpoint_Theta);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)ts,"TSThetaSetEndpoint_C",TSThetaSetEndpoint_Theta);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TSThetaGetTheta"
/*@
  TSThetaGetTheta - Get the abscissa of the stage in (0,1].

  Not Collective

  Input Parameter:
.  ts - timestepping context

  Output Parameter:
.  theta - stage abscissa

  Note:
  Use of this function is normally only required to hack TSTHETA to use a modified integration scheme.

  Level: Advanced

.seealso: TSThetaSetTheta()
@*/
PetscErrorCode  TSThetaGetTheta(TS ts,PetscReal *theta)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts,TS_CLASSID,1);
  PetscValidPointer(theta,2);
  ierr = PetscUseMethod(ts,"TSThetaGetTheta_C",(TS,PetscReal*),(ts,theta));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TSThetaSetTheta"
/*@
  TSThetaSetTheta - Set the abscissa of the stage in (0,1].

  Not Collective

  Input Parameter:
+  ts - timestepping context
-  theta - stage abscissa

  Options Database:
.  -ts_theta_theta <theta>

  Level: Intermediate

.seealso: TSThetaGetTheta()
@*/
PetscErrorCode  TSThetaSetTheta(TS ts,PetscReal theta)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts,TS_CLASSID,1);
  ierr = PetscTryMethod(ts,"TSThetaSetTheta_C",(TS,PetscReal),(ts,theta));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TSThetaGetEndpoint"
/*@
  TSThetaGetEndpoint - Gets whether to use the endpoint variant of the method (e.g. trapezoid/Crank-Nicolson instead of midpoint rule).

  Not Collective

  Input Parameter:
.  ts - timestepping context

  Output Parameter:
.  endpoint - PETSC_TRUE when using the endpoint variant

  Level: Advanced

.seealso: TSThetaSetEndpoint(), TSTHETA, TSCN
@*/
PetscErrorCode TSThetaGetEndpoint(TS ts,PetscBool *endpoint)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts,TS_CLASSID,1);
  PetscValidPointer(endpoint,2);
  ierr = PetscTryMethod(ts,"TSThetaGetEndpoint_C",(TS,PetscBool*),(ts,endpoint));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TSThetaSetEndpoint"
/*@
  TSThetaSetEndpoint - Sets whether to use the endpoint variant of the method (e.g. trapezoid/Crank-Nicolson instead of midpoint rule).

  Not Collective

  Input Parameter:
+  ts - timestepping context
-  flg - PETSC_TRUE to use the endpoint variant

  Options Database:
.  -ts_theta_endpoint <flg>

  Level: Intermediate

.seealso: TSTHETA, TSCN
@*/
PetscErrorCode TSThetaSetEndpoint(TS ts,PetscBool flg)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ts,TS_CLASSID,1);
  ierr = PetscTryMethod(ts,"TSThetaSetEndpoint_C",(TS,PetscBool),(ts,flg));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/*
 * TSBEULER and TSCN are straightforward specializations of TSTHETA.
 * The creation functions for these specializations are below.
 */

#undef __FUNCT__
#define __FUNCT__ "TSView_BEuler"
static PetscErrorCode TSView_BEuler(TS ts,PetscViewer viewer)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = SNESView(ts->snes,viewer);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/*MC
      TSBEULER - ODE solver using the implicit backward Euler method

  Level: beginner

  Notes:
  TSBEULER is equivalent to TSTHETA with Theta=1.0

$  -ts_type theta -ts_theta_theta 1.

.seealso:  TSCreate(), TS, TSSetType(), TSEULER, TSCN, TSTHETA

M*/
#undef __FUNCT__
#define __FUNCT__ "TSCreate_BEuler"
PETSC_EXTERN PetscErrorCode TSCreate_BEuler(TS ts)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = TSCreate_Theta(ts);CHKERRQ(ierr);
  ierr = TSThetaSetTheta(ts,1.0);CHKERRQ(ierr);
  ts->ops->setup = TSSetUp_BEuler;
  ts->ops->view = TSView_BEuler;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TSView_CN"
static PetscErrorCode TSView_CN(TS ts,PetscViewer viewer)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = SNESView(ts->snes,viewer);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/*MC
      TSCN - ODE solver using the implicit Crank-Nicolson method.

  Level: beginner

  Notes:
  TSCN is equivalent to TSTHETA with Theta=0.5 and the "endpoint" option set. I.e.

$  -ts_type theta -ts_theta_theta 0.5 -ts_theta_endpoint

.seealso:  TSCreate(), TS, TSSetType(), TSBEULER, TSTHETA

M*/
#undef __FUNCT__
#define __FUNCT__ "TSCreate_CN"
PETSC_EXTERN PetscErrorCode TSCreate_CN(TS ts)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = TSCreate_Theta(ts);CHKERRQ(ierr);
  ierr = TSThetaSetTheta(ts,0.5);CHKERRQ(ierr);
  ierr = TSThetaSetEndpoint(ts,PETSC_TRUE);CHKERRQ(ierr);
  ts->ops->setup = TSSetUp_CN;
  ts->ops->view = TSView_CN;
  PetscFunctionReturn(0);
}
