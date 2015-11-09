#include <../src/tao/leastsquares/impls/pounders/pounders.h>

#undef __FUNCT__
#define __FUNCT__ "pounders_h"
static PetscErrorCode pounders_h(Tao subtao, Vec v, Mat H, Mat Hpre, void *ctx)
{
  PetscFunctionBegin;
  PetscFunctionReturn(0);
}
#undef __FUNCT__
#define __FUNCT__ "pounders_fg"
static PetscErrorCode  pounders_fg(Tao subtao, Vec x, PetscReal *f, Vec g, void *ctx)
{
  TAO_POUNDERS   *mfqP = (TAO_POUNDERS*)ctx;
  PetscReal      d1,d2;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  /* g = A*x  (add b later)*/
  ierr = MatMult(mfqP->subH,x,g);CHKERRQ(ierr);

  /* f = 1/2 * x'*(Ax) + b'*x  */
  ierr = VecDot(x,g,&d1);CHKERRQ(ierr);
  ierr = VecDot(mfqP->subb,x,&d2);CHKERRQ(ierr);
  *f = 0.5 *d1 + d2;

  /* now  g = g + b */
  ierr = VecAXPY(g, 1.0, mfqP->subb);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "gqtwrap"
PetscErrorCode gqtwrap(Tao tao,PetscReal *gnorm, PetscReal *qmin)
{
  PetscErrorCode ierr;
#if defined(PETSC_USE_REAL_SINGLE)
  PetscReal      atol=1.0e-5;
#else
  PetscReal      atol=1.0e-10;
#endif
  PetscInt       info,its;
  TAO_POUNDERS   *mfqP = (TAO_POUNDERS*)tao->data;

  PetscFunctionBegin;
  if (! mfqP->usegqt) {
    PetscReal maxval;
    PetscInt  i,j;

    ierr = VecSetValues(mfqP->subb,mfqP->n,mfqP->indices,mfqP->Gres,INSERT_VALUES);CHKERRQ(ierr);
    ierr = VecAssemblyBegin(mfqP->subb);CHKERRQ(ierr);
    ierr = VecAssemblyEnd(mfqP->subb);CHKERRQ(ierr);

    ierr = VecSet(mfqP->subx,0.0);CHKERRQ(ierr);

    ierr = VecSet(mfqP->subndel,-mfqP->delta);CHKERRQ(ierr);
    ierr = VecSet(mfqP->subpdel,mfqP->delta);CHKERRQ(ierr);

    for (i=0;i<mfqP->n;i++) {
      for (j=i;j<mfqP->n;j++) {
        mfqP->Hres[j+mfqP->n*i] = mfqP->Hres[mfqP->n*j+i];
      }
    }
    ierr = MatSetValues(mfqP->subH,mfqP->n,mfqP->indices,mfqP->n,mfqP->indices,mfqP->Hres,INSERT_VALUES);CHKERRQ(ierr);
    ierr = MatAssemblyBegin(mfqP->subH,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
    ierr = MatAssemblyEnd(mfqP->subH,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);

    ierr = TaoResetStatistics(mfqP->subtao);CHKERRQ(ierr);
    ierr = TaoSetTolerances(mfqP->subtao,*gnorm,*gnorm,PETSC_DEFAULT);CHKERRQ(ierr);
    /* enforce bound constraints -- experimental */
    if (tao->XU && tao->XL) {
      ierr = VecCopy(tao->XU,mfqP->subxu);CHKERRQ(ierr);
      ierr = VecAXPY(mfqP->subxu,-1.0,tao->solution);CHKERRQ(ierr);
      ierr = VecScale(mfqP->subxu,1.0/mfqP->delta);CHKERRQ(ierr);
      ierr = VecCopy(tao->XL,mfqP->subxl);CHKERRQ(ierr);
      ierr = VecAXPY(mfqP->subxl,-1.0,tao->solution);CHKERRQ(ierr);
      ierr = VecScale(mfqP->subxl,1.0/mfqP->delta);CHKERRQ(ierr);

      ierr = VecPointwiseMin(mfqP->subxu,mfqP->subxu,mfqP->subpdel);CHKERRQ(ierr);
      ierr = VecPointwiseMax(mfqP->subxl,mfqP->subxl,mfqP->subndel);CHKERRQ(ierr);
    } else {
      ierr = VecCopy(mfqP->subpdel,mfqP->subxu);CHKERRQ(ierr);
      ierr = VecCopy(mfqP->subndel,mfqP->subxl);CHKERRQ(ierr);
    }
    /* Make sure xu > xl */
    ierr = VecCopy(mfqP->subxl,mfqP->subpdel);CHKERRQ(ierr);
    ierr = VecAXPY(mfqP->subpdel,-1.0,mfqP->subxu);CHKERRQ(ierr);
    ierr = VecMax(mfqP->subpdel,NULL,&maxval);CHKERRQ(ierr);
    if (maxval > 1e-10) SETERRQ(PETSC_COMM_WORLD,1,"upper bound < lower bound in subproblem");
    /* Make sure xu > tao->solution > xl */
    ierr = VecCopy(mfqP->subxl,mfqP->subpdel);CHKERRQ(ierr);
    ierr = VecAXPY(mfqP->subpdel,-1.0,mfqP->subx);CHKERRQ(ierr);
    ierr = VecMax(mfqP->subpdel,NULL,&maxval);CHKERRQ(ierr);
    if (maxval > 1e-10) SETERRQ(PETSC_COMM_WORLD,1,"initial guess < lower bound in subproblem");

    ierr = VecCopy(mfqP->subx,mfqP->subpdel);CHKERRQ(ierr);
    ierr = VecAXPY(mfqP->subpdel,-1.0,mfqP->subxu);CHKERRQ(ierr);
    ierr = VecMax(mfqP->subpdel,NULL,&maxval);CHKERRQ(ierr);
    if (maxval > 1e-10) SETERRQ(PETSC_COMM_WORLD,1,"initial guess > upper bound in subproblem");

    ierr = TaoSolve(mfqP->subtao);CHKERRQ(ierr);
    ierr = TaoGetSolutionStatus(mfqP->subtao,NULL,qmin,NULL,NULL,NULL,NULL);CHKERRQ(ierr);

    /* test bounds post-solution*/
    ierr = VecCopy(mfqP->subxl,mfqP->subpdel);CHKERRQ(ierr);
    ierr = VecAXPY(mfqP->subpdel,-1.0,mfqP->subx);CHKERRQ(ierr);
    ierr = VecMax(mfqP->subpdel,NULL,&maxval);CHKERRQ(ierr);
    if (maxval > 1e-5) {
      ierr = PetscInfo(tao,"subproblem solution < lower bound\n");CHKERRQ(ierr);
      tao->reason = TAO_DIVERGED_TR_REDUCTION;
    }

    ierr = VecCopy(mfqP->subx,mfqP->subpdel);CHKERRQ(ierr);
    ierr = VecAXPY(mfqP->subpdel,-1.0,mfqP->subxu);CHKERRQ(ierr);
    ierr = VecMax(mfqP->subpdel,NULL,&maxval);CHKERRQ(ierr);
    if (maxval > 1e-5) {
      ierr = PetscInfo(tao,"subproblem solution > upper bound\n");CHKERRQ(ierr);
      tao->reason = TAO_DIVERGED_TR_REDUCTION;
    }
  } else {
    gqt(mfqP->n,mfqP->Hres,mfqP->n,mfqP->Gres,1.0,mfqP->gqt_rtol,atol,mfqP->gqt_maxits,gnorm,qmin,mfqP->Xsubproblem,&info,&its,mfqP->work,mfqP->work2, mfqP->work3);
  }
  *qmin *= -1;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "phi2eval"
PetscErrorCode phi2eval(PetscReal *x, PetscInt n, PetscReal *phi)
{
/* Phi = .5*[x(1)^2  sqrt(2)*x(1)*x(2) ... sqrt(2)*x(1)*x(n) ... x(2)^2 sqrt(2)*x(2)*x(3) .. x(n)^2] */
  PetscInt  i,j,k;
  PetscReal sqrt2 = PetscSqrtReal(2.0);

  PetscFunctionBegin;
  j=0;
  for (i=0;i<n;i++) {
    phi[j] = 0.5 * x[i]*x[i];
    j++;
    for (k=i+1;k<n;k++) {
      phi[j]  = x[i]*x[k]/sqrt2;
      j++;
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "getquadpounders"
PetscErrorCode getquadpounders(TAO_POUNDERS *mfqP)
{
/* Computes the parameters of the quadratic Q(x) = c + g'*x + 0.5*x*G*x'
   that satisfies the interpolation conditions Q(X[:,j]) = f(j)
   for j=1,...,m and with a Hessian matrix of least Frobenius norm */

    /* NB --we are ignoring c */
  PetscInt     i,j,k,num,np = mfqP->nmodelpoints;
  PetscReal    one = 1.0,zero=0.0,negone=-1.0;
  PetscBLASInt blasnpmax = mfqP->npmax;
  PetscBLASInt blasnplus1 = mfqP->n+1;
  PetscBLASInt blasnp = np;
  PetscBLASInt blasint = mfqP->n*(mfqP->n+1) / 2;
  PetscBLASInt blasint2 = np - mfqP->n-1;
  PetscBLASInt info,ione=1;
  PetscReal    sqrt2 = PetscSqrtReal(2.0);

  PetscFunctionBegin;
  for (i=0;i<mfqP->n*mfqP->m;i++) {
    mfqP->Gdel[i] = 0;
  }
  for (i=0;i<mfqP->n*mfqP->n*mfqP->m;i++) {
    mfqP->Hdel[i] = 0;
  }

    /* factor M */
  PetscStackCallBLAS("LAPACKgetrf",LAPACKgetrf_(&blasnplus1,&blasnp,mfqP->M,&blasnplus1,mfqP->npmaxiwork,&info));
  if (info != 0) SETERRQ1(PETSC_COMM_SELF,1,"LAPACK routine getrf returned with value %d\n",info);

  if (np == mfqP->n+1) {
    for (i=0;i<mfqP->npmax-mfqP->n-1;i++) {
      mfqP->omega[i]=0.0;
    }
    for (i=0;i<mfqP->n*(mfqP->n+1)/2;i++) {
      mfqP->beta[i]=0.0;
    }
  } else {
    /* Let Ltmp = (L'*L) */
    PetscStackCallBLAS("BLASgemm",BLASgemm_("T","N",&blasint2,&blasint2,&blasint,&one,&mfqP->L[(mfqP->n+1)*blasint],&blasint,&mfqP->L[(mfqP->n+1)*blasint],&blasint,&zero,mfqP->L_tmp,&blasint));

    /* factor Ltmp */
    PetscStackCallBLAS("LAPACKpotrf",LAPACKpotrf_("L",&blasint2,mfqP->L_tmp,&blasint,&info));
    if (info != 0) SETERRQ1(PETSC_COMM_SELF,1,"LAPACK routine potrf returned with value %d\n",info);
  }

  for (k=0;k<mfqP->m;k++) {
    if (np != mfqP->n+1) {
      /* Solve L'*L*Omega = Z' * RESk*/
      PetscStackCallBLAS("BLASgemv",BLASgemv_("T",&blasnp,&blasint2,&one,mfqP->Z,&blasnpmax,&mfqP->RES[mfqP->npmax*k],&ione,&zero,mfqP->omega,&ione));
      PetscStackCallBLAS("LAPACKpotrs",LAPACKpotrs_("L",&blasint2,&ione,mfqP->L_tmp,&blasint,mfqP->omega,&blasint2,&info));
      if (info != 0) SETERRQ1(PETSC_COMM_SELF,1,"LAPACK routine potrs returned with value %d\n",info);

      /* Beta = L*Omega */
      PetscStackCallBLAS("BLASgemv",BLASgemv_("N",&blasint,&blasint2,&one,&mfqP->L[(mfqP->n+1)*blasint],&blasint,mfqP->omega,&ione,&zero,mfqP->beta,&ione));
    }

    /* solve M'*Alpha = RESk - N'*Beta */
    PetscStackCallBLAS("BLASgemv",BLASgemv_("T",&blasint,&blasnp,&negone,mfqP->N,&blasint,mfqP->beta,&ione,&one,&mfqP->RES[mfqP->npmax*k],&ione));
    PetscStackCallBLAS("LAPACKgetrs",LAPACKgetrs_("T",&blasnplus1,&ione,mfqP->M,&blasnplus1,mfqP->npmaxiwork,&mfqP->RES[mfqP->npmax*k],&blasnplus1,&info));
    if (info != 0) SETERRQ1(PETSC_COMM_SELF,1,"LAPACK routine getrs returned with value %d\n",info);

    /* Gdel(:,k) = Alpha(2:n+1) */
    for (i=0;i<mfqP->n;i++) {
      mfqP->Gdel[i + mfqP->n*k] = mfqP->RES[mfqP->npmax*k + i+1];
    }

    /* Set Hdels */
    num=0;
    for (i=0;i<mfqP->n;i++) {
      /* H[i,i,k] = Beta(num) */
      mfqP->Hdel[(i*mfqP->n + i)*mfqP->m + k] = mfqP->beta[num];
      num++;
      for (j=i+1;j<mfqP->n;j++) {
        /* H[i,j,k] = H[j,i,k] = Beta(num)/sqrt(2) */
        mfqP->Hdel[(j*mfqP->n + i)*mfqP->m + k] = mfqP->beta[num]/sqrt2;
        mfqP->Hdel[(i*mfqP->n + j)*mfqP->m + k] = mfqP->beta[num]/sqrt2;
        num++;
      }
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "morepoints"
PetscErrorCode morepoints(TAO_POUNDERS *mfqP)
{
  /* Assumes mfqP->model_indices[0]  is minimum index
   Finishes adding points to mfqP->model_indices (up to npmax)
   Computes L,Z,M,N
   np is actual number of points in model (should equal npmax?) */
  PetscInt        point,i,j,offset;
  PetscInt        reject;
  PetscBLASInt    blasn=mfqP->n,blasnpmax=mfqP->npmax,blasnplus1=mfqP->n+1,info,blasnmax=mfqP->nmax,blasint,blasint2,blasnp,blasmaxmn;
  const PetscReal *x;
  PetscReal       normd;
  PetscErrorCode  ierr;

  PetscFunctionBegin;
  /* Initialize M,N */
  for (i=0;i<mfqP->n+1;i++) {
    ierr = VecGetArrayRead(mfqP->Xhist[mfqP->model_indices[i]],&x);CHKERRQ(ierr);
    mfqP->M[(mfqP->n+1)*i] = 1.0;
    for (j=0;j<mfqP->n;j++) {
      mfqP->M[j+1+((mfqP->n+1)*i)] = (x[j]  - mfqP->xmin[j]) / mfqP->delta;
    }
    ierr = VecRestoreArrayRead(mfqP->Xhist[mfqP->model_indices[i]],&x);CHKERRQ(ierr);
    ierr = phi2eval(&mfqP->M[1+((mfqP->n+1)*i)],mfqP->n,&mfqP->N[mfqP->n*(mfqP->n+1)/2 * i]);CHKERRQ(ierr);
  }

  /* Now we add points until we have npmax starting with the most recent ones */
  point = mfqP->nHist-1;
  mfqP->nmodelpoints = mfqP->n+1;
  while (mfqP->nmodelpoints < mfqP->npmax && point>=0) {
    /* Reject any points already in the model */
    reject = 0;
    for (j=0;j<mfqP->n+1;j++) {
      if (point == mfqP->model_indices[j]) {
        reject = 1;
        break;
      }
    }

    /* Reject if norm(d) >c2 */
    if (!reject) {
      ierr = VecCopy(mfqP->Xhist[point],mfqP->workxvec);CHKERRQ(ierr);
      ierr = VecAXPY(mfqP->workxvec,-1.0,mfqP->Xhist[mfqP->minindex]);CHKERRQ(ierr);
      ierr = VecNorm(mfqP->workxvec,NORM_2,&normd);CHKERRQ(ierr);
      normd /= mfqP->delta;
      if (normd > mfqP->c2) {
        reject =1;
      }
    }
    if (reject){
      point--;
      continue;
    }

    ierr = VecGetArrayRead(mfqP->Xhist[point],&x);CHKERRQ(ierr);
    mfqP->M[(mfqP->n+1)*mfqP->nmodelpoints] = 1.0;
    for (j=0;j<mfqP->n;j++) {
      mfqP->M[j+1+((mfqP->n+1)*mfqP->nmodelpoints)] = (x[j]  - mfqP->xmin[j]) / mfqP->delta;
    }
    ierr = VecRestoreArrayRead(mfqP->Xhist[point],&x);CHKERRQ(ierr);
    ierr = phi2eval(&mfqP->M[1+(mfqP->n+1)*mfqP->nmodelpoints],mfqP->n,&mfqP->N[mfqP->n*(mfqP->n+1)/2 * (mfqP->nmodelpoints)]);CHKERRQ(ierr);

    /* Update QR factorization */
    /* Copy M' to Q_tmp */
    for (i=0;i<mfqP->n+1;i++) {
      for (j=0;j<mfqP->npmax;j++) {
        mfqP->Q_tmp[j+mfqP->npmax*i] = mfqP->M[i+(mfqP->n+1)*j];
      }
    }
    blasnp = mfqP->nmodelpoints+1;
    /* Q_tmp,R = qr(M') */
    blasmaxmn=PetscMax(mfqP->m,mfqP->n+1);
    PetscStackCallBLAS("LAPACKgeqrf",LAPACKgeqrf_(&blasnp,&blasnplus1,mfqP->Q_tmp,&blasnpmax,mfqP->tau_tmp,mfqP->mwork,&blasmaxmn,&info));
    if (info != 0) SETERRQ1(PETSC_COMM_SELF,1,"LAPACK routine geqrf returned with value %d\n",info);

    /* Reject if min(svd(N*Q(:,n+2:np+1)) <= theta2 */
    /* L = N*Qtmp */
    blasint2 = mfqP->n * (mfqP->n+1) / 2;
    /* Copy N to L_tmp */
    for (i=0;i<mfqP->n*(mfqP->n+1)/2 * mfqP->npmax;i++) {
      mfqP->L_tmp[i]= mfqP->N[i];
    }
    /* Copy L_save to L_tmp */

    /* L_tmp = N*Qtmp' */
    PetscStackCallBLAS("LAPACKormqr",LAPACKormqr_("R","N",&blasint2,&blasnp,&blasnplus1,mfqP->Q_tmp,&blasnpmax,mfqP->tau_tmp,mfqP->L_tmp,&blasint2,mfqP->npmaxwork,&blasnmax,&info));
    if (info != 0) SETERRQ1(PETSC_COMM_SELF,1,"LAPACK routine ormqr returned with value %d\n",info);

    /* Copy L_tmp to L_save */
    for (i=0;i<mfqP->npmax * mfqP->n*(mfqP->n+1)/2;i++) {
      mfqP->L_save[i] = mfqP->L_tmp[i];
    }

    /* Get svd for L_tmp(:,n+2:np+1) (L_tmp is modified in process) */
    blasint = mfqP->nmodelpoints - mfqP->n;
    ierr = PetscFPTrapPush(PETSC_FP_TRAP_OFF);CHKERRQ(ierr);
    PetscStackCallBLAS("LAPACKgesvd",LAPACKgesvd_("N","N",&blasint2,&blasint,&mfqP->L_tmp[(mfqP->n+1)*blasint2],&blasint2,mfqP->beta,mfqP->work,&blasn,mfqP->work,&blasn,mfqP->npmaxwork,&blasnmax,&info));
    ierr = PetscFPTrapPop();CHKERRQ(ierr);
    if (info != 0) SETERRQ1(PETSC_COMM_SELF,1,"LAPACK routine gesvd returned with value %d\n",info);

    if (mfqP->beta[PetscMin(blasint,blasint2)-1] > mfqP->theta2) {
      /* accept point */
      mfqP->model_indices[mfqP->nmodelpoints] = point;
      /* Copy Q_tmp to Q */
      for (i=0;i<mfqP->npmax* mfqP->npmax;i++) {
        mfqP->Q[i] = mfqP->Q_tmp[i];
      }
      for (i=0;i<mfqP->npmax;i++){
        mfqP->tau[i] = mfqP->tau_tmp[i];
      }
      mfqP->nmodelpoints++;
      blasnp = mfqP->nmodelpoints;

      /* Copy L_save to L */
      for (i=0;i<mfqP->npmax * mfqP->n*(mfqP->n+1)/2;i++) {
        mfqP->L[i] = mfqP->L_save[i];
      }
    }
    point--;
  }

  blasnp = mfqP->nmodelpoints;
  /* Copy Q(:,n+2:np) to Z */
  /* First set Q_tmp to I */
  for (i=0;i<mfqP->npmax*mfqP->npmax;i++) {
    mfqP->Q_tmp[i] = 0.0;
  }
  for (i=0;i<mfqP->npmax;i++) {
    mfqP->Q_tmp[i + mfqP->npmax*i] = 1.0;
  }

  /* Q_tmp = I * Q */
  PetscStackCallBLAS("LAPACKormqr",LAPACKormqr_("R","N",&blasnp,&blasnp,&blasnplus1,mfqP->Q,&blasnpmax,mfqP->tau,mfqP->Q_tmp,&blasnpmax,mfqP->npmaxwork,&blasnmax,&info));
  if (info != 0) SETERRQ1(PETSC_COMM_SELF,1,"LAPACK routine ormqr returned with value %d\n",info);

  /* Copy Q_tmp(:,n+2:np) to Z) */
  offset = mfqP->npmax * (mfqP->n+1);
  for (i=offset;i<mfqP->npmax*mfqP->npmax;i++) {
    mfqP->Z[i-offset] = mfqP->Q_tmp[i];
  }

  if (mfqP->nmodelpoints == mfqP->n + 1) {
    /* Set L to I_{n+1} */
    for (i=0;i<mfqP->npmax * mfqP->n*(mfqP->n+1)/2;i++) {
      mfqP->L[i] = 0.0;
    }
    for (i=0;i<mfqP->n;i++) {
      mfqP->L[(mfqP->n*(mfqP->n+1)/2)*i + i] = 1.0;
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "addpoint"
/* Only call from modelimprove, addpoint() needs ->Q_tmp and ->work to be set */
PetscErrorCode addpoint(Tao tao, TAO_POUNDERS *mfqP, PetscInt index)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  /* Create new vector in history: X[newidx] = X[mfqP->index] + delta*X[index]*/
  ierr = VecDuplicate(mfqP->Xhist[0],&mfqP->Xhist[mfqP->nHist]);CHKERRQ(ierr);
  ierr = VecSetValues(mfqP->Xhist[mfqP->nHist],mfqP->n,mfqP->indices,&mfqP->Q_tmp[index*mfqP->npmax],INSERT_VALUES);CHKERRQ(ierr);
  ierr = VecAssemblyBegin(mfqP->Xhist[mfqP->nHist]);CHKERRQ(ierr);
  ierr = VecAssemblyEnd(mfqP->Xhist[mfqP->nHist]);CHKERRQ(ierr);
  ierr = VecAYPX(mfqP->Xhist[mfqP->nHist],mfqP->delta,mfqP->Xhist[mfqP->minindex]);CHKERRQ(ierr);

  /* Project into feasible region */
  if (tao->XU && tao->XL) {
    ierr = VecMedian(mfqP->Xhist[mfqP->nHist], tao->XL, tao->XU, mfqP->Xhist[mfqP->nHist]);CHKERRQ(ierr);
  }

  /* Compute value of new vector */
  ierr = VecDuplicate(mfqP->Fhist[0],&mfqP->Fhist[mfqP->nHist]);CHKERRQ(ierr);
  CHKMEMQ;
  ierr = TaoComputeSeparableObjective(tao,mfqP->Xhist[mfqP->nHist],mfqP->Fhist[mfqP->nHist]);CHKERRQ(ierr);
  ierr = VecNorm(mfqP->Fhist[mfqP->nHist],NORM_2,&mfqP->Fres[mfqP->nHist]);CHKERRQ(ierr);
  if (PetscIsInfOrNanReal(mfqP->Fres[mfqP->nHist])) SETERRQ(PETSC_COMM_SELF,1, "User provided compute function generated Inf or NaN");
  mfqP->Fres[mfqP->nHist]*=mfqP->Fres[mfqP->nHist];

  /* Add new vector to model */
  mfqP->model_indices[mfqP->nmodelpoints] = mfqP->nHist;
  mfqP->nmodelpoints++;
  mfqP->nHist++;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "modelimprove"
PetscErrorCode modelimprove(Tao tao, TAO_POUNDERS *mfqP, PetscInt addallpoints)
{
  /* modeld = Q(:,np+1:n)' */
  PetscErrorCode ierr;
  PetscInt       i,j,minindex=0;
  PetscReal      dp,half=0.5,one=1.0,minvalue=PETSC_INFINITY;
  PetscBLASInt   blasn=mfqP->n,  blasnpmax = mfqP->npmax, blask,info;
  PetscBLASInt   blas1=1,blasnmax = mfqP->nmax;

  blask = mfqP->nmodelpoints;
  /* Qtmp = I(n x n) */
  for (i=0;i<mfqP->n;i++) {
    for (j=0;j<mfqP->n;j++) {
      mfqP->Q_tmp[i + mfqP->npmax*j] = 0.0;
    }
  }
  for (j=0;j<mfqP->n;j++) {
    mfqP->Q_tmp[j + mfqP->npmax*j] = 1.0;
  }

  /* Qtmp = Q * I */
  PetscStackCallBLAS("LAPACKormqr",LAPACKormqr_("R","N",&blasn,&blasn,&blask,mfqP->Q,&blasnpmax,mfqP->tau, mfqP->Q_tmp, &blasnpmax, mfqP->npmaxwork,&blasnmax, &info));

  for (i=mfqP->nmodelpoints;i<mfqP->n;i++) {
    dp = BLASdot_(&blasn,&mfqP->Q_tmp[i*mfqP->npmax],&blas1,mfqP->Gres,&blas1);
    if (dp>0.0) { /* Model says use the other direction! */
      for (j=0;j<mfqP->n;j++) {
        mfqP->Q_tmp[i*mfqP->npmax+j] *= -1;
      }
    }
    /* mfqP->work[i] = Cres+Modeld(i,:)*(Gres+.5*Hres*Modeld(i,:)') */
    for (j=0;j<mfqP->n;j++) {
      mfqP->work2[j] = mfqP->Gres[j];
    }
    PetscStackCallBLAS("BLASgemv",BLASgemv_("N",&blasn,&blasn,&half,mfqP->Hres,&blasn,&mfqP->Q_tmp[i*mfqP->npmax], &blas1, &one, mfqP->work2,&blas1));
    mfqP->work[i] = BLASdot_(&blasn,&mfqP->Q_tmp[i*mfqP->npmax],&blas1,mfqP->work2,&blas1);
    if (i==mfqP->nmodelpoints || mfqP->work[i] < minvalue) {
      minindex=i;
      minvalue = mfqP->work[i];
    }
    if (addallpoints != 0) {
      ierr = addpoint(tao,mfqP,i);CHKERRQ(ierr);
    }
  }
  if (!addallpoints) {
    ierr = addpoint(tao,mfqP,minindex);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "affpoints"
PetscErrorCode affpoints(TAO_POUNDERS *mfqP, PetscReal *xmin,PetscReal c)
{
  PetscInt        i,j;
  PetscBLASInt    blasm=mfqP->m,blasj,blask,blasn=mfqP->n,ione=1,info;
  PetscBLASInt    blasnpmax = mfqP->npmax,blasmaxmn;
  PetscReal       proj,normd;
  const PetscReal *x;
  PetscErrorCode  ierr;

  PetscFunctionBegin;
  for (i=mfqP->nHist-1;i>=0;i--) {
    ierr = VecGetArrayRead(mfqP->Xhist[i],&x);CHKERRQ(ierr);
    for (j=0;j<mfqP->n;j++) {
      mfqP->work[j] = (x[j] - xmin[j])/mfqP->delta;
    }
    ierr = VecRestoreArrayRead(mfqP->Xhist[i],&x);CHKERRQ(ierr);
    PetscStackCallBLAS("BLAScopy",BLAScopy_(&blasn,mfqP->work,&ione,mfqP->work2,&ione));
    normd = BLASnrm2_(&blasn,mfqP->work,&ione);
    if (normd <= c*c) {
      blasj=PetscMax((mfqP->n - mfqP->nmodelpoints),0);
      if (!mfqP->q_is_I) {
        /* project D onto null */
        blask=(mfqP->nmodelpoints);
        PetscStackCallBLAS("LAPACKormqr",LAPACKormqr_("R","N",&ione,&blasn,&blask,mfqP->Q,&blasnpmax,mfqP->tau,mfqP->work2,&ione,mfqP->mwork,&blasm,&info));
        if (info < 0) SETERRQ1(PETSC_COMM_SELF,1,"ormqr returned value %d\n",info);
      }
      proj = BLASnrm2_(&blasj,&mfqP->work2[mfqP->nmodelpoints],&ione);

      if (proj >= mfqP->theta1) { /* add this index to model */
        mfqP->model_indices[mfqP->nmodelpoints]=i;
        mfqP->nmodelpoints++;
        PetscStackCallBLAS("BLAScopy",BLAScopy_(&blasn,mfqP->work,&ione,&mfqP->Q_tmp[mfqP->npmax*(mfqP->nmodelpoints-1)],&ione));
        blask=mfqP->npmax*(mfqP->nmodelpoints);
        PetscStackCallBLAS("BLAScopy",BLAScopy_(&blask,mfqP->Q_tmp,&ione,mfqP->Q,&ione));
        blask = mfqP->nmodelpoints;
        blasmaxmn = PetscMax(mfqP->m,mfqP->n);
        PetscStackCallBLAS("LAPACKgeqrf",LAPACKgeqrf_(&blasn,&blask,mfqP->Q,&blasnpmax,mfqP->tau,mfqP->mwork,&blasmaxmn,&info));
        if (info < 0) SETERRQ1(PETSC_COMM_SELF,1,"geqrf returned value %d\n",info);
        mfqP->q_is_I = 0;
      }
      if (mfqP->nmodelpoints == mfqP->n)  {
        break;
      }
    }
  }

  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoSolve_POUNDERS"
static PetscErrorCode TaoSolve_POUNDERS(Tao tao)
{
  TAO_POUNDERS       *mfqP = (TAO_POUNDERS *)tao->data;
  PetscInt           i,ii,j,k,l;
  PetscReal          step=1.0;
  TaoConvergedReason reason = TAO_CONTINUE_ITERATING;
  PetscInt           low,high;
  PetscReal          minnorm;
  PetscReal          *x,*f;
  const PetscReal    *xmint,*fmin;
  PetscReal          cres,deltaold;
  PetscReal          gnorm;
  PetscBLASInt       info,ione=1,iblas;
  PetscBool          valid,same;
  PetscReal          mdec, rho, normxsp;
  PetscReal          one=1.0,zero=0.0,ratio;
  PetscBLASInt       blasm,blasn,blasnpmax,blasn2;
  PetscErrorCode     ierr;
  static PetscBool   set = PETSC_FALSE;

  /* n = # of parameters
     m = dimension (components) of function  */
  PetscFunctionBegin;
  ierr = PetscCitationsRegister("@article{UNEDF0,\n"
                                "title = {Nuclear energy density optimization},\n"
                                "author = {Kortelainen, M.  and Lesinski, T.  and Mor\'e, J.  and Nazarewicz, W.\n"
                                "          and Sarich, J.  and Schunck, N.  and Stoitsov, M. V. and Wild, S. },\n"
                                "journal = {Phys. Rev. C},\n"
                                "volume = {82},\n"
                                "number = {2},\n"
                                "pages = {024313},\n"
                                "numpages = {18},\n"
                                "year = {2010},\n"
                                "month = {Aug},\n"
                                "doi = {10.1103/PhysRevC.82.024313}\n}\n",&set);CHKERRQ(ierr);
  tao->niter=0;
  if (tao->XL && tao->XU) {
    /* Check x0 <= XU */
    PetscReal val;
    ierr = VecCopy(tao->solution,mfqP->Xhist[0]);CHKERRQ(ierr);
    ierr = VecAXPY(mfqP->Xhist[0],-1.0,tao->XU);CHKERRQ(ierr);
    ierr = VecMax(mfqP->Xhist[0],NULL,&val);CHKERRQ(ierr);
    if (val > 1e-10) SETERRQ(PETSC_COMM_WORLD,1,"X0 > upper bound");

    /* Check x0 >= xl */
    ierr = VecCopy(tao->XL,mfqP->Xhist[0]);CHKERRQ(ierr);
    ierr = VecAXPY(mfqP->Xhist[0],-1.0,tao->solution);CHKERRQ(ierr);
    ierr = VecMax(mfqP->Xhist[0],NULL,&val);CHKERRQ(ierr);
    if (val > 1e-10) SETERRQ(PETSC_COMM_WORLD,1,"X0 < lower bound");

    /* Check x0 + delta < XU  -- should be able to get around this eventually */

    ierr = VecSet(mfqP->Xhist[0],mfqP->delta);CHKERRQ(ierr);
    ierr = VecAXPY(mfqP->Xhist[0],1.0,tao->solution);CHKERRQ(ierr);
    ierr = VecAXPY(mfqP->Xhist[0],-1.0,tao->XU);CHKERRQ(ierr);
    ierr = VecMax(mfqP->Xhist[0],NULL,&val);CHKERRQ(ierr);
    if (val > 1e-10) SETERRQ(PETSC_COMM_WORLD,1,"X0 + delta > upper bound");
  }

  CHKMEMQ;
  blasm = mfqP->m; blasn=mfqP->n; blasnpmax = mfqP->npmax;
  for (i=0;i<mfqP->n*mfqP->n*mfqP->m;i++) mfqP->H[i]=0;

  ierr = VecCopy(tao->solution,mfqP->Xhist[0]);CHKERRQ(ierr);
  CHKMEMQ;
  ierr = TaoComputeSeparableObjective(tao,tao->solution,mfqP->Fhist[0]);CHKERRQ(ierr);

  ierr = VecNorm(mfqP->Fhist[0],NORM_2,&mfqP->Fres[0]);CHKERRQ(ierr);
  if (PetscIsInfOrNanReal(mfqP->Fres[0])) SETERRQ(PETSC_COMM_SELF,1, "User provided compute function generated Inf or NaN");
  mfqP->Fres[0]*=mfqP->Fres[0];
  mfqP->minindex = 0;
  minnorm = mfqP->Fres[0];
  ierr = TaoMonitor(tao, tao->niter, minnorm, PETSC_INFINITY, 0.0, step, &reason);CHKERRQ(ierr);
  tao->niter++;

  ierr = VecGetOwnershipRange(mfqP->Xhist[0],&low,&high);CHKERRQ(ierr);
  for (i=1;i<mfqP->n+1;i++) {
    ierr = VecCopy(tao->solution,mfqP->Xhist[i]);CHKERRQ(ierr);
    if (i-1 >= low && i-1 < high) {
      ierr = VecGetArray(mfqP->Xhist[i],&x);CHKERRQ(ierr);
      x[i-1-low] += mfqP->delta;
      ierr = VecRestoreArray(mfqP->Xhist[i],&x);CHKERRQ(ierr);
    }
    CHKMEMQ;
    ierr = TaoComputeSeparableObjective(tao,mfqP->Xhist[i],mfqP->Fhist[i]);CHKERRQ(ierr);
    ierr = VecNorm(mfqP->Fhist[i],NORM_2,&mfqP->Fres[i]);CHKERRQ(ierr);
    if (PetscIsInfOrNanReal(mfqP->Fres[i])) SETERRQ(PETSC_COMM_SELF,1, "User provided compute function generated Inf or NaN");
    mfqP->Fres[i]*=mfqP->Fres[i];
    if (mfqP->Fres[i] < minnorm) {
      mfqP->minindex = i;
      minnorm = mfqP->Fres[i];
    }
  }
  ierr = VecCopy(mfqP->Xhist[mfqP->minindex],tao->solution);CHKERRQ(ierr);
  ierr = VecCopy(mfqP->Fhist[mfqP->minindex],tao->sep_objective);CHKERRQ(ierr);
  /* Gather mpi vecs to one big local vec */

  /* Begin serial code */

  /* Disp[i] = Xi-xmin, i=1,..,mfqP->minindex-1,mfqP->minindex+1,..,n */
  /* Fdiff[i] = (Fi-Fmin)', i=1,..,mfqP->minindex-1,mfqP->minindex+1,..,n */
  /* (Column oriented for blas calls) */
  ii=0;

  if (mfqP->size == 1) {
    ierr = VecGetArrayRead(mfqP->Xhist[mfqP->minindex],&xmint);CHKERRQ(ierr);
    for (i=0;i<mfqP->n;i++) mfqP->xmin[i] = xmint[i];
    ierr = VecRestoreArrayRead(mfqP->Xhist[mfqP->minindex],&xmint);CHKERRQ(ierr);
    ierr = VecGetArrayRead(mfqP->Fhist[mfqP->minindex],&fmin);CHKERRQ(ierr);
    for (i=0;i<mfqP->n+1;i++) {
      if (i == mfqP->minindex) continue;

      ierr = VecGetArray(mfqP->Xhist[i],&x);CHKERRQ(ierr);
      for (j=0;j<mfqP->n;j++) {
        mfqP->Disp[ii+mfqP->npmax*j] = (x[j] - mfqP->xmin[j])/mfqP->delta;
      }
      ierr = VecRestoreArray(mfqP->Xhist[i],&x);CHKERRQ(ierr);

      ierr = VecGetArray(mfqP->Fhist[i],&f);CHKERRQ(ierr);
      for (j=0;j<mfqP->m;j++) {
        mfqP->Fdiff[ii+mfqP->n*j] = f[j] - fmin[j];
      }
      ierr = VecRestoreArray(mfqP->Fhist[i],&f);CHKERRQ(ierr);
      mfqP->model_indices[ii++] = i;

    }
    for (j=0;j<mfqP->m;j++) {
      mfqP->C[j] = fmin[j];
    }
    ierr = VecRestoreArrayRead(mfqP->Fhist[mfqP->minindex],&fmin);CHKERRQ(ierr);
  } else {
    ierr = VecScatterBegin(mfqP->scatterx,mfqP->Xhist[mfqP->minindex],mfqP->localxmin,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
    ierr = VecScatterEnd(mfqP->scatterx,mfqP->Xhist[mfqP->minindex],mfqP->localxmin,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
    ierr = VecGetArrayRead(mfqP->localxmin,&xmint);CHKERRQ(ierr);
    for (i=0;i<mfqP->n;i++) mfqP->xmin[i] = xmint[i];
    ierr = VecRestoreArrayRead(mfqP->localxmin,&xmint);CHKERRQ(ierr);

    ierr = VecScatterBegin(mfqP->scatterf,mfqP->Fhist[mfqP->minindex],mfqP->localfmin,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
    ierr = VecScatterEnd(mfqP->scatterf,mfqP->Fhist[mfqP->minindex],mfqP->localfmin,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
    ierr = VecGetArrayRead(mfqP->localfmin,&fmin);CHKERRQ(ierr);
    for (i=0;i<mfqP->n+1;i++) {
      if (i == mfqP->minindex) continue;

      mfqP->model_indices[ii++] = i;
      ierr = VecScatterBegin(mfqP->scatterx,mfqP->Xhist[ii],mfqP->localx,INSERT_VALUES, SCATTER_FORWARD);CHKERRQ(ierr);
      ierr = VecScatterEnd(mfqP->scatterx,mfqP->Xhist[ii],mfqP->localx,INSERT_VALUES, SCATTER_FORWARD);CHKERRQ(ierr);
      ierr = VecGetArray(mfqP->localx,&x);CHKERRQ(ierr);
      for (j=0;j<mfqP->n;j++) {
        mfqP->Disp[i+mfqP->npmax*j] = (x[j] - mfqP->xmin[j])/mfqP->delta;
      }
      ierr = VecRestoreArray(mfqP->localx,&x);CHKERRQ(ierr);

      ierr = VecScatterBegin(mfqP->scatterf,mfqP->Fhist[ii],mfqP->localf,INSERT_VALUES, SCATTER_FORWARD);CHKERRQ(ierr);
      ierr = VecScatterEnd(mfqP->scatterf,mfqP->Fhist[ii],mfqP->localf,INSERT_VALUES, SCATTER_FORWARD);CHKERRQ(ierr);
      ierr = VecGetArray(mfqP->localf,&f);CHKERRQ(ierr);
      for (j=0;j<mfqP->m;j++) {
        mfqP->Fdiff[i*mfqP->n+j] = f[j] - fmin[j];
      }
      ierr = VecRestoreArray(mfqP->localf,&f);CHKERRQ(ierr);
    }
    for (j=0;j<mfqP->m;j++) {
      mfqP->C[j] = fmin[j];
    }
    ierr = VecRestoreArrayRead(mfqP->localfmin,&fmin);CHKERRQ(ierr);
  }

  /* Determine the initial quadratic models */
  /* G = D(ModelIn,:) \ (F(ModelIn,1:m)-repmat(F(xkin,1:m),n,1)); */
  /* D (nxn) Fdiff (nxm)  => G (nxm) */
  blasn2 = blasn;
  PetscStackCallBLAS("LAPACKgesv",LAPACKgesv_(&blasn,&blasm,mfqP->Disp,&blasnpmax,mfqP->iwork,mfqP->Fdiff,&blasn2,&info));
  ierr = PetscInfo1(tao,"gesv returned %d\n",info);CHKERRQ(ierr);

  cres = minnorm;
  /* Gres = G*F(xkin,1:m)'  G (nxm)   Fk (m)   */
  PetscStackCallBLAS("BLASgemv",BLASgemv_("N",&blasn,&blasm,&one,mfqP->Fdiff,&blasn,mfqP->C,&ione,&zero,mfqP->Gres,&ione));

  /*  Hres = G*G'  */
  PetscStackCallBLAS("BLASgemm",BLASgemm_("N","T",&blasn,&blasn,&blasm,&one,mfqP->Fdiff, &blasn,mfqP->Fdiff,&blasn,&zero,mfqP->Hres,&blasn));

  valid = PETSC_TRUE;

  ierr = VecSetValues(tao->gradient,mfqP->n,mfqP->indices,mfqP->Gres,INSERT_VALUES);CHKERRQ(ierr);
  ierr = VecAssemblyBegin(tao->gradient);CHKERRQ(ierr);
  ierr = VecAssemblyEnd(tao->gradient);CHKERRQ(ierr);
  ierr = VecNorm(tao->gradient,NORM_2,&gnorm);CHKERRQ(ierr);
  gnorm *= mfqP->delta;
  ierr = VecCopy(mfqP->Xhist[mfqP->minindex],tao->solution);CHKERRQ(ierr);
  ierr = TaoMonitor(tao, tao->niter, minnorm, gnorm, 0.0, step, &reason);CHKERRQ(ierr);
  mfqP->nHist = mfqP->n+1;
  mfqP->nmodelpoints = mfqP->n+1;

  while (reason == TAO_CONTINUE_ITERATING) {
    PetscReal gnm = 1e-4;
    tao->niter++;
    /* Solve the subproblem min{Q(s): ||s|| <= delta} */
    ierr = gqtwrap(tao,&gnm,&mdec);CHKERRQ(ierr);
    /* Evaluate the function at the new point */

    for (i=0;i<mfqP->n;i++) {
        mfqP->work[i] = mfqP->Xsubproblem[i]*mfqP->delta + mfqP->xmin[i];
    }
    ierr = VecDuplicate(tao->solution,&mfqP->Xhist[mfqP->nHist]);CHKERRQ(ierr);
    ierr = VecDuplicate(tao->sep_objective,&mfqP->Fhist[mfqP->nHist]);CHKERRQ(ierr);
    ierr = VecSetValues(mfqP->Xhist[mfqP->nHist],mfqP->n,mfqP->indices,mfqP->work,INSERT_VALUES);CHKERRQ(ierr);
    ierr = VecAssemblyBegin(mfqP->Xhist[mfqP->nHist]);CHKERRQ(ierr);
    ierr = VecAssemblyEnd(mfqP->Xhist[mfqP->nHist]);CHKERRQ(ierr);

    ierr = TaoComputeSeparableObjective(tao,mfqP->Xhist[mfqP->nHist],mfqP->Fhist[mfqP->nHist]);CHKERRQ(ierr);
    ierr = VecNorm(mfqP->Fhist[mfqP->nHist],NORM_2,&mfqP->Fres[mfqP->nHist]);CHKERRQ(ierr);
    if (PetscIsInfOrNanReal(mfqP->Fres[mfqP->nHist])) SETERRQ(PETSC_COMM_SELF,1, "User provided compute function generated Inf or NaN");
    mfqP->Fres[mfqP->nHist]*=mfqP->Fres[mfqP->nHist];
    rho = (mfqP->Fres[mfqP->minindex] - mfqP->Fres[mfqP->nHist]) / mdec;
    mfqP->nHist++;

    /* Update the center */
    if ((rho >= mfqP->eta1) || (rho > mfqP->eta0 && valid==PETSC_TRUE)) {
      /* Update model to reflect new base point */
      for (i=0;i<mfqP->n;i++) {
        mfqP->work[i] = (mfqP->work[i] - mfqP->xmin[i])/mfqP->delta;
      }
      for (j=0;j<mfqP->m;j++) {
        /* C(j) = C(j) + work*G(:,j) + .5*work*H(:,:,j)*work';
         G(:,j) = G(:,j) + H(:,:,j)*work' */
        for (k=0;k<mfqP->n;k++) {
          mfqP->work2[k]=0.0;
          for (l=0;l<mfqP->n;l++) {
            mfqP->work2[k]+=mfqP->H[j + mfqP->m*(k + l*mfqP->n)]*mfqP->work[l];
          }
        }
        for (i=0;i<mfqP->n;i++) {
          mfqP->C[j]+=mfqP->work[i]*(mfqP->Fdiff[i + mfqP->n* j] + 0.5*mfqP->work2[i]);
          mfqP->Fdiff[i+mfqP->n*j] +=mfqP-> work2[i];
        }
      }
      /* Cres += work*Gres + .5*work*Hres*work';
       Gres += Hres*work'; */

      PetscStackCallBLAS("BLASgemv",BLASgemv_("N",&blasn,&blasn,&one,mfqP->Hres,&blasn,mfqP->work,&ione,&zero,mfqP->work2,&ione));
      for (i=0;i<mfqP->n;i++) {
        cres += mfqP->work[i]*(mfqP->Gres[i]  + 0.5*mfqP->work2[i]);
        mfqP->Gres[i] += mfqP->work2[i];
      }
      mfqP->minindex = mfqP->nHist-1;
      minnorm = mfqP->Fres[mfqP->minindex];
      ierr = VecCopy(mfqP->Fhist[mfqP->minindex],tao->sep_objective);CHKERRQ(ierr);
      /* Change current center */
      ierr = VecGetArrayRead(mfqP->Xhist[mfqP->minindex],&xmint);CHKERRQ(ierr);
      for (i=0;i<mfqP->n;i++) {
        mfqP->xmin[i] = xmint[i];
      }
      ierr = VecRestoreArrayRead(mfqP->Xhist[mfqP->minindex],&xmint);CHKERRQ(ierr);
    }

    /* Evaluate at a model-improving point if necessary */
    if (valid == PETSC_FALSE) {
      mfqP->q_is_I = 1;
      mfqP->nmodelpoints = 0;
      ierr = affpoints(mfqP,mfqP->xmin,mfqP->c1);CHKERRQ(ierr);
      if (mfqP->nmodelpoints < mfqP->n) {
        ierr = PetscInfo(tao,"Model not valid -- model-improving\n");CHKERRQ(ierr);
        ierr = modelimprove(tao,mfqP,1);CHKERRQ(ierr);
      }
    }

    /* Update the trust region radius */
    deltaold = mfqP->delta;
    normxsp = 0;
    for (i=0;i<mfqP->n;i++) {
      normxsp += mfqP->Xsubproblem[i]*mfqP->Xsubproblem[i];
    }
    normxsp = PetscSqrtReal(normxsp);
    if (rho >= mfqP->eta1 && normxsp > 0.5*mfqP->delta) {
      mfqP->delta = PetscMin(mfqP->delta*mfqP->gamma1,mfqP->deltamax);
    } else if (valid == PETSC_TRUE) {
      mfqP->delta = PetscMax(mfqP->delta*mfqP->gamma0,mfqP->deltamin);
    }

    /* Compute the next interpolation set */
    mfqP->q_is_I = 1;
    mfqP->nmodelpoints=0;
    ierr = affpoints(mfqP,mfqP->xmin,mfqP->c1);CHKERRQ(ierr);
    if (mfqP->nmodelpoints == mfqP->n) {
      valid = PETSC_TRUE;
    } else {
      valid = PETSC_FALSE;
      ierr = affpoints(mfqP,mfqP->xmin,mfqP->c2);CHKERRQ(ierr);
      if (mfqP->n > mfqP->nmodelpoints) {
        ierr = PetscInfo(tao,"Model not valid -- adding geometry points\n");CHKERRQ(ierr);
        ierr = modelimprove(tao,mfqP,mfqP->n - mfqP->nmodelpoints);CHKERRQ(ierr);
      }
    }
    for (i=mfqP->nmodelpoints;i>0;i--) {
      mfqP->model_indices[i] = mfqP->model_indices[i-1];
    }
    mfqP->nmodelpoints++;
    mfqP->model_indices[0] = mfqP->minindex;
    ierr = morepoints(mfqP);CHKERRQ(ierr);
    for (i=0;i<mfqP->nmodelpoints;i++) {
      ierr = VecGetArray(mfqP->Xhist[mfqP->model_indices[i]],&x);CHKERRQ(ierr);
      for (j=0;j<mfqP->n;j++) {
        mfqP->Disp[i + mfqP->npmax*j] = (x[j]  - mfqP->xmin[j]) / deltaold;
      }
      ierr = VecRestoreArray(mfqP->Xhist[mfqP->model_indices[i]],&x);CHKERRQ(ierr);
      ierr = VecGetArray(mfqP->Fhist[mfqP->model_indices[i]],&f);CHKERRQ(ierr);
      for (j=0;j<mfqP->m;j++) {
        for (k=0;k<mfqP->n;k++)  {
          mfqP->work[k]=0.0;
          for (l=0;l<mfqP->n;l++) {
            mfqP->work[k] += mfqP->H[j + mfqP->m*(k + mfqP->n*l)] * mfqP->Disp[i + mfqP->npmax*l];
          }
        }
        mfqP->RES[j*mfqP->npmax + i] = -mfqP->C[j] - BLASdot_(&blasn,&mfqP->Fdiff[j*mfqP->n],&ione,&mfqP->Disp[i],&blasnpmax) - 0.5*BLASdot_(&blasn,mfqP->work,&ione,&mfqP->Disp[i],&blasnpmax) + f[j];
      }
      ierr = VecRestoreArray(mfqP->Fhist[mfqP->model_indices[i]],&f);CHKERRQ(ierr);
    }

    /* Update the quadratic model */
    ierr = getquadpounders(mfqP);CHKERRQ(ierr);
    ierr = VecGetArrayRead(mfqP->Fhist[mfqP->minindex],&fmin);CHKERRQ(ierr);
    PetscStackCallBLAS("BLAScopy",BLAScopy_(&blasm,fmin,&ione,mfqP->C,&ione));
    /* G = G*(delta/deltaold) + Gdel */
    ratio = mfqP->delta/deltaold;
    iblas = blasm*blasn;
    PetscStackCallBLAS("BLASscal",BLASscal_(&iblas,&ratio,mfqP->Fdiff,&ione));
    PetscStackCallBLAS("BLASaxpy",BLASaxpy_(&iblas,&one,mfqP->Gdel,&ione,mfqP->Fdiff,&ione));
    /* H = H*(delta/deltaold) + Hdel */
    iblas = blasm*blasn*blasn;
    ratio *= ratio;
    PetscStackCallBLAS("BLASscal",BLASscal_(&iblas,&ratio,mfqP->H,&ione));
    PetscStackCallBLAS("BLASaxpy",BLASaxpy_(&iblas,&one,mfqP->Hdel,&ione,mfqP->H,&ione));

    /* Get residuals */
    cres = mfqP->Fres[mfqP->minindex];
    /* Gres = G*F(xkin,1:m)' */
    PetscStackCallBLAS("BLASgemv",BLASgemv_("N",&blasn,&blasm,&one,mfqP->Fdiff,&blasn,mfqP->C,&ione,&zero,mfqP->Gres,&ione));
    /* Hres = sum i=1..m {F(xkin,i)*H(:,:,i)}   + G*G' */
    PetscStackCallBLAS("BLASgemm",BLASgemm_("N","T",&blasn,&blasn,&blasm,&one,mfqP->Fdiff,&blasn,mfqP->Fdiff,&blasn,&zero,mfqP->Hres,&blasn));

    iblas = mfqP->n*mfqP->n;

    for (j=0;j<mfqP->m;j++) {
      PetscStackCallBLAS("BLASaxpy",BLASaxpy_(&iblas,&fmin[j],&mfqP->H[j],&blasm,mfqP->Hres,&ione));
    }

    /* Export solution and gradient residual to TAO */
    ierr = VecCopy(mfqP->Xhist[mfqP->minindex],tao->solution);CHKERRQ(ierr);
    ierr = VecSetValues(tao->gradient,mfqP->n,mfqP->indices,mfqP->Gres,INSERT_VALUES);CHKERRQ(ierr);
    ierr = VecAssemblyBegin(tao->gradient);CHKERRQ(ierr);
    ierr = VecAssemblyEnd(tao->gradient);CHKERRQ(ierr);
    ierr = VecNorm(tao->gradient,NORM_2,&gnorm);CHKERRQ(ierr);
    gnorm *= mfqP->delta;
    /*  final criticality test */
    ierr = TaoMonitor(tao, tao->niter, minnorm, gnorm, 0.0, step, &reason);CHKERRQ(ierr);
    /* test for repeated model */
    if (mfqP->nmodelpoints==mfqP->last_nmodelpoints) {
      same = PETSC_TRUE;
    } else {
      same = PETSC_FALSE;
    }
    for (i=0;i<mfqP->nmodelpoints;i++) {
      if (same) {
        if (mfqP->model_indices[i] == mfqP->last_model_indices[i]) {
          same = PETSC_TRUE;
        } else {
          same = PETSC_FALSE;
        }
      }
      mfqP->last_model_indices[i] = mfqP->model_indices[i];
    }
    mfqP->last_nmodelpoints = mfqP->nmodelpoints;
    if (same && mfqP->delta == deltaold) {
      ierr = PetscInfo(tao,"Identical model used in successive iterations\n");CHKERRQ(ierr);
      reason = TAO_CONVERGED_STEPTOL;
      tao->reason = TAO_CONVERGED_STEPTOL;
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoSetUp_POUNDERS"
static PetscErrorCode TaoSetUp_POUNDERS(Tao tao)
{
  TAO_POUNDERS   *mfqP = (TAO_POUNDERS*)tao->data;
  PetscInt       i;
  IS             isfloc,isfglob,isxloc,isxglob;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (!tao->gradient) {ierr = VecDuplicate(tao->solution,&tao->gradient);CHKERRQ(ierr);  }
  if (!tao->stepdirection) {ierr = VecDuplicate(tao->solution,&tao->stepdirection);CHKERRQ(ierr);  }
  ierr = VecGetSize(tao->solution,&mfqP->n);CHKERRQ(ierr);
  ierr = VecGetSize(tao->sep_objective,&mfqP->m);CHKERRQ(ierr);
  mfqP->c1 = PetscSqrtReal((PetscReal)mfqP->n);
  if (mfqP->npmax == PETSC_DEFAULT) {
    mfqP->npmax = 2*mfqP->n + 1;
  }
  mfqP->npmax = PetscMin((mfqP->n+1)*(mfqP->n+2)/2,mfqP->npmax);
  mfqP->npmax = PetscMax(mfqP->npmax, mfqP->n+2);

  ierr = PetscMalloc1(tao->max_funcs+10,&mfqP->Xhist);CHKERRQ(ierr);
  ierr = PetscMalloc1(tao->max_funcs+10,&mfqP->Fhist);CHKERRQ(ierr);
  for (i=0;i<mfqP->n +1;i++) {
    ierr = VecDuplicate(tao->solution,&mfqP->Xhist[i]);CHKERRQ(ierr);
    ierr = VecDuplicate(tao->sep_objective,&mfqP->Fhist[i]);CHKERRQ(ierr);
  }
  ierr = VecDuplicate(tao->solution,&mfqP->workxvec);CHKERRQ(ierr);
  mfqP->nHist = 0;

  ierr = PetscMalloc1(tao->max_funcs+10,&mfqP->Fres);CHKERRQ(ierr);
  ierr = PetscMalloc1(mfqP->npmax*mfqP->m,&mfqP->RES);CHKERRQ(ierr);
  ierr = PetscMalloc1(mfqP->n,&mfqP->work);CHKERRQ(ierr);
  ierr = PetscMalloc1(mfqP->n,&mfqP->work2);CHKERRQ(ierr);
  ierr = PetscMalloc1(mfqP->n,&mfqP->work3);CHKERRQ(ierr);
  ierr = PetscMalloc1(PetscMax(mfqP->m,mfqP->n+1),&mfqP->mwork);CHKERRQ(ierr);
  ierr = PetscMalloc1(mfqP->npmax - mfqP->n - 1,&mfqP->omega);CHKERRQ(ierr);
  ierr = PetscMalloc1(mfqP->n * (mfqP->n+1) / 2,&mfqP->beta);CHKERRQ(ierr);
  ierr = PetscMalloc1(mfqP->n + 1 ,&mfqP->alpha);CHKERRQ(ierr);

  ierr = PetscMalloc1(mfqP->n*mfqP->n*mfqP->m,&mfqP->H);CHKERRQ(ierr);
  ierr = PetscMalloc1(mfqP->npmax*mfqP->npmax,&mfqP->Q);CHKERRQ(ierr);
  ierr = PetscMalloc1(mfqP->npmax*mfqP->npmax,&mfqP->Q_tmp);CHKERRQ(ierr);
  ierr = PetscMalloc1(mfqP->n*(mfqP->n+1)/2*(mfqP->npmax),&mfqP->L);CHKERRQ(ierr);
  ierr = PetscMalloc1(mfqP->n*(mfqP->n+1)/2*(mfqP->npmax),&mfqP->L_tmp);CHKERRQ(ierr);
  ierr = PetscMalloc1(mfqP->n*(mfqP->n+1)/2*(mfqP->npmax),&mfqP->L_save);CHKERRQ(ierr);
  ierr = PetscMalloc1(mfqP->n*(mfqP->n+1)/2*(mfqP->npmax),&mfqP->N);CHKERRQ(ierr);
  ierr = PetscMalloc1(mfqP->npmax * (mfqP->n+1) ,&mfqP->M);CHKERRQ(ierr);
  ierr = PetscMalloc1(mfqP->npmax * (mfqP->npmax - mfqP->n - 1) , &mfqP->Z);CHKERRQ(ierr);
  ierr = PetscMalloc1(mfqP->npmax,&mfqP->tau);CHKERRQ(ierr);
  ierr = PetscMalloc1(mfqP->npmax,&mfqP->tau_tmp);CHKERRQ(ierr);
  mfqP->nmax = PetscMax(5*mfqP->npmax,mfqP->n*(mfqP->n+1)/2);
  ierr = PetscMalloc1(mfqP->nmax,&mfqP->npmaxwork);CHKERRQ(ierr);
  ierr = PetscMalloc1(mfqP->nmax,&mfqP->npmaxiwork);CHKERRQ(ierr);
  ierr = PetscMalloc1(mfqP->n,&mfqP->xmin);CHKERRQ(ierr);
  ierr = PetscMalloc1(mfqP->m,&mfqP->C);CHKERRQ(ierr);
  ierr = PetscMalloc1(mfqP->m*mfqP->n,&mfqP->Fdiff);CHKERRQ(ierr);
  ierr = PetscMalloc1(mfqP->npmax*mfqP->n,&mfqP->Disp);CHKERRQ(ierr);
  ierr = PetscMalloc1(mfqP->n,&mfqP->Gres);CHKERRQ(ierr);
  ierr = PetscMalloc1(mfqP->n*mfqP->n,&mfqP->Hres);CHKERRQ(ierr);
  ierr = PetscMalloc1(mfqP->n*mfqP->n,&mfqP->Gpoints);CHKERRQ(ierr);
  ierr = PetscMalloc1(mfqP->npmax,&mfqP->model_indices);CHKERRQ(ierr);
  ierr = PetscMalloc1(mfqP->npmax,&mfqP->last_model_indices);CHKERRQ(ierr);
  ierr = PetscMalloc1(mfqP->n,&mfqP->Xsubproblem);CHKERRQ(ierr);
  ierr = PetscMalloc1(mfqP->m*mfqP->n,&mfqP->Gdel);CHKERRQ(ierr);
  ierr = PetscMalloc1(mfqP->n*mfqP->n*mfqP->m, &mfqP->Hdel);CHKERRQ(ierr);
  ierr = PetscMalloc1(PetscMax(mfqP->m,mfqP->n),&mfqP->indices);CHKERRQ(ierr);
  ierr = PetscMalloc1(mfqP->n,&mfqP->iwork);CHKERRQ(ierr);
  for (i=0;i<PetscMax(mfqP->m,mfqP->n);i++) {
    mfqP->indices[i] = i;
  }
  ierr = MPI_Comm_size(((PetscObject)tao)->comm,&mfqP->size);CHKERRQ(ierr);
  if (mfqP->size > 1) {
    VecCreateSeq(PETSC_COMM_SELF,mfqP->n,&mfqP->localx);CHKERRQ(ierr);
    VecCreateSeq(PETSC_COMM_SELF,mfqP->n,&mfqP->localxmin);CHKERRQ(ierr);
    VecCreateSeq(PETSC_COMM_SELF,mfqP->m,&mfqP->localf);CHKERRQ(ierr);
    VecCreateSeq(PETSC_COMM_SELF,mfqP->m,&mfqP->localfmin);CHKERRQ(ierr);
    ierr = ISCreateStride(MPI_COMM_SELF,mfqP->n,0,1,&isxloc);CHKERRQ(ierr);
    ierr = ISCreateStride(MPI_COMM_SELF,mfqP->n,0,1,&isxglob);CHKERRQ(ierr);
    ierr = ISCreateStride(MPI_COMM_SELF,mfqP->m,0,1,&isfloc);CHKERRQ(ierr);
    ierr = ISCreateStride(MPI_COMM_SELF,mfqP->m,0,1,&isfglob);CHKERRQ(ierr);


    ierr = VecScatterCreate(tao->solution,isxglob,mfqP->localx,isxloc,&mfqP->scatterx);CHKERRQ(ierr);
    ierr = VecScatterCreate(tao->sep_objective,isfglob,mfqP->localf,isfloc,&mfqP->scatterf);CHKERRQ(ierr);

    ierr = ISDestroy(&isxloc);CHKERRQ(ierr);
    ierr = ISDestroy(&isxglob);CHKERRQ(ierr);
    ierr = ISDestroy(&isfloc);CHKERRQ(ierr);
    ierr = ISDestroy(&isfglob);CHKERRQ(ierr);
  }

  if (!mfqP->usegqt) {
    KSP       ksp;
    PC        pc;
    ierr = VecCreateSeqWithArray(PETSC_COMM_SELF,mfqP->n,mfqP->n,mfqP->Xsubproblem,&mfqP->subx);CHKERRQ(ierr);
    ierr = VecCreateSeq(PETSC_COMM_SELF,mfqP->n,&mfqP->subxl);CHKERRQ(ierr);
    ierr = VecDuplicate(mfqP->subxl,&mfqP->subb);CHKERRQ(ierr);
    ierr = VecDuplicate(mfqP->subxl,&mfqP->subxu);CHKERRQ(ierr);
    ierr = VecDuplicate(mfqP->subxl,&mfqP->subpdel);CHKERRQ(ierr);
    ierr = VecDuplicate(mfqP->subxl,&mfqP->subndel);CHKERRQ(ierr);
    ierr = TaoCreate(PETSC_COMM_SELF,&mfqP->subtao);CHKERRQ(ierr);
    ierr = TaoSetType(mfqP->subtao,TAOTRON);CHKERRQ(ierr);
    ierr = TaoSetOptionsPrefix(mfqP->subtao,"pounders_subsolver_");CHKERRQ(ierr);
    ierr = TaoSetInitialVector(mfqP->subtao,mfqP->subx);CHKERRQ(ierr);
    ierr = TaoSetObjectiveAndGradientRoutine(mfqP->subtao,pounders_fg,(void*)mfqP);CHKERRQ(ierr);
    ierr = TaoSetMaximumIterations(mfqP->subtao,mfqP->gqt_maxits);CHKERRQ(ierr);
    ierr = TaoSetFromOptions(mfqP->subtao);CHKERRQ(ierr);
    ierr = TaoGetKSP(mfqP->subtao,&ksp);CHKERRQ(ierr);
    if (ksp) {
      ierr = KSPGetPC(ksp,&pc);CHKERRQ(ierr);
      ierr = PCSetType(pc,PCNONE);CHKERRQ(ierr);
    }
    ierr = TaoSetVariableBounds(mfqP->subtao,mfqP->subxl,mfqP->subxu);CHKERRQ(ierr);
    ierr = MatCreateSeqDense(PETSC_COMM_SELF,mfqP->n,mfqP->n,mfqP->Hres,&mfqP->subH);CHKERRQ(ierr);
    ierr = TaoSetHessianRoutine(mfqP->subtao,mfqP->subH,mfqP->subH,pounders_h,(void*)mfqP);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoDestroy_POUNDERS"
static PetscErrorCode TaoDestroy_POUNDERS(Tao tao)
{
  TAO_POUNDERS   *mfqP = (TAO_POUNDERS*)tao->data;
  PetscInt       i;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (!mfqP->usegqt) {
    ierr = TaoDestroy(&mfqP->subtao);CHKERRQ(ierr);
    ierr = VecDestroy(&mfqP->subx);CHKERRQ(ierr);
    ierr = VecDestroy(&mfqP->subxl);CHKERRQ(ierr);
    ierr = VecDestroy(&mfqP->subxu);CHKERRQ(ierr);
    ierr = VecDestroy(&mfqP->subb);CHKERRQ(ierr);
    ierr = VecDestroy(&mfqP->subpdel);CHKERRQ(ierr);
    ierr = VecDestroy(&mfqP->subndel);CHKERRQ(ierr);
    ierr = MatDestroy(&mfqP->subH);CHKERRQ(ierr);
  }
  ierr = PetscFree(mfqP->Fres);CHKERRQ(ierr);
  ierr = PetscFree(mfqP->RES);CHKERRQ(ierr);
  ierr = PetscFree(mfqP->work);CHKERRQ(ierr);
  ierr = PetscFree(mfqP->work2);CHKERRQ(ierr);
  ierr = PetscFree(mfqP->work3);CHKERRQ(ierr);
  ierr = PetscFree(mfqP->mwork);CHKERRQ(ierr);
  ierr = PetscFree(mfqP->omega);CHKERRQ(ierr);
  ierr = PetscFree(mfqP->beta);CHKERRQ(ierr);
  ierr = PetscFree(mfqP->alpha);CHKERRQ(ierr);
  ierr = PetscFree(mfqP->H);CHKERRQ(ierr);
  ierr = PetscFree(mfqP->Q);CHKERRQ(ierr);
  ierr = PetscFree(mfqP->Q_tmp);CHKERRQ(ierr);
  ierr = PetscFree(mfqP->L);CHKERRQ(ierr);
  ierr = PetscFree(mfqP->L_tmp);CHKERRQ(ierr);
  ierr = PetscFree(mfqP->L_save);CHKERRQ(ierr);
  ierr = PetscFree(mfqP->N);CHKERRQ(ierr);
  ierr = PetscFree(mfqP->M);CHKERRQ(ierr);
  ierr = PetscFree(mfqP->Z);CHKERRQ(ierr);
  ierr = PetscFree(mfqP->tau);CHKERRQ(ierr);
  ierr = PetscFree(mfqP->tau_tmp);CHKERRQ(ierr);
  ierr = PetscFree(mfqP->npmaxwork);CHKERRQ(ierr);
  ierr = PetscFree(mfqP->npmaxiwork);CHKERRQ(ierr);
  ierr = PetscFree(mfqP->xmin);CHKERRQ(ierr);
  ierr = PetscFree(mfqP->C);CHKERRQ(ierr);
  ierr = PetscFree(mfqP->Fdiff);CHKERRQ(ierr);
  ierr = PetscFree(mfqP->Disp);CHKERRQ(ierr);
  ierr = PetscFree(mfqP->Gres);CHKERRQ(ierr);
  ierr = PetscFree(mfqP->Hres);CHKERRQ(ierr);
  ierr = PetscFree(mfqP->Gpoints);CHKERRQ(ierr);
  ierr = PetscFree(mfqP->model_indices);CHKERRQ(ierr);
  ierr = PetscFree(mfqP->last_model_indices);CHKERRQ(ierr);
  ierr = PetscFree(mfqP->Xsubproblem);CHKERRQ(ierr);
  ierr = PetscFree(mfqP->Gdel);CHKERRQ(ierr);
  ierr = PetscFree(mfqP->Hdel);CHKERRQ(ierr);
  ierr = PetscFree(mfqP->indices);CHKERRQ(ierr);
  ierr = PetscFree(mfqP->iwork);CHKERRQ(ierr);

  for (i=0;i<mfqP->nHist;i++) {
    ierr = VecDestroy(&mfqP->Xhist[i]);CHKERRQ(ierr);
    ierr = VecDestroy(&mfqP->Fhist[i]);CHKERRQ(ierr);
  }
  if (mfqP->workxvec) {
    ierr = VecDestroy(&mfqP->workxvec);CHKERRQ(ierr);
  }
  ierr = PetscFree(mfqP->Xhist);CHKERRQ(ierr);
  ierr = PetscFree(mfqP->Fhist);CHKERRQ(ierr);

  if (mfqP->size > 1) {
    ierr = VecDestroy(&mfqP->localx);CHKERRQ(ierr);
    ierr = VecDestroy(&mfqP->localxmin);CHKERRQ(ierr);
    ierr = VecDestroy(&mfqP->localf);CHKERRQ(ierr);
    ierr = VecDestroy(&mfqP->localfmin);CHKERRQ(ierr);
  }
  ierr = PetscFree(tao->data);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoSetFromOptions_POUNDERS"
static PetscErrorCode TaoSetFromOptions_POUNDERS(PetscOptionItems *PetscOptionsObject,Tao tao)
{
  TAO_POUNDERS   *mfqP = (TAO_POUNDERS*)tao->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscOptionsHead(PetscOptionsObject,"POUNDERS method for least-squares optimization");CHKERRQ(ierr);
  ierr = PetscOptionsReal("-tao_pounders_delta","initial delta","",mfqP->delta,&mfqP->delta0,NULL);CHKERRQ(ierr);
  mfqP->delta = mfqP->delta0;
  mfqP->npmax = PETSC_DEFAULT;
  ierr = PetscOptionsInt("-tao_pounders_npmax","max number of points in model","",mfqP->npmax,&mfqP->npmax,NULL);CHKERRQ(ierr);
  mfqP->usegqt = PETSC_FALSE;
  ierr = PetscOptionsBool("-tao_pounders_gqt","use gqt algorithm for subproblem","",mfqP->usegqt,&mfqP->usegqt,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsTail();CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TaoView_POUNDERS"
static PetscErrorCode TaoView_POUNDERS(Tao tao, PetscViewer viewer)
{
  TAO_POUNDERS   *mfqP = (TAO_POUNDERS *)tao->data;
  PetscBool      isascii;
  PetscInt       nits;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERASCII,&isascii);CHKERRQ(ierr);
  if (isascii) {
    ierr = PetscViewerASCIIPushTab(viewer);CHKERRQ(ierr);
    ierr = PetscViewerASCIIPrintf(viewer, "initial delta: %g\n",(double)mfqP->delta0);CHKERRQ(ierr);
    ierr = PetscViewerASCIIPrintf(viewer, "final delta: %g\n",(double)mfqP->delta);CHKERRQ(ierr);
    ierr = PetscViewerASCIIPrintf(viewer, "model points: %D\n",mfqP->nmodelpoints);CHKERRQ(ierr);
    if (mfqP->usegqt) {
      ierr = PetscViewerASCIIPrintf(viewer, "subproblem solver: gqt\n");CHKERRQ(ierr);
    } else {
      ierr = PetscViewerASCIIPrintf(viewer, "subproblem solver: %s\n",((PetscObject)mfqP->subtao)->type_name);CHKERRQ(ierr);
      ierr = TaoGetTotalIterationNumber(mfqP->subtao,&nits);CHKERRQ(ierr);
      ierr = PetscViewerASCIIPrintf(viewer, "total subproblem iterations: %D\n",nits);CHKERRQ(ierr);
    }
    ierr = PetscViewerASCIIPopTab(viewer);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}
/*MC
  TAOPOUNDERS - POUNDERS derivate-free model-based algorithm for nonlinear least squares

  Options Database Keys:
+ -tao_pounders_delta - initial step length
. -tao_pounders_npmax - maximum number of points in model
- -tao_pounders_gqt - use gqt algorithm for subproblem instead of TRON

  Level: beginner
 
M*/

#undef __FUNCT__
#define __FUNCT__ "TaoCreate_POUNDERS"
PETSC_EXTERN PetscErrorCode TaoCreate_POUNDERS(Tao tao)
{
  TAO_POUNDERS   *mfqP = (TAO_POUNDERS*)tao->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  tao->ops->setup = TaoSetUp_POUNDERS;
  tao->ops->solve = TaoSolve_POUNDERS;
  tao->ops->view = TaoView_POUNDERS;
  tao->ops->setfromoptions = TaoSetFromOptions_POUNDERS;
  tao->ops->destroy = TaoDestroy_POUNDERS;

  ierr = PetscNewLog(tao,&mfqP);CHKERRQ(ierr);
  tao->data = (void*)mfqP;
  /* Override default settings (unless already changed) */
  if (!tao->max_it_changed) tao->max_it = 2000;
  if (!tao->max_funcs_changed) tao->max_funcs = 4000;
  mfqP->delta0 = 0.1;
  mfqP->delta = 0.1;
  mfqP->deltamax=1e3;
  mfqP->c2 = 100.0;
  mfqP->theta1=1e-5;
  mfqP->theta2=1e-4;
  mfqP->gamma0=0.5;
  mfqP->gamma1=2.0;
  mfqP->eta0 = 0.0;
  mfqP->eta1 = 0.1;
  mfqP->gqt_rtol = 0.001;
  mfqP->gqt_maxits = 50;
  mfqP->workxvec = 0;
  PetscFunctionReturn(0);
}


