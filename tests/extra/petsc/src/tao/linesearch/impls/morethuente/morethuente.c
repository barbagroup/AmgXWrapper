#include <petsc/private/taolinesearchimpl.h>
#include <../src/tao/linesearch/impls/morethuente/morethuente.h>

/*
   This algorithm is taken from More' and Thuente, "Line search algorithms
   with guaranteed sufficient decrease", Argonne National Laboratory,
   Technical Report MCS-P330-1092.
*/

static PetscErrorCode Tao_mcstep(TaoLineSearch ls,PetscReal *stx,PetscReal *fx,PetscReal *dx,PetscReal *sty,PetscReal *fy,PetscReal *dy,PetscReal *stp,PetscReal *fp,PetscReal *dp);

#undef __FUNCT__
#define __FUNCT__ "TaoLineSearchDestroy_MT"
static PetscErrorCode TaoLineSearchDestroy_MT(TaoLineSearch ls)
{
  PetscErrorCode   ierr;
  TaoLineSearch_MT *mt;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ls,TAOLINESEARCH_CLASSID,1);
  mt = (TaoLineSearch_MT*)(ls->data);
  if (mt->x) {
    ierr = PetscObjectDereference((PetscObject)mt->x);CHKERRQ(ierr);
  }
  ierr = VecDestroy(&mt->work);CHKERRQ(ierr);
  ierr = PetscFree(ls->data);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoLineSearchSetFromOptions_MT"
static PetscErrorCode TaoLineSearchSetFromOptions_MT(PetscOptionItems *PetscOptionsObject,TaoLineSearch ls)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(ls,TAOLINESEARCH_CLASSID,1);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoLineSearchView_MT"
static PetscErrorCode TaoLineSearchView_MT(TaoLineSearch ls, PetscViewer pv)
{
  PetscErrorCode ierr;
  PetscBool      isascii;

  PetscFunctionBegin;
  ierr = PetscObjectTypeCompare((PetscObject)pv, PETSCVIEWERASCII, &isascii);CHKERRQ(ierr);
  if (isascii) {
    ierr = PetscViewerASCIIPrintf(pv,"  maxf=%D, ftol=%g, gtol=%g\n",ls->max_funcs,(double)ls->rtol,(double)ls->ftol);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoLineSearchApply_MT"
/* @ TaoApply_LineSearch - This routine takes step length of 1.0.

   Input Parameters:
+  tao - Tao context
.  X - current iterate (on output X contains new iterate, X + step*S)
.  f - objective function evaluated at X
.  G - gradient evaluated at X
-  D - search direction


   Info is set to 0.

@ */

static PetscErrorCode TaoLineSearchApply_MT(TaoLineSearch ls, Vec x, PetscReal *f, Vec g, Vec s)
{
  PetscErrorCode   ierr;
  TaoLineSearch_MT *mt;

  PetscReal        xtrapf = 4.0;
  PetscReal        finit, width, width1, dginit, fm, fxm, fym, dgm, dgxm, dgym;
  PetscReal        dgx, dgy, dg, dg2, fx, fy, stx, sty, dgtest;
  PetscReal        ftest1=0.0, ftest2=0.0;
  PetscInt         i, stage1,n1,n2,nn1,nn2;
  PetscReal        bstepmin1, bstepmin2, bstepmax;
  PetscBool        g_computed=PETSC_FALSE; /* to prevent extra gradient computation */

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ls,TAOLINESEARCH_CLASSID,1);
  PetscValidHeaderSpecific(x,VEC_CLASSID,2);
  PetscValidScalarPointer(f,3);
  PetscValidHeaderSpecific(g,VEC_CLASSID,4);
  PetscValidHeaderSpecific(s,VEC_CLASSID,5);

  /* comm,type,size checks are done in interface TaoLineSearchApply */
  mt = (TaoLineSearch_MT*)(ls->data);
  ls->reason = TAOLINESEARCH_CONTINUE_ITERATING;

  /* Check work vector */
  if (!mt->work) {
    ierr = VecDuplicate(x,&mt->work);CHKERRQ(ierr);
    mt->x = x;
    ierr = PetscObjectReference((PetscObject)mt->x);CHKERRQ(ierr);
  } else if (x != mt->x) {
    ierr = VecDestroy(&mt->work);CHKERRQ(ierr);
    ierr = VecDuplicate(x,&mt->work);CHKERRQ(ierr);
    ierr = PetscObjectDereference((PetscObject)mt->x);CHKERRQ(ierr);
    mt->x = x;
    ierr = PetscObjectReference((PetscObject)mt->x);CHKERRQ(ierr);
  }

  if (ls->bounded) {
    /* Compute step length needed to make all variables equal a bound */
    /* Compute the smallest steplength that will make one nonbinding variable
     equal the bound */
    ierr = VecGetLocalSize(ls->upper,&n1);CHKERRQ(ierr);
    ierr = VecGetLocalSize(mt->x, &n2);CHKERRQ(ierr);
    ierr = VecGetSize(ls->upper,&nn1);CHKERRQ(ierr);
    ierr = VecGetSize(mt->x,&nn2);CHKERRQ(ierr);
    if (n1 != n2 || nn1 != nn2) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_SIZ,"Variable vector not compatible with bounds vector");
    ierr = VecScale(s,-1.0);CHKERRQ(ierr);
    ierr = VecBoundGradientProjection(s,x,ls->lower,ls->upper,s);CHKERRQ(ierr);
    ierr = VecScale(s,-1.0);CHKERRQ(ierr);
    ierr = VecStepBoundInfo(x,s,ls->lower,ls->upper,&bstepmin1,&bstepmin2,&bstepmax);CHKERRQ(ierr);
    ls->stepmax = PetscMin(bstepmax,1.0e15);
  }

  ierr = VecDot(g,s,&dginit);CHKERRQ(ierr);
  if (PetscIsInfOrNanReal(dginit)) {
    ierr = PetscInfo1(ls,"Initial Line Search step * g is Inf or Nan (%g)\n",(double)dginit);CHKERRQ(ierr);
    ls->reason=TAOLINESEARCH_FAILED_INFORNAN;
    PetscFunctionReturn(0);
  }
  if (dginit >= 0.0) {
    ierr = PetscInfo1(ls,"Initial Line Search step * g is not descent direction (%g)\n",(double)dginit);CHKERRQ(ierr);
    ls->reason = TAOLINESEARCH_FAILED_ASCENT;
    PetscFunctionReturn(0);
  }


  /* Initialization */
  mt->bracket = 0;
  stage1 = 1;
  finit = *f;
  dgtest = ls->ftol * dginit;
  width = ls->stepmax - ls->stepmin;
  width1 = width * 2.0;
  ierr = VecCopy(x,mt->work);CHKERRQ(ierr);
  /* Variable dictionary:
   stx, fx, dgx - the step, function, and derivative at the best step
   sty, fy, dgy - the step, function, and derivative at the other endpoint
   of the interval of uncertainty
   step, f, dg - the step, function, and derivative at the current step */

  stx = 0.0;
  fx  = finit;
  dgx = dginit;
  sty = 0.0;
  fy  = finit;
  dgy = dginit;

  ls->step=ls->initstep;
  for (i=0; i< ls->max_funcs; i++) {
    /* Set min and max steps to correspond to the interval of uncertainty */
    if (mt->bracket) {
      ls->stepmin = PetscMin(stx,sty);
      ls->stepmax = PetscMax(stx,sty);
    } else {
      ls->stepmin = stx;
      ls->stepmax = ls->step + xtrapf * (ls->step - stx);
    }

    /* Force the step to be within the bounds */
    ls->step = PetscMax(ls->step,ls->stepmin);
    ls->step = PetscMin(ls->step,ls->stepmax);

    /* If an unusual termination is to occur, then let step be the lowest
     point obtained thus far */
    if ((stx!=0) && (((mt->bracket) && (ls->step <= ls->stepmin || ls->step >= ls->stepmax)) || ((mt->bracket) && (ls->stepmax - ls->stepmin <= ls->rtol * ls->stepmax)) ||
                     ((ls->nfeval+ls->nfgeval) >= ls->max_funcs - 1) || (mt->infoc == 0))) {
      ls->step = stx;
    }

    ierr = VecCopy(x,mt->work);CHKERRQ(ierr);
    ierr = VecAXPY(mt->work,ls->step,s);CHKERRQ(ierr);   /* W = X + step*S */

    if (ls->bounded) {
      ierr = VecMedian(ls->lower, mt->work, ls->upper, mt->work);CHKERRQ(ierr);
    }
    if (ls->usegts) {
      ierr = TaoLineSearchComputeObjectiveAndGTS(ls,mt->work,f,&dg);CHKERRQ(ierr);
      g_computed=PETSC_FALSE;
    } else {
      ierr = TaoLineSearchComputeObjectiveAndGradient(ls,mt->work,f,g);CHKERRQ(ierr);
      g_computed=PETSC_TRUE;
      if (ls->bounded) {
        ierr = VecDot(g,x,&dg);CHKERRQ(ierr);
        ierr = VecDot(g,mt->work,&dg2);CHKERRQ(ierr);
        dg = (dg2 - dg)/ls->step;
      } else {
        ierr = VecDot(g,s,&dg);CHKERRQ(ierr);
      }
    }

    if (0 == i) {
      ls->f_fullstep=*f;
    }

    if (PetscIsInfOrNanReal(*f) || PetscIsInfOrNanReal(dg)) {
      /* User provided compute function generated Not-a-Number, assume
       domain violation and set function value and directional
       derivative to infinity. */
      *f = PETSC_INFINITY;
      dg = PETSC_INFINITY;
    }

    ftest1 = finit + ls->step * dgtest;
    if (ls->bounded) {
      ftest2 = finit + ls->step * dgtest * ls->ftol;
    }
    /* Convergence testing */
    if (((*f - ftest1 <= 1.0e-10 * PetscAbsReal(finit)) &&  (PetscAbsReal(dg) + ls->gtol*dginit <= 0.0))) {
      ierr = PetscInfo(ls, "Line search success: Sufficient decrease and directional deriv conditions hold\n");CHKERRQ(ierr);
      ls->reason = TAOLINESEARCH_SUCCESS;
      break;
    }

    /* Check Armijo if beyond the first breakpoint */
    if (ls->bounded && (*f <= ftest2) && (ls->step >= bstepmin2)) {
      ierr = PetscInfo(ls,"Line search success: Sufficient decrease.\n");CHKERRQ(ierr);
      ls->reason = TAOLINESEARCH_SUCCESS;
      break;
    }

    /* Checks for bad cases */
    if (((mt->bracket) && (ls->step <= ls->stepmin||ls->step >= ls->stepmax)) || (!mt->infoc)) {
      ierr = PetscInfo(ls,"Rounding errors may prevent further progress.  May not be a step satisfying\n");CHKERRQ(ierr);
      ierr = PetscInfo(ls,"sufficient decrease and curvature conditions. Tolerances may be too small.\n");CHKERRQ(ierr);
      ls->reason = TAOLINESEARCH_HALTED_OTHER;
      break;
    }
    if ((ls->step == ls->stepmax) && (*f <= ftest1) && (dg <= dgtest)) {
      ierr = PetscInfo1(ls,"Step is at the upper bound, stepmax (%g)\n",(double)ls->stepmax);CHKERRQ(ierr);
      ls->reason = TAOLINESEARCH_HALTED_UPPERBOUND;
      break;
    }
    if ((ls->step == ls->stepmin) && (*f >= ftest1) && (dg >= dgtest)) {
      ierr = PetscInfo1(ls,"Step is at the lower bound, stepmin (%g)\n",(double)ls->stepmin);CHKERRQ(ierr);
      ls->reason = TAOLINESEARCH_HALTED_LOWERBOUND;
      break;
    }
    if ((mt->bracket) && (ls->stepmax - ls->stepmin <= ls->rtol*ls->stepmax)){
      ierr = PetscInfo1(ls,"Relative width of interval of uncertainty is at most rtol (%g)\n",(double)ls->rtol);CHKERRQ(ierr);
      ls->reason = TAOLINESEARCH_HALTED_RTOL;
      break;
    }

    /* In the first stage, we seek a step for which the modified function
     has a nonpositive value and nonnegative derivative */
    if ((stage1) && (*f <= ftest1) && (dg >= dginit * PetscMin(ls->ftol, ls->gtol))) {
      stage1 = 0;
    }

    /* A modified function is used to predict the step only if we
     have not obtained a step for which the modified function has a
     nonpositive function value and nonnegative derivative, and if a
     lower function value has been obtained but the decrease is not
     sufficient */

    if ((stage1) && (*f <= fx) && (*f > ftest1)) {
      fm   = *f - ls->step * dgtest;    /* Define modified function */
      fxm  = fx - stx * dgtest;         /* and derivatives */
      fym  = fy - sty * dgtest;
      dgm  = dg - dgtest;
      dgxm = dgx - dgtest;
      dgym = dgy - dgtest;

      /* if (dgxm * (ls->step - stx) >= 0.0) */
      /* Update the interval of uncertainty and compute the new step */
      ierr = Tao_mcstep(ls,&stx,&fxm,&dgxm,&sty,&fym,&dgym,&ls->step,&fm,&dgm);CHKERRQ(ierr);

      fx  = fxm + stx * dgtest; /* Reset the function and */
      fy  = fym + sty * dgtest; /* gradient values */
      dgx = dgxm + dgtest;
      dgy = dgym + dgtest;
    } else {
      /* Update the interval of uncertainty and compute the new step */
      ierr = Tao_mcstep(ls,&stx,&fx,&dgx,&sty,&fy,&dgy,&ls->step,f,&dg);CHKERRQ(ierr);
    }

    /* Force a sufficient decrease in the interval of uncertainty */
    if (mt->bracket) {
      if (PetscAbsReal(sty - stx) >= 0.66 * width1) ls->step = stx + 0.5*(sty - stx);
      width1 = width;
      width = PetscAbsReal(sty - stx);
    }
  }
  if ((ls->nfeval+ls->nfgeval) > ls->max_funcs) {
    ierr = PetscInfo2(ls,"Number of line search function evals (%D) > maximum (%D)\n",(ls->nfeval+ls->nfgeval),ls->max_funcs);CHKERRQ(ierr);
    ls->reason = TAOLINESEARCH_HALTED_MAXFCN;
  }

  /* Finish computations */
  ierr = PetscInfo2(ls,"%D function evals in line search, step = %g\n",(ls->nfeval+ls->nfgeval),(double)ls->step);CHKERRQ(ierr);

  /* Set new solution vector and compute gradient if needed */
  ierr = VecCopy(mt->work,x);CHKERRQ(ierr);
  if (!g_computed) {
    ierr = TaoLineSearchComputeGradient(ls,mt->work,g);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoLineSearchCreate_MT"
PETSC_EXTERN PetscErrorCode TaoLineSearchCreate_MT(TaoLineSearch ls)
{
  PetscErrorCode   ierr;
  TaoLineSearch_MT *ctx;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ls,TAOLINESEARCH_CLASSID,1);
  ierr = PetscNewLog(ls,&ctx);CHKERRQ(ierr);
  ctx->bracket=0;
  ctx->infoc=1;
  ls->data = (void*)ctx;
  ls->initstep = 1.0;
  ls->ops->setup=0;
  ls->ops->reset=0;
  ls->ops->apply=TaoLineSearchApply_MT;
  ls->ops->view =TaoLineSearchView_MT;
  ls->ops->destroy=TaoLineSearchDestroy_MT;
  ls->ops->setfromoptions=TaoLineSearchSetFromOptions_MT;
  PetscFunctionReturn(0);
}

/*
     The subroutine mcstep is taken from the work of Jorge Nocedal.
     this is a variant of More' and Thuente's routine.

     subroutine mcstep

     the purpose of mcstep is to compute a safeguarded step for
     a linesearch and to update an interval of uncertainty for
     a minimizer of the function.

     the parameter stx contains the step with the least function
     value. the parameter stp contains the current step. it is
     assumed that the derivative at stx is negative in the
     direction of the step. if bracket is set true then a
     minimizer has been bracketed in an interval of uncertainty
     with endpoints stx and sty.

     the subroutine statement is

     subroutine mcstep(stx,fx,dx,sty,fy,dy,stp,fp,dp,bracket,
                       stpmin,stpmax,info)

     where

       stx, fx, and dx are variables which specify the step,
         the function, and the derivative at the best step obtained
         so far. The derivative must be negative in the direction
         of the step, that is, dx and stp-stx must have opposite
         signs. On output these parameters are updated appropriately.

       sty, fy, and dy are variables which specify the step,
         the function, and the derivative at the other endpoint of
         the interval of uncertainty. On output these parameters are
         updated appropriately.

       stp, fp, and dp are variables which specify the step,
         the function, and the derivative at the current step.
         If bracket is set true then on input stp must be
         between stx and sty. On output stp is set to the new step.

       bracket is a logical variable which specifies if a minimizer
         has been bracketed.  If the minimizer has not been bracketed
         then on input bracket must be set false.  If the minimizer
         is bracketed then on output bracket is set true.

       stpmin and stpmax are input variables which specify lower
         and upper bounds for the step.

       info is an integer output variable set as follows:
         if info = 1,2,3,4,5, then the step has been computed
         according to one of the five cases below. otherwise
         info = 0, and this indicates improper input parameters.

     subprograms called

       fortran-supplied ... abs,max,min,sqrt

     argonne national laboratory. minpack project. june 1983
     jorge j. more', david j. thuente

*/

#undef __FUNCT__
#define __FUNCT__ "Tao_mcstep"
static PetscErrorCode Tao_mcstep(TaoLineSearch ls,PetscReal *stx,PetscReal *fx,PetscReal *dx,PetscReal *sty,PetscReal *fy,PetscReal *dy,PetscReal *stp,PetscReal *fp,PetscReal *dp)
{
  TaoLineSearch_MT *mtP = (TaoLineSearch_MT *) ls->data;
  PetscReal        gamma1, p, q, r, s, sgnd, stpc, stpf, stpq, theta;
  PetscInt         bound;

  PetscFunctionBegin;
  /* Check the input parameters for errors */
  mtP->infoc = 0;
  if (mtP->bracket && (*stp <= PetscMin(*stx,*sty) || (*stp >= PetscMax(*stx,*sty)))) SETERRQ(PETSC_COMM_SELF,1,"bad stp in bracket");
  if (*dx * (*stp-*stx) >= 0.0) SETERRQ(PETSC_COMM_SELF,1,"dx * (stp-stx) >= 0.0");
  if (ls->stepmax < ls->stepmin) SETERRQ(PETSC_COMM_SELF,1,"stepmax > stepmin");

  /* Determine if the derivatives have opposite sign */
  sgnd = *dp * (*dx / PetscAbsReal(*dx));

  if (*fp > *fx) {
    /* Case 1: a higher function value.
     The minimum is bracketed. If the cubic step is closer
     to stx than the quadratic step, the cubic step is taken,
     else the average of the cubic and quadratic steps is taken. */

    mtP->infoc = 1;
    bound = 1;
    theta = 3 * (*fx - *fp) / (*stp - *stx) + *dx + *dp;
    s = PetscMax(PetscAbsReal(theta),PetscAbsReal(*dx));
    s = PetscMax(s,PetscAbsReal(*dp));
    gamma1 = s*PetscSqrtScalar(PetscPowScalar(theta/s,2.0) - (*dx/s)*(*dp/s));
    if (*stp < *stx) gamma1 = -gamma1;
    /* Can p be 0?  Check */
    p = (gamma1 - *dx) + theta;
    q = ((gamma1 - *dx) + gamma1) + *dp;
    r = p/q;
    stpc = *stx + r*(*stp - *stx);
    stpq = *stx + ((*dx/((*fx-*fp)/(*stp-*stx)+*dx))*0.5) * (*stp - *stx);

    if (PetscAbsReal(stpc-*stx) < PetscAbsReal(stpq-*stx)) {
      stpf = stpc;
    } else {
      stpf = stpc + 0.5*(stpq - stpc);
    }
    mtP->bracket = 1;
  } else if (sgnd < 0.0) {
    /* Case 2: A lower function value and derivatives of
     opposite sign. The minimum is bracketed. If the cubic
     step is closer to stx than the quadratic (secant) step,
     the cubic step is taken, else the quadratic step is taken. */

    mtP->infoc = 2;
    bound = 0;
    theta = 3*(*fx - *fp)/(*stp - *stx) + *dx + *dp;
    s = PetscMax(PetscAbsReal(theta),PetscAbsReal(*dx));
    s = PetscMax(s,PetscAbsReal(*dp));
    gamma1 = s*PetscSqrtScalar(PetscPowScalar(theta/s,2.0) - (*dx/s)*(*dp/s));
    if (*stp > *stx) gamma1 = -gamma1;
    p = (gamma1 - *dp) + theta;
    q = ((gamma1 - *dp) + gamma1) + *dx;
    r = p/q;
    stpc = *stp + r*(*stx - *stp);
    stpq = *stp + (*dp/(*dp-*dx))*(*stx - *stp);

    if (PetscAbsReal(stpc-*stp) > PetscAbsReal(stpq-*stp)) {
      stpf = stpc;
    } else {
      stpf = stpq;
    }
    mtP->bracket = 1;
  } else if (PetscAbsReal(*dp) < PetscAbsReal(*dx)) {
    /* Case 3: A lower function value, derivatives of the
     same sign, and the magnitude of the derivative decreases.
     The cubic step is only used if the cubic tends to infinity
     in the direction of the step or if the minimum of the cubic
     is beyond stp. Otherwise the cubic step is defined to be
     either stepmin or stepmax. The quadratic (secant) step is also
     computed and if the minimum is bracketed then the step
     closest to stx is taken, else the step farthest away is taken. */

    mtP->infoc = 3;
    bound = 1;
    theta = 3*(*fx - *fp)/(*stp - *stx) + *dx + *dp;
    s = PetscMax(PetscAbsReal(theta),PetscAbsReal(*dx));
    s = PetscMax(s,PetscAbsReal(*dp));

    /* The case gamma1 = 0 only arises if the cubic does not tend
       to infinity in the direction of the step. */
    gamma1 = s*PetscSqrtScalar(PetscMax(0.0,PetscPowScalar(theta/s,2.0) - (*dx/s)*(*dp/s)));
    if (*stp > *stx) gamma1 = -gamma1;
    p = (gamma1 - *dp) + theta;
    q = (gamma1 + (*dx - *dp)) + gamma1;
    r = p/q;
    if (r < 0.0 && gamma1 != 0.0) stpc = *stp + r*(*stx - *stp);
    else if (*stp > *stx)        stpc = ls->stepmax;
    else                         stpc = ls->stepmin;
    stpq = *stp + (*dp/(*dp-*dx)) * (*stx - *stp);

    if (mtP->bracket) {
      if (PetscAbsReal(*stp-stpc) < PetscAbsReal(*stp-stpq)) {
        stpf = stpc;
      } else {
        stpf = stpq;
      }
    } else {
      if (PetscAbsReal(*stp-stpc) > PetscAbsReal(*stp-stpq)) {
        stpf = stpc;
      } else {
        stpf = stpq;
      }
    }
  } else {
    /* Case 4: A lower function value, derivatives of the
       same sign, and the magnitude of the derivative does
       not decrease. If the minimum is not bracketed, the step
       is either stpmin or stpmax, else the cubic step is taken. */

    mtP->infoc = 4;
    bound = 0;
    if (mtP->bracket) {
      theta = 3*(*fp - *fy)/(*sty - *stp) + *dy + *dp;
      s = PetscMax(PetscAbsReal(theta),PetscAbsReal(*dy));
      s = PetscMax(s,PetscAbsReal(*dp));
      gamma1 = s*PetscSqrtScalar(PetscPowScalar(theta/s,2.0) - (*dy/s)*(*dp/s));
      if (*stp > *sty) gamma1 = -gamma1;
      p = (gamma1 - *dp) + theta;
      q = ((gamma1 - *dp) + gamma1) + *dy;
      r = p/q;
      stpc = *stp + r*(*sty - *stp);
      stpq = *stp + (*dp/(*dp-*dx)) * (*stx - *stp);

      stpf = stpc;
    } else if (*stp > *stx) {
      stpf = ls->stepmax;
    } else {
      stpf = ls->stepmin;
    }
  }

  /* Update the interval of uncertainty.  This update does not
     depend on the new step or the case analysis above. */

  if (*fp > *fx) {
    *sty = *stp;
    *fy = *fp;
    *dy = *dp;
  } else {
    if (sgnd < 0.0) {
      *sty = *stx;
      *fy = *fx;
      *dy = *dx;
    }
    *stx = *stp;
    *fx = *fp;
    *dx = *dp;
  }

  /* Compute the new step and safeguard it. */
  stpf = PetscMin(ls->stepmax,stpf);
  stpf = PetscMax(ls->stepmin,stpf);
  *stp = stpf;
  if (mtP->bracket && bound) {
    if (*sty > *stx) {
      *stp = PetscMin(*stx+0.66*(*sty-*stx),*stp);
    } else {
      *stp = PetscMax(*stx+0.66*(*sty-*stx),*stp);
    }
  }
  PetscFunctionReturn(0);
}
