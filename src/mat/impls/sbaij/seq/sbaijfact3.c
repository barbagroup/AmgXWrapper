
#include <../src/mat/impls/sbaij/seq/sbaij.h>
#include <petsc/private/kernels/blockinvert.h>

/* Version for when blocks are 3 by 3  */
#undef __FUNCT__
#define __FUNCT__ "MatCholeskyFactorNumeric_SeqSBAIJ_3"
PetscErrorCode MatCholeskyFactorNumeric_SeqSBAIJ_3(Mat C,Mat A,const MatFactorInfo *info)
{
  Mat_SeqSBAIJ   *a   = (Mat_SeqSBAIJ*)A->data,*b = (Mat_SeqSBAIJ*)C->data;
  IS             perm = b->row;
  PetscErrorCode ierr;
  const PetscInt *ai,*aj,*perm_ptr,mbs=a->mbs,*bi=b->i,*bj=b->j;
  PetscInt       *a2anew,i,j,k,k1,jmin,jmax,*jl,*il,vj,nexti,ili;
  MatScalar      *ba = b->a,*aa,*ap,*dk,*uik;
  MatScalar      *u,*diag,*rtmp,*rtmp_ptr;
  PetscReal      shift = info->shiftamount;
  PetscBool      allowzeropivot,zeropivotdetected;

  PetscFunctionBegin;
  /* initialization */
  allowzeropivot = PetscNot(A->erroriffailure);
  ierr = PetscCalloc1(9*mbs,&rtmp);CHKERRQ(ierr);
  ierr = PetscMalloc2(mbs,&il,mbs,&jl);CHKERRQ(ierr);
  for (i=0; i<mbs; i++) {
    jl[i] = mbs; il[0] = 0;
  }
  ierr = PetscMalloc2(9,&dk,9,&uik);CHKERRQ(ierr);
  ierr = ISGetIndices(perm,&perm_ptr);CHKERRQ(ierr);

  /* check permutation */
  if (!a->permute) {
    ai = a->i; aj = a->j; aa = a->a;
  } else {
    ai   = a->inew; aj = a->jnew;
    ierr = PetscMalloc1(9*ai[mbs],&aa);CHKERRQ(ierr);
    ierr = PetscMemcpy(aa,a->a,9*ai[mbs]*sizeof(MatScalar));CHKERRQ(ierr);
    ierr = PetscMalloc1(ai[mbs],&a2anew);CHKERRQ(ierr);
    ierr = PetscMemcpy(a2anew,a->a2anew,(ai[mbs])*sizeof(PetscInt));CHKERRQ(ierr);

    for (i=0; i<mbs; i++) {
      jmin = ai[i]; jmax = ai[i+1];
      for (j=jmin; j<jmax; j++) {
        while (a2anew[j] != j) {
          k = a2anew[j]; a2anew[j] = a2anew[k]; a2anew[k] = k;
          for (k1=0; k1<9; k1++) {
            dk[k1]     = aa[k*9+k1];
            aa[k*9+k1] = aa[j*9+k1];
            aa[j*9+k1] = dk[k1];
          }
        }
        /* transform columnoriented blocks that lie in the lower triangle to roworiented blocks */
        if (i > aj[j]) {
          /* printf("change orientation, row: %d, col: %d\n",i,aj[j]); */
          ap = aa + j*9;                     /* ptr to the beginning of j-th block of aa */
          for (k=0; k<9; k++) dk[k] = ap[k]; /* dk <- j-th block of aa */
          for (k=0; k<3; k++) {               /* j-th block of aa <- dk^T */
            for (k1=0; k1<3; k1++) *ap++ = dk[k + 3*k1];
          }
        }
      }
    }
    ierr = PetscFree(a2anew);CHKERRQ(ierr);
  }

  /* for each row k */
  for (k = 0; k<mbs; k++) {

    /*initialize k-th row with elements nonzero in row perm(k) of A */
    jmin = ai[perm_ptr[k]]; jmax = ai[perm_ptr[k]+1];
    if (jmin < jmax) {
      ap = aa + jmin*9;
      for (j = jmin; j < jmax; j++) {
        vj       = perm_ptr[aj[j]];   /* block col. index */
        rtmp_ptr = rtmp + vj*9;
        for (i=0; i<9; i++) *rtmp_ptr++ = *ap++;
      }
    }

    /* modify k-th row by adding in those rows i with U(i,k) != 0 */
    ierr = PetscMemcpy(dk,rtmp+k*9,9*sizeof(MatScalar));CHKERRQ(ierr);
    i    = jl[k]; /* first row to be added to k_th row  */

    while (i < mbs) {
      nexti = jl[i]; /* next row to be added to k_th row */

      /* compute multiplier */
      ili = il[i];  /* index of first nonzero element in U(i,k:bms-1) */

      /* uik = -inv(Di)*U_bar(i,k) */
      diag = ba + i*9;
      u    = ba + ili*9;

      uik[0] = -(diag[0]*u[0] + diag[3]*u[1] + diag[6]*u[2]);
      uik[1] = -(diag[1]*u[0] + diag[4]*u[1] + diag[7]*u[2]);
      uik[2] = -(diag[2]*u[0] + diag[5]*u[1] + diag[8]*u[2]);

      uik[3] = -(diag[0]*u[3] + diag[3]*u[4] + diag[6]*u[5]);
      uik[4] = -(diag[1]*u[3] + diag[4]*u[4] + diag[7]*u[5]);
      uik[5] = -(diag[2]*u[3] + diag[5]*u[4] + diag[8]*u[5]);

      uik[6] = -(diag[0]*u[6] + diag[3]*u[7] + diag[6]*u[8]);
      uik[7] = -(diag[1]*u[6] + diag[4]*u[7] + diag[7]*u[8]);
      uik[8] = -(diag[2]*u[6] + diag[5]*u[7] + diag[8]*u[8]);

      /* update D(k) += -U(i,k)^T * U_bar(i,k) */
      dk[0] += uik[0]*u[0] + uik[1]*u[1] + uik[2]*u[2];
      dk[1] += uik[3]*u[0] + uik[4]*u[1] + uik[5]*u[2];
      dk[2] += uik[6]*u[0] + uik[7]*u[1] + uik[8]*u[2];

      dk[3] += uik[0]*u[3] + uik[1]*u[4] + uik[2]*u[5];
      dk[4] += uik[3]*u[3] + uik[4]*u[4] + uik[5]*u[5];
      dk[5] += uik[6]*u[3] + uik[7]*u[4] + uik[8]*u[5];

      dk[6] += uik[0]*u[6] + uik[1]*u[7] + uik[2]*u[8];
      dk[7] += uik[3]*u[6] + uik[4]*u[7] + uik[5]*u[8];
      dk[8] += uik[6]*u[6] + uik[7]*u[7] + uik[8]*u[8];

      ierr = PetscLogFlops(27.0*4.0);CHKERRQ(ierr);

      /* update -U(i,k) */
      ierr = PetscMemcpy(ba+ili*9,uik,9*sizeof(MatScalar));CHKERRQ(ierr);

      /* add multiple of row i to k-th row ... */
      jmin = ili + 1; jmax = bi[i+1];
      if (jmin < jmax) {
        for (j=jmin; j<jmax; j++) {
          /* rtmp += -U(i,k)^T * U_bar(i,j) */
          rtmp_ptr     = rtmp + bj[j]*9;
          u            = ba + j*9;
          rtmp_ptr[0] += uik[0]*u[0] + uik[1]*u[1] + uik[2]*u[2];
          rtmp_ptr[1] += uik[3]*u[0] + uik[4]*u[1] + uik[5]*u[2];
          rtmp_ptr[2] += uik[6]*u[0] + uik[7]*u[1] + uik[8]*u[2];

          rtmp_ptr[3] += uik[0]*u[3] + uik[1]*u[4] + uik[2]*u[5];
          rtmp_ptr[4] += uik[3]*u[3] + uik[4]*u[4] + uik[5]*u[5];
          rtmp_ptr[5] += uik[6]*u[3] + uik[7]*u[4] + uik[8]*u[5];

          rtmp_ptr[6] += uik[0]*u[6] + uik[1]*u[7] + uik[2]*u[8];
          rtmp_ptr[7] += uik[3]*u[6] + uik[4]*u[7] + uik[5]*u[8];
          rtmp_ptr[8] += uik[6]*u[6] + uik[7]*u[7] + uik[8]*u[8];
        }
        ierr = PetscLogFlops(2.0*27.0*(jmax-jmin));CHKERRQ(ierr);

        /* ... add i to row list for next nonzero entry */
        il[i] = jmin;             /* update il(i) in column k+1, ... mbs-1 */
        j     = bj[jmin];
        jl[i] = jl[j]; jl[j] = i; /* update jl */
      }
      i = nexti;
    }

    /* save nonzero entries in k-th row of U ... */

    /* invert diagonal block */
    diag = ba+k*9;
    ierr = PetscMemcpy(diag,dk,9*sizeof(MatScalar));CHKERRQ(ierr);
    ierr = PetscKernel_A_gets_inverse_A_3(diag,shift,allowzeropivot,&zeropivotdetected);CHKERRQ(ierr);
    if (zeropivotdetected) C->errortype = MAT_FACTOR_NUMERIC_ZEROPIVOT;

    jmin = bi[k]; jmax = bi[k+1];
    if (jmin < jmax) {
      for (j=jmin; j<jmax; j++) {
        vj       = bj[j];      /* block col. index of U */
        u        = ba + j*9;
        rtmp_ptr = rtmp + vj*9;
        for (k1=0; k1<9; k1++) {
          *u++        = *rtmp_ptr;
          *rtmp_ptr++ = 0.0;
        }
      }

      /* ... add k to row list for first nonzero entry in k-th row */
      il[k] = jmin;
      i     = bj[jmin];
      jl[k] = jl[i]; jl[i] = k;
    }
  }

  ierr = PetscFree(rtmp);CHKERRQ(ierr);
  ierr = PetscFree2(il,jl);CHKERRQ(ierr);
  ierr = PetscFree2(dk,uik);CHKERRQ(ierr);
  if (a->permute) {
    ierr = PetscFree(aa);CHKERRQ(ierr);
  }

  ierr = ISRestoreIndices(perm,&perm_ptr);CHKERRQ(ierr);

  C->ops->solve          = MatSolve_SeqSBAIJ_3_inplace;
  C->ops->solvetranspose = MatSolve_SeqSBAIJ_3_inplace;
  C->assembled           = PETSC_TRUE;
  C->preallocated        = PETSC_TRUE;

  ierr = PetscLogFlops(1.3333*27*b->mbs);CHKERRQ(ierr); /* from inverting diagonal blocks */
  PetscFunctionReturn(0);
}
