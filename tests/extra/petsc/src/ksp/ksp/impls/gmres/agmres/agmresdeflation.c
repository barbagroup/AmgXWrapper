/*
 *  This file computes data for the deflated restarting in the Newton-basis GMRES. At each restart (or at each detected stagnation in the adaptive strategy), a basis of an (approximated)invariant subspace corresponding to the smallest eigenvalues is extracted from the Krylov subspace. It is then used to augment the Newton basis.
 *
 * References : D. Nuentsa Wakam and J. Erhel, Parallelism and robustness in GMRES with the Newton basis and the deflation of eigenvalues. Research report INRIA RR-7787.
 * Author: Desire NUENTSA WAKAM <desire.nuentsa_wakam@inria.fr>, 2011
 */

#include <../src/ksp/ksp/impls/gmres/agmres/agmresimpl.h>

/* Quicksort algorithm to sort  the eigenvalues in increasing orders
 * val_r - real part of eigenvalues, unchanged on exit.
 * val_i - Imaginary part of eigenvalues unchanged on exit.
 * size - Number of eigenvalues (with complex conjugates)
 * perm - contains on exit the permutation vector to reorder the vectors val_r and val_i.
 */
#undef __FUNCT__
#define __FUNCT__ "KSPAGMRESQuickSort"
#define  DEPTH  500
static PetscErrorCode KSPAGMRESQuickSort(PetscScalar *val_r, PetscScalar *val_i, PetscInt size, PetscInt *perm)
{

  PetscInt    deb[DEPTH], fin[DEPTH];
  PetscInt    ipivot;
  PetscScalar pivot_r, pivot_i;
  PetscInt    i, L, R, j;
  PetscScalar abs_pivot;
  PetscScalar abs_val;

  PetscFunctionBegin;
  /* initialize perm vector */
  for (j = 0; j < size; j++) perm[j] = j;

  deb[0] = 0;
  fin[0] = size;
  i      = 0;
  while (i >= 0) {
    L = deb[i];
    R = fin[i] - 1;
    if (L < R) {
      pivot_r   = val_r[L];
      pivot_i   = val_i[L];
      abs_pivot = PetscSqrtReal(pivot_r * pivot_r + pivot_i * pivot_i);
      ipivot    = perm[L];
      if (i == DEPTH - 1) SETERRQ(PETSC_COMM_SELF, PETSC_ERR_MEM, "Could cause stack overflow: Try to increase the value of DEPTH ");
      while (L < R) {
        abs_val = PetscSqrtReal(val_r[R] * val_r[R] + val_i[R] * val_i[R]);
        while (abs_val >= abs_pivot && L < R) {
          R--;
          abs_val = PetscSqrtReal(val_r[R] * val_r[R] + val_i[R] * val_i[R]);
        }
        if (L < R) {
          val_r[L] = val_r[R];
          val_i[L] = val_i[R];
          perm[L]  = perm[R];
          L       += 1;
        }
        abs_val = PetscSqrtReal(val_r[L] * val_r[L] + val_i[L] * val_i[L]);
        while (abs_val <= abs_pivot && L < R) {
          L++;
          abs_val = PetscSqrtReal(val_r[L] * val_r[L] + val_i[L] * val_i[L]);
        }
        if (L < R) {
          val_r[R] = val_r[L];
          val_i[R] = val_i[L];
          perm[R]  = perm[L];
          R       -= 1;
        }
      }
      val_r[L] = pivot_r;
      val_i[L] = pivot_i;
      perm[L]  = ipivot;
      deb[i+1] = L + 1;
      fin[i+1] = fin[i];
      fin[i]   = L;
      i       += 1;
      if (i == DEPTH - 1) SETERRQ(PETSC_COMM_SELF, PETSC_ERR_MEM, "Could cause stack overflow: Try to increase the value of DEPTH ");
    } else i--;
  }
  PetscFunctionReturn(0);
}

/*
 * Compute the Schur vectors from the generalized eigenvalue problem A.u =\lamba.B.u
 * KspSize -  rank of the matrices A and B, size of the current Krylov basis
 * A - Left matrix
 * B - Right matrix
 * ldA - first dimension of A as declared  in the calling program
 * ldB - first dimension of B as declared  in the calling program
 * IsReduced - specifies if the matrices are already in the reduced form,
 * i.e A is a Hessenberg matrix and B is upper triangular.
 * Sr - on exit, the extracted Schur vectors corresponding
 * the smallest eigenvalues (with complex conjugates)
 * CurNeig - Number of extracted eigenvalues
 */
#undef __FUNCT__
#define __FUNCT__ "KSPAGMRESSchurForm"
static PetscErrorCode KSPAGMRESSchurForm(KSP ksp, PetscBLASInt KspSize, PetscScalar *A, PetscBLASInt ldA, PetscScalar *B, PetscBLASInt ldB, PetscBool IsReduced, PetscScalar *Sr, PetscInt *CurNeig)
{
  KSP_AGMRES     *agmres = (KSP_AGMRES*)ksp->data;
  PetscInt       max_k   = agmres->max_k;
  PetscBLASInt   r;
  PetscInt       neig    = agmres->neig;
  PetscScalar    *wr     = agmres->wr;
  PetscScalar    *wi     = agmres->wi;
  PetscScalar    *beta   = agmres->beta;
  PetscScalar    *Q      = agmres->Q;
  PetscScalar    *Z      = agmres->Z;
  PetscScalar    *work   = agmres->work;
  PetscInt       *select = agmres->select;
  PetscInt       *perm   = agmres->perm;
  PetscInt       sdim    = 0;
  PetscInt       i,j, info;
  PetscErrorCode ierr;
  PetscInt       *iwork = agmres->iwork;
  PetscBLASInt   N;
  PetscBLASInt   lwork,liwork;
  PetscBLASInt   ilo,ihi;
  PetscBLASInt   ijob,wantQ,wantZ;
  PetscScalar    Dif[2];

  PetscFunctionBegin;
  ijob  = 2;
  wantQ = 1;
  wantZ = 1;
  ierr  = PetscBLASIntCast(PetscMax(8*N+16,4*neig*(N-neig)),&lwork);CHKERRQ(ierr);
  ierr  = PetscBLASIntCast(2*N*neig,&liwork);CHKERRQ(ierr);
  ilo   = 1;
  ierr  = PetscBLASIntCast(KspSize,&ihi);CHKERRQ(ierr);
  N     = MAXKSPSIZE;

  /* Compute the Schur form */
  if (IsReduced) {                /* The eigenvalue problem is already in reduced form, meaning that A is upper Hessenberg and B is triangular */
#if defined(PETSC_MISSING_LAPACK_HGEQZ)
    SETERRQ(PetscObjectComm((PetscObject)ksp),PETSC_ERR_SUP,"HGEQZ - Lapack routine is unavailable.");
#else
    PetscStackCallBLAS("LAPACKhgeqz",LAPACKhgeqz_("S", "I", "I", &KspSize, &ilo, &ihi, A, &ldA, B, &ldB, wr, wi, beta, Q, &N, Z, &N, work, &lwork, &info));
    if (info) SETERRQ(PetscObjectComm((PetscObject)ksp),PETSC_ERR_PLIB, "Error while calling LAPACK routine xhgeqz_");
#endif
  } else {
#if defined(PETSC_MISSING_LAPACK_GGES)
    SETERRQ(PetscObjectComm((PetscObject)ksp),PETSC_ERR_SUP,"GGES - Lapack routine is unavailable.");
#else
    PetscStackCallBLAS("LAPACKgges",LAPACKgges_("V", "V", "N", NULL, &KspSize, A, &ldA, B, &ldB, &sdim, wr, wi, beta, Q, &N, Z, &N, work, &lwork, NULL, &info));
    if (info) SETERRQ(PetscObjectComm((PetscObject)ksp),PETSC_ERR_PLIB, "Error while calling LAPACK routine xgges_");
#endif
  }

  /* We should avoid computing these ratio...  */
  for (i = 0; i < KspSize; i++) {
    if (beta[i] != 0.0) {
      wr[i] /= beta[i];
      wi[i] /= beta[i];
    }
  }

  /* Sort the eigenvalues to extract the smallest ones */
  ierr = KSPAGMRESQuickSort(wr, wi, KspSize, perm);CHKERRQ(ierr);

  /* Count the number of extracted eigenvalues (with complex conjugates) */
  r = 0;
  while (r < neig) {
    if (wi[r] != 0) r += 2;
    else r += 1;
  }
  /* Reorder the Schur decomposition so that the cluster of smallest/largest eigenvalues appears in the leading diagonal blocks of A (and B)*/
  ierr = PetscMemzero(select, N*sizeof(PetscInt));CHKERRQ(ierr);
  if (!agmres->GreatestEig) {
    for (j = 0; j < r; j++) select[perm[j]] = 1;
  } else {
    for (j = 0; j < r; j++) select[perm[KspSize-j-1]] = 1;
  }
#if defined(PETSC_MISSING_LAPACK_TGSEN)
  SETERRQ(PetscObjectComm((PetscObject)ksp),PETSC_ERR_SUP,"GGES - Lapack routine is unavailable.");
#else
  PetscStackCallBLAS("LAPACKtgsen",LAPACKtgsen_(&ijob, &wantQ, &wantZ, select, &KspSize, A, &ldA, B, &ldB, wr, wi, beta, Q, &N, Z, &N, &r, NULL, NULL, &(Dif[0]), work, &lwork, iwork, &liwork, &info));
  if (info == 1) SETERRQ(PetscObjectComm((PetscObject)ksp),PETSC_ERR_PLIB, "UNABLE TO REORDER THE EIGENVALUES WITH THE LAPACK ROUTINE : ILL-CONDITIONED PROBLEM");
#endif
  /*Extract the Schur vectors associated to the r smallest eigenvalues */
  ierr = PetscMemzero(Sr,(N+1)*r*sizeof(PetscScalar));CHKERRQ(ierr);
  for (j = 0; j < r; j++) {
    for (i = 0; i < KspSize; i++) {
      Sr[j*(N+1)+i] = Z[j*N+i];
    }
  }

  /* Broadcast Sr to all other processes to have consistent data;
   * FIXME should investigate how to get unique Schur vectors (unique QR factorization, probably the sign of rotations) */
  ierr = MPI_Bcast(Sr, (N+1)*r, MPIU_SCALAR, agmres->First, PetscObjectComm((PetscObject)ksp));
  /* Update the Shift values for the Newton basis. This is surely necessary when applying the DeflationPrecond */
  if (agmres->DeflPrecond) {
    ierr = KSPAGMRESLejaOrdering(wr, wi, agmres->Rshift, agmres->Ishift, max_k);CHKERRQ(ierr);
  }
  *CurNeig = r; /* Number of extracted eigenvalues */
  PetscFunctionReturn(0);

}

/*
 * This function form the matrices for the generalized eigenvalue problem,
 * it then compute the Schur vectors needed to augment the Newton basis.
 */
#undef __FUNCT__
#define __FUNCT__ "KSPAGMRESComputeDeflationData"
PetscErrorCode KSPAGMRESComputeDeflationData(KSP ksp)
{
  KSP_AGMRES     *agmres  = (KSP_AGMRES*)ksp->data;
  Vec            *U       = agmres->U;
  Vec            *TmpU    = agmres->TmpU;
  PetscScalar    *MatEigL = agmres->MatEigL;
  PetscScalar    *MatEigR = agmres->MatEigR;
  PetscScalar    *Sr      = agmres->Sr;
  PetscScalar    alpha, beta;
  PetscInt       i,j;
  PetscErrorCode ierr;
  PetscInt       max_k = agmres->max_k;     /* size of the non - augmented subspace */
  PetscInt       CurNeig;       /* CUrrent number of extracted eigenvalues */
  PetscInt       N        = MAXKSPSIZE;
  PetscInt       lC       = N + 1;
  PetscInt       KspSize  = KSPSIZE;
  PetscInt       PrevNeig = agmres->r;

  PetscFunctionBegin;
  ierr = PetscLogEventBegin(KSP_AGMRESComputeDeflationData, ksp, 0,0,0);CHKERRQ(ierr);
  if (agmres->neig <= 1) PetscFunctionReturn(0);
  /* Explicitly form MatEigL = H^T*H, It can also be formed as H^T+h_{N+1,N}H^-1e^T */
  alpha = 1.0;
  beta  = 0.0;
  PetscStackCallBLAS("BLASgemm",BLASgemm_("T", "N", &KspSize, &KspSize, &lC, &alpha, agmres->hes_origin, &lC, agmres->hes_origin, &lC, &beta, MatEigL, &N));
  if (!agmres->ritz) {
    /* Form TmpU = V*H where V is the Newton basis orthogonalized  with roddec*/
    for (j = 0; j < KspSize; j++) {
      /* Apply the elementary reflectors (stored in Qloc) on H */
      ierr = KSPAGMRESRodvec(ksp, KspSize+1, &agmres->hes_origin[j*lC], TmpU[j]);CHKERRQ(ierr);
    }
    /* Now form MatEigR = TmpU^T*W where W is [VEC_V(1:max_k); U] */
    for (j = 0; j < max_k; j++) {
      ierr = VecMDot(VEC_V(j), KspSize, TmpU, &MatEigR[j*N]);CHKERRQ(ierr);
    }
    for (j = max_k; j < KspSize; j++) {
      ierr = VecMDot(U[j-max_k], KspSize, TmpU, &MatEigR[j*N]);CHKERRQ(ierr);
    }
  } else { /* Form H^T */
    for (j = 0; j < N; j++) {
      for (i = 0; i < N; i++) {
        MatEigR[j*N+i] = agmres->hes_origin[i*lC+j];
      }
    }
  }
  /* Obtain the Schur form of  the generalized eigenvalue problem MatEigL*y = \lambda*MatEigR*y */
  if (agmres->DeflPrecond) {
    ierr = KSPAGMRESSchurForm(ksp, KspSize, agmres->hes_origin, lC, agmres->Rloc, lC, PETSC_TRUE, Sr, &CurNeig);CHKERRQ(ierr);
  } else {
    ierr = KSPAGMRESSchurForm(ksp, KspSize, MatEigL, N, MatEigR, N, PETSC_FALSE, Sr, &CurNeig);CHKERRQ(ierr);
  }

  if (agmres->DeflPrecond) { /* Switch to DGMRES to improve the basis of the invariant subspace associated to the deflation */
    agmres->HasSchur = PETSC_TRUE;
    ierr             = KSPDGMRESComputeDeflationData_DGMRES(ksp, &CurNeig);CHKERRQ(ierr);
    PetscFunctionReturn(0);
  }
  /* Form the Schur vectors in the entire subspace: U = W * Sr where W = [VEC_V(1:max_k); U]*/
  for (j = 0; j < PrevNeig; j++) { /* First, copy U to a temporary place */
    ierr = VecCopy(U[j], TmpU[j]);CHKERRQ(ierr);
  }

  for (j = 0; j < CurNeig; j++) {
    ierr = VecZeroEntries(U[j]);CHKERRQ(ierr);
    ierr = VecMAXPY(U[j], max_k, &Sr[j*(N+1)], &VEC_V(0));CHKERRQ(ierr);
    ierr = VecMAXPY(U[j], PrevNeig, &Sr[j*(N+1)+max_k], TmpU);CHKERRQ(ierr);
  }
  agmres->r = CurNeig;
  ierr      = PetscLogEventEnd(KSP_AGMRESComputeDeflationData, ksp, 0,0,0);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
