/*
    This file implements the FCG (Flexible Conjugate Gradient) method

*/

#include <../src/ksp/ksp/impls/fcg/fcgimpl.h>       /*I  "petscksp.h"  I*/
extern PetscErrorCode KSPComputeExtremeSingularValues_CG(KSP,PetscReal*,PetscReal*);
extern PetscErrorCode KSPComputeEigenvalues_CG(KSP,PetscInt,PetscReal*,PetscReal*,PetscInt*);

const char *const KSPFCGTruncationTypes[]     = {"STANDARD","NOTAY","KSPFCGTrunctionTypes","KSP_FCG_TRUNC_TYPE_",0};

#define KSPFCG_DEFAULT_MMAX 30          /* maximum number of search directions to keep */
#define KSPFCG_DEFAULT_NPREALLOC 10     /* number of search directions to preallocate */
#define KSPFCG_DEFAULT_VECB 5           /* number of search directions to allocate each time new direction vectors are needed */
#define KSPFCG_DEFAULT_TRUNCSTRAT KSP_FCG_TRUNC_TYPE_NOTAY

#undef __FUNCT__
#define __FUNCT__ "KSPAllocateVectors_FCG"
static PetscErrorCode KSPAllocateVectors_FCG(KSP ksp, PetscInt nvecsneeded, PetscInt chunksize)
{
  PetscErrorCode  ierr;
  PetscInt        i;
  KSP_FCG         *fcg = (KSP_FCG*)ksp->data;
  PetscInt        nnewvecs, nvecsprev;

  PetscFunctionBegin;
  /* Allocate enough new vectors to add chunksize new vectors, reach nvecsneedtotal, or to reach mmax+1, whichever is smallest */
  if (fcg->nvecs < PetscMin(fcg->mmax+1,nvecsneeded)){
    nvecsprev = fcg->nvecs;
    nnewvecs = PetscMin(PetscMax(nvecsneeded-fcg->nvecs,chunksize),fcg->mmax+1-fcg->nvecs);
    ierr = KSPCreateVecs(ksp,nnewvecs,&fcg->pCvecs[fcg->nchunks],0,NULL);CHKERRQ(ierr);
    ierr = PetscLogObjectParents((PetscObject)ksp,nnewvecs,fcg->pCvecs[fcg->nchunks]);CHKERRQ(ierr);
    ierr = KSPCreateVecs(ksp,nnewvecs,&fcg->pPvecs[fcg->nchunks],0,NULL);CHKERRQ(ierr);
    ierr = PetscLogObjectParents((PetscObject)ksp,nnewvecs,fcg->pPvecs[fcg->nchunks]);CHKERRQ(ierr);
    fcg->nvecs += nnewvecs;
    for (i=0;i<nnewvecs;++i){
      fcg->Cvecs[nvecsprev + i] = fcg->pCvecs[fcg->nchunks][i];
      fcg->Pvecs[nvecsprev + i] = fcg->pPvecs[fcg->nchunks][i];
    }
    fcg->chunksizes[fcg->nchunks] = nnewvecs;
    ++fcg->nchunks;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "KSPSetUp_FCG"
PetscErrorCode    KSPSetUp_FCG(KSP ksp)
{
  PetscErrorCode ierr;
  KSP_FCG        *fcg = (KSP_FCG*)ksp->data;
  PetscInt       maxit = ksp->max_it;
  const PetscInt nworkstd = 2;

  PetscFunctionBegin;

  /* Allocate "standard" work vectors (not including the basis and transformed basis vectors) */
  ierr = KSPSetWorkVecs(ksp,nworkstd);CHKERRQ(ierr);

  /* Allocated space for pointers to additional work vectors
   note that mmax is the number of previous directions, so we add 1 for the current direction,
   and an extra 1 for the prealloc (which might be empty) */
  ierr = PetscMalloc5(fcg->mmax+1,&fcg->Pvecs,fcg->mmax+1,&fcg->Cvecs,fcg->mmax+1,&fcg->pPvecs,fcg->mmax+1,&fcg->pCvecs,fcg->mmax+2,&fcg->chunksizes);CHKERRQ(ierr);
  ierr = PetscLogObjectMemory((PetscObject)ksp,2*(fcg->mmax+1)*sizeof(Vec*) + 2*(fcg->mmax + 1)*sizeof(Vec**) + (fcg->mmax + 2)*sizeof(PetscInt));CHKERRQ(ierr);

  /* Preallocate additional work vectors */
  ierr = KSPAllocateVectors_FCG(ksp,fcg->nprealloc,fcg->nprealloc);CHKERRQ(ierr);
  /*
  If user requested computations of eigenvalues then allocate work
  work space needed
  */
  if (ksp->calc_sings) {
    /* get space to store tridiagonal matrix for Lanczos */
    ierr = PetscMalloc4(maxit,&fcg->e,maxit,&fcg->d,maxit,&fcg->ee,maxit,&fcg->dd);CHKERRQ(ierr);
    ierr = PetscLogObjectMemory((PetscObject)ksp,2*(maxit+1)*(sizeof(PetscScalar)+sizeof(PetscReal)));CHKERRQ(ierr);

    ksp->ops->computeextremesingularvalues = KSPComputeExtremeSingularValues_CG;
    ksp->ops->computeeigenvalues           = KSPComputeEigenvalues_CG;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "KSPSolve_FCG"
PetscErrorCode KSPSolve_FCG(KSP ksp)
{
  PetscErrorCode ierr;
  PetscInt       i,k,idx,mi;
  KSP_FCG        *fcg = (KSP_FCG*)ksp->data;
  PetscScalar    alpha=0.0,beta = 0.0,dpi,s;
  PetscReal      dp=0.0;
  Vec            B,R,Z,X,Pcurr,Ccurr;
  Mat            Amat,Pmat;
  PetscInt       eigs = ksp->calc_sings; /* Variables for eigen estimation - START*/
  PetscInt       stored_max_it = ksp->max_it;
  PetscScalar    alphaold = 0,betaold = 1.0,*e = 0,*d = 0;/* Variables for eigen estimation  - FINISH */

  PetscFunctionBegin;

#define VecXDot(x,y,a) (((fcg->type) == (KSP_CG_HERMITIAN)) ? VecDot(x,y,a) : VecTDot(x,y,a))
#define VecXMDot(a,b,c,d) (((fcg->type) == (KSP_CG_HERMITIAN)) ? VecMDot(a,b,c,d) : VecMTDot(a,b,c,d))

  X             = ksp->vec_sol;
  B             = ksp->vec_rhs;
  R             = ksp->work[0];
  Z             = ksp->work[1];

  ierr = PCGetOperators(ksp->pc,&Amat,&Pmat);CHKERRQ(ierr);
  if (eigs) {e = fcg->e; d = fcg->d; e[0] = 0.0; }
  /* Compute initial residual needed for convergence check*/
  ksp->its = 0;
  if (!ksp->guess_zero) {
    ierr = KSP_MatMult(ksp,Amat,X,R);CHKERRQ(ierr);
    ierr = VecAYPX(R,-1.0,B);CHKERRQ(ierr);                    /*   r <- b - Ax     */
  } else {
    ierr = VecCopy(B,R);CHKERRQ(ierr);                         /*   r <- b (x is 0) */
  }
  switch (ksp->normtype) {
    case KSP_NORM_PRECONDITIONED:
      ierr = KSP_PCApply(ksp,R,Z);CHKERRQ(ierr);               /*   z <- Br         */
      ierr = VecNorm(Z,NORM_2,&dp);CHKERRQ(ierr);              /*   dp <- dqrt(z'*z) = sqrt(e'*A'*B'*B*A*e)     */
      break;
    case KSP_NORM_UNPRECONDITIONED:
      ierr = VecNorm(R,NORM_2,&dp);CHKERRQ(ierr);              /*   dp <- sqrt(r'*r) = sqrt(e'*A'*A*e)     */
      break;
    case KSP_NORM_NATURAL:
      ierr = KSP_PCApply(ksp,R,Z);CHKERRQ(ierr);               /*   z <- Br         */
      ierr = VecXDot(R,Z,&s);CHKERRQ(ierr);
      dp = PetscSqrtReal(PetscAbsScalar(s));                   /*   dp <- sqrt(r'*z) = sqrt(e'*A'*B*A*e)  */
      break;
    case KSP_NORM_NONE:
      dp = 0.0;
      break;
    default: SETERRQ1(PetscObjectComm((PetscObject)ksp),PETSC_ERR_SUP,"%s",KSPNormTypes[ksp->normtype]);
  }

  /* Initial Convergence Check */
  ierr       = KSPLogResidualHistory(ksp,dp);CHKERRQ(ierr);
  ierr       = KSPMonitor(ksp,0,dp);CHKERRQ(ierr);
  ksp->rnorm = dp;
  if (ksp->normtype == KSP_NORM_NONE) {
    ierr = KSPConvergedSkip(ksp,0,dp,&ksp->reason,ksp->cnvP);CHKERRQ(ierr);
  } else {
    ierr = (*ksp->converged)(ksp,0,dp,&ksp->reason,ksp->cnvP);CHKERRQ(ierr);
  }
  if (ksp->reason) PetscFunctionReturn(0);

  /* Apply PC if not already done for convergence check */
  if(ksp->normtype == KSP_NORM_UNPRECONDITIONED || ksp->normtype == KSP_NORM_NONE){
    ierr = KSP_PCApply(ksp,R,Z);CHKERRQ(ierr);               /*   z <- Br         */
  }

  i = 0;
  do {
    ksp->its = i+1;

    /*  If needbe, allocate a new chunk of vectors in P and C */
    ierr = KSPAllocateVectors_FCG(ksp,i+1,fcg->vecb);CHKERRQ(ierr);

    /* Note that we wrap around and start clobbering old vectors */
    idx = i % (fcg->mmax+1);
    Pcurr = fcg->Pvecs[idx];
    Ccurr = fcg->Cvecs[idx];

    /* Compute a new column of P (Currently does not support modified G-S or iterative refinement)*/
    switch(fcg->truncstrat){
      case KSP_FCG_TRUNC_TYPE_NOTAY :
        mi = PetscMax(1,i%(fcg->mmax+1));
        break;
      case KSP_FCG_TRUNC_TYPE_STANDARD :
        mi = fcg->mmax;
        break;
      default:
        SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONG,"Unrecognized FCG Truncation Strategy");CHKERRQ(ierr);
    }
    ierr = VecCopy(Z,Pcurr);CHKERRQ(ierr);

    {
      PetscInt l,ndots;

      l = PetscMax(0,i-mi);
      ndots = i-l;
      if (ndots){
        PetscInt    j;
        Vec         *Pold,  *Cold;
        PetscScalar *dots;

        ierr = PetscMalloc3(ndots,&dots,ndots,&Cold,ndots,&Pold);CHKERRQ(ierr);
        for(k=l,j=0;j<ndots;++k,++j){
          idx = k % (fcg->mmax+1);
          Cold[j] = fcg->Cvecs[idx];
          Pold[j] = fcg->Pvecs[idx];
        }
        ierr = VecXMDot(Z,ndots,Cold,dots);CHKERRQ(ierr);
        for(k=0;k<ndots;++k){
          dots[k] = -dots[k];
        }
        ierr = VecMAXPY(Pcurr,ndots,dots,Pold);CHKERRQ(ierr);
        ierr = PetscFree3(dots,Cold,Pold);CHKERRQ(ierr);
      }
    }

    /* Update X and R */
    betaold = beta;
    ierr = VecXDot(Pcurr,R,&beta);CHKERRQ(ierr);                 /*  beta <- pi'*r       */
    ierr = KSP_MatMult(ksp,Amat,Pcurr,Ccurr);CHKERRQ(ierr);      /*  w <- A*pi (stored in ci)   */
    ierr = VecXDot(Pcurr,Ccurr,&dpi);CHKERRQ(ierr);              /*  dpi <- pi'*w        */
    alphaold = alpha;
    alpha = beta / dpi;                                          /*  alpha <- beta/dpi    */
    ierr = VecAXPY(X,alpha,Pcurr);CHKERRQ(ierr);                 /*  x <- x + alpha * pi  */
    ierr = VecAXPY(R,-alpha,Ccurr);CHKERRQ(ierr);                /*  r <- r - alpha * wi  */

    /* Compute norm for convergence check */
    switch (ksp->normtype) {
      case KSP_NORM_PRECONDITIONED:
        ierr = KSP_PCApply(ksp,R,Z);CHKERRQ(ierr);               /*   z <- Br             */
        ierr = VecNorm(Z,NORM_2,&dp);CHKERRQ(ierr);              /*   dp <- sqrt(z'*z) = sqrt(e'*A'*B'*B*A*e)  */
        break;
      case KSP_NORM_UNPRECONDITIONED:
        ierr = VecNorm(R,NORM_2,&dp);CHKERRQ(ierr);              /*   dp <- sqrt(r'*r) = sqrt(e'*A'*A*e)   */
        break;
      case KSP_NORM_NATURAL:
        ierr = KSP_PCApply(ksp,R,Z);CHKERRQ(ierr);               /*   z <- Br             */
        ierr = VecXDot(R,Z,&s);CHKERRQ(ierr);
        dp = PetscSqrtReal(PetscAbsScalar(s));                   /*   dp <- sqrt(r'*z) = sqrt(e'*A'*B*A*e)  */
        break;
      case KSP_NORM_NONE:
        dp = 0.0;
        break;
      default: SETERRQ1(PetscObjectComm((PetscObject)ksp),PETSC_ERR_SUP,"%s",KSPNormTypes[ksp->normtype]);
    }

    /* Check for convergence */
    ksp->rnorm = dp;
    KSPLogResidualHistory(ksp,dp);CHKERRQ(ierr);
    ierr = KSPMonitor(ksp,i+1,dp);CHKERRQ(ierr);
    ierr = (*ksp->converged)(ksp,i+1,dp,&ksp->reason,ksp->cnvP);CHKERRQ(ierr);
    if (ksp->reason) break;

    /* Apply PC if not already done for convergence check */
    if(ksp->normtype == KSP_NORM_UNPRECONDITIONED || ksp->normtype == KSP_NORM_NONE){
      ierr = KSP_PCApply(ksp,R,Z);CHKERRQ(ierr);               /*   z <- Br         */
    }

    /* Compute current C (which is W/dpi) */
    ierr = VecScale(Ccurr,1.0/dpi);CHKERRQ(ierr);              /*   w <- ci/dpi   */

    if (eigs) {
      if (i > 0) {
        if (ksp->max_it != stored_max_it) SETERRQ(PetscObjectComm((PetscObject)ksp),PETSC_ERR_SUP,"Can not change maxit AND calculate eigenvalues");
        e[i] = PetscSqrtReal(PetscAbsScalar(beta/betaold))/alphaold;
        d[i] = PetscSqrtReal(PetscAbsScalar(beta/betaold))*e[i] + 1.0/alpha;
      } else {
        d[i] = PetscSqrtReal(PetscAbsScalar(beta))*e[i] + 1.0/alpha;
      }
      fcg->ned = ksp->its-1;
    }
    ++i;
  } while (i<ksp->max_it);
  if (i >= ksp->max_it) ksp->reason = KSP_DIVERGED_ITS;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "KSPDestroy_FCG"
PetscErrorCode KSPDestroy_FCG(KSP ksp)
{
  PetscErrorCode ierr;
  PetscInt       i;
  KSP_FCG        *fcg = (KSP_FCG*)ksp->data;

  PetscFunctionBegin;

  /* Destroy "standard" work vecs */
  VecDestroyVecs(ksp->nwork,&ksp->work);

  /* Destroy P and C vectors and the arrays that manage pointers to them */
  if (fcg->nvecs){
    for (i=0;i<fcg->nchunks;++i){
      ierr = VecDestroyVecs(fcg->chunksizes[i],&fcg->pPvecs[i]);CHKERRQ(ierr);
      ierr = VecDestroyVecs(fcg->chunksizes[i],&fcg->pCvecs[i]);CHKERRQ(ierr);
    }
  }
  ierr = PetscFree5(fcg->Pvecs,fcg->Cvecs,fcg->pPvecs,fcg->pCvecs,fcg->chunksizes);CHKERRQ(ierr);
  /* free space used for singular value calculations */
  if (ksp->calc_sings) {
    ierr = PetscFree4(fcg->e,fcg->d,fcg->ee,fcg->dd);CHKERRQ(ierr);
  }
  ierr = KSPDestroyDefault(ksp);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "KSPView_FCG"
PetscErrorCode KSPView_FCG(KSP ksp,PetscViewer viewer)
{
  KSP_FCG        *fcg = (KSP_FCG*)ksp->data;
  PetscErrorCode ierr;
  PetscBool      iascii,isstring;
  const char     *truncstr;

  PetscFunctionBegin;
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERASCII,&iascii);CHKERRQ(ierr);
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERSTRING,&isstring);CHKERRQ(ierr);

  if (fcg->truncstrat == KSP_FCG_TRUNC_TYPE_STANDARD) truncstr = "Using standard truncation strategy";
  else if (fcg->truncstrat == KSP_FCG_TRUNC_TYPE_NOTAY) truncstr = "Using Notay's truncation strategy";
  else SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONGSTATE,"Undefined FCG truncation strategy");

  if (iascii) {
    ierr = PetscViewerASCIIPrintf(viewer,"  FCG: m_max=%D\n",fcg->mmax);CHKERRQ(ierr);
    ierr = PetscViewerASCIIPrintf(viewer,"  FCG: preallocated %D directions\n",PetscMin(fcg->nprealloc,fcg->mmax+1));CHKERRQ(ierr);
    ierr = PetscViewerASCIIPrintf(viewer,"  FCG: %s\n",truncstr);CHKERRQ(ierr);
  } else if (isstring) {
    ierr = PetscViewerStringSPrintf(viewer,"m_max %D nprealloc %D %s",fcg->mmax,fcg->nprealloc,truncstr);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "KSPFCGSetMmax"
/*@
  KSPFCGSetMmax - set the maximum number of previous directions FCG will store for orthogonalization

  Note: mmax + 1 directions are stored (mmax previous ones along with a current one)
  and whether all are used in each iteration also depends on the truncation strategy
  (see KSPFCGSetTruncationType())

  Logically Collective on KSP

  Input Parameters:
+  ksp - the Krylov space context
-  mmax - the maximum number of previous directions to orthogonalize againt

  Level: intermediate

  Options Database:
. -ksp_fcg_mmax <N>

.seealso: KSPFCG, KSPFCGGetTruncationType(), KSPFCGGetNprealloc()
@*/
PetscErrorCode KSPFCGSetMmax(KSP ksp,PetscInt mmax)
{
  KSP_FCG *fcg = (KSP_FCG*)ksp->data;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ksp,KSP_CLASSID,1);
  PetscValidLogicalCollectiveEnum(ksp,mmax,2);
  fcg->mmax = mmax;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "KSPFCGGetMmax"
/*@
  KSPFCGGetMmax - get the maximum number of previous directions FCG will store

  Note: FCG stores mmax+1 directions at most (mmax previous ones, and one current one)

   Not Collective

   Input Parameter:
.  ksp - the Krylov space context

   Output Parameter:
.  mmax - the maximum number of previous directons allowed for orthogonalization

  Options Database:
. -ksp_fcg_mmax <N>

   Level: intermediate

.keywords: KSP, FCG, truncation

.seealso: KSPFCG, KSPFCGGetTruncationType(), KSPFCGGetNprealloc(), KSPFCGSetMmax()
@*/

PetscErrorCode KSPFCGGetMmax(KSP ksp,PetscInt *mmax)
{
  KSP_FCG *fcg=(KSP_FCG*)ksp->data;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ksp,KSP_CLASSID,1);
  *mmax = fcg->mmax;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "KSPFCGSetNprealloc"
/*@
  KSPFCGSetNprealloc - set the number of directions to preallocate with FCG

  Logically Collective on KSP

  Input Parameters:
+  ksp - the Krylov space context
-  nprealloc - the number of vectors to preallocate

  Level: advanced

  Options Database:
. -ksp_fcg_nprealloc <N>

.seealso: KSPFCG, KSPFCGGetTruncationType(), KSPFCGGetNprealloc()
@*/
PetscErrorCode KSPFCGSetNprealloc(KSP ksp,PetscInt nprealloc)
{
  KSP_FCG *fcg=(KSP_FCG*)ksp->data;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ksp,KSP_CLASSID,1);
  PetscValidLogicalCollectiveEnum(ksp,nprealloc,2);
  if(nprealloc > fcg->mmax+1) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Cannot preallocate more than m_max+1 vectors");
  fcg->nprealloc = nprealloc;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "KSPFCGGetNprealloc"
/*@
  KSPFCGGetNprealloc - get the number of directions preallocate by FCG

   Not Collective

   Input Parameter:
.  ksp - the Krylov space context

   Output Parameter:
.  nprealloc - the number of directions preallocated

  Options Database:
. -ksp_fcg_nprealloc <N>

   Level: advanced

.keywords: KSP, FCG, truncation

.seealso: KSPFCG, KSPFCGGetTruncationType(), KSPFCGSetNprealloc()
@*/
PetscErrorCode KSPFCGGetNprealloc(KSP ksp,PetscInt *nprealloc)
{
  KSP_FCG *fcg=(KSP_FCG*)ksp->data;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ksp,KSP_CLASSID,1);
  *nprealloc = fcg->nprealloc;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "KSPFCGSetTruncationType"
/*@
  KSPFCGSetTruncationType - specify how many of its stored previous directions FCG uses during orthoganalization

  Logically Collective on KSP

  KSP_FCG_TRUNC_TYPE_STANDARD uses all (up to mmax) stored directions
  KSP_FCG_TRUNC_TYPE_NOTAY uses the last max(1,mod(i,mmax)) stored directions at iteration i=0,1..

  Input Parameters:
+  ksp - the Krylov space context
-  truncstrat - the choice of strategy

  Level: intermediate

  Options Database:
. -ksp_fcg_truncation_type <standard, notay>

  .seealso: KSPFCGTruncationType, KSPFCGGetTruncationType
@*/
PetscErrorCode KSPFCGSetTruncationType(KSP ksp,KSPFCGTruncationType truncstrat)
{
  KSP_FCG *fcg=(KSP_FCG*)ksp->data;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ksp,KSP_CLASSID,1);
  PetscValidLogicalCollectiveEnum(ksp,truncstrat,2);
  fcg->truncstrat=truncstrat;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "KSPFCGGetTruncationType"
/*@
  KSPFCGGetTruncationType - get the truncation strategy employed by FCG

   Not Collective

   Input Parameter:
.  ksp - the Krylov space context

   Output Parameter:
.  truncstrat - the strategy type

  Options Database:
. -ksp_fcg_truncation_type <standard, notay>

   Level: intermediate

.keywords: KSP, FCG, truncation

.seealso: KSPFCG, KSPFCGSetTruncationType, KSPFCGTruncationType
@*/
PetscErrorCode KSPFCGGetTruncationType(KSP ksp,KSPFCGTruncationType *truncstrat)
{
  KSP_FCG *fcg=(KSP_FCG*)ksp->data;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(ksp,KSP_CLASSID,1);
  *truncstrat=fcg->truncstrat;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "KSPSetFromOptions_FCG"
PetscErrorCode KSPSetFromOptions_FCG(PetscOptionItems *PetscOptionsObject,KSP ksp)
{
  PetscErrorCode ierr;
  KSP_FCG        *fcg=(KSP_FCG*)ksp->data;
  PetscInt       mmax,nprealloc;
  PetscBool      flg;

  PetscFunctionBegin;
  ierr = PetscOptionsHead(PetscOptionsObject,"KSP FCG Options");CHKERRQ(ierr);
  ierr = PetscOptionsInt("-ksp_fcg_mmax","Maximum number of search directions to store","KSPFCGSetMmax",fcg->mmax,&mmax,&flg);CHKERRQ(ierr);
  if (flg) {
    ierr = KSPFCGSetMmax(ksp,mmax);CHKERRQ(ierr); 
  }
  ierr = PetscOptionsInt("-ksp_fcg_nprealloc","Number of directions to preallocate","KSPFCGSetNprealloc",fcg->nprealloc,&nprealloc,&flg);CHKERRQ(ierr);
  if (flg) { 
    ierr = KSPFCGSetNprealloc(ksp,nprealloc);CHKERRQ(ierr); 
  }
  ierr = PetscOptionsEnum("-ksp_fcg_truncation_type","Truncation approach for directions","KSPFCGSetTruncationType",KSPFCGTruncationTypes,(PetscEnum)fcg->truncstrat,(PetscEnum*)&fcg->truncstrat,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsTail();CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/*MC
      KSPFCG - Implements the Flexible Conjugate Gradient method (FCG)

  Options Database Keys:
+   -ksp_fcg_mmax <N>
.   -ksp_fcg_nprealloc <N>
-   -ksp_fcg_truncation_type <standard,notay>

    Contributed by Patrick Sanan

   Notes:
   Supports left preconditioning only.

   Level: beginner

  References:
+    1. - Notay, Y."Flexible Conjugate Gradients", SIAM J. Sci. Comput. 22:4, 2000
-    2. - Axelsson, O. and Vassilevski, P. S. "A Black Box Generalized Conjugate Gradient Solver with Inner Iterations and Variable step Preconditioning",
    SIAM J. Matrix Anal. Appl. 12:4, 1991

 .seealso : KSPGCR, KSPFGMRES, KSPCG, KSPFCGSetMmax(), KSPFCGGetMmax(), KSPFCGSetNprealloc(), KSPFCGGetNprealloc(), KSPFCGSetTruncationType(), KSPFCGGetTruncationType()

M*/
#undef __FUNCT__
#define __FUNCT__ "KSPCreate_FCG"
PETSC_EXTERN PetscErrorCode KSPCreate_FCG(KSP ksp)
{
  PetscErrorCode ierr;
  KSP_FCG        *fcg;

  PetscFunctionBegin;
  ierr = PetscNewLog(ksp,&fcg);CHKERRQ(ierr);
#if !defined(PETSC_USE_COMPLEX)
  fcg->type       = KSP_CG_SYMMETRIC;
#else
  fcg->type       = KSP_CG_HERMITIAN;
#endif
  fcg->mmax       = KSPFCG_DEFAULT_MMAX;
  fcg->nprealloc  = KSPFCG_DEFAULT_NPREALLOC;
  fcg->nvecs      = 0;
  fcg->vecb       = KSPFCG_DEFAULT_VECB;
  fcg->nchunks    = 0;
  fcg->truncstrat = KSPFCG_DEFAULT_TRUNCSTRAT;

  ksp->data = (void*)fcg;

  ierr = KSPSetSupportedNorm(ksp,KSP_NORM_PRECONDITIONED,PC_LEFT,2);CHKERRQ(ierr);
  ierr = KSPSetSupportedNorm(ksp,KSP_NORM_UNPRECONDITIONED,PC_LEFT,1);CHKERRQ(ierr);
  ierr = KSPSetSupportedNorm(ksp,KSP_NORM_NATURAL,PC_LEFT,1);CHKERRQ(ierr);
  ierr = KSPSetSupportedNorm(ksp,KSP_NORM_NONE,PC_LEFT,1);CHKERRQ(ierr);

  ksp->ops->setup          = KSPSetUp_FCG;
  ksp->ops->solve          = KSPSolve_FCG;
  ksp->ops->destroy        = KSPDestroy_FCG;
  ksp->ops->view           = KSPView_FCG;
  ksp->ops->setfromoptions = KSPSetFromOptions_FCG;
  ksp->ops->buildsolution  = KSPBuildSolutionDefault;
  ksp->ops->buildresidual  = KSPBuildResidualDefault;
  PetscFunctionReturn(0);
}

