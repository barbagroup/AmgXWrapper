#include <petsc/private/kspimpl.h>

/*
     KSPSetUp_PIPECR - Sets up the workspace needed by the PIPECR method.

      This is called once, usually automatically by KSPSolve() or KSPSetUp()
     but can be called directly by KSPSetUp()
*/
#undef __FUNCT__
#define __FUNCT__ "KSPSetUp_PIPECR"
PetscErrorCode KSPSetUp_PIPECR(KSP ksp)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  /* get work vectors needed by PIPECR */
  ierr = KSPSetWorkVecs(ksp,7);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/*
 KSPSolve_PIPECR - This routine actually applies the pipelined conjugate residual method

 Input Parameter:
 .     ksp - the Krylov space object that was set to use conjugate gradient, by, for
             example, KSPCreate(MPI_Comm,KSP *ksp); KSPSetType(ksp,KSPCG);
*/
#undef __FUNCT__
#define __FUNCT__ "KSPSolve_PIPECR"
PetscErrorCode  KSPSolve_PIPECR(KSP ksp)
{
  PetscErrorCode ierr;
  PetscInt       i;
  PetscScalar    alpha=0.0,beta=0.0,gamma,gammaold=0.0,delta;
  PetscReal      dp   = 0.0;
  Vec            X,B,Z,P,W,Q,U,M,N;
  Mat            Amat,Pmat;
  PetscBool      diagonalscale;

  PetscFunctionBegin;
  ierr = PCGetDiagonalScale(ksp->pc,&diagonalscale);CHKERRQ(ierr);
  if (diagonalscale) SETERRQ1(PetscObjectComm((PetscObject)ksp),PETSC_ERR_SUP,"Krylov method %s does not support diagonal scaling",((PetscObject)ksp)->type_name);

  X = ksp->vec_sol;
  B = ksp->vec_rhs;
  M = ksp->work[0];
  Z = ksp->work[1];
  P = ksp->work[2];
  N = ksp->work[3];
  W = ksp->work[4];
  Q = ksp->work[5];
  U = ksp->work[6];

  ierr = PCGetOperators(ksp->pc,&Amat,&Pmat);CHKERRQ(ierr);

  ksp->its = 0;
  /* we don't have an R vector, so put the (unpreconditioned) residual in w for now */
  if (!ksp->guess_zero) {
    ierr = KSP_MatMult(ksp,Amat,X,W);CHKERRQ(ierr);            /*     w <- b - Ax     */
    ierr = VecAYPX(W,-1.0,B);CHKERRQ(ierr);
  } else {
    ierr = VecCopy(B,W);CHKERRQ(ierr);                         /*     w <- b (x is 0) */
  }
  ierr = KSP_PCApply(ksp,W,U);CHKERRQ(ierr);                   /*     u <- Bw   */

  switch (ksp->normtype) {
  case KSP_NORM_PRECONDITIONED:
    ierr = VecNormBegin(U,NORM_2,&dp);CHKERRQ(ierr);           /*     dp <- u'*u = e'*A'*B'*B*A'*e'     */
    ierr = PetscCommSplitReductionBegin(PetscObjectComm((PetscObject)U));CHKERRQ(ierr);
    ierr = KSP_MatMult(ksp,Amat,U,W);CHKERRQ(ierr);            /*     w <- Au   */
    ierr = VecNormEnd(U,NORM_2,&dp);CHKERRQ(ierr);
    break;
  case KSP_NORM_NONE:
    ierr = KSP_MatMult(ksp,Amat,U,W);CHKERRQ(ierr);
    dp   = 0.0;
    break;
  default: SETERRQ1(PetscObjectComm((PetscObject)ksp),PETSC_ERR_SUP,"%s",KSPNormTypes[ksp->normtype]);
  }
  ierr       = KSPLogResidualHistory(ksp,dp);CHKERRQ(ierr);
  ierr       = KSPMonitor(ksp,0,dp);CHKERRQ(ierr);
  ksp->rnorm = dp;
  ierr       = (*ksp->converged)(ksp,0,dp,&ksp->reason,ksp->cnvP);CHKERRQ(ierr); /* test for convergence */
  if (ksp->reason) PetscFunctionReturn(0);

  i = 0;
  do {
    ierr = KSP_PCApply(ksp,W,M);CHKERRQ(ierr);            /*   m <- Bw       */

    if (i > 0 && ksp->normtype == KSP_NORM_PRECONDITIONED) {
      ierr = VecNormBegin(U,NORM_2,&dp);CHKERRQ(ierr);
    }
    ierr = VecDotBegin(W,U,&gamma);CHKERRQ(ierr);
    ierr = VecDotBegin(M,W,&delta);CHKERRQ(ierr);
    ierr = PetscCommSplitReductionBegin(PetscObjectComm((PetscObject)U));CHKERRQ(ierr);

    ierr = KSP_MatMult(ksp,Amat,M,N);CHKERRQ(ierr);       /*   n <- Am       */

    if (i > 0 && ksp->normtype == KSP_NORM_PRECONDITIONED) {
      ierr = VecNormEnd(U,NORM_2,&dp);CHKERRQ(ierr);
    }
    ierr = VecDotEnd(W,U,&gamma);CHKERRQ(ierr);
    ierr = VecDotEnd(M,W,&delta);CHKERRQ(ierr);

    if (i > 0) {
      if (ksp->normtype == KSP_NORM_NONE) dp = 0.0;
      ksp->rnorm = dp;
      ierr = KSPLogResidualHistory(ksp,dp);CHKERRQ(ierr);
      ierr = KSPMonitor(ksp,i,dp);CHKERRQ(ierr);
      ierr = (*ksp->converged)(ksp,i,dp,&ksp->reason,ksp->cnvP);CHKERRQ(ierr);
      if (ksp->reason) break;
    }

    if (i == 0) {
      alpha = gamma / delta;
      ierr  = VecCopy(N,Z);CHKERRQ(ierr);        /*     z <- n          */
      ierr  = VecCopy(M,Q);CHKERRQ(ierr);        /*     q <- m          */
      ierr  = VecCopy(U,P);CHKERRQ(ierr);        /*     p <- u          */
    } else {
      beta  = gamma / gammaold;
      alpha = gamma / (delta - beta / alpha * gamma);
      ierr  = VecAYPX(Z,beta,N);CHKERRQ(ierr);   /*     z <- n + beta * z   */
      ierr  = VecAYPX(Q,beta,M);CHKERRQ(ierr);   /*     q <- m + beta * q   */
      ierr  = VecAYPX(P,beta,U);CHKERRQ(ierr);   /*     p <- u + beta * p   */
    }
    ierr     = VecAXPY(X, alpha,P);CHKERRQ(ierr); /*     x <- x + alpha * p   */
    ierr     = VecAXPY(U,-alpha,Q);CHKERRQ(ierr); /*     u <- u - alpha * q   */
    ierr     = VecAXPY(W,-alpha,Z);CHKERRQ(ierr); /*     w <- w - alpha * z   */
    gammaold = gamma;
    i++;
    ksp->its = i;

    /* if (i%50 == 0) { */
    /*   ierr = KSP_MatMult(ksp,Amat,X,W);CHKERRQ(ierr);            /\*     w <- b - Ax     *\/ */
    /*   ierr = VecAYPX(W,-1.0,B);CHKERRQ(ierr); */
    /*   ierr = KSP_PCApply(ksp,W,U);CHKERRQ(ierr); */
    /*   ierr = KSP_MatMult(ksp,Amat,U,W);CHKERRQ(ierr); */
    /* } */

  } while (i<ksp->max_it);
  if (i >= ksp->max_it) ksp->reason = KSP_DIVERGED_ITS;
  PetscFunctionReturn(0);
}

/*MC
   KSPPIPECR - Pipelined conjugate residual method

   This method has only a single non-blocking reduction per iteration, compared to 2 blocking for standard CR.  The
   non-blocking reduction is overlapped by the matrix-vector product, but not the preconditioner application.

   See also KSPPIPECG, where the reduction is only overlapped with the matrix-vector product.

   Level: intermediate

   Notes:
   MPI configuration may be necessary for reductions to make asynchronous progress, which is important for performance of pipelined methods.
   See the FAQ on the PETSc website for details.

   Contributed by:
   Pieter Ghysels, Universiteit Antwerpen, Intel Exascience lab Flanders

   Reference:
   P. Ghysels and W. Vanroose, "Hiding global synchronization latency in the preconditioned Conjugate Gradient algorithm",
   Submitted to Parallel Computing, 2012.

.seealso: KSPCreate(), KSPSetType(), KSPPIPECG, KSPGROPPCG, KSPPGMRES, KSPCG, KSPCGUseSingleReduction()
M*/

#undef __FUNCT__
#define __FUNCT__ "KSPCreate_PIPECR"
PETSC_EXTERN PetscErrorCode KSPCreate_PIPECR(KSP ksp)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = KSPSetSupportedNorm(ksp,KSP_NORM_PRECONDITIONED,PC_LEFT,2);CHKERRQ(ierr);
  ierr = KSPSetSupportedNorm(ksp,KSP_NORM_NONE,PC_LEFT,2);CHKERRQ(ierr);

  ksp->ops->setup          = KSPSetUp_PIPECR;
  ksp->ops->solve          = KSPSolve_PIPECR;
  ksp->ops->destroy        = KSPDestroyDefault;
  ksp->ops->view           = 0;
  ksp->ops->setfromoptions = 0;
  ksp->ops->buildsolution  = KSPBuildSolutionDefault;
  ksp->ops->buildresidual  = KSPBuildResidualDefault;
  PetscFunctionReturn(0);
}
