
/*
    Defines the basic matrix operations for the BAIJ (compressed row)
  matrix storage format.
*/
#include <../src/mat/impls/baij/seq/baij.h>  /*I   "petscmat.h"  I*/
#include <petscblaslapack.h>
#include <petsc/private/kernels/blockinvert.h>
#include <petsc/private/kernels/blockmatmult.h>

#undef __FUNCT__
#define __FUNCT__ "MatInvertBlockDiagonal_SeqBAIJ"
PetscErrorCode MatInvertBlockDiagonal_SeqBAIJ(Mat A,const PetscScalar **values)
{
  Mat_SeqBAIJ    *a = (Mat_SeqBAIJ*) A->data;
  PetscErrorCode ierr;
  PetscInt       *diag_offset,i,bs = A->rmap->bs,mbs = a->mbs,ipvt[5],bs2 = bs*bs,*v_pivots;
  MatScalar      *v    = a->a,*odiag,*diag,*mdiag,work[25],*v_work;
  PetscReal      shift = 0.0;
  PetscBool      allowzeropivot,zeropivotdetected=PETSC_FALSE;

  PetscFunctionBegin;
  allowzeropivot = PetscNot(A->erroriffailure);

  if (a->idiagvalid) {
    if (values) *values = a->idiag;
    PetscFunctionReturn(0);
  }
  ierr        = MatMarkDiagonal_SeqBAIJ(A);CHKERRQ(ierr);
  diag_offset = a->diag;
  if (!a->idiag) {
    ierr = PetscMalloc1(2*bs2*mbs,&a->idiag);CHKERRQ(ierr);
    ierr = PetscLogObjectMemory((PetscObject)A,2*bs2*mbs*sizeof(PetscScalar));CHKERRQ(ierr);
  }
  diag  = a->idiag;
  mdiag = a->idiag+bs2*mbs;
  if (values) *values = a->idiag;
  /* factor and invert each block */
  switch (bs) {
  case 1:
    for (i=0; i<mbs; i++) {
      odiag    = v + 1*diag_offset[i];
      diag[0]  = odiag[0];
      mdiag[0] = odiag[0];
      
      if (PetscAbsScalar(diag[0] + shift) < PETSC_MACHINE_EPSILON) {
        if (allowzeropivot) {
          A->errortype = MAT_FACTOR_NUMERIC_ZEROPIVOT;
          ierr = PetscInfo1(A,"Zero pivot, row %D\n",i);CHKERRQ(ierr);
        } else SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_MAT_LU_ZRPVT,"Zero pivot, row %D",i);
      } 
      
      diag[0]  = (PetscScalar)1.0 / (diag[0] + shift);
      diag    += 1;
      mdiag   += 1;
    }
    break;
  case 2:
    for (i=0; i<mbs; i++) {
      odiag    = v + 4*diag_offset[i];
      diag[0]  = odiag[0]; diag[1] = odiag[1]; diag[2] = odiag[2]; diag[3] = odiag[3];
      mdiag[0] = odiag[0]; mdiag[1] = odiag[1]; mdiag[2] = odiag[2]; mdiag[3] = odiag[3];
      ierr     = PetscKernel_A_gets_inverse_A_2(diag,shift,allowzeropivot,&zeropivotdetected);CHKERRQ(ierr);
      if (zeropivotdetected) A->errortype = MAT_FACTOR_NUMERIC_ZEROPIVOT;
      diag    += 4;
      mdiag   += 4;
    }
    break;
  case 3:
    for (i=0; i<mbs; i++) {
      odiag    = v + 9*diag_offset[i];
      diag[0]  = odiag[0]; diag[1] = odiag[1]; diag[2] = odiag[2]; diag[3] = odiag[3];
      diag[4]  = odiag[4]; diag[5] = odiag[5]; diag[6] = odiag[6]; diag[7] = odiag[7];
      diag[8]  = odiag[8];
      mdiag[0] = odiag[0]; mdiag[1] = odiag[1]; mdiag[2] = odiag[2]; mdiag[3] = odiag[3];
      mdiag[4] = odiag[4]; mdiag[5] = odiag[5]; mdiag[6] = odiag[6]; mdiag[7] = odiag[7];
      mdiag[8] = odiag[8];
      ierr     = PetscKernel_A_gets_inverse_A_3(diag,shift,allowzeropivot,&zeropivotdetected);CHKERRQ(ierr);
      if (zeropivotdetected) A->errortype = MAT_FACTOR_NUMERIC_ZEROPIVOT;
      diag    += 9;
      mdiag   += 9;
    }
    break;
  case 4:
    for (i=0; i<mbs; i++) {
      odiag  = v + 16*diag_offset[i];
      ierr   = PetscMemcpy(diag,odiag,16*sizeof(PetscScalar));CHKERRQ(ierr);
      ierr   = PetscMemcpy(mdiag,odiag,16*sizeof(PetscScalar));CHKERRQ(ierr);
      ierr   = PetscKernel_A_gets_inverse_A_4(diag,shift,allowzeropivot,&zeropivotdetected);CHKERRQ(ierr);
      if (zeropivotdetected) A->errortype = MAT_FACTOR_NUMERIC_ZEROPIVOT;
      diag  += 16;
      mdiag += 16;
    }
    break;
  case 5:
    for (i=0; i<mbs; i++) {
      odiag  = v + 25*diag_offset[i];
      ierr   = PetscMemcpy(diag,odiag,25*sizeof(PetscScalar));CHKERRQ(ierr);
      ierr   = PetscMemcpy(mdiag,odiag,25*sizeof(PetscScalar));CHKERRQ(ierr);
      ierr   = PetscKernel_A_gets_inverse_A_5(diag,ipvt,work,shift,allowzeropivot,&zeropivotdetected);CHKERRQ(ierr);
      if (zeropivotdetected) A->errortype = MAT_FACTOR_NUMERIC_ZEROPIVOT;
      diag  += 25;
      mdiag += 25;
    }
    break;
  case 6:
    for (i=0; i<mbs; i++) {
      odiag  = v + 36*diag_offset[i];
      ierr   = PetscMemcpy(diag,odiag,36*sizeof(PetscScalar));CHKERRQ(ierr);
      ierr   = PetscMemcpy(mdiag,odiag,36*sizeof(PetscScalar));CHKERRQ(ierr);
      ierr   = PetscKernel_A_gets_inverse_A_6(diag,shift,allowzeropivot,&zeropivotdetected);CHKERRQ(ierr);
      if (zeropivotdetected) A->errortype = MAT_FACTOR_NUMERIC_ZEROPIVOT;
      diag  += 36;
      mdiag += 36;
    }
    break;
  case 7:
    for (i=0; i<mbs; i++) {
      odiag  = v + 49*diag_offset[i];
      ierr   = PetscMemcpy(diag,odiag,49*sizeof(PetscScalar));CHKERRQ(ierr);
      ierr   = PetscMemcpy(mdiag,odiag,49*sizeof(PetscScalar));CHKERRQ(ierr);
      ierr   = PetscKernel_A_gets_inverse_A_7(diag,shift,allowzeropivot,&zeropivotdetected);CHKERRQ(ierr);
      if (zeropivotdetected) A->errortype = MAT_FACTOR_NUMERIC_ZEROPIVOT;
      diag  += 49;
      mdiag += 49;
    }
    break;
  default:
    ierr = PetscMalloc2(bs,&v_work,bs,&v_pivots);CHKERRQ(ierr);
    for (i=0; i<mbs; i++) {
      odiag  = v + bs2*diag_offset[i];
      ierr   = PetscMemcpy(diag,odiag,bs2*sizeof(PetscScalar));CHKERRQ(ierr);
      ierr   = PetscMemcpy(mdiag,odiag,bs2*sizeof(PetscScalar));CHKERRQ(ierr);
      ierr   = PetscKernel_A_gets_inverse_A(bs,diag,v_pivots,v_work,allowzeropivot,&zeropivotdetected);CHKERRQ(ierr);
      if (zeropivotdetected) A->errortype = MAT_FACTOR_NUMERIC_ZEROPIVOT;
      diag  += bs2;
      mdiag += bs2;
    }
    ierr = PetscFree2(v_work,v_pivots);CHKERRQ(ierr);
  }
  a->idiagvalid = PETSC_TRUE;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSOR_SeqBAIJ"
PetscErrorCode MatSOR_SeqBAIJ(Mat A,Vec bb,PetscReal omega,MatSORType flag,PetscReal fshift,PetscInt its,PetscInt lits,Vec xx)
{
  Mat_SeqBAIJ       *a = (Mat_SeqBAIJ*)A->data;
  PetscScalar       *x,*work,*w,*workt,*t;
  const MatScalar   *v,*aa = a->a, *idiag;
  const PetscScalar *b,*xb;
  PetscScalar       s[7], xw[7]={0}; /* avoid some compilers thinking xw is uninitialized */
  PetscErrorCode    ierr;
  PetscInt          m = a->mbs,i,i2,nz,bs = A->rmap->bs,bs2 = bs*bs,k,j,idx,it;
  const PetscInt    *diag,*ai = a->i,*aj = a->j,*vi;

  PetscFunctionBegin;
  if (fshift == -1.0) fshift = 0.0; /* negative fshift indicates do not error on zero diagonal; this code never errors on zero diagonal */
  its = its*lits;
  if (flag & SOR_EISENSTAT) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SUP,"No support yet for Eisenstat");
  if (its <= 0) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONG,"Relaxation requires global its %D and local its %D both positive",its,lits);
  if (fshift) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SUP,"Sorry, no support for diagonal shift");
  if (omega != 1.0) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SUP,"Sorry, no support for non-trivial relaxation factor");
  if ((flag & SOR_APPLY_UPPER) || (flag & SOR_APPLY_LOWER)) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SUP,"Sorry, no support for applying upper or lower triangular parts");

  if (!a->idiagvalid) {ierr = MatInvertBlockDiagonal(A,NULL);CHKERRQ(ierr);}

  if (!m) PetscFunctionReturn(0);
  diag  = a->diag;
  idiag = a->idiag;
  k    = PetscMax(A->rmap->n,A->cmap->n);
  if (!a->mult_work) {
    ierr = PetscMalloc1(k+1,&a->mult_work);CHKERRQ(ierr);
  }
  if (!a->sor_workt) {
    ierr = PetscMalloc1(k,&a->sor_workt);CHKERRQ(ierr);
  }
  if (!a->sor_work) {
    ierr = PetscMalloc1(bs,&a->sor_work);CHKERRQ(ierr);
  }
  work = a->mult_work;
  t    = a->sor_workt;
  w    = a->sor_work;

  ierr = VecGetArray(xx,&x);CHKERRQ(ierr);
  ierr = VecGetArrayRead(bb,&b);CHKERRQ(ierr);

  if (flag & SOR_ZERO_INITIAL_GUESS) {
    if (flag & SOR_FORWARD_SWEEP || flag & SOR_LOCAL_FORWARD_SWEEP) {
      switch (bs) {
      case 1:
        PetscKernel_v_gets_A_times_w_1(x,idiag,b);
        t[0] = b[0];
        i2     = 1;
        idiag += 1;
        for (i=1; i<m; i++) {
          v  = aa + ai[i];
          vi = aj + ai[i];
          nz = diag[i] - ai[i];
          s[0] = b[i2];
          for (j=0; j<nz; j++) {
            xw[0] = x[vi[j]];
            PetscKernel_v_gets_v_minus_A_times_w_1(s,(v+j),xw);
          }
          t[i2] = s[0];
          PetscKernel_v_gets_A_times_w_1(xw,idiag,s);
          x[i2]  = xw[0];
          idiag += 1;
          i2    += 1;
        }
        break;
      case 2:
        PetscKernel_v_gets_A_times_w_2(x,idiag,b);
        t[0] = b[0]; t[1] = b[1];
        i2     = 2;
        idiag += 4;
        for (i=1; i<m; i++) {
          v  = aa + 4*ai[i];
          vi = aj + ai[i];
          nz = diag[i] - ai[i];
          s[0] = b[i2]; s[1] = b[i2+1];
          for (j=0; j<nz; j++) {
            idx = 2*vi[j];
            it  = 4*j;
            xw[0] = x[idx]; xw[1] = x[1+idx];
            PetscKernel_v_gets_v_minus_A_times_w_2(s,(v+it),xw);
          }
          t[i2] = s[0]; t[i2+1] = s[1];
          PetscKernel_v_gets_A_times_w_2(xw,idiag,s);
          x[i2]   = xw[0]; x[i2+1] = xw[1];
          idiag  += 4;
          i2     += 2;
        }
        break;
      case 3:
        PetscKernel_v_gets_A_times_w_3(x,idiag,b);
        t[0] = b[0]; t[1] = b[1]; t[2] = b[2];
        i2     = 3;
        idiag += 9;
        for (i=1; i<m; i++) {
          v  = aa + 9*ai[i];
          vi = aj + ai[i];
          nz = diag[i] - ai[i];
          s[0] = b[i2]; s[1] = b[i2+1]; s[2] = b[i2+2];
          while (nz--) {
            idx = 3*(*vi++);
            xw[0] = x[idx]; xw[1] = x[1+idx]; xw[2] = x[2+idx];
            PetscKernel_v_gets_v_minus_A_times_w_3(s,v,xw);
            v  += 9;
          }
          t[i2] = s[0]; t[i2+1] = s[1]; t[i2+2] = s[2];
          PetscKernel_v_gets_A_times_w_3(xw,idiag,s);
          x[i2] = xw[0]; x[i2+1] = xw[1]; x[i2+2] = xw[2];
          idiag  += 9;
          i2     += 3;
        }
        break;
      case 4:
        PetscKernel_v_gets_A_times_w_4(x,idiag,b);
        t[0] = b[0]; t[1] = b[1]; t[2] = b[2]; t[3] = b[3];
        i2     = 4;
        idiag += 16;
        for (i=1; i<m; i++) {
          v  = aa + 16*ai[i];
          vi = aj + ai[i];
          nz = diag[i] - ai[i];
          s[0] = b[i2]; s[1] = b[i2+1]; s[2] = b[i2+2]; s[3] = b[i2+3];
          while (nz--) {
            idx = 4*(*vi++);
            xw[0]  = x[idx]; xw[1] = x[1+idx]; xw[2] = x[2+idx]; xw[3] = x[3+idx];
            PetscKernel_v_gets_v_minus_A_times_w_4(s,v,xw);
            v  += 16;
          }
          t[i2] = s[0]; t[i2+1] = s[1]; t[i2+2] = s[2]; t[i2 + 3] = s[3];
          PetscKernel_v_gets_A_times_w_4(xw,idiag,s);
          x[i2] = xw[0]; x[i2+1] = xw[1]; x[i2+2] = xw[2]; x[i2+3] = xw[3];
          idiag  += 16;
          i2     += 4;
        }
        break;
      case 5:
        PetscKernel_v_gets_A_times_w_5(x,idiag,b);
        t[0] = b[0]; t[1] = b[1]; t[2] = b[2]; t[3] = b[3]; t[4] = b[4];
        i2     = 5;
        idiag += 25;
        for (i=1; i<m; i++) {
          v  = aa + 25*ai[i];
          vi = aj + ai[i];
          nz = diag[i] - ai[i];
          s[0] = b[i2]; s[1] = b[i2+1]; s[2] = b[i2+2]; s[3] = b[i2+3]; s[4] = b[i2+4];
          while (nz--) {
            idx = 5*(*vi++);
            xw[0]  = x[idx]; xw[1] = x[1+idx]; xw[2] = x[2+idx]; xw[3] = x[3+idx]; xw[4] = x[4+idx];
            PetscKernel_v_gets_v_minus_A_times_w_5(s,v,xw);
            v  += 25;
          }
          t[i2] = s[0]; t[i2+1] = s[1]; t[i2+2] = s[2]; t[i2+3] = s[3]; t[i2+4] = s[4];
          PetscKernel_v_gets_A_times_w_5(xw,idiag,s);
          x[i2] = xw[0]; x[i2+1] = xw[1]; x[i2+2] = xw[2]; x[i2+3] = xw[3]; x[i2+4] = xw[4];
          idiag  += 25;
          i2     += 5;
        }
        break;
      case 6:
        PetscKernel_v_gets_A_times_w_6(x,idiag,b);
        t[0] = b[0]; t[1] = b[1]; t[2] = b[2]; t[3] = b[3]; t[4] = b[4]; t[5] = b[5];
        i2     = 6;
        idiag += 36;
        for (i=1; i<m; i++) {
          v  = aa + 36*ai[i];
          vi = aj + ai[i];
          nz = diag[i] - ai[i];
          s[0] = b[i2]; s[1] = b[i2+1]; s[2] = b[i2+2]; s[3] = b[i2+3]; s[4] = b[i2+4]; s[5] = b[i2+5];
          while (nz--) {
            idx = 6*(*vi++);
            xw[0] = x[idx];   xw[1] = x[1+idx]; xw[2] = x[2+idx];
            xw[3] = x[3+idx]; xw[4] = x[4+idx]; xw[5] = x[5+idx];
            PetscKernel_v_gets_v_minus_A_times_w_6(s,v,xw);
            v  += 36;
          }
          t[i2]   = s[0]; t[i2+1] = s[1]; t[i2+2] = s[2];
          t[i2+3] = s[3]; t[i2+4] = s[4]; t[i2+5] = s[5];
          PetscKernel_v_gets_A_times_w_6(xw,idiag,s);
          x[i2] = xw[0]; x[i2+1] = xw[1]; x[i2+2] = xw[2]; x[i2+3] = xw[3]; x[i2+4] = xw[4]; x[i2+5] = xw[5];
          idiag  += 36;
          i2     += 6;
        }
        break;
      case 7:
        PetscKernel_v_gets_A_times_w_7(x,idiag,b);
        t[0] = b[0]; t[1] = b[1]; t[2] = b[2];
        t[3] = b[3]; t[4] = b[4]; t[5] = b[5]; t[6] = b[6];
        i2     = 7;
        idiag += 49;
        for (i=1; i<m; i++) {
          v  = aa + 49*ai[i];
          vi = aj + ai[i];
          nz = diag[i] - ai[i];
          s[0] = b[i2];   s[1] = b[i2+1]; s[2] = b[i2+2];
          s[3] = b[i2+3]; s[4] = b[i2+4]; s[5] = b[i2+5]; s[6] = b[i2+6];
          while (nz--) {
            idx = 7*(*vi++);
            xw[0] = x[idx];   xw[1] = x[1+idx]; xw[2] = x[2+idx];
            xw[3] = x[3+idx]; xw[4] = x[4+idx]; xw[5] = x[5+idx]; xw[6] = x[6+idx];
            PetscKernel_v_gets_v_minus_A_times_w_7(s,v,xw);
            v  += 49;
          }
          t[i2]   = s[0]; t[i2+1] = s[1]; t[i2+2] = s[2];
          t[i2+3] = s[3]; t[i2+4] = s[4]; t[i2+5] = s[5]; t[i2+6] = s[6];
          PetscKernel_v_gets_A_times_w_7(xw,idiag,s);
          x[i2] =   xw[0]; x[i2+1] = xw[1]; x[i2+2] = xw[2];
          x[i2+3] = xw[3]; x[i2+4] = xw[4]; x[i2+5] = xw[5]; x[i2+6] = xw[6];
          idiag  += 49;
          i2     += 7;
        }
        break;
      default:
        PetscKernel_w_gets_Ar_times_v(bs,bs,b,idiag,x);
        ierr = PetscMemcpy(t,b,bs*sizeof(PetscScalar));CHKERRQ(ierr);
        i2     = bs;
        idiag += bs2;
        for (i=1; i<m; i++) {
          v  = aa + bs2*ai[i];
          vi = aj + ai[i];
          nz = diag[i] - ai[i];

          ierr = PetscMemcpy(w,b+i2,bs*sizeof(PetscScalar));CHKERRQ(ierr);
          /* copy all rows of x that are needed into contiguous space */
          workt = work;
          for (j=0; j<nz; j++) {
            ierr   = PetscMemcpy(workt,x + bs*(*vi++),bs*sizeof(PetscScalar));CHKERRQ(ierr);
            workt += bs;
          }
          PetscKernel_w_gets_w_minus_Ar_times_v(bs,bs*nz,w,v,work);
          ierr = PetscMemcpy(t+i2,w,bs*sizeof(PetscScalar));CHKERRQ(ierr);
          PetscKernel_w_gets_Ar_times_v(bs,bs,w,idiag,x+i2);

          idiag += bs2;
          i2    += bs;
        }
        break;
      }
      /* for logging purposes assume number of nonzero in lower half is 1/2 of total */
      ierr = PetscLogFlops(1.0*bs2*a->nz);CHKERRQ(ierr);
      xb = t;
    }
    else xb = b;
    if (flag & SOR_BACKWARD_SWEEP || flag & SOR_LOCAL_BACKWARD_SWEEP) {
      idiag = a->idiag+bs2*(a->mbs-1);
      i2 = bs * (m-1);
      switch (bs) {
      case 1:
        s[0]  = xb[i2];
        PetscKernel_v_gets_A_times_w_1(xw,idiag,s);
        x[i2] = xw[0];
        i2   -= 1;
        for (i=m-2; i>=0; i--) {
          v  = aa + (diag[i]+1);
          vi = aj + diag[i] + 1;
          nz = ai[i+1] - diag[i] - 1;
          s[0] = xb[i2];
          for (j=0; j<nz; j++) {
            xw[0] = x[vi[j]];
            PetscKernel_v_gets_v_minus_A_times_w_1(s,(v+j),xw);
          }
          PetscKernel_v_gets_A_times_w_1(xw,idiag,s);
          x[i2]  = xw[0];
          idiag -= 1;
          i2    -= 1;
        }
        break;
      case 2:
        s[0]  = xb[i2]; s[1] = xb[i2+1];
        PetscKernel_v_gets_A_times_w_2(xw,idiag,s);
        x[i2] = xw[0]; x[i2+1] = xw[1];
        i2    -= 2;
        idiag -= 4;
        for (i=m-2; i>=0; i--) {
          v  = aa + 4*(diag[i] + 1);
          vi = aj + diag[i] + 1;
          nz = ai[i+1] - diag[i] - 1;
          s[0] = xb[i2]; s[1] = xb[i2+1];
          for (j=0; j<nz; j++) {
            idx = 2*vi[j];
            it  = 4*j;
            xw[0] = x[idx]; xw[1] = x[1+idx];
            PetscKernel_v_gets_v_minus_A_times_w_2(s,(v+it),xw);
          }
          PetscKernel_v_gets_A_times_w_2(xw,idiag,s);
          x[i2]   = xw[0]; x[i2+1] = xw[1];
          idiag  -= 4;
          i2     -= 2;
        }
        break;
      case 3:
        s[0]  = xb[i2]; s[1] = xb[i2+1]; s[2] = xb[i2+2];
        PetscKernel_v_gets_A_times_w_3(xw,idiag,s);
        x[i2] = xw[0]; x[i2+1] = xw[1]; x[i2+2] = xw[2];
        i2    -= 3;
        idiag -= 9;
        for (i=m-2; i>=0; i--) {
          v  = aa + 9*(diag[i]+1);
          vi = aj + diag[i] + 1;
          nz = ai[i+1] - diag[i] - 1;
          s[0] = xb[i2]; s[1] = xb[i2+1]; s[2] = xb[i2+2];
          while (nz--) {
            idx = 3*(*vi++);
            xw[0] = x[idx]; xw[1] = x[1+idx]; xw[2] = x[2+idx];
            PetscKernel_v_gets_v_minus_A_times_w_3(s,v,xw);
            v  += 9;
          }
          PetscKernel_v_gets_A_times_w_3(xw,idiag,s);
          x[i2] = xw[0]; x[i2+1] = xw[1]; x[i2+2] = xw[2];
          idiag  -= 9;
          i2     -= 3;
        }
        break;
      case 4:
        s[0]  = xb[i2]; s[1] = xb[i2+1]; s[2] = xb[i2+2]; s[3] = xb[i2+3];
        PetscKernel_v_gets_A_times_w_4(xw,idiag,s);
        x[i2] = xw[0]; x[i2+1] = xw[1]; x[i2+2] = xw[2]; x[i2+3] = xw[3];
        i2    -= 4;
        idiag -= 16;
        for (i=m-2; i>=0; i--) {
          v  = aa + 16*(diag[i]+1);
          vi = aj + diag[i] + 1;
          nz = ai[i+1] - diag[i] - 1;
          s[0] = xb[i2]; s[1] = xb[i2+1]; s[2] = xb[i2+2]; s[3] = xb[i2+3];
          while (nz--) {
            idx = 4*(*vi++);
            xw[0]  = x[idx]; xw[1] = x[1+idx]; xw[2] = x[2+idx]; xw[3] = x[3+idx];
            PetscKernel_v_gets_v_minus_A_times_w_4(s,v,xw);
            v  += 16;
          }
          PetscKernel_v_gets_A_times_w_4(xw,idiag,s);
          x[i2] = xw[0]; x[i2+1] = xw[1]; x[i2+2] = xw[2]; x[i2+3] = xw[3];
          idiag  -= 16;
          i2     -= 4;
        }
        break;
      case 5:
        s[0]  = xb[i2]; s[1] = xb[i2+1]; s[2] = xb[i2+2]; s[3] = xb[i2+3]; s[4] = xb[i2+4];
        PetscKernel_v_gets_A_times_w_5(xw,idiag,s);
        x[i2] = xw[0]; x[i2+1] = xw[1]; x[i2+2] = xw[2]; x[i2+3] = xw[3]; x[i2+4] = xw[4];
        i2    -= 5;
        idiag -= 25;
        for (i=m-2; i>=0; i--) {
          v  = aa + 25*(diag[i]+1);
          vi = aj + diag[i] + 1;
          nz = ai[i+1] - diag[i] - 1;
          s[0] = xb[i2]; s[1] = xb[i2+1]; s[2] = xb[i2+2]; s[3] = xb[i2+3]; s[4] = xb[i2+4];
          while (nz--) {
            idx = 5*(*vi++);
            xw[0]  = x[idx]; xw[1] = x[1+idx]; xw[2] = x[2+idx]; xw[3] = x[3+idx]; xw[4] = x[4+idx];
            PetscKernel_v_gets_v_minus_A_times_w_5(s,v,xw);
            v  += 25;
          }
          PetscKernel_v_gets_A_times_w_5(xw,idiag,s);
          x[i2] = xw[0]; x[i2+1] = xw[1]; x[i2+2] = xw[2]; x[i2+3] = xw[3]; x[i2+4] = xw[4];
          idiag  -= 25;
          i2     -= 5;
        }
        break;
      case 6:
        s[0]  = xb[i2]; s[1] = xb[i2+1]; s[2] = xb[i2+2]; s[3] = xb[i2+3]; s[4] = xb[i2+4]; s[5] = xb[i2+5];
        PetscKernel_v_gets_A_times_w_6(xw,idiag,s);
        x[i2] = xw[0]; x[i2+1] = xw[1]; x[i2+2] = xw[2]; x[i2+3] = xw[3]; x[i2+4] = xw[4]; x[i2+5] = xw[5];
        i2    -= 6;
        idiag -= 36;
        for (i=m-2; i>=0; i--) {
          v  = aa + 36*(diag[i]+1);
          vi = aj + diag[i] + 1;
          nz = ai[i+1] - diag[i] - 1;
          s[0] = xb[i2]; s[1] = xb[i2+1]; s[2] = xb[i2+2]; s[3] = xb[i2+3]; s[4] = xb[i2+4]; s[5] = xb[i2+5];
          while (nz--) {
            idx = 6*(*vi++);
            xw[0] = x[idx];   xw[1] = x[1+idx]; xw[2] = x[2+idx];
            xw[3] = x[3+idx]; xw[4] = x[4+idx]; xw[5] = x[5+idx];
            PetscKernel_v_gets_v_minus_A_times_w_6(s,v,xw);
            v  += 36;
          }
          PetscKernel_v_gets_A_times_w_6(xw,idiag,s);
          x[i2] = xw[0]; x[i2+1] = xw[1]; x[i2+2] = xw[2]; x[i2+3] = xw[3]; x[i2+4] = xw[4]; x[i2+5] = xw[5];
          idiag  -= 36;
          i2     -= 6;
        }
        break;
      case 7:
        s[0] = xb[i2];   s[1] = xb[i2+1]; s[2] = xb[i2+2];
        s[3] = xb[i2+3]; s[4] = xb[i2+4]; s[5] = xb[i2+5]; s[6] = xb[i2+6];
        PetscKernel_v_gets_A_times_w_7(x,idiag,b);
        x[i2]   = xw[0]; x[i2+1] = xw[1]; x[i2+2] = xw[2];
        x[i2+3] = xw[3]; x[i2+4] = xw[4]; x[i2+5] = xw[5]; x[i2+6] = xw[6];
        i2    -= 7;
        idiag -= 49;
        for (i=m-2; i>=0; i--) {
          v  = aa + 49*(diag[i]+1);
          vi = aj + diag[i] + 1;
          nz = ai[i+1] - diag[i] - 1;
          s[0] = xb[i2];   s[1] = xb[i2+1]; s[2] = xb[i2+2];
          s[3] = xb[i2+3]; s[4] = xb[i2+4]; s[5] = xb[i2+5]; s[6] = xb[i2+6];
          while (nz--) {
            idx = 7*(*vi++);
            xw[0] = x[idx];   xw[1] = x[1+idx]; xw[2] = x[2+idx];
            xw[3] = x[3+idx]; xw[4] = x[4+idx]; xw[5] = x[5+idx]; xw[6] = x[6+idx];
            PetscKernel_v_gets_v_minus_A_times_w_7(s,v,xw);
            v  += 49;
          }
          PetscKernel_v_gets_A_times_w_7(xw,idiag,s);
          x[i2] =   xw[0]; x[i2+1] = xw[1]; x[i2+2] = xw[2];
          x[i2+3] = xw[3]; x[i2+4] = xw[4]; x[i2+5] = xw[5]; x[i2+6] = xw[6];
          idiag  -= 49;
          i2     -= 7;
        }
        break;
      default:
        ierr  = PetscMemcpy(w,xb+i2,bs*sizeof(PetscScalar));CHKERRQ(ierr);
        PetscKernel_w_gets_Ar_times_v(bs,bs,w,idiag,x+i2);
        i2    -= bs;
        idiag -= bs2;
        for (i=m-2; i>=0; i--) {
          v  = aa + bs2*(diag[i]+1);
          vi = aj + diag[i] + 1;
          nz = ai[i+1] - diag[i] - 1;

          ierr = PetscMemcpy(w,xb+i2,bs*sizeof(PetscScalar));CHKERRQ(ierr);
          /* copy all rows of x that are needed into contiguous space */
          workt = work;
          for (j=0; j<nz; j++) {
            ierr   = PetscMemcpy(workt,x + bs*(*vi++),bs*sizeof(PetscScalar));CHKERRQ(ierr);
            workt += bs;
          }
          PetscKernel_w_gets_w_minus_Ar_times_v(bs,bs*nz,w,v,work);
          PetscKernel_w_gets_Ar_times_v(bs,bs,w,idiag,x+i2);

          idiag -= bs2;
          i2    -= bs;
        }
        break;
      }
      ierr = PetscLogFlops(1.0*bs2*(a->nz));CHKERRQ(ierr);
    }
    its--;
  }
  while (its--) {
    if (flag & SOR_FORWARD_SWEEP || flag & SOR_LOCAL_FORWARD_SWEEP) {
      idiag = a->idiag;
      i2 = 0;
      switch (bs) {
      case 1:
        for (i=0; i<m; i++) {
          v  = aa + ai[i];
          vi = aj + ai[i];
          nz = ai[i+1] - ai[i];
          s[0] = b[i2];
          for (j=0; j<nz; j++) {
            xw[0] = x[vi[j]];
            PetscKernel_v_gets_v_minus_A_times_w_1(s,(v+j),xw);
          }
          PetscKernel_v_gets_A_times_w_1(xw,idiag,s);
          x[i2] += xw[0];
          idiag += 1;
          i2    += 1;
        }
        break;
      case 2:
        for (i=0; i<m; i++) {
          v  = aa + 4*ai[i];
          vi = aj + ai[i];
          nz = ai[i+1] - ai[i];
          s[0] = b[i2]; s[1] = b[i2+1];
          for (j=0; j<nz; j++) {
            idx = 2*vi[j];
            it  = 4*j;
            xw[0] = x[idx]; xw[1] = x[1+idx];
            PetscKernel_v_gets_v_minus_A_times_w_2(s,(v+it),xw);
          }
          PetscKernel_v_gets_A_times_w_2(xw,idiag,s);
          x[i2]  += xw[0]; x[i2+1] += xw[1];
          idiag  += 4;
          i2     += 2;
        }
        break;
      case 3:
        for (i=0; i<m; i++) {
          v  = aa + 9*ai[i];
          vi = aj + ai[i];
          nz = ai[i+1] - ai[i];
          s[0] = b[i2]; s[1] = b[i2+1]; s[2] = b[i2+2];
          while (nz--) {
            idx = 3*(*vi++);
            xw[0] = x[idx]; xw[1] = x[1+idx]; xw[2] = x[2+idx];
            PetscKernel_v_gets_v_minus_A_times_w_3(s,v,xw);
            v  += 9;
          }
          PetscKernel_v_gets_A_times_w_3(xw,idiag,s);
          x[i2] += xw[0]; x[i2+1] += xw[1]; x[i2+2] += xw[2];
          idiag  += 9;
          i2     += 3;
        }
        break;
      case 4:
        for (i=0; i<m; i++) {
          v  = aa + 16*ai[i];
          vi = aj + ai[i];
          nz = ai[i+1] - ai[i];
          s[0] = b[i2]; s[1] = b[i2+1]; s[2] = b[i2+2]; s[3] = b[i2+3];
          while (nz--) {
            idx = 4*(*vi++);
            xw[0]  = x[idx]; xw[1] = x[1+idx]; xw[2] = x[2+idx]; xw[3] = x[3+idx];
            PetscKernel_v_gets_v_minus_A_times_w_4(s,v,xw);
            v  += 16;
          }
          PetscKernel_v_gets_A_times_w_4(xw,idiag,s);
          x[i2] += xw[0]; x[i2+1] += xw[1]; x[i2+2] += xw[2]; x[i2+3] += xw[3];
          idiag  += 16;
          i2     += 4;
        }
        break;
      case 5:
        for (i=0; i<m; i++) {
          v  = aa + 25*ai[i];
          vi = aj + ai[i];
          nz = ai[i+1] - ai[i];
          s[0] = b[i2]; s[1] = b[i2+1]; s[2] = b[i2+2]; s[3] = b[i2+3]; s[4] = b[i2+4];
          while (nz--) {
            idx = 5*(*vi++);
            xw[0]  = x[idx]; xw[1] = x[1+idx]; xw[2] = x[2+idx]; xw[3] = x[3+idx]; xw[4] = x[4+idx];
            PetscKernel_v_gets_v_minus_A_times_w_5(s,v,xw);
            v  += 25;
          }
          PetscKernel_v_gets_A_times_w_5(xw,idiag,s);
          x[i2] += xw[0]; x[i2+1] += xw[1]; x[i2+2] += xw[2]; x[i2+3] += xw[3]; x[i2+4] += xw[4];
          idiag  += 25;
          i2     += 5;
        }
        break;
      case 6:
        for (i=0; i<m; i++) {
          v  = aa + 36*ai[i];
          vi = aj + ai[i];
          nz = ai[i+1] - ai[i];
          s[0] = b[i2]; s[1] = b[i2+1]; s[2] = b[i2+2]; s[3] = b[i2+3]; s[4] = b[i2+4]; s[5] = b[i2+5];
          while (nz--) {
            idx = 6*(*vi++);
            xw[0] = x[idx];   xw[1] = x[1+idx]; xw[2] = x[2+idx];
            xw[3] = x[3+idx]; xw[4] = x[4+idx]; xw[5] = x[5+idx];
            PetscKernel_v_gets_v_minus_A_times_w_6(s,v,xw);
            v  += 36;
          }
          PetscKernel_v_gets_A_times_w_6(xw,idiag,s);
          x[i2] += xw[0]; x[i2+1] += xw[1]; x[i2+2] += xw[2];
          x[i2+3] += xw[3]; x[i2+4] += xw[4]; x[i2+5] += xw[5];
          idiag  += 36;
          i2     += 6;
        }
        break;
      case 7:
        for (i=0; i<m; i++) {
          v  = aa + 49*ai[i];
          vi = aj + ai[i];
          nz = ai[i+1] - ai[i];
          s[0] = b[i2];   s[1] = b[i2+1]; s[2] = b[i2+2];
          s[3] = b[i2+3]; s[4] = b[i2+4]; s[5] = b[i2+5]; s[6] = b[i2+6];
          while (nz--) {
            idx = 7*(*vi++);
            xw[0] = x[idx];   xw[1] = x[1+idx]; xw[2] = x[2+idx];
            xw[3] = x[3+idx]; xw[4] = x[4+idx]; xw[5] = x[5+idx]; xw[6] = x[6+idx];
            PetscKernel_v_gets_v_minus_A_times_w_7(s,v,xw);
            v  += 49;
          }
          PetscKernel_v_gets_A_times_w_7(xw,idiag,s);
          x[i2]   += xw[0]; x[i2+1] += xw[1]; x[i2+2] += xw[2];
          x[i2+3] += xw[3]; x[i2+4] += xw[4]; x[i2+5] += xw[5]; x[i2+6] += xw[6];
          idiag  += 49;
          i2     += 7;
        }
        break;
      default:
        for (i=0; i<m; i++) {
          v  = aa + bs2*ai[i];
          vi = aj + ai[i];
          nz = ai[i+1] - ai[i];

          ierr = PetscMemcpy(w,b+i2,bs*sizeof(PetscScalar));CHKERRQ(ierr);
          /* copy all rows of x that are needed into contiguous space */
          workt = work;
          for (j=0; j<nz; j++) {
            ierr   = PetscMemcpy(workt,x + bs*(*vi++),bs*sizeof(PetscScalar));CHKERRQ(ierr);
            workt += bs;
          }
          PetscKernel_w_gets_w_minus_Ar_times_v(bs,bs*nz,w,v,work);
          PetscKernel_w_gets_w_plus_Ar_times_v(bs,bs,w,idiag,x+i2);

          idiag += bs2;
          i2    += bs;
        }
        break;
      }
      ierr = PetscLogFlops(2.0*bs2*a->nz);CHKERRQ(ierr);
    }
    if (flag & SOR_BACKWARD_SWEEP || flag & SOR_LOCAL_BACKWARD_SWEEP) {
      idiag = a->idiag+bs2*(a->mbs-1);
      i2 = bs * (m-1);
      switch (bs) {
      case 1:
        for (i=m-1; i>=0; i--) {
          v  = aa + ai[i];
          vi = aj + ai[i];
          nz = ai[i+1] - ai[i];
          s[0] = b[i2];
          for (j=0; j<nz; j++) {
            xw[0] = x[vi[j]];
            PetscKernel_v_gets_v_minus_A_times_w_1(s,(v+j),xw);
          }
          PetscKernel_v_gets_A_times_w_1(xw,idiag,s);
          x[i2] += xw[0];
          idiag -= 1;
          i2    -= 1;
        }
        break;
      case 2:
        for (i=m-1; i>=0; i--) {
          v  = aa + 4*ai[i];
          vi = aj + ai[i];
          nz = ai[i+1] - ai[i];
          s[0] = b[i2]; s[1] = b[i2+1];
          for (j=0; j<nz; j++) {
            idx = 2*vi[j];
            it  = 4*j;
            xw[0] = x[idx]; xw[1] = x[1+idx];
            PetscKernel_v_gets_v_minus_A_times_w_2(s,(v+it),xw);
          }
          PetscKernel_v_gets_A_times_w_2(xw,idiag,s);
          x[i2]  += xw[0]; x[i2+1] += xw[1];
          idiag  -= 4;
          i2     -= 2;
        }
        break;
      case 3:
        for (i=m-1; i>=0; i--) {
          v  = aa + 9*ai[i];
          vi = aj + ai[i];
          nz = ai[i+1] - ai[i];
          s[0] = b[i2]; s[1] = b[i2+1]; s[2] = b[i2+2];
          while (nz--) {
            idx = 3*(*vi++);
            xw[0] = x[idx]; xw[1] = x[1+idx]; xw[2] = x[2+idx];
            PetscKernel_v_gets_v_minus_A_times_w_3(s,v,xw);
            v  += 9;
          }
          PetscKernel_v_gets_A_times_w_3(xw,idiag,s);
          x[i2] += xw[0]; x[i2+1] += xw[1]; x[i2+2] += xw[2];
          idiag  -= 9;
          i2     -= 3;
        }
        break;
      case 4:
        for (i=m-1; i>=0; i--) {
          v  = aa + 16*ai[i];
          vi = aj + ai[i];
          nz = ai[i+1] - ai[i];
          s[0] = b[i2]; s[1] = b[i2+1]; s[2] = b[i2+2]; s[3] = b[i2+3];
          while (nz--) {
            idx = 4*(*vi++);
            xw[0]  = x[idx]; xw[1] = x[1+idx]; xw[2] = x[2+idx]; xw[3] = x[3+idx];
            PetscKernel_v_gets_v_minus_A_times_w_4(s,v,xw);
            v  += 16;
          }
          PetscKernel_v_gets_A_times_w_4(xw,idiag,s);
          x[i2] += xw[0]; x[i2+1] += xw[1]; x[i2+2] += xw[2]; x[i2+3] += xw[3];
          idiag  -= 16;
          i2     -= 4;
        }
        break;
      case 5:
        for (i=m-1; i>=0; i--) {
          v  = aa + 25*ai[i];
          vi = aj + ai[i];
          nz = ai[i+1] - ai[i];
          s[0] = b[i2]; s[1] = b[i2+1]; s[2] = b[i2+2]; s[3] = b[i2+3]; s[4] = b[i2+4];
          while (nz--) {
            idx = 5*(*vi++);
            xw[0]  = x[idx]; xw[1] = x[1+idx]; xw[2] = x[2+idx]; xw[3] = x[3+idx]; xw[4] = x[4+idx];
            PetscKernel_v_gets_v_minus_A_times_w_5(s,v,xw);
            v  += 25;
          }
          PetscKernel_v_gets_A_times_w_5(xw,idiag,s);
          x[i2] += xw[0]; x[i2+1] += xw[1]; x[i2+2] += xw[2]; x[i2+3] += xw[3]; x[i2+4] += xw[4];
          idiag  -= 25;
          i2     -= 5;
        }
        break;
      case 6:
        for (i=m-1; i>=0; i--) {
          v  = aa + 36*ai[i];
          vi = aj + ai[i];
          nz = ai[i+1] - ai[i];
          s[0] = b[i2]; s[1] = b[i2+1]; s[2] = b[i2+2]; s[3] = b[i2+3]; s[4] = b[i2+4]; s[5] = b[i2+5];
          while (nz--) {
            idx = 6*(*vi++);
            xw[0] = x[idx];   xw[1] = x[1+idx]; xw[2] = x[2+idx];
            xw[3] = x[3+idx]; xw[4] = x[4+idx]; xw[5] = x[5+idx];
            PetscKernel_v_gets_v_minus_A_times_w_6(s,v,xw);
            v  += 36;
          }
          PetscKernel_v_gets_A_times_w_6(xw,idiag,s);
          x[i2] += xw[0]; x[i2+1] += xw[1]; x[i2+2] += xw[2];
          x[i2+3] += xw[3]; x[i2+4] += xw[4]; x[i2+5] += xw[5];
          idiag  -= 36;
          i2     -= 6;
        }
        break;
      case 7:
        for (i=m-1; i>=0; i--) {
          v  = aa + 49*ai[i];
          vi = aj + ai[i];
          nz = ai[i+1] - ai[i];
          s[0] = b[i2];   s[1] = b[i2+1]; s[2] = b[i2+2];
          s[3] = b[i2+3]; s[4] = b[i2+4]; s[5] = b[i2+5]; s[6] = b[i2+6];
          while (nz--) {
            idx = 7*(*vi++);
            xw[0] = x[idx];   xw[1] = x[1+idx]; xw[2] = x[2+idx];
            xw[3] = x[3+idx]; xw[4] = x[4+idx]; xw[5] = x[5+idx]; xw[6] = x[6+idx];
            PetscKernel_v_gets_v_minus_A_times_w_7(s,v,xw);
            v  += 49;
          }
          PetscKernel_v_gets_A_times_w_7(xw,idiag,s);
          x[i2] +=   xw[0]; x[i2+1] += xw[1]; x[i2+2] += xw[2];
          x[i2+3] += xw[3]; x[i2+4] += xw[4]; x[i2+5] += xw[5]; x[i2+6] += xw[6];
          idiag  -= 49;
          i2     -= 7;
        }
        break;
      default:
        for (i=m-1; i>=0; i--) {
          v  = aa + bs2*ai[i];
          vi = aj + ai[i];
          nz = ai[i+1] - ai[i];

          ierr = PetscMemcpy(w,b+i2,bs*sizeof(PetscScalar));CHKERRQ(ierr);
          /* copy all rows of x that are needed into contiguous space */
          workt = work;
          for (j=0; j<nz; j++) {
            ierr   = PetscMemcpy(workt,x + bs*(*vi++),bs*sizeof(PetscScalar));CHKERRQ(ierr);
            workt += bs;
          }
          PetscKernel_w_gets_w_minus_Ar_times_v(bs,bs*nz,w,v,work);
          PetscKernel_w_gets_w_plus_Ar_times_v(bs,bs,w,idiag,x+i2);

          idiag -= bs2;
          i2    -= bs;
        }
        break;
      }
      ierr = PetscLogFlops(2.0*bs2*(a->nz));CHKERRQ(ierr);
    }
  }
  ierr = VecRestoreArray(xx,&x);CHKERRQ(ierr);
  ierr = VecRestoreArrayRead(bb,&b);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}


/*
    Special version for direct calls from Fortran (Used in PETSc-fun3d)
*/
#if defined(PETSC_HAVE_FORTRAN_CAPS)
#define matsetvaluesblocked4_ MATSETVALUESBLOCKED4
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE)
#define matsetvaluesblocked4_ matsetvaluesblocked4
#endif

#undef __FUNCT__
#define __FUNCT__ "matsetvaluesblocked4_"
PETSC_EXTERN void matsetvaluesblocked4_(Mat *AA,PetscInt *mm,const PetscInt im[],PetscInt *nn,const PetscInt in[],const PetscScalar v[])
{
  Mat               A  = *AA;
  Mat_SeqBAIJ       *a = (Mat_SeqBAIJ*)A->data;
  PetscInt          *rp,k,low,high,t,ii,jj,row,nrow,i,col,l,N,m = *mm,n = *nn;
  PetscInt          *ai    =a->i,*ailen=a->ilen;
  PetscInt          *aj    =a->j,stepval,lastcol = -1;
  const PetscScalar *value = v;
  MatScalar         *ap,*aa = a->a,*bap;

  PetscFunctionBegin;
  if (A->rmap->bs != 4) SETERRABORT(PetscObjectComm((PetscObject)A),PETSC_ERR_ARG_WRONG,"Can only be called with a block size of 4");
  stepval = (n-1)*4;
  for (k=0; k<m; k++) { /* loop over added rows */
    row  = im[k];
    rp   = aj + ai[row];
    ap   = aa + 16*ai[row];
    nrow = ailen[row];
    low  = 0;
    high = nrow;
    for (l=0; l<n; l++) { /* loop over added columns */
      col = in[l];
      if (col <= lastcol)  low = 0;
      else                high = nrow;
      lastcol = col;
      value   = v + k*(stepval+4 + l)*4;
      while (high-low > 7) {
        t = (low+high)/2;
        if (rp[t] > col) high = t;
        else             low  = t;
      }
      for (i=low; i<high; i++) {
        if (rp[i] > col) break;
        if (rp[i] == col) {
          bap = ap +  16*i;
          for (ii=0; ii<4; ii++,value+=stepval) {
            for (jj=ii; jj<16; jj+=4) {
              bap[jj] += *value++;
            }
          }
          goto noinsert2;
        }
      }
      N = nrow++ - 1;
      high++; /* added new column index thus must search to one higher than before */
      /* shift up all the later entries in this row */
      for (ii=N; ii>=i; ii--) {
        rp[ii+1] = rp[ii];
        PetscMemcpy(ap+16*(ii+1),ap+16*(ii),16*sizeof(MatScalar));
      }
      if (N >= i) {
        PetscMemzero(ap+16*i,16*sizeof(MatScalar));
      }
      rp[i] = col;
      bap   = ap +  16*i;
      for (ii=0; ii<4; ii++,value+=stepval) {
        for (jj=ii; jj<16; jj+=4) {
          bap[jj] = *value++;
        }
      }
      noinsert2:;
      low = i;
    }
    ailen[row] = nrow;
  }
  PetscFunctionReturnVoid();
}

#if defined(PETSC_HAVE_FORTRAN_CAPS)
#define matsetvalues4_ MATSETVALUES4
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE)
#define matsetvalues4_ matsetvalues4
#endif

#undef __FUNCT__
#define __FUNCT__ "MatSetValues4_"
PETSC_EXTERN void matsetvalues4_(Mat *AA,PetscInt *mm,PetscInt *im,PetscInt *nn,PetscInt *in,PetscScalar *v)
{
  Mat         A  = *AA;
  Mat_SeqBAIJ *a = (Mat_SeqBAIJ*)A->data;
  PetscInt    *rp,k,low,high,t,ii,row,nrow,i,col,l,N,n = *nn,m = *mm;
  PetscInt    *ai=a->i,*ailen=a->ilen;
  PetscInt    *aj=a->j,brow,bcol;
  PetscInt    ridx,cidx,lastcol = -1;
  MatScalar   *ap,value,*aa=a->a,*bap;

  PetscFunctionBegin;
  for (k=0; k<m; k++) { /* loop over added rows */
    row  = im[k]; brow = row/4;
    rp   = aj + ai[brow];
    ap   = aa + 16*ai[brow];
    nrow = ailen[brow];
    low  = 0;
    high = nrow;
    for (l=0; l<n; l++) { /* loop over added columns */
      col   = in[l]; bcol = col/4;
      ridx  = row % 4; cidx = col % 4;
      value = v[l + k*n];
      if (col <= lastcol)  low = 0;
      else                high = nrow;
      lastcol = col;
      while (high-low > 7) {
        t = (low+high)/2;
        if (rp[t] > bcol) high = t;
        else              low  = t;
      }
      for (i=low; i<high; i++) {
        if (rp[i] > bcol) break;
        if (rp[i] == bcol) {
          bap   = ap +  16*i + 4*cidx + ridx;
          *bap += value;
          goto noinsert1;
        }
      }
      N = nrow++ - 1;
      high++; /* added new column thus must search to one higher than before */
      /* shift up all the later entries in this row */
      for (ii=N; ii>=i; ii--) {
        rp[ii+1] = rp[ii];
        PetscMemcpy(ap+16*(ii+1),ap+16*(ii),16*sizeof(MatScalar));
      }
      if (N>=i) {
        PetscMemzero(ap+16*i,16*sizeof(MatScalar));
      }
      rp[i]                    = bcol;
      ap[16*i + 4*cidx + ridx] = value;
noinsert1:;
      low = i;
    }
    ailen[brow] = nrow;
  }
  PetscFunctionReturnVoid();
}

/*
     Checks for missing diagonals
*/
#undef __FUNCT__
#define __FUNCT__ "MatMissingDiagonal_SeqBAIJ"
PetscErrorCode MatMissingDiagonal_SeqBAIJ(Mat A,PetscBool  *missing,PetscInt *d)
{
  Mat_SeqBAIJ    *a = (Mat_SeqBAIJ*)A->data;
  PetscErrorCode ierr;
  PetscInt       *diag,*ii = a->i,i;

  PetscFunctionBegin;
  ierr     = MatMarkDiagonal_SeqBAIJ(A);CHKERRQ(ierr);
  *missing = PETSC_FALSE;
  if (A->rmap->n > 0 && !ii) {
    *missing = PETSC_TRUE;
    if (d) *d = 0;
    PetscInfo(A,"Matrix has no entries therefore is missing diagonal\n");
  } else {
    diag = a->diag;
    for (i=0; i<a->mbs; i++) {
      if (diag[i] >= ii[i+1]) {
        *missing = PETSC_TRUE;
        if (d) *d = i;
        PetscInfo1(A,"Matrix is missing block diagonal number %D\n",i);
        break;
      }
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatMarkDiagonal_SeqBAIJ"
PetscErrorCode MatMarkDiagonal_SeqBAIJ(Mat A)
{
  Mat_SeqBAIJ    *a = (Mat_SeqBAIJ*)A->data;
  PetscErrorCode ierr;
  PetscInt       i,j,m = a->mbs;

  PetscFunctionBegin;
  if (!a->diag) {
    ierr         = PetscMalloc1(m,&a->diag);CHKERRQ(ierr);
    ierr         = PetscLogObjectMemory((PetscObject)A,m*sizeof(PetscInt));CHKERRQ(ierr);
    a->free_diag = PETSC_TRUE;
  }
  for (i=0; i<m; i++) {
    a->diag[i] = a->i[i+1];
    for (j=a->i[i]; j<a->i[i+1]; j++) {
      if (a->j[j] == i) {
        a->diag[i] = j;
        break;
      }
    }
  }
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "MatGetRowIJ_SeqBAIJ"
static PetscErrorCode MatGetRowIJ_SeqBAIJ(Mat A,PetscInt oshift,PetscBool symmetric,PetscBool blockcompressed,PetscInt *nn,const PetscInt *inia[],const PetscInt *inja[],PetscBool  *done)
{
  Mat_SeqBAIJ    *a = (Mat_SeqBAIJ*)A->data;
  PetscErrorCode ierr;
  PetscInt       i,j,n = a->mbs,nz = a->i[n],*tia,*tja,bs = A->rmap->bs,k,l,cnt;
  PetscInt       **ia = (PetscInt**)inia,**ja = (PetscInt**)inja;

  PetscFunctionBegin;
  *nn = n;
  if (!ia) PetscFunctionReturn(0);
  if (symmetric) {
    ierr = MatToSymmetricIJ_SeqAIJ(n,a->i,a->j,0,0,&tia,&tja);CHKERRQ(ierr);
    nz   = tia[n];
  } else {
    tia = a->i; tja = a->j;
  }

  if (!blockcompressed && bs > 1) {
    (*nn) *= bs;
    /* malloc & create the natural set of indices */
    ierr = PetscMalloc1((n+1)*bs,ia);CHKERRQ(ierr);
    if (n) {
      (*ia)[0] = 0;
      for (j=1; j<bs; j++) {
        (*ia)[j] = (tia[1]-tia[0])*bs+(*ia)[j-1];
      }
    }

    for (i=1; i<n; i++) {
      (*ia)[i*bs] = (tia[i]-tia[i-1])*bs + (*ia)[i*bs-1];
      for (j=1; j<bs; j++) {
        (*ia)[i*bs+j] = (tia[i+1]-tia[i])*bs + (*ia)[i*bs+j-1];
      }
    }
    if (n) {
      (*ia)[n*bs] = (tia[n]-tia[n-1])*bs + (*ia)[n*bs-1];
    }

    if (inja) {
      ierr = PetscMalloc1(nz*bs*bs,ja);CHKERRQ(ierr);
      cnt = 0;
      for (i=0; i<n; i++) {
        for (j=0; j<bs; j++) {
          for (k=tia[i]; k<tia[i+1]; k++) {
            for (l=0; l<bs; l++) {
              (*ja)[cnt++] = bs*tja[k] + l;
            }
          }
        }
      }
    }

    if (symmetric) { /* deallocate memory allocated in MatToSymmetricIJ_SeqAIJ() */
      ierr = PetscFree(tia);CHKERRQ(ierr);
      ierr = PetscFree(tja);CHKERRQ(ierr);
    }
  } else if (oshift == 1) {
    if (symmetric) {
      nz = tia[A->rmap->n/bs];
      /*  add 1 to i and j indices */
      for (i=0; i<A->rmap->n/bs+1; i++) tia[i] = tia[i] + 1;
      *ia = tia;
      if (ja) {
        for (i=0; i<nz; i++) tja[i] = tja[i] + 1;
        *ja = tja;
      }
    } else {
      nz = a->i[A->rmap->n/bs];
      /* malloc space and  add 1 to i and j indices */
      ierr = PetscMalloc1(A->rmap->n/bs+1,ia);CHKERRQ(ierr);
      for (i=0; i<A->rmap->n/bs+1; i++) (*ia)[i] = a->i[i] + 1;
      if (ja) {
        ierr = PetscMalloc1(nz,ja);CHKERRQ(ierr);
        for (i=0; i<nz; i++) (*ja)[i] = a->j[i] + 1;
      }
    }
  } else {
    *ia = tia;
    if (ja) *ja = tja;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatRestoreRowIJ_SeqBAIJ"
static PetscErrorCode MatRestoreRowIJ_SeqBAIJ(Mat A,PetscInt oshift,PetscBool symmetric,PetscBool blockcompressed,PetscInt *nn,const PetscInt *ia[],const PetscInt *ja[],PetscBool  *done)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (!ia) PetscFunctionReturn(0);
  if ((!blockcompressed && A->rmap->bs > 1) || (symmetric || oshift == 1)) {
    ierr = PetscFree(*ia);CHKERRQ(ierr);
    if (ja) {ierr = PetscFree(*ja);CHKERRQ(ierr);}
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatDestroy_SeqBAIJ"
PetscErrorCode MatDestroy_SeqBAIJ(Mat A)
{
  Mat_SeqBAIJ    *a = (Mat_SeqBAIJ*)A->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
#if defined(PETSC_USE_LOG)
  PetscLogObjectState((PetscObject)A,"Rows=%D, Cols=%D, NZ=%D",A->rmap->N,A->cmap->n,a->nz);
#endif
  ierr = MatSeqXAIJFreeAIJ(A,&a->a,&a->j,&a->i);CHKERRQ(ierr);
  ierr = ISDestroy(&a->row);CHKERRQ(ierr);
  ierr = ISDestroy(&a->col);CHKERRQ(ierr);
  if (a->free_diag) {ierr = PetscFree(a->diag);CHKERRQ(ierr);}
  ierr = PetscFree(a->idiag);CHKERRQ(ierr);
  if (a->free_imax_ilen) {ierr = PetscFree2(a->imax,a->ilen);CHKERRQ(ierr);}
  ierr = PetscFree(a->solve_work);CHKERRQ(ierr);
  ierr = PetscFree(a->mult_work);CHKERRQ(ierr);
  ierr = PetscFree(a->sor_workt);CHKERRQ(ierr);
  ierr = PetscFree(a->sor_work);CHKERRQ(ierr);
  ierr = ISDestroy(&a->icol);CHKERRQ(ierr);
  ierr = PetscFree(a->saved_values);CHKERRQ(ierr);
  ierr = PetscFree2(a->compressedrow.i,a->compressedrow.rindex);CHKERRQ(ierr);

  ierr = MatDestroy(&a->sbaijMat);CHKERRQ(ierr);
  ierr = MatDestroy(&a->parent);CHKERRQ(ierr);
  ierr = PetscFree(A->data);CHKERRQ(ierr);

  ierr = PetscObjectChangeTypeName((PetscObject)A,0);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)A,"MatInvertBlockDiagonal_C",NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)A,"MatStoreValues_C",NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)A,"MatRetrieveValues_C",NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)A,"MatSeqBAIJSetColumnIndices_C",NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)A,"MatConvert_seqbaij_seqaij_C",NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)A,"MatConvert_seqbaij_seqsbaij_C",NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)A,"MatSeqBAIJSetPreallocation_C",NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)A,"MatSeqBAIJSetPreallocationCSR_C",NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)A,"MatConvert_seqbaij_seqbstrm_C",NULL);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)A,"MatIsTranspose_C",NULL);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSetOption_SeqBAIJ"
PetscErrorCode MatSetOption_SeqBAIJ(Mat A,MatOption op,PetscBool flg)
{
  Mat_SeqBAIJ    *a = (Mat_SeqBAIJ*)A->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  switch (op) {
  case MAT_ROW_ORIENTED:
    a->roworiented = flg;
    break;
  case MAT_KEEP_NONZERO_PATTERN:
    a->keepnonzeropattern = flg;
    break;
  case MAT_NEW_NONZERO_LOCATIONS:
    a->nonew = (flg ? 0 : 1);
    break;
  case MAT_NEW_NONZERO_LOCATION_ERR:
    a->nonew = (flg ? -1 : 0);
    break;
  case MAT_NEW_NONZERO_ALLOCATION_ERR:
    a->nonew = (flg ? -2 : 0);
    break;
  case MAT_UNUSED_NONZERO_LOCATION_ERR:
    a->nounused = (flg ? -1 : 0);
    break;
  case MAT_NEW_DIAGONALS:
  case MAT_IGNORE_OFF_PROC_ENTRIES:
  case MAT_USE_HASH_TABLE:
    ierr = PetscInfo1(A,"Option %s ignored\n",MatOptions[op]);CHKERRQ(ierr);
    break;
  case MAT_SPD:
  case MAT_SYMMETRIC:
  case MAT_STRUCTURALLY_SYMMETRIC:
  case MAT_HERMITIAN:
  case MAT_SYMMETRY_ETERNAL:
    /* These options are handled directly by MatSetOption() */
    break;
  default:
    SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_SUP,"unknown option %d",op);
  }
  PetscFunctionReturn(0);
}

/* used for both SeqBAIJ and SeqSBAIJ matrices */
#undef __FUNCT__
#define __FUNCT__ "MatGetRow_SeqBAIJ_private"
PetscErrorCode MatGetRow_SeqBAIJ_private(Mat A,PetscInt row,PetscInt *nz,PetscInt **idx,PetscScalar **v,PetscInt *ai,PetscInt *aj,PetscScalar *aa)
{
  PetscErrorCode ierr;
  PetscInt       itmp,i,j,k,M,bn,bp,*idx_i,bs,bs2;
  MatScalar      *aa_i;
  PetscScalar    *v_i;

  PetscFunctionBegin;
  bs  = A->rmap->bs;
  bs2 = bs*bs;
  if (row < 0 || row >= A->rmap->N) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Row %D out of range", row);

  bn  = row/bs;   /* Block number */
  bp  = row % bs; /* Block Position */
  M   = ai[bn+1] - ai[bn];
  *nz = bs*M;

  if (v) {
    *v = 0;
    if (*nz) {
      ierr = PetscMalloc1(*nz,v);CHKERRQ(ierr);
      for (i=0; i<M; i++) { /* for each block in the block row */
        v_i  = *v + i*bs;
        aa_i = aa + bs2*(ai[bn] + i);
        for (j=bp,k=0; j<bs2; j+=bs,k++) v_i[k] = aa_i[j];
      }
    }
  }

  if (idx) {
    *idx = 0;
    if (*nz) {
      ierr = PetscMalloc1(*nz,idx);CHKERRQ(ierr);
      for (i=0; i<M; i++) { /* for each block in the block row */
        idx_i = *idx + i*bs;
        itmp  = bs*aj[ai[bn] + i];
        for (j=0; j<bs; j++) idx_i[j] = itmp++;
      }
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatGetRow_SeqBAIJ"
PetscErrorCode MatGetRow_SeqBAIJ(Mat A,PetscInt row,PetscInt *nz,PetscInt **idx,PetscScalar **v)
{
  Mat_SeqBAIJ    *a = (Mat_SeqBAIJ*)A->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MatGetRow_SeqBAIJ_private(A,row,nz,idx,v,a->i,a->j,a->a);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatRestoreRow_SeqBAIJ"
PetscErrorCode MatRestoreRow_SeqBAIJ(Mat A,PetscInt row,PetscInt *nz,PetscInt **idx,PetscScalar **v)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (idx) {ierr = PetscFree(*idx);CHKERRQ(ierr);}
  if (v)   {ierr = PetscFree(*v);CHKERRQ(ierr);}
  PetscFunctionReturn(0);
}

extern PetscErrorCode MatSetValues_SeqBAIJ(Mat,PetscInt,const PetscInt[],PetscInt,const PetscInt[],const PetscScalar[],InsertMode);

#undef __FUNCT__
#define __FUNCT__ "MatTranspose_SeqBAIJ"
PetscErrorCode MatTranspose_SeqBAIJ(Mat A,MatReuse reuse,Mat *B)
{
  Mat_SeqBAIJ    *a=(Mat_SeqBAIJ*)A->data;
  Mat            C;
  PetscErrorCode ierr;
  PetscInt       i,j,k,*aj=a->j,*ai=a->i,bs=A->rmap->bs,mbs=a->mbs,nbs=a->nbs,len,*col;
  PetscInt       *rows,*cols,bs2=a->bs2;
  MatScalar      *array;

  PetscFunctionBegin;
  if (reuse == MAT_REUSE_MATRIX && A == *B && mbs != nbs) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_SIZ,"Square matrix only for in-place");
  if (reuse == MAT_INITIAL_MATRIX || A == *B) {
    ierr = PetscCalloc1(1+nbs,&col);CHKERRQ(ierr);

    for (i=0; i<ai[mbs]; i++) col[aj[i]] += 1;
    ierr = MatCreate(PetscObjectComm((PetscObject)A),&C);CHKERRQ(ierr);
    ierr = MatSetSizes(C,A->cmap->n,A->rmap->N,A->cmap->n,A->rmap->N);CHKERRQ(ierr);
    ierr = MatSetType(C,((PetscObject)A)->type_name);CHKERRQ(ierr);
    ierr = MatSeqBAIJSetPreallocation_SeqBAIJ(C,bs,0,col);CHKERRQ(ierr);
    ierr = PetscFree(col);CHKERRQ(ierr);
  } else {
    C = *B;
  }

  array = a->a;
  ierr  = PetscMalloc2(bs,&rows,bs,&cols);CHKERRQ(ierr);
  for (i=0; i<mbs; i++) {
    cols[0] = i*bs;
    for (k=1; k<bs; k++) cols[k] = cols[k-1] + 1;
    len = ai[i+1] - ai[i];
    for (j=0; j<len; j++) {
      rows[0] = (*aj++)*bs;
      for (k=1; k<bs; k++) rows[k] = rows[k-1] + 1;
      ierr   = MatSetValues_SeqBAIJ(C,bs,rows,bs,cols,array,INSERT_VALUES);CHKERRQ(ierr);
      array += bs2;
    }
  }
  ierr = PetscFree2(rows,cols);CHKERRQ(ierr);

  ierr = MatAssemblyBegin(C,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(C,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);

  if (reuse == MAT_INITIAL_MATRIX || *B != A) {
    *B = C;
  } else {
    ierr = MatHeaderMerge(A,&C);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatIsTranspose_SeqBAIJ"
PetscErrorCode MatIsTranspose_SeqBAIJ(Mat A,Mat B,PetscReal tol,PetscBool  *f)
{
  PetscErrorCode ierr;
  Mat            Btrans;

  PetscFunctionBegin;
  *f   = PETSC_FALSE;
  ierr = MatTranspose_SeqBAIJ(A,MAT_INITIAL_MATRIX,&Btrans);CHKERRQ(ierr);
  ierr = MatEqual_SeqBAIJ(B,Btrans,f);CHKERRQ(ierr);
  ierr = MatDestroy(&Btrans);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatView_SeqBAIJ_Binary"
static PetscErrorCode MatView_SeqBAIJ_Binary(Mat A,PetscViewer viewer)
{
  Mat_SeqBAIJ    *a = (Mat_SeqBAIJ*)A->data;
  PetscErrorCode ierr;
  PetscInt       i,*col_lens,bs = A->rmap->bs,count,*jj,j,k,l,bs2=a->bs2;
  int            fd;
  PetscScalar    *aa;
  FILE           *file;

  PetscFunctionBegin;
  ierr        = PetscViewerBinaryGetDescriptor(viewer,&fd);CHKERRQ(ierr);
  ierr        = PetscMalloc1(4+A->rmap->N,&col_lens);CHKERRQ(ierr);
  col_lens[0] = MAT_FILE_CLASSID;

  col_lens[1] = A->rmap->N;
  col_lens[2] = A->cmap->n;
  col_lens[3] = a->nz*bs2;

  /* store lengths of each row and write (including header) to file */
  count = 0;
  for (i=0; i<a->mbs; i++) {
    for (j=0; j<bs; j++) {
      col_lens[4+count++] = bs*(a->i[i+1] - a->i[i]);
    }
  }
  ierr = PetscBinaryWrite(fd,col_lens,4+A->rmap->N,PETSC_INT,PETSC_TRUE);CHKERRQ(ierr);
  ierr = PetscFree(col_lens);CHKERRQ(ierr);

  /* store column indices (zero start index) */
  ierr  = PetscMalloc1((a->nz+1)*bs2,&jj);CHKERRQ(ierr);
  count = 0;
  for (i=0; i<a->mbs; i++) {
    for (j=0; j<bs; j++) {
      for (k=a->i[i]; k<a->i[i+1]; k++) {
        for (l=0; l<bs; l++) {
          jj[count++] = bs*a->j[k] + l;
        }
      }
    }
  }
  ierr = PetscBinaryWrite(fd,jj,bs2*a->nz,PETSC_INT,PETSC_FALSE);CHKERRQ(ierr);
  ierr = PetscFree(jj);CHKERRQ(ierr);

  /* store nonzero values */
  ierr  = PetscMalloc1((a->nz+1)*bs2,&aa);CHKERRQ(ierr);
  count = 0;
  for (i=0; i<a->mbs; i++) {
    for (j=0; j<bs; j++) {
      for (k=a->i[i]; k<a->i[i+1]; k++) {
        for (l=0; l<bs; l++) {
          aa[count++] = a->a[bs2*k + l*bs + j];
        }
      }
    }
  }
  ierr = PetscBinaryWrite(fd,aa,bs2*a->nz,PETSC_SCALAR,PETSC_FALSE);CHKERRQ(ierr);
  ierr = PetscFree(aa);CHKERRQ(ierr);

  ierr = PetscViewerBinaryGetInfoPointer(viewer,&file);CHKERRQ(ierr);
  if (file) {
    fprintf(file,"-matload_block_size %d\n",(int)A->rmap->bs);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatView_SeqBAIJ_ASCII"
static PetscErrorCode MatView_SeqBAIJ_ASCII(Mat A,PetscViewer viewer)
{
  Mat_SeqBAIJ       *a = (Mat_SeqBAIJ*)A->data;
  PetscErrorCode    ierr;
  PetscInt          i,j,bs = A->rmap->bs,k,l,bs2=a->bs2;
  PetscViewerFormat format;

  PetscFunctionBegin;
  ierr = PetscViewerGetFormat(viewer,&format);CHKERRQ(ierr);
  if (format == PETSC_VIEWER_ASCII_INFO || format == PETSC_VIEWER_ASCII_INFO_DETAIL) {
    ierr = PetscViewerASCIIPrintf(viewer,"  block size is %D\n",bs);CHKERRQ(ierr);
  } else if (format == PETSC_VIEWER_ASCII_MATLAB) {
    const char *matname;
    Mat        aij;
    ierr = MatConvert(A,MATSEQAIJ,MAT_INITIAL_MATRIX,&aij);CHKERRQ(ierr);
    ierr = PetscObjectGetName((PetscObject)A,&matname);CHKERRQ(ierr);
    ierr = PetscObjectSetName((PetscObject)aij,matname);CHKERRQ(ierr);
    ierr = MatView(aij,viewer);CHKERRQ(ierr);
    ierr = MatDestroy(&aij);CHKERRQ(ierr);
  } else if (format == PETSC_VIEWER_ASCII_FACTOR_INFO) {
      PetscFunctionReturn(0);
  } else if (format == PETSC_VIEWER_ASCII_COMMON) {
    ierr = PetscViewerASCIIUseTabs(viewer,PETSC_FALSE);CHKERRQ(ierr);
    for (i=0; i<a->mbs; i++) {
      for (j=0; j<bs; j++) {
        ierr = PetscViewerASCIIPrintf(viewer,"row %D:",i*bs+j);CHKERRQ(ierr);
        for (k=a->i[i]; k<a->i[i+1]; k++) {
          for (l=0; l<bs; l++) {
#if defined(PETSC_USE_COMPLEX)
            if (PetscImaginaryPart(a->a[bs2*k + l*bs + j]) > 0.0 && PetscRealPart(a->a[bs2*k + l*bs + j]) != 0.0) {
              ierr = PetscViewerASCIIPrintf(viewer," (%D, %g + %gi) ",bs*a->j[k]+l,
                                            (double)PetscRealPart(a->a[bs2*k + l*bs + j]),(double)PetscImaginaryPart(a->a[bs2*k + l*bs + j]));CHKERRQ(ierr);
            } else if (PetscImaginaryPart(a->a[bs2*k + l*bs + j]) < 0.0 && PetscRealPart(a->a[bs2*k + l*bs + j]) != 0.0) {
              ierr = PetscViewerASCIIPrintf(viewer," (%D, %g - %gi) ",bs*a->j[k]+l,
                                            (double)PetscRealPart(a->a[bs2*k + l*bs + j]),-(double)PetscImaginaryPart(a->a[bs2*k + l*bs + j]));CHKERRQ(ierr);
            } else if (PetscRealPart(a->a[bs2*k + l*bs + j]) != 0.0) {
              ierr = PetscViewerASCIIPrintf(viewer," (%D, %g) ",bs*a->j[k]+l,(double)PetscRealPart(a->a[bs2*k + l*bs + j]));CHKERRQ(ierr);
            }
#else
            if (a->a[bs2*k + l*bs + j] != 0.0) {
              ierr = PetscViewerASCIIPrintf(viewer," (%D, %g) ",bs*a->j[k]+l,(double)a->a[bs2*k + l*bs + j]);CHKERRQ(ierr);
            }
#endif
          }
        }
        ierr = PetscViewerASCIIPrintf(viewer,"\n");CHKERRQ(ierr);
      }
    }
    ierr = PetscViewerASCIIUseTabs(viewer,PETSC_TRUE);CHKERRQ(ierr);
  } else {
    ierr = PetscViewerASCIIUseTabs(viewer,PETSC_FALSE);CHKERRQ(ierr);
    for (i=0; i<a->mbs; i++) {
      for (j=0; j<bs; j++) {
        ierr = PetscViewerASCIIPrintf(viewer,"row %D:",i*bs+j);CHKERRQ(ierr);
        for (k=a->i[i]; k<a->i[i+1]; k++) {
          for (l=0; l<bs; l++) {
#if defined(PETSC_USE_COMPLEX)
            if (PetscImaginaryPart(a->a[bs2*k + l*bs + j]) > 0.0) {
              ierr = PetscViewerASCIIPrintf(viewer," (%D, %g + %g i) ",bs*a->j[k]+l,
                                            (double)PetscRealPart(a->a[bs2*k + l*bs + j]),(double)PetscImaginaryPart(a->a[bs2*k + l*bs + j]));CHKERRQ(ierr);
            } else if (PetscImaginaryPart(a->a[bs2*k + l*bs + j]) < 0.0) {
              ierr = PetscViewerASCIIPrintf(viewer," (%D, %g - %g i) ",bs*a->j[k]+l,
                                            (double)PetscRealPart(a->a[bs2*k + l*bs + j]),-(double)PetscImaginaryPart(a->a[bs2*k + l*bs + j]));CHKERRQ(ierr);
            } else {
              ierr = PetscViewerASCIIPrintf(viewer," (%D, %g) ",bs*a->j[k]+l,(double)PetscRealPart(a->a[bs2*k + l*bs + j]));CHKERRQ(ierr);
            }
#else
            ierr = PetscViewerASCIIPrintf(viewer," (%D, %g) ",bs*a->j[k]+l,(double)a->a[bs2*k + l*bs + j]);CHKERRQ(ierr);
#endif
          }
        }
        ierr = PetscViewerASCIIPrintf(viewer,"\n");CHKERRQ(ierr);
      }
    }
    ierr = PetscViewerASCIIUseTabs(viewer,PETSC_TRUE);CHKERRQ(ierr);
  }
  ierr = PetscViewerFlush(viewer);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#include <petscdraw.h>
#undef __FUNCT__
#define __FUNCT__ "MatView_SeqBAIJ_Draw_Zoom"
static PetscErrorCode MatView_SeqBAIJ_Draw_Zoom(PetscDraw draw,void *Aa)
{
  Mat               A = (Mat) Aa;
  Mat_SeqBAIJ       *a=(Mat_SeqBAIJ*)A->data;
  PetscErrorCode    ierr;
  PetscInt          row,i,j,k,l,mbs=a->mbs,color,bs=A->rmap->bs,bs2=a->bs2;
  PetscReal         xl,yl,xr,yr,x_l,x_r,y_l,y_r;
  MatScalar         *aa;
  PetscViewer       viewer;
  PetscViewerFormat format;

  PetscFunctionBegin;
  ierr = PetscObjectQuery((PetscObject)A,"Zoomviewer",(PetscObject*)&viewer);CHKERRQ(ierr);
  ierr = PetscViewerGetFormat(viewer,&format);CHKERRQ(ierr);

  ierr = PetscDrawGetCoordinates(draw,&xl,&yl,&xr,&yr);CHKERRQ(ierr);

  /* loop over matrix elements drawing boxes */

  if (format != PETSC_VIEWER_DRAW_CONTOUR) {
    color = PETSC_DRAW_BLUE;
    for (i=0,row=0; i<mbs; i++,row+=bs) {
      for (j=a->i[i]; j<a->i[i+1]; j++) {
        y_l = A->rmap->N - row - 1.0; y_r = y_l + 1.0;
        x_l = a->j[j]*bs; x_r = x_l + 1.0;
        aa  = a->a + j*bs2;
        for (k=0; k<bs; k++) {
          for (l=0; l<bs; l++) {
            if (PetscRealPart(*aa++) >=  0.) continue;
            ierr = PetscDrawRectangle(draw,x_l+k,y_l-l,x_r+k,y_r-l,color,color,color,color);CHKERRQ(ierr);
          }
        }
      }
    }
    color = PETSC_DRAW_CYAN;
    for (i=0,row=0; i<mbs; i++,row+=bs) {
      for (j=a->i[i]; j<a->i[i+1]; j++) {
        y_l = A->rmap->N - row - 1.0; y_r = y_l + 1.0;
        x_l = a->j[j]*bs; x_r = x_l + 1.0;
        aa  = a->a + j*bs2;
        for (k=0; k<bs; k++) {
          for (l=0; l<bs; l++) {
            if (PetscRealPart(*aa++) != 0.) continue;
            ierr = PetscDrawRectangle(draw,x_l+k,y_l-l,x_r+k,y_r-l,color,color,color,color);CHKERRQ(ierr);
          }
        }
      }
    }
    color = PETSC_DRAW_RED;
    for (i=0,row=0; i<mbs; i++,row+=bs) {
      for (j=a->i[i]; j<a->i[i+1]; j++) {
        y_l = A->rmap->N - row - 1.0; y_r = y_l + 1.0;
        x_l = a->j[j]*bs; x_r = x_l + 1.0;
        aa  = a->a + j*bs2;
        for (k=0; k<bs; k++) {
          for (l=0; l<bs; l++) {
            if (PetscRealPart(*aa++) <= 0.) continue;
            ierr = PetscDrawRectangle(draw,x_l+k,y_l-l,x_r+k,y_r-l,color,color,color,color);CHKERRQ(ierr);
          }
        }
      }
    }
  } else {
    /* use contour shading to indicate magnitude of values */
    /* first determine max of all nonzero values */
    PetscDraw popup;
    PetscReal scale,maxv = 0.0;

    for (i=0; i<a->nz*a->bs2; i++) {
      if (PetscAbsScalar(a->a[i]) > maxv) maxv = PetscAbsScalar(a->a[i]);
    }
    scale = (245.0 - PETSC_DRAW_BASIC_COLORS)/maxv;
    ierr  = PetscDrawGetPopup(draw,&popup);CHKERRQ(ierr);
    if (popup) {
      ierr = PetscDrawScalePopup(popup,0.0,maxv);CHKERRQ(ierr);
    }
    for (i=0,row=0; i<mbs; i++,row+=bs) {
      for (j=a->i[i]; j<a->i[i+1]; j++) {
        y_l = A->rmap->N - row - 1.0; y_r = y_l + 1.0;
        x_l = a->j[j]*bs; x_r = x_l + 1.0;
        aa  = a->a + j*bs2;
        for (k=0; k<bs; k++) {
          for (l=0; l<bs; l++) {
            color = PETSC_DRAW_BASIC_COLORS + (PetscInt)(scale*PetscAbsScalar(*aa++));
            ierr  = PetscDrawRectangle(draw,x_l+k,y_l-l,x_r+k,y_r-l,color,color,color,color);CHKERRQ(ierr);
          }
        }
      }
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatView_SeqBAIJ_Draw"
static PetscErrorCode MatView_SeqBAIJ_Draw(Mat A,PetscViewer viewer)
{
  PetscErrorCode ierr;
  PetscReal      xl,yl,xr,yr,w,h;
  PetscDraw      draw;
  PetscBool      isnull;

  PetscFunctionBegin;
  ierr = PetscViewerDrawGetDraw(viewer,0,&draw);CHKERRQ(ierr);
  ierr = PetscDrawIsNull(draw,&isnull);CHKERRQ(ierr); if (isnull) PetscFunctionReturn(0);

  ierr = PetscObjectCompose((PetscObject)A,"Zoomviewer",(PetscObject)viewer);CHKERRQ(ierr);
  xr   = A->cmap->n; yr = A->rmap->N; h = yr/10.0; w = xr/10.0;
  xr  += w;    yr += h;  xl = -w;     yl = -h;
  ierr = PetscDrawSetCoordinates(draw,xl,yl,xr,yr);CHKERRQ(ierr);
  ierr = PetscDrawZoom(draw,MatView_SeqBAIJ_Draw_Zoom,A);CHKERRQ(ierr);
  ierr = PetscObjectCompose((PetscObject)A,"Zoomviewer",NULL);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatView_SeqBAIJ"
PetscErrorCode MatView_SeqBAIJ(Mat A,PetscViewer viewer)
{
  PetscErrorCode ierr;
  PetscBool      iascii,isbinary,isdraw;

  PetscFunctionBegin;
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERASCII,&iascii);CHKERRQ(ierr);
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERBINARY,&isbinary);CHKERRQ(ierr);
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERDRAW,&isdraw);CHKERRQ(ierr);
  if (iascii) {
    ierr = MatView_SeqBAIJ_ASCII(A,viewer);CHKERRQ(ierr);
  } else if (isbinary) {
    ierr = MatView_SeqBAIJ_Binary(A,viewer);CHKERRQ(ierr);
  } else if (isdraw) {
    ierr = MatView_SeqBAIJ_Draw(A,viewer);CHKERRQ(ierr);
  } else {
    Mat B;
    ierr = MatConvert(A,MATSEQAIJ,MAT_INITIAL_MATRIX,&B);CHKERRQ(ierr);
    ierr = MatView(B,viewer);CHKERRQ(ierr);
    ierr = MatDestroy(&B);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "MatGetValues_SeqBAIJ"
PetscErrorCode MatGetValues_SeqBAIJ(Mat A,PetscInt m,const PetscInt im[],PetscInt n,const PetscInt in[],PetscScalar v[])
{
  Mat_SeqBAIJ *a = (Mat_SeqBAIJ*)A->data;
  PetscInt    *rp,k,low,high,t,row,nrow,i,col,l,*aj = a->j;
  PetscInt    *ai = a->i,*ailen = a->ilen;
  PetscInt    brow,bcol,ridx,cidx,bs=A->rmap->bs,bs2=a->bs2;
  MatScalar   *ap,*aa = a->a;

  PetscFunctionBegin;
  for (k=0; k<m; k++) { /* loop over rows */
    row = im[k]; brow = row/bs;
    if (row < 0) {v += n; continue;} /* SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Negative row"); */
    if (row >= A->rmap->N) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Row %D too large", row);
    rp   = aj + ai[brow]; ap = aa + bs2*ai[brow];
    nrow = ailen[brow];
    for (l=0; l<n; l++) { /* loop over columns */
      if (in[l] < 0) {v++; continue;} /* SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Negative column"); */
      if (in[l] >= A->cmap->n) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Column %D too large", in[l]);
      col  = in[l];
      bcol = col/bs;
      cidx = col%bs;
      ridx = row%bs;
      high = nrow;
      low  = 0; /* assume unsorted */
      while (high-low > 5) {
        t = (low+high)/2;
        if (rp[t] > bcol) high = t;
        else             low  = t;
      }
      for (i=low; i<high; i++) {
        if (rp[i] > bcol) break;
        if (rp[i] == bcol) {
          *v++ = ap[bs2*i+bs*cidx+ridx];
          goto finished;
        }
      }
      *v++ = 0.0;
finished:;
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSetValuesBlocked_SeqBAIJ"
PetscErrorCode MatSetValuesBlocked_SeqBAIJ(Mat A,PetscInt m,const PetscInt im[],PetscInt n,const PetscInt in[],const PetscScalar v[],InsertMode is)
{
  Mat_SeqBAIJ       *a = (Mat_SeqBAIJ*)A->data;
  PetscInt          *rp,k,low,high,t,ii,jj,row,nrow,i,col,l,rmax,N,lastcol = -1;
  PetscInt          *imax=a->imax,*ai=a->i,*ailen=a->ilen;
  PetscErrorCode    ierr;
  PetscInt          *aj        =a->j,nonew=a->nonew,bs2=a->bs2,bs=A->rmap->bs,stepval;
  PetscBool         roworiented=a->roworiented;
  const PetscScalar *value     = v;
  MatScalar         *ap,*aa = a->a,*bap;

  PetscFunctionBegin;
  if (roworiented) {
    stepval = (n-1)*bs;
  } else {
    stepval = (m-1)*bs;
  }
  for (k=0; k<m; k++) { /* loop over added rows */
    row = im[k];
    if (row < 0) continue;
#if defined(PETSC_USE_DEBUG)
    if (row >= a->mbs) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Block row index too large %D max %D",row,a->mbs-1);
#endif
    rp   = aj + ai[row];
    ap   = aa + bs2*ai[row];
    rmax = imax[row];
    nrow = ailen[row];
    low  = 0;
    high = nrow;
    for (l=0; l<n; l++) { /* loop over added columns */
      if (in[l] < 0) continue;
#if defined(PETSC_USE_DEBUG)
      if (in[l] >= a->nbs) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Block column index too large %D max %D",in[l],a->nbs-1);
#endif
      col = in[l];
      if (roworiented) {
        value = v + (k*(stepval+bs) + l)*bs;
      } else {
        value = v + (l*(stepval+bs) + k)*bs;
      }
      if (col <= lastcol) low = 0;
      else high = nrow;
      lastcol = col;
      while (high-low > 7) {
        t = (low+high)/2;
        if (rp[t] > col) high = t;
        else             low  = t;
      }
      for (i=low; i<high; i++) {
        if (rp[i] > col) break;
        if (rp[i] == col) {
          bap = ap +  bs2*i;
          if (roworiented) {
            if (is == ADD_VALUES) {
              for (ii=0; ii<bs; ii++,value+=stepval) {
                for (jj=ii; jj<bs2; jj+=bs) {
                  bap[jj] += *value++;
                }
              }
            } else {
              for (ii=0; ii<bs; ii++,value+=stepval) {
                for (jj=ii; jj<bs2; jj+=bs) {
                  bap[jj] = *value++;
                }
              }
            }
          } else {
            if (is == ADD_VALUES) {
              for (ii=0; ii<bs; ii++,value+=bs+stepval) {
                for (jj=0; jj<bs; jj++) {
                  bap[jj] += value[jj];
                }
                bap += bs;
              }
            } else {
              for (ii=0; ii<bs; ii++,value+=bs+stepval) {
                for (jj=0; jj<bs; jj++) {
                  bap[jj]  = value[jj];
                }
                bap += bs;
              }
            }
          }
          goto noinsert2;
        }
      }
      if (nonew == 1) goto noinsert2;
      if (nonew == -1) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Inserting a new blocked index new nonzero block (%D, %D) in the matrix", row, col);
      MatSeqXAIJReallocateAIJ(A,a->mbs,bs2,nrow,row,col,rmax,aa,ai,aj,rp,ap,imax,nonew,MatScalar);
      N = nrow++ - 1; high++;
      /* shift up all the later entries in this row */
      for (ii=N; ii>=i; ii--) {
        rp[ii+1] = rp[ii];
        ierr     = PetscMemcpy(ap+bs2*(ii+1),ap+bs2*(ii),bs2*sizeof(MatScalar));CHKERRQ(ierr);
      }
      if (N >= i) {
        ierr = PetscMemzero(ap+bs2*i,bs2*sizeof(MatScalar));CHKERRQ(ierr);
      }
      rp[i] = col;
      bap   = ap +  bs2*i;
      if (roworiented) {
        for (ii=0; ii<bs; ii++,value+=stepval) {
          for (jj=ii; jj<bs2; jj+=bs) {
            bap[jj] = *value++;
          }
        }
      } else {
        for (ii=0; ii<bs; ii++,value+=stepval) {
          for (jj=0; jj<bs; jj++) {
            *bap++ = *value++;
          }
        }
      }
noinsert2:;
      low = i;
    }
    ailen[row] = nrow;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatAssemblyEnd_SeqBAIJ"
PetscErrorCode MatAssemblyEnd_SeqBAIJ(Mat A,MatAssemblyType mode)
{
  Mat_SeqBAIJ    *a     = (Mat_SeqBAIJ*)A->data;
  PetscInt       fshift = 0,i,j,*ai = a->i,*aj = a->j,*imax = a->imax;
  PetscInt       m      = A->rmap->N,*ip,N,*ailen = a->ilen;
  PetscErrorCode ierr;
  PetscInt       mbs  = a->mbs,bs2 = a->bs2,rmax = 0;
  MatScalar      *aa  = a->a,*ap;
  PetscReal      ratio=0.6;

  PetscFunctionBegin;
  if (mode == MAT_FLUSH_ASSEMBLY) PetscFunctionReturn(0);

  if (m) rmax = ailen[0];
  for (i=1; i<mbs; i++) {
    /* move each row back by the amount of empty slots (fshift) before it*/
    fshift += imax[i-1] - ailen[i-1];
    rmax    = PetscMax(rmax,ailen[i]);
    if (fshift) {
      ip = aj + ai[i]; ap = aa + bs2*ai[i];
      N  = ailen[i];
      for (j=0; j<N; j++) {
        ip[j-fshift] = ip[j];

        ierr = PetscMemcpy(ap+(j-fshift)*bs2,ap+j*bs2,bs2*sizeof(MatScalar));CHKERRQ(ierr);
      }
    }
    ai[i] = ai[i-1] + ailen[i-1];
  }
  if (mbs) {
    fshift += imax[mbs-1] - ailen[mbs-1];
    ai[mbs] = ai[mbs-1] + ailen[mbs-1];
  }

  /* reset ilen and imax for each row */
  a->nonzerorowcnt = 0;
  for (i=0; i<mbs; i++) {
    ailen[i] = imax[i] = ai[i+1] - ai[i];
    a->nonzerorowcnt += ((ai[i+1] - ai[i]) > 0);
  }
  a->nz = ai[mbs];

  /* diagonals may have moved, so kill the diagonal pointers */
  a->idiagvalid = PETSC_FALSE;
  if (fshift && a->diag) {
    ierr    = PetscFree(a->diag);CHKERRQ(ierr);
    ierr    = PetscLogObjectMemory((PetscObject)A,-(mbs+1)*sizeof(PetscInt));CHKERRQ(ierr);
    a->diag = 0;
  }
  if (fshift && a->nounused == -1) SETERRQ4(PETSC_COMM_SELF,PETSC_ERR_PLIB, "Unused space detected in matrix: %D X %D block size %D, %D unneeded", m, A->cmap->n, A->rmap->bs, fshift*bs2);
  ierr = PetscInfo5(A,"Matrix size: %D X %D, block size %D; storage space: %D unneeded, %D used\n",m,A->cmap->n,A->rmap->bs,fshift*bs2,a->nz*bs2);CHKERRQ(ierr);
  ierr = PetscInfo1(A,"Number of mallocs during MatSetValues is %D\n",a->reallocs);CHKERRQ(ierr);
  ierr = PetscInfo1(A,"Most nonzeros blocks in any row is %D\n",rmax);CHKERRQ(ierr);

  A->info.mallocs    += a->reallocs;
  a->reallocs         = 0;
  A->info.nz_unneeded = (PetscReal)fshift*bs2;
  a->rmax             = rmax;

  ierr = MatCheckCompressedRow(A,a->nonzerorowcnt,&a->compressedrow,a->i,mbs,ratio);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/*
   This function returns an array of flags which indicate the locations of contiguous
   blocks that should be zeroed. for eg: if bs = 3  and is = [0,1,2,3,5,6,7,8,9]
   then the resulting sizes = [3,1,1,3,1] correspondig to sets [(0,1,2),(3),(5),(6,7,8),(9)]
   Assume: sizes should be long enough to hold all the values.
*/
#undef __FUNCT__
#define __FUNCT__ "MatZeroRows_SeqBAIJ_Check_Blocks"
static PetscErrorCode MatZeroRows_SeqBAIJ_Check_Blocks(PetscInt idx[],PetscInt n,PetscInt bs,PetscInt sizes[], PetscInt *bs_max)
{
  PetscInt  i,j,k,row;
  PetscBool flg;

  PetscFunctionBegin;
  for (i=0,j=0; i<n; j++) {
    row = idx[i];
    if (row%bs!=0) { /* Not the begining of a block */
      sizes[j] = 1;
      i++;
    } else if (i+bs > n) { /* complete block doesn't exist (at idx end) */
      sizes[j] = 1;         /* Also makes sure atleast 'bs' values exist for next else */
      i++;
    } else { /* Begining of the block, so check if the complete block exists */
      flg = PETSC_TRUE;
      for (k=1; k<bs; k++) {
        if (row+k != idx[i+k]) { /* break in the block */
          flg = PETSC_FALSE;
          break;
        }
      }
      if (flg) { /* No break in the bs */
        sizes[j] = bs;
        i       += bs;
      } else {
        sizes[j] = 1;
        i++;
      }
    }
  }
  *bs_max = j;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatZeroRows_SeqBAIJ"
PetscErrorCode MatZeroRows_SeqBAIJ(Mat A,PetscInt is_n,const PetscInt is_idx[],PetscScalar diag,Vec x, Vec b)
{
  Mat_SeqBAIJ       *baij=(Mat_SeqBAIJ*)A->data;
  PetscErrorCode    ierr;
  PetscInt          i,j,k,count,*rows;
  PetscInt          bs=A->rmap->bs,bs2=baij->bs2,*sizes,row,bs_max;
  PetscScalar       zero = 0.0;
  MatScalar         *aa;
  const PetscScalar *xx;
  PetscScalar       *bb;

  PetscFunctionBegin;
  /* fix right hand side if needed */
  if (x && b) {
    ierr = VecGetArrayRead(x,&xx);CHKERRQ(ierr);
    ierr = VecGetArray(b,&bb);CHKERRQ(ierr);
    for (i=0; i<is_n; i++) {
      bb[is_idx[i]] = diag*xx[is_idx[i]];
    }
    ierr = VecRestoreArrayRead(x,&xx);CHKERRQ(ierr);
    ierr = VecRestoreArray(b,&bb);CHKERRQ(ierr);
  }

  /* Make a copy of the IS and  sort it */
  /* allocate memory for rows,sizes */
  ierr = PetscMalloc2(is_n,&rows,2*is_n,&sizes);CHKERRQ(ierr);

  /* copy IS values to rows, and sort them */
  for (i=0; i<is_n; i++) rows[i] = is_idx[i];
  ierr = PetscSortInt(is_n,rows);CHKERRQ(ierr);

  if (baij->keepnonzeropattern) {
    for (i=0; i<is_n; i++) sizes[i] = 1;
    bs_max          = is_n;
  } else {
    ierr = MatZeroRows_SeqBAIJ_Check_Blocks(rows,is_n,bs,sizes,&bs_max);CHKERRQ(ierr);
    A->nonzerostate++;
  }

  for (i=0,j=0; i<bs_max; j+=sizes[i],i++) {
    row = rows[j];
    if (row < 0 || row > A->rmap->N) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"row %D out of range",row);
    count = (baij->i[row/bs +1] - baij->i[row/bs])*bs;
    aa    = ((MatScalar*)(baij->a)) + baij->i[row/bs]*bs2 + (row%bs);
    if (sizes[i] == bs && !baij->keepnonzeropattern) {
      if (diag != (PetscScalar)0.0) {
        if (baij->ilen[row/bs] > 0) {
          baij->ilen[row/bs]       = 1;
          baij->j[baij->i[row/bs]] = row/bs;

          ierr = PetscMemzero(aa,count*bs*sizeof(MatScalar));CHKERRQ(ierr);
        }
        /* Now insert all the diagonal values for this bs */
        for (k=0; k<bs; k++) {
          ierr = (*A->ops->setvalues)(A,1,rows+j+k,1,rows+j+k,&diag,INSERT_VALUES);CHKERRQ(ierr);
        }
      } else { /* (diag == 0.0) */
        baij->ilen[row/bs] = 0;
      } /* end (diag == 0.0) */
    } else { /* (sizes[i] != bs) */
#if defined(PETSC_USE_DEBUG)
      if (sizes[i] != 1) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_PLIB,"Internal Error. Value should be 1");
#endif
      for (k=0; k<count; k++) {
        aa[0] =  zero;
        aa   += bs;
      }
      if (diag != (PetscScalar)0.0) {
        ierr = (*A->ops->setvalues)(A,1,rows+j,1,rows+j,&diag,INSERT_VALUES);CHKERRQ(ierr);
      }
    }
  }

  ierr = PetscFree2(rows,sizes);CHKERRQ(ierr);
  ierr = MatAssemblyEnd_SeqBAIJ(A,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatZeroRowsColumns_SeqBAIJ"
PetscErrorCode MatZeroRowsColumns_SeqBAIJ(Mat A,PetscInt is_n,const PetscInt is_idx[],PetscScalar diag,Vec x, Vec b)
{
  Mat_SeqBAIJ       *baij=(Mat_SeqBAIJ*)A->data;
  PetscErrorCode    ierr;
  PetscInt          i,j,k,count;
  PetscInt          bs   =A->rmap->bs,bs2=baij->bs2,row,col;
  PetscScalar       zero = 0.0;
  MatScalar         *aa;
  const PetscScalar *xx;
  PetscScalar       *bb;
  PetscBool         *zeroed,vecs = PETSC_FALSE;

  PetscFunctionBegin;
  /* fix right hand side if needed */
  if (x && b) {
    ierr = VecGetArrayRead(x,&xx);CHKERRQ(ierr);
    ierr = VecGetArray(b,&bb);CHKERRQ(ierr);
    vecs = PETSC_TRUE;
  }

  /* zero the columns */
  ierr = PetscCalloc1(A->rmap->n,&zeroed);CHKERRQ(ierr);
  for (i=0; i<is_n; i++) {
    if (is_idx[i] < 0 || is_idx[i] >= A->rmap->N) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"row %D out of range",is_idx[i]);
    zeroed[is_idx[i]] = PETSC_TRUE;
  }
  for (i=0; i<A->rmap->N; i++) {
    if (!zeroed[i]) {
      row = i/bs;
      for (j=baij->i[row]; j<baij->i[row+1]; j++) {
        for (k=0; k<bs; k++) {
          col = bs*baij->j[j] + k;
          if (zeroed[col]) {
            aa = ((MatScalar*)(baij->a)) + j*bs2 + (i%bs) + bs*k;
            if (vecs) bb[i] -= aa[0]*xx[col];
            aa[0] = 0.0;
          }
        }
      }
    } else if (vecs) bb[i] = diag*xx[i];
  }
  ierr = PetscFree(zeroed);CHKERRQ(ierr);
  if (vecs) {
    ierr = VecRestoreArrayRead(x,&xx);CHKERRQ(ierr);
    ierr = VecRestoreArray(b,&bb);CHKERRQ(ierr);
  }

  /* zero the rows */
  for (i=0; i<is_n; i++) {
    row   = is_idx[i];
    count = (baij->i[row/bs +1] - baij->i[row/bs])*bs;
    aa    = ((MatScalar*)(baij->a)) + baij->i[row/bs]*bs2 + (row%bs);
    for (k=0; k<count; k++) {
      aa[0] =  zero;
      aa   += bs;
    }
    if (diag != (PetscScalar)0.0) {
      ierr = (*A->ops->setvalues)(A,1,&row,1,&row,&diag,INSERT_VALUES);CHKERRQ(ierr);
    }
  }
  ierr = MatAssemblyEnd_SeqBAIJ(A,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSetValues_SeqBAIJ"
PetscErrorCode MatSetValues_SeqBAIJ(Mat A,PetscInt m,const PetscInt im[],PetscInt n,const PetscInt in[],const PetscScalar v[],InsertMode is)
{
  Mat_SeqBAIJ    *a = (Mat_SeqBAIJ*)A->data;
  PetscInt       *rp,k,low,high,t,ii,row,nrow,i,col,l,rmax,N,lastcol = -1;
  PetscInt       *imax=a->imax,*ai=a->i,*ailen=a->ilen;
  PetscInt       *aj  =a->j,nonew=a->nonew,bs=A->rmap->bs,brow,bcol;
  PetscErrorCode ierr;
  PetscInt       ridx,cidx,bs2=a->bs2;
  PetscBool      roworiented=a->roworiented;
  MatScalar      *ap,value,*aa=a->a,*bap;

  PetscFunctionBegin;
  for (k=0; k<m; k++) { /* loop over added rows */
    row  = im[k];
    brow = row/bs;
    if (row < 0) continue;
#if defined(PETSC_USE_DEBUG)
    if (row >= A->rmap->N) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Row too large: row %D max %D",row,A->rmap->N-1);
#endif
    rp   = aj + ai[brow];
    ap   = aa + bs2*ai[brow];
    rmax = imax[brow];
    nrow = ailen[brow];
    low  = 0;
    high = nrow;
    for (l=0; l<n; l++) { /* loop over added columns */
      if (in[l] < 0) continue;
#if defined(PETSC_USE_DEBUG)
      if (in[l] >= A->cmap->n) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Column too large: col %D max %D",in[l],A->cmap->n-1);
#endif
      col  = in[l]; bcol = col/bs;
      ridx = row % bs; cidx = col % bs;
      if (roworiented) {
        value = v[l + k*n];
      } else {
        value = v[k + l*m];
      }
      if (col <= lastcol) low = 0; else high = nrow;
      lastcol = col;
      while (high-low > 7) {
        t = (low+high)/2;
        if (rp[t] > bcol) high = t;
        else              low  = t;
      }
      for (i=low; i<high; i++) {
        if (rp[i] > bcol) break;
        if (rp[i] == bcol) {
          bap = ap +  bs2*i + bs*cidx + ridx;
          if (is == ADD_VALUES) *bap += value;
          else                  *bap  = value;
          goto noinsert1;
        }
      }
      if (nonew == 1) goto noinsert1;
      if (nonew == -1) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Inserting a new nonzero (%D, %D) in the matrix", row, col);
      MatSeqXAIJReallocateAIJ(A,a->mbs,bs2,nrow,brow,bcol,rmax,aa,ai,aj,rp,ap,imax,nonew,MatScalar);
      N = nrow++ - 1; high++;
      /* shift up all the later entries in this row */
      for (ii=N; ii>=i; ii--) {
        rp[ii+1] = rp[ii];
        ierr     = PetscMemcpy(ap+bs2*(ii+1),ap+bs2*(ii),bs2*sizeof(MatScalar));CHKERRQ(ierr);
      }
      if (N>=i) {
        ierr = PetscMemzero(ap+bs2*i,bs2*sizeof(MatScalar));CHKERRQ(ierr);
      }
      rp[i]                      = bcol;
      ap[bs2*i + bs*cidx + ridx] = value;
      a->nz++;
      A->nonzerostate++;
noinsert1:;
      low = i;
    }
    ailen[brow] = nrow;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatILUFactor_SeqBAIJ"
PetscErrorCode MatILUFactor_SeqBAIJ(Mat inA,IS row,IS col,const MatFactorInfo *info)
{
  Mat_SeqBAIJ    *a = (Mat_SeqBAIJ*)inA->data;
  Mat            outA;
  PetscErrorCode ierr;
  PetscBool      row_identity,col_identity;

  PetscFunctionBegin;
  if (info->levels != 0) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SUP,"Only levels = 0 supported for in-place ILU");
  ierr = ISIdentity(row,&row_identity);CHKERRQ(ierr);
  ierr = ISIdentity(col,&col_identity);CHKERRQ(ierr);
  if (!row_identity || !col_identity) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONG,"Row and column permutations must be identity for in-place ILU");

  outA            = inA;
  inA->factortype = MAT_FACTOR_LU;

  ierr = MatMarkDiagonal_SeqBAIJ(inA);CHKERRQ(ierr);

  ierr   = PetscObjectReference((PetscObject)row);CHKERRQ(ierr);
  ierr   = ISDestroy(&a->row);CHKERRQ(ierr);
  a->row = row;
  ierr   = PetscObjectReference((PetscObject)col);CHKERRQ(ierr);
  ierr   = ISDestroy(&a->col);CHKERRQ(ierr);
  a->col = col;

  /* Create the invert permutation so that it can be used in MatLUFactorNumeric() */
  ierr = ISDestroy(&a->icol);CHKERRQ(ierr);
  ierr = ISInvertPermutation(col,PETSC_DECIDE,&a->icol);CHKERRQ(ierr);
  ierr = PetscLogObjectParent((PetscObject)inA,(PetscObject)a->icol);CHKERRQ(ierr);

  ierr = MatSeqBAIJSetNumericFactorization_inplace(inA,(PetscBool)(row_identity && col_identity));CHKERRQ(ierr);
  if (!a->solve_work) {
    ierr = PetscMalloc1(inA->rmap->N+inA->rmap->bs,&a->solve_work);CHKERRQ(ierr);
    ierr = PetscLogObjectMemory((PetscObject)inA,(inA->rmap->N+inA->rmap->bs)*sizeof(PetscScalar));CHKERRQ(ierr);
  }
  ierr = MatLUFactorNumeric(outA,inA,info);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSeqBAIJSetColumnIndices_SeqBAIJ"
PetscErrorCode  MatSeqBAIJSetColumnIndices_SeqBAIJ(Mat mat,PetscInt *indices)
{
  Mat_SeqBAIJ *baij = (Mat_SeqBAIJ*)mat->data;
  PetscInt    i,nz,mbs;

  PetscFunctionBegin;
  nz  = baij->maxnz;
  mbs = baij->mbs;
  for (i=0; i<nz; i++) {
    baij->j[i] = indices[i];
  }
  baij->nz = nz;
  for (i=0; i<mbs; i++) {
    baij->ilen[i] = baij->imax[i];
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSeqBAIJSetColumnIndices"
/*@
    MatSeqBAIJSetColumnIndices - Set the column indices for all the rows
       in the matrix.

  Input Parameters:
+  mat - the SeqBAIJ matrix
-  indices - the column indices

  Level: advanced

  Notes:
    This can be called if you have precomputed the nonzero structure of the
  matrix and want to provide it to the matrix object to improve the performance
  of the MatSetValues() operation.

    You MUST have set the correct numbers of nonzeros per row in the call to
  MatCreateSeqBAIJ(), and the columns indices MUST be sorted.

    MUST be called before any calls to MatSetValues();

@*/
PetscErrorCode  MatSeqBAIJSetColumnIndices(Mat mat,PetscInt *indices)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(mat,MAT_CLASSID,1);
  PetscValidPointer(indices,2);
  ierr = PetscUseMethod(mat,"MatSeqBAIJSetColumnIndices_C",(Mat,PetscInt*),(mat,indices));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatGetRowMaxAbs_SeqBAIJ"
PetscErrorCode MatGetRowMaxAbs_SeqBAIJ(Mat A,Vec v,PetscInt idx[])
{
  Mat_SeqBAIJ    *a = (Mat_SeqBAIJ*)A->data;
  PetscErrorCode ierr;
  PetscInt       i,j,n,row,bs,*ai,*aj,mbs;
  PetscReal      atmp;
  PetscScalar    *x,zero = 0.0;
  MatScalar      *aa;
  PetscInt       ncols,brow,krow,kcol;

  PetscFunctionBegin;
  if (A->factortype) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONGSTATE,"Not for factored matrix");
  bs  = A->rmap->bs;
  aa  = a->a;
  ai  = a->i;
  aj  = a->j;
  mbs = a->mbs;

  ierr = VecSet(v,zero);CHKERRQ(ierr);
  ierr = VecGetArray(v,&x);CHKERRQ(ierr);
  ierr = VecGetLocalSize(v,&n);CHKERRQ(ierr);
  if (n != A->rmap->N) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_SIZ,"Nonconforming matrix and vector");
  for (i=0; i<mbs; i++) {
    ncols = ai[1] - ai[0]; ai++;
    brow  = bs*i;
    for (j=0; j<ncols; j++) {
      for (kcol=0; kcol<bs; kcol++) {
        for (krow=0; krow<bs; krow++) {
          atmp = PetscAbsScalar(*aa);aa++;
          row  = brow + krow;   /* row index */
          if (PetscAbsScalar(x[row]) < atmp) {x[row] = atmp; if (idx) idx[row] = bs*(*aj) + kcol;}
        }
      }
      aj++;
    }
  }
  ierr = VecRestoreArray(v,&x);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatCopy_SeqBAIJ"
PetscErrorCode MatCopy_SeqBAIJ(Mat A,Mat B,MatStructure str)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  /* If the two matrices have the same copy implementation, use fast copy. */
  if (str == SAME_NONZERO_PATTERN && (A->ops->copy == B->ops->copy)) {
    Mat_SeqBAIJ *a  = (Mat_SeqBAIJ*)A->data;
    Mat_SeqBAIJ *b  = (Mat_SeqBAIJ*)B->data;
    PetscInt    ambs=a->mbs,bmbs=b->mbs,abs=A->rmap->bs,bbs=B->rmap->bs,bs2=abs*abs;

    if (a->i[ambs] != b->i[bmbs]) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_INCOMP,"Number of nonzero blocks in matrices A %D and B %D are different",a->i[ambs],b->i[bmbs]);
    if (abs != bbs) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_INCOMP,"Block size A %D and B %D are different",abs,bbs);
    ierr = PetscMemcpy(b->a,a->a,(bs2*a->i[ambs])*sizeof(PetscScalar));CHKERRQ(ierr);
  } else {
    ierr = MatCopy_Basic(A,B,str);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSetUp_SeqBAIJ"
PetscErrorCode MatSetUp_SeqBAIJ(Mat A)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MatSeqBAIJSetPreallocation_SeqBAIJ(A,A->rmap->bs,PETSC_DEFAULT,0);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSeqBAIJGetArray_SeqBAIJ"
PetscErrorCode MatSeqBAIJGetArray_SeqBAIJ(Mat A,PetscScalar *array[])
{
  Mat_SeqBAIJ *a = (Mat_SeqBAIJ*)A->data;

  PetscFunctionBegin;
  *array = a->a;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSeqBAIJRestoreArray_SeqBAIJ"
PetscErrorCode MatSeqBAIJRestoreArray_SeqBAIJ(Mat A,PetscScalar *array[])
{
  PetscFunctionBegin;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatAXPYGetPreallocation_SeqBAIJ"
PetscErrorCode MatAXPYGetPreallocation_SeqBAIJ(Mat Y,Mat X,PetscInt *nnz)
{
  PetscInt       bs = Y->rmap->bs,mbs = Y->rmap->N/bs;
  Mat_SeqBAIJ    *x = (Mat_SeqBAIJ*)X->data;
  Mat_SeqBAIJ    *y = (Mat_SeqBAIJ*)Y->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  /* Set the number of nonzeros in the new matrix */
  ierr = MatAXPYGetPreallocation_SeqX_private(mbs,x->i,x->j,y->i,y->j,nnz);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatAXPY_SeqBAIJ"
PetscErrorCode MatAXPY_SeqBAIJ(Mat Y,PetscScalar a,Mat X,MatStructure str)
{
  Mat_SeqBAIJ    *x = (Mat_SeqBAIJ*)X->data,*y = (Mat_SeqBAIJ*)Y->data;
  PetscErrorCode ierr;
  PetscInt       bs=Y->rmap->bs,bs2=bs*bs;
  PetscBLASInt   one=1;

  PetscFunctionBegin;
  if (str == SAME_NONZERO_PATTERN) {
    PetscScalar  alpha = a;
    PetscBLASInt bnz;
    ierr = PetscBLASIntCast(x->nz*bs2,&bnz);CHKERRQ(ierr);
    PetscStackCallBLAS("BLASaxpy",BLASaxpy_(&bnz,&alpha,x->a,&one,y->a,&one));
    ierr = PetscObjectStateIncrease((PetscObject)Y);CHKERRQ(ierr);
  } else if (str == SUBSET_NONZERO_PATTERN) { /* nonzeros of X is a subset of Y's */
    ierr = MatAXPY_Basic(Y,a,X,str);CHKERRQ(ierr);
  } else {
    Mat      B;
    PetscInt *nnz;
    if (bs != X->rmap->bs) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_SIZ,"Matrices must have same block size");
    ierr = PetscMalloc1(Y->rmap->N,&nnz);CHKERRQ(ierr);
    ierr = MatCreate(PetscObjectComm((PetscObject)Y),&B);CHKERRQ(ierr);
    ierr = PetscObjectSetName((PetscObject)B,((PetscObject)Y)->name);CHKERRQ(ierr);
    ierr = MatSetSizes(B,Y->rmap->n,Y->cmap->n,Y->rmap->N,Y->cmap->N);CHKERRQ(ierr);
    ierr = MatSetBlockSizesFromMats(B,Y,Y);CHKERRQ(ierr);
    ierr = MatSetType(B,(MatType) ((PetscObject)Y)->type_name);CHKERRQ(ierr);
    ierr = MatAXPYGetPreallocation_SeqBAIJ(Y,X,nnz);CHKERRQ(ierr);
    ierr = MatSeqBAIJSetPreallocation(B,bs,0,nnz);CHKERRQ(ierr);
    ierr = MatAXPY_BasicWithPreallocation(B,Y,a,X,str);CHKERRQ(ierr);
    ierr = MatHeaderReplace(Y,&B);CHKERRQ(ierr);
    ierr = PetscFree(nnz);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatRealPart_SeqBAIJ"
PetscErrorCode MatRealPart_SeqBAIJ(Mat A)
{
  Mat_SeqBAIJ *a = (Mat_SeqBAIJ*)A->data;
  PetscInt    i,nz = a->bs2*a->i[a->mbs];
  MatScalar   *aa = a->a;

  PetscFunctionBegin;
  for (i=0; i<nz; i++) aa[i] = PetscRealPart(aa[i]);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatImaginaryPart_SeqBAIJ"
PetscErrorCode MatImaginaryPart_SeqBAIJ(Mat A)
{
  Mat_SeqBAIJ *a = (Mat_SeqBAIJ*)A->data;
  PetscInt    i,nz = a->bs2*a->i[a->mbs];
  MatScalar   *aa = a->a;

  PetscFunctionBegin;
  for (i=0; i<nz; i++) aa[i] = PetscImaginaryPart(aa[i]);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatGetColumnIJ_SeqBAIJ"
/*
    Code almost idential to MatGetColumnIJ_SeqAIJ() should share common code
*/
PetscErrorCode MatGetColumnIJ_SeqBAIJ(Mat A,PetscInt oshift,PetscBool symmetric,PetscBool inodecompressed,PetscInt *nn,const PetscInt *ia[],const PetscInt *ja[],PetscBool  *done)
{
  Mat_SeqBAIJ    *a = (Mat_SeqBAIJ*)A->data;
  PetscErrorCode ierr;
  PetscInt       bs = A->rmap->bs,i,*collengths,*cia,*cja,n = A->cmap->n/bs,m = A->rmap->n/bs;
  PetscInt       nz = a->i[m],row,*jj,mr,col;

  PetscFunctionBegin;
  *nn = n;
  if (!ia) PetscFunctionReturn(0);
  if (symmetric) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SUP,"Not for BAIJ matrices");
  else {
    ierr = PetscCalloc1(n+1,&collengths);CHKERRQ(ierr);
    ierr = PetscMalloc1(n+1,&cia);CHKERRQ(ierr);
    ierr = PetscMalloc1(nz+1,&cja);CHKERRQ(ierr);
    jj   = a->j;
    for (i=0; i<nz; i++) {
      collengths[jj[i]]++;
    }
    cia[0] = oshift;
    for (i=0; i<n; i++) {
      cia[i+1] = cia[i] + collengths[i];
    }
    ierr = PetscMemzero(collengths,n*sizeof(PetscInt));CHKERRQ(ierr);
    jj   = a->j;
    for (row=0; row<m; row++) {
      mr = a->i[row+1] - a->i[row];
      for (i=0; i<mr; i++) {
        col = *jj++;

        cja[cia[col] + collengths[col]++ - oshift] = row + oshift;
      }
    }
    ierr = PetscFree(collengths);CHKERRQ(ierr);
    *ia  = cia; *ja = cja;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatRestoreColumnIJ_SeqBAIJ"
PetscErrorCode MatRestoreColumnIJ_SeqBAIJ(Mat A,PetscInt oshift,PetscBool symmetric,PetscBool inodecompressed,PetscInt *n,const PetscInt *ia[],const PetscInt *ja[],PetscBool  *done)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (!ia) PetscFunctionReturn(0);
  ierr = PetscFree(*ia);CHKERRQ(ierr);
  ierr = PetscFree(*ja);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/*
 MatGetColumnIJ_SeqBAIJ_Color() and MatRestoreColumnIJ_SeqBAIJ_Color() are customized from
 MatGetColumnIJ_SeqBAIJ() and MatRestoreColumnIJ_SeqBAIJ() by adding an output
 spidx[], index of a->a, to be used in MatTransposeColoringCreate() and MatFDColoringCreate()
 */
#undef __FUNCT__
#define __FUNCT__ "MatGetColumnIJ_SeqBAIJ_Color"
PetscErrorCode MatGetColumnIJ_SeqBAIJ_Color(Mat A,PetscInt oshift,PetscBool symmetric,PetscBool inodecompressed,PetscInt *nn,const PetscInt *ia[],const PetscInt *ja[],PetscInt *spidx[],PetscBool  *done)
{
  Mat_SeqBAIJ    *a = (Mat_SeqBAIJ*)A->data;
  PetscErrorCode ierr;
  PetscInt       i,*collengths,*cia,*cja,n=a->nbs,m=a->mbs;
  PetscInt       nz = a->i[m],row,*jj,mr,col;
  PetscInt       *cspidx;

  PetscFunctionBegin;
  *nn = n;
  if (!ia) PetscFunctionReturn(0);

  ierr = PetscCalloc1(n+1,&collengths);CHKERRQ(ierr);
  ierr = PetscMalloc1(n+1,&cia);CHKERRQ(ierr);
  ierr = PetscMalloc1(nz+1,&cja);CHKERRQ(ierr);
  ierr = PetscMalloc1(nz+1,&cspidx);CHKERRQ(ierr);
  jj   = a->j;
  for (i=0; i<nz; i++) {
    collengths[jj[i]]++;
  }
  cia[0] = oshift;
  for (i=0; i<n; i++) {
    cia[i+1] = cia[i] + collengths[i];
  }
  ierr = PetscMemzero(collengths,n*sizeof(PetscInt));CHKERRQ(ierr);
  jj   = a->j;
  for (row=0; row<m; row++) {
    mr = a->i[row+1] - a->i[row];
    for (i=0; i<mr; i++) {
      col = *jj++;
      cspidx[cia[col] + collengths[col] - oshift] = a->i[row] + i; /* index of a->j */
      cja[cia[col] + collengths[col]++ - oshift]  = row + oshift;
    }
  }
  ierr   = PetscFree(collengths);CHKERRQ(ierr);
  *ia    = cia; *ja = cja;
  *spidx = cspidx;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatRestoreColumnIJ_SeqBAIJ_Color"
PetscErrorCode MatRestoreColumnIJ_SeqBAIJ_Color(Mat A,PetscInt oshift,PetscBool symmetric,PetscBool inodecompressed,PetscInt *n,const PetscInt *ia[],const PetscInt *ja[],PetscInt *spidx[],PetscBool  *done)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MatRestoreColumnIJ_SeqBAIJ(A,oshift,symmetric,inodecompressed,n,ia,ja,done);CHKERRQ(ierr);
  ierr = PetscFree(*spidx);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatShift_SeqBAIJ"
PetscErrorCode MatShift_SeqBAIJ(Mat Y,PetscScalar a)
{
  PetscErrorCode ierr;
  Mat_SeqBAIJ     *aij = (Mat_SeqBAIJ*)Y->data;

  PetscFunctionBegin;
  if (!Y->preallocated || !aij->nz) {
    ierr = MatSeqBAIJSetPreallocation(Y,Y->rmap->bs,1,NULL);CHKERRQ(ierr);
  }
  ierr = MatShift_Basic(Y,a);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/* -------------------------------------------------------------------*/
static struct _MatOps MatOps_Values = {MatSetValues_SeqBAIJ,
                                       MatGetRow_SeqBAIJ,
                                       MatRestoreRow_SeqBAIJ,
                                       MatMult_SeqBAIJ_N,
                               /* 4*/  MatMultAdd_SeqBAIJ_N,
                                       MatMultTranspose_SeqBAIJ,
                                       MatMultTransposeAdd_SeqBAIJ,
                                       0,
                                       0,
                                       0,
                               /* 10*/ 0,
                                       MatLUFactor_SeqBAIJ,
                                       0,
                                       0,
                                       MatTranspose_SeqBAIJ,
                               /* 15*/ MatGetInfo_SeqBAIJ,
                                       MatEqual_SeqBAIJ,
                                       MatGetDiagonal_SeqBAIJ,
                                       MatDiagonalScale_SeqBAIJ,
                                       MatNorm_SeqBAIJ,
                               /* 20*/ 0,
                                       MatAssemblyEnd_SeqBAIJ,
                                       MatSetOption_SeqBAIJ,
                                       MatZeroEntries_SeqBAIJ,
                               /* 24*/ MatZeroRows_SeqBAIJ,
                                       0,
                                       0,
                                       0,
                                       0,
                               /* 29*/ MatSetUp_SeqBAIJ,
                                       0,
                                       0,
                                       0,
                                       0,
                               /* 34*/ MatDuplicate_SeqBAIJ,
                                       0,
                                       0,
                                       MatILUFactor_SeqBAIJ,
                                       0,
                               /* 39*/ MatAXPY_SeqBAIJ,
                                       MatGetSubMatrices_SeqBAIJ,
                                       MatIncreaseOverlap_SeqBAIJ,
                                       MatGetValues_SeqBAIJ,
                                       MatCopy_SeqBAIJ,
                               /* 44*/ 0,
                                       MatScale_SeqBAIJ,
                                       MatShift_SeqBAIJ,
                                       0,
                                       MatZeroRowsColumns_SeqBAIJ,
                               /* 49*/ 0,
                                       MatGetRowIJ_SeqBAIJ,
                                       MatRestoreRowIJ_SeqBAIJ,
                                       MatGetColumnIJ_SeqBAIJ,
                                       MatRestoreColumnIJ_SeqBAIJ,
                               /* 54*/ MatFDColoringCreate_SeqXAIJ,
                                       0,
                                       0,
                                       0,
                                       MatSetValuesBlocked_SeqBAIJ,
                               /* 59*/ MatGetSubMatrix_SeqBAIJ,
                                       MatDestroy_SeqBAIJ,
                                       MatView_SeqBAIJ,
                                       0,
                                       0,
                               /* 64*/ 0,
                                       0,
                                       0,
                                       0,
                                       0,
                               /* 69*/ MatGetRowMaxAbs_SeqBAIJ,
                                       0,
                                       MatConvert_Basic,
                                       0,
                                       0,
                               /* 74*/ 0,
                                       MatFDColoringApply_BAIJ,
                                       0,
                                       0,
                                       0,
                               /* 79*/ 0,
                                       0,
                                       0,
                                       0,
                                       MatLoad_SeqBAIJ,
                               /* 84*/ 0,
                                       0,
                                       0,
                                       0,
                                       0,
                               /* 89*/ 0,
                                       0,
                                       0,
                                       0,
                                       0,
                               /* 94*/ 0,
                                       0,
                                       0,
                                       0,
                                       0,
                               /* 99*/ 0,
                                       0,
                                       0,
                                       0,
                                       0,
                               /*104*/ 0,
                                       MatRealPart_SeqBAIJ,
                                       MatImaginaryPart_SeqBAIJ,
                                       0,
                                       0,
                               /*109*/ 0,
                                       0,
                                       0,
                                       0,
                                       MatMissingDiagonal_SeqBAIJ,
                               /*114*/ 0,
                                       0,
                                       0,
                                       0,
                                       0,
                               /*119*/ 0,
                                       0,
                                       MatMultHermitianTranspose_SeqBAIJ,
                                       MatMultHermitianTransposeAdd_SeqBAIJ,
                                       0,
                               /*124*/ 0,
                                       0,
                                       MatInvertBlockDiagonal_SeqBAIJ,
                                       0,
                                       0,
                               /*129*/ 0,
                                       0,
                                       0,
                                       0,
                                       0,
                               /*134*/ 0,
                                       0,
                                       0,
                                       0,
                                       0,
                               /*139*/ 0,
                                       0,
                                       0,
                                       MatFDColoringSetUp_SeqXAIJ,
                                       0,
                                /*144*/MatCreateMPIMatConcatenateSeqMat_SeqBAIJ
};

#undef __FUNCT__
#define __FUNCT__ "MatStoreValues_SeqBAIJ"
PetscErrorCode  MatStoreValues_SeqBAIJ(Mat mat)
{
  Mat_SeqBAIJ    *aij = (Mat_SeqBAIJ*)mat->data;
  PetscInt       nz   = aij->i[aij->mbs]*aij->bs2;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (aij->nonew != 1) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ORDER,"Must call MatSetOption(A,MAT_NEW_NONZERO_LOCATIONS,PETSC_FALSE);first");

  /* allocate space for values if not already there */
  if (!aij->saved_values) {
    ierr = PetscMalloc1(nz+1,&aij->saved_values);CHKERRQ(ierr);
    ierr = PetscLogObjectMemory((PetscObject)mat,(nz+1)*sizeof(PetscScalar));CHKERRQ(ierr);
  }

  /* copy values over */
  ierr = PetscMemcpy(aij->saved_values,aij->a,nz*sizeof(PetscScalar));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatRetrieveValues_SeqBAIJ"
PetscErrorCode  MatRetrieveValues_SeqBAIJ(Mat mat)
{
  Mat_SeqBAIJ    *aij = (Mat_SeqBAIJ*)mat->data;
  PetscErrorCode ierr;
  PetscInt       nz = aij->i[aij->mbs]*aij->bs2;

  PetscFunctionBegin;
  if (aij->nonew != 1) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ORDER,"Must call MatSetOption(A,MAT_NEW_NONZERO_LOCATIONS,PETSC_FALSE);first");
  if (!aij->saved_values) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ORDER,"Must call MatStoreValues(A);first");

  /* copy values over */
  ierr = PetscMemcpy(aij->a,aij->saved_values,nz*sizeof(PetscScalar));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

PETSC_EXTERN PetscErrorCode MatConvert_SeqBAIJ_SeqAIJ(Mat, MatType,MatReuse,Mat*);
PETSC_EXTERN PetscErrorCode MatConvert_SeqBAIJ_SeqSBAIJ(Mat, MatType,MatReuse,Mat*);

#undef __FUNCT__
#define __FUNCT__ "MatSeqBAIJSetPreallocation_SeqBAIJ"
PetscErrorCode  MatSeqBAIJSetPreallocation_SeqBAIJ(Mat B,PetscInt bs,PetscInt nz,PetscInt *nnz)
{
  Mat_SeqBAIJ    *b;
  PetscErrorCode ierr;
  PetscInt       i,mbs,nbs,bs2;
  PetscBool      flg = PETSC_FALSE,skipallocation = PETSC_FALSE,realalloc = PETSC_FALSE;

  PetscFunctionBegin;
  if (nz >= 0 || nnz) realalloc = PETSC_TRUE;
  if (nz == MAT_SKIP_ALLOCATION) {
    skipallocation = PETSC_TRUE;
    nz             = 0;
  }

  ierr = MatSetBlockSize(B,PetscAbs(bs));CHKERRQ(ierr);
  ierr = PetscLayoutSetUp(B->rmap);CHKERRQ(ierr);
  ierr = PetscLayoutSetUp(B->cmap);CHKERRQ(ierr);
  ierr = PetscLayoutGetBlockSize(B->rmap,&bs);CHKERRQ(ierr);

  B->preallocated = PETSC_TRUE;

  mbs = B->rmap->n/bs;
  nbs = B->cmap->n/bs;
  bs2 = bs*bs;

  if (mbs*bs!=B->rmap->n || nbs*bs!=B->cmap->n) SETERRQ3(PETSC_COMM_SELF,PETSC_ERR_ARG_SIZ,"Number rows %D, cols %D must be divisible by blocksize %D",B->rmap->N,B->cmap->n,bs);

  if (nz == PETSC_DEFAULT || nz == PETSC_DECIDE) nz = 5;
  if (nz < 0) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"nz cannot be less than 0: value %D",nz);
  if (nnz) {
    for (i=0; i<mbs; i++) {
      if (nnz[i] < 0) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"nnz cannot be less than 0: local row %D value %D",i,nnz[i]);
      if (nnz[i] > nbs) SETERRQ3(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"nnz cannot be greater than block row length: local row %D value %D rowlength %D",i,nnz[i],nbs);
    }
  }

  b    = (Mat_SeqBAIJ*)B->data;
  ierr = PetscOptionsBegin(PetscObjectComm((PetscObject)B),NULL,"Optimize options for SEQBAIJ matrix 2 ","Mat");CHKERRQ(ierr);
  ierr = PetscOptionsBool("-mat_no_unroll","Do not optimize for block size (slow)",NULL,flg,&flg,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsEnd();CHKERRQ(ierr);

  if (!flg) {
    switch (bs) {
    case 1:
      B->ops->mult    = MatMult_SeqBAIJ_1;
      B->ops->multadd = MatMultAdd_SeqBAIJ_1;
      break;
    case 2:
      B->ops->mult    = MatMult_SeqBAIJ_2;
      B->ops->multadd = MatMultAdd_SeqBAIJ_2;
      break;
    case 3:
      B->ops->mult    = MatMult_SeqBAIJ_3;
      B->ops->multadd = MatMultAdd_SeqBAIJ_3;
      break;
    case 4:
      B->ops->mult    = MatMult_SeqBAIJ_4;
      B->ops->multadd = MatMultAdd_SeqBAIJ_4;
      break;
    case 5:
      B->ops->mult    = MatMult_SeqBAIJ_5;
      B->ops->multadd = MatMultAdd_SeqBAIJ_5;
      break;
    case 6:
      B->ops->mult    = MatMult_SeqBAIJ_6;
      B->ops->multadd = MatMultAdd_SeqBAIJ_6;
      break;
    case 7:
      B->ops->mult    = MatMult_SeqBAIJ_7;
      B->ops->multadd = MatMultAdd_SeqBAIJ_7;
      break;
    case 15:
      B->ops->mult    = MatMult_SeqBAIJ_15_ver1;
      B->ops->multadd = MatMultAdd_SeqBAIJ_N;
      break;
    default:
      B->ops->mult    = MatMult_SeqBAIJ_N;
      B->ops->multadd = MatMultAdd_SeqBAIJ_N;
      break;
    }
  }
  B->ops->sor = MatSOR_SeqBAIJ;
  b->mbs = mbs;
  b->nbs = nbs;
  if (!skipallocation) {
    if (!b->imax) {
      ierr = PetscMalloc2(mbs,&b->imax,mbs,&b->ilen);CHKERRQ(ierr);
      ierr = PetscLogObjectMemory((PetscObject)B,2*mbs*sizeof(PetscInt));CHKERRQ(ierr);

      b->free_imax_ilen = PETSC_TRUE;
    }
    /* b->ilen will count nonzeros in each block row so far. */
    for (i=0; i<mbs; i++) b->ilen[i] = 0;
    if (!nnz) {
      if (nz == PETSC_DEFAULT || nz == PETSC_DECIDE) nz = 5;
      else if (nz < 0) nz = 1;
      for (i=0; i<mbs; i++) b->imax[i] = nz;
      nz = nz*mbs;
    } else {
      nz = 0;
      for (i=0; i<mbs; i++) {b->imax[i] = nnz[i]; nz += nnz[i];}
    }

    /* allocate the matrix space */
    ierr = MatSeqXAIJFreeAIJ(B,&b->a,&b->j,&b->i);CHKERRQ(ierr);
    ierr = PetscMalloc3(bs2*nz,&b->a,nz,&b->j,B->rmap->N+1,&b->i);CHKERRQ(ierr);
    ierr = PetscLogObjectMemory((PetscObject)B,(B->rmap->N+1)*sizeof(PetscInt)+nz*(bs2*sizeof(PetscScalar)+sizeof(PetscInt)));CHKERRQ(ierr);
    ierr = PetscMemzero(b->a,nz*bs2*sizeof(MatScalar));CHKERRQ(ierr);
    ierr = PetscMemzero(b->j,nz*sizeof(PetscInt));CHKERRQ(ierr);

    b->singlemalloc = PETSC_TRUE;
    b->i[0]         = 0;
    for (i=1; i<mbs+1; i++) {
      b->i[i] = b->i[i-1] + b->imax[i-1];
    }
    b->free_a  = PETSC_TRUE;
    b->free_ij = PETSC_TRUE;
  } else {
    b->free_a  = PETSC_FALSE;
    b->free_ij = PETSC_FALSE;
  }

  b->bs2              = bs2;
  b->mbs              = mbs;
  b->nz               = 0;
  b->maxnz            = nz;
  B->info.nz_unneeded = (PetscReal)b->maxnz*bs2;
  if (realalloc) {ierr = MatSetOption(B,MAT_NEW_NONZERO_ALLOCATION_ERR,PETSC_TRUE);CHKERRQ(ierr);}
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSeqBAIJSetPreallocationCSR_SeqBAIJ"
PetscErrorCode MatSeqBAIJSetPreallocationCSR_SeqBAIJ(Mat B,PetscInt bs,const PetscInt ii[],const PetscInt jj[],const PetscScalar V[])
{
  PetscInt       i,m,nz,nz_max=0,*nnz;
  PetscScalar    *values=0;
  PetscBool      roworiented = ((Mat_SeqBAIJ*)B->data)->roworiented;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (bs < 1) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Invalid block size specified, must be positive but it is %D",bs);
  ierr = PetscLayoutSetBlockSize(B->rmap,bs);CHKERRQ(ierr);
  ierr = PetscLayoutSetBlockSize(B->cmap,bs);CHKERRQ(ierr);
  ierr = PetscLayoutSetUp(B->rmap);CHKERRQ(ierr);
  ierr = PetscLayoutSetUp(B->cmap);CHKERRQ(ierr);
  ierr = PetscLayoutGetBlockSize(B->rmap,&bs);CHKERRQ(ierr);
  m    = B->rmap->n/bs;

  if (ii[0] != 0) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE, "ii[0] must be 0 but it is %D",ii[0]);
  ierr = PetscMalloc1(m+1, &nnz);CHKERRQ(ierr);
  for (i=0; i<m; i++) {
    nz = ii[i+1]- ii[i];
    if (nz < 0) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE, "Local row %D has a negative number of columns %D",i,nz);
    nz_max = PetscMax(nz_max, nz);
    nnz[i] = nz;
  }
  ierr = MatSeqBAIJSetPreallocation(B,bs,0,nnz);CHKERRQ(ierr);
  ierr = PetscFree(nnz);CHKERRQ(ierr);

  values = (PetscScalar*)V;
  if (!values) {
    ierr = PetscCalloc1(bs*bs*(nz_max+1),&values);CHKERRQ(ierr);
  }
  for (i=0; i<m; i++) {
    PetscInt          ncols  = ii[i+1] - ii[i];
    const PetscInt    *icols = jj + ii[i];
    const PetscScalar *svals = values + (V ? (bs*bs*ii[i]) : 0);
    if (!roworiented) {
      ierr = MatSetValuesBlocked_SeqBAIJ(B,1,&i,ncols,icols,svals,INSERT_VALUES);CHKERRQ(ierr);
    } else {
      PetscInt j;
      for (j=0; j<ncols; j++) {
        const PetscScalar *svals = values + (V ? (bs*bs*(ii[i]+j)) : 0);
        ierr = MatSetValuesBlocked_SeqBAIJ(B,1,&i,1,&icols[j],svals,INSERT_VALUES);CHKERRQ(ierr);
      }
    }
  }
  if (!V) { ierr = PetscFree(values);CHKERRQ(ierr); }
  ierr = MatAssemblyBegin(B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatSetOption(B,MAT_NEW_NONZERO_LOCATION_ERR,PETSC_TRUE);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/*MC
   MATSEQBAIJ - MATSEQBAIJ = "seqbaij" - A matrix type to be used for sequential block sparse matrices, based on
   block sparse compressed row format.

   Options Database Keys:
. -mat_type seqbaij - sets the matrix type to "seqbaij" during a call to MatSetFromOptions()

  Level: beginner

.seealso: MatCreateSeqBAIJ()
M*/

PETSC_EXTERN PetscErrorCode MatConvert_SeqBAIJ_SeqBSTRM(Mat, MatType,MatReuse,Mat*);

#undef __FUNCT__
#define __FUNCT__ "MatCreate_SeqBAIJ"
PETSC_EXTERN PetscErrorCode MatCreate_SeqBAIJ(Mat B)
{
  PetscErrorCode ierr;
  PetscMPIInt    size;
  Mat_SeqBAIJ    *b;

  PetscFunctionBegin;
  ierr = MPI_Comm_size(PetscObjectComm((PetscObject)B),&size);CHKERRQ(ierr);
  if (size > 1) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONG,"Comm must be of size 1");

  ierr    = PetscNewLog(B,&b);CHKERRQ(ierr);
  B->data = (void*)b;
  ierr    = PetscMemcpy(B->ops,&MatOps_Values,sizeof(struct _MatOps));CHKERRQ(ierr);

  b->row          = 0;
  b->col          = 0;
  b->icol         = 0;
  b->reallocs     = 0;
  b->saved_values = 0;

  b->roworiented        = PETSC_TRUE;
  b->nonew              = 0;
  b->diag               = 0;
  B->spptr              = 0;
  B->info.nz_unneeded   = (PetscReal)b->maxnz*b->bs2;
  b->keepnonzeropattern = PETSC_FALSE;

  ierr = PetscObjectComposeFunction((PetscObject)B,"MatInvertBlockDiagonal_C",MatInvertBlockDiagonal_SeqBAIJ);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatStoreValues_C",MatStoreValues_SeqBAIJ);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatRetrieveValues_C",MatRetrieveValues_SeqBAIJ);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatSeqBAIJSetColumnIndices_C",MatSeqBAIJSetColumnIndices_SeqBAIJ);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatConvert_seqbaij_seqaij_C",MatConvert_SeqBAIJ_SeqAIJ);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatConvert_seqbaij_seqsbaij_C",MatConvert_SeqBAIJ_SeqSBAIJ);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatSeqBAIJSetPreallocation_C",MatSeqBAIJSetPreallocation_SeqBAIJ);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatSeqBAIJSetPreallocationCSR_C",MatSeqBAIJSetPreallocationCSR_SeqBAIJ);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatConvert_seqbaij_seqbstrm_C",MatConvert_SeqBAIJ_SeqBSTRM);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)B,"MatIsTranspose_C",MatIsTranspose_SeqBAIJ);CHKERRQ(ierr);
  ierr = PetscObjectChangeTypeName((PetscObject)B,MATSEQBAIJ);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatDuplicateNoCreate_SeqBAIJ"
PetscErrorCode MatDuplicateNoCreate_SeqBAIJ(Mat C,Mat A,MatDuplicateOption cpvalues,PetscBool mallocmatspace)
{
  Mat_SeqBAIJ    *c = (Mat_SeqBAIJ*)C->data,*a = (Mat_SeqBAIJ*)A->data;
  PetscErrorCode ierr;
  PetscInt       i,mbs = a->mbs,nz = a->nz,bs2 = a->bs2;

  PetscFunctionBegin;
  if (a->i[mbs] != nz) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_PLIB,"Corrupt matrix");

  if (cpvalues == MAT_SHARE_NONZERO_PATTERN) {
    c->imax           = a->imax;
    c->ilen           = a->ilen;
    c->free_imax_ilen = PETSC_FALSE;
  } else {
    ierr = PetscMalloc2(mbs,&c->imax,mbs,&c->ilen);CHKERRQ(ierr);
    ierr = PetscLogObjectMemory((PetscObject)C,2*mbs*sizeof(PetscInt));CHKERRQ(ierr);
    for (i=0; i<mbs; i++) {
      c->imax[i] = a->imax[i];
      c->ilen[i] = a->ilen[i];
    }
    c->free_imax_ilen = PETSC_TRUE;
  }

  /* allocate the matrix space */
  if (mallocmatspace) {
    if (cpvalues == MAT_SHARE_NONZERO_PATTERN) {
      ierr = PetscCalloc1(bs2*nz,&c->a);CHKERRQ(ierr);
      ierr = PetscLogObjectMemory((PetscObject)C,a->i[mbs]*bs2*sizeof(PetscScalar));CHKERRQ(ierr);

      c->i            = a->i;
      c->j            = a->j;
      c->singlemalloc = PETSC_FALSE;
      c->free_a       = PETSC_TRUE;
      c->free_ij      = PETSC_FALSE;
      c->parent       = A;
      C->preallocated = PETSC_TRUE;
      C->assembled    = PETSC_TRUE;

      ierr = PetscObjectReference((PetscObject)A);CHKERRQ(ierr);
      ierr = MatSetOption(A,MAT_NEW_NONZERO_LOCATION_ERR,PETSC_TRUE);CHKERRQ(ierr);
      ierr = MatSetOption(C,MAT_NEW_NONZERO_LOCATION_ERR,PETSC_TRUE);CHKERRQ(ierr);
    } else {
      ierr = PetscMalloc3(bs2*nz,&c->a,nz,&c->j,mbs+1,&c->i);CHKERRQ(ierr);
      ierr = PetscLogObjectMemory((PetscObject)C,a->i[mbs]*(bs2*sizeof(PetscScalar)+sizeof(PetscInt))+(mbs+1)*sizeof(PetscInt));CHKERRQ(ierr);

      c->singlemalloc = PETSC_TRUE;
      c->free_a       = PETSC_TRUE;
      c->free_ij      = PETSC_TRUE;

      ierr = PetscMemcpy(c->i,a->i,(mbs+1)*sizeof(PetscInt));CHKERRQ(ierr);
      if (mbs > 0) {
        ierr = PetscMemcpy(c->j,a->j,nz*sizeof(PetscInt));CHKERRQ(ierr);
        if (cpvalues == MAT_COPY_VALUES) {
          ierr = PetscMemcpy(c->a,a->a,bs2*nz*sizeof(MatScalar));CHKERRQ(ierr);
        } else {
          ierr = PetscMemzero(c->a,bs2*nz*sizeof(MatScalar));CHKERRQ(ierr);
        }
      }
      C->preallocated = PETSC_TRUE;
      C->assembled    = PETSC_TRUE;
    }
  }

  c->roworiented = a->roworiented;
  c->nonew       = a->nonew;

  ierr = PetscLayoutReference(A->rmap,&C->rmap);CHKERRQ(ierr);
  ierr = PetscLayoutReference(A->cmap,&C->cmap);CHKERRQ(ierr);

  c->bs2         = a->bs2;
  c->mbs         = a->mbs;
  c->nbs         = a->nbs;

  if (a->diag) {
    if (cpvalues == MAT_SHARE_NONZERO_PATTERN) {
      c->diag      = a->diag;
      c->free_diag = PETSC_FALSE;
    } else {
      ierr = PetscMalloc1(mbs+1,&c->diag);CHKERRQ(ierr);
      ierr = PetscLogObjectMemory((PetscObject)C,(mbs+1)*sizeof(PetscInt));CHKERRQ(ierr);
      for (i=0; i<mbs; i++) c->diag[i] = a->diag[i];
      c->free_diag = PETSC_TRUE;
    }
  } else c->diag = 0;

  c->nz         = a->nz;
  c->maxnz      = a->nz;         /* Since we allocate exactly the right amount */
  c->solve_work = NULL;
  c->mult_work  = NULL;
  c->sor_workt  = NULL;
  c->sor_work   = NULL;

  c->compressedrow.use   = a->compressedrow.use;
  c->compressedrow.nrows = a->compressedrow.nrows;
  if (a->compressedrow.use) {
    i    = a->compressedrow.nrows;
    ierr = PetscMalloc2(i+1,&c->compressedrow.i,i+1,&c->compressedrow.rindex);CHKERRQ(ierr);
    ierr = PetscLogObjectMemory((PetscObject)C,(2*i+1)*sizeof(PetscInt));CHKERRQ(ierr);
    ierr = PetscMemcpy(c->compressedrow.i,a->compressedrow.i,(i+1)*sizeof(PetscInt));CHKERRQ(ierr);
    ierr = PetscMemcpy(c->compressedrow.rindex,a->compressedrow.rindex,i*sizeof(PetscInt));CHKERRQ(ierr);
  } else {
    c->compressedrow.use    = PETSC_FALSE;
    c->compressedrow.i      = NULL;
    c->compressedrow.rindex = NULL;
  }
  C->nonzerostate = A->nonzerostate;

  ierr = PetscFunctionListDuplicate(((PetscObject)A)->qlist,&((PetscObject)C)->qlist);CHKERRQ(ierr);
  ierr = PetscMemcpy(C->ops,A->ops,sizeof(struct _MatOps));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatDuplicate_SeqBAIJ"
PetscErrorCode MatDuplicate_SeqBAIJ(Mat A,MatDuplicateOption cpvalues,Mat *B)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MatCreate(PetscObjectComm((PetscObject)A),B);CHKERRQ(ierr);
  ierr = MatSetSizes(*B,A->rmap->N,A->cmap->n,A->rmap->N,A->cmap->n);CHKERRQ(ierr);
  ierr = MatSetType(*B,MATSEQBAIJ);CHKERRQ(ierr);
  ierr = MatDuplicateNoCreate_SeqBAIJ(*B,A,cpvalues,PETSC_TRUE);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatLoad_SeqBAIJ"
PetscErrorCode MatLoad_SeqBAIJ(Mat newmat,PetscViewer viewer)
{
  Mat_SeqBAIJ    *a;
  PetscErrorCode ierr;
  PetscInt       i,nz,header[4],*rowlengths=0,M,N,bs = newmat->rmap->bs;
  PetscInt       *mask,mbs,*jj,j,rowcount,nzcount,k,*browlengths,maskcount;
  PetscInt       kmax,jcount,block,idx,point,nzcountb,extra_rows,rows,cols;
  PetscInt       *masked,nmask,tmp,bs2,ishift;
  PetscMPIInt    size;
  int            fd;
  PetscScalar    *aa;
  MPI_Comm       comm;

  PetscFunctionBegin;
  /* force binary viewer to load .info file if it has not yet done so */
  ierr = PetscViewerSetUp(viewer);CHKERRQ(ierr);
  ierr = PetscObjectGetComm((PetscObject)viewer,&comm);CHKERRQ(ierr);
  ierr = PetscOptionsBegin(comm,NULL,"Options for loading SEQBAIJ matrix","Mat");CHKERRQ(ierr);
  ierr = PetscOptionsInt("-matload_block_size","Set the blocksize used to store the matrix","MatLoad",bs,&bs,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsEnd();CHKERRQ(ierr);
  if (bs < 0) bs = 1;
  bs2  = bs*bs;

  ierr = MPI_Comm_size(comm,&size);CHKERRQ(ierr);
  if (size > 1) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONG,"view must have one processor");
  ierr = PetscViewerBinaryGetDescriptor(viewer,&fd);CHKERRQ(ierr);
  ierr = PetscBinaryRead(fd,header,4,PETSC_INT);CHKERRQ(ierr);
  if (header[0] != MAT_FILE_CLASSID) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_FILE_UNEXPECTED,"not Mat object");
  M = header[1]; N = header[2]; nz = header[3];

  if (header[3] < 0) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_FILE_UNEXPECTED,"Matrix stored in special format, cannot load as SeqBAIJ");
  if (M != N) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SUP,"Can only do square matrices");

  /*
     This code adds extra rows to make sure the number of rows is
    divisible by the blocksize
  */
  mbs        = M/bs;
  extra_rows = bs - M + bs*(mbs);
  if (extra_rows == bs) extra_rows = 0;
  else mbs++;
  if (extra_rows) {
    ierr = PetscInfo(viewer,"Padding loaded matrix to match blocksize\n");CHKERRQ(ierr);
  }

  /* Set global sizes if not already set */
  if (newmat->rmap->n < 0 && newmat->rmap->N < 0 && newmat->cmap->n < 0 && newmat->cmap->N < 0) {
    ierr = MatSetSizes(newmat,PETSC_DECIDE,PETSC_DECIDE,M+extra_rows,N+extra_rows);CHKERRQ(ierr);
  } else { /* Check if the matrix global sizes are correct */
    ierr = MatGetSize(newmat,&rows,&cols);CHKERRQ(ierr);
    if (rows < 0 && cols < 0) { /* user might provide local size instead of global size */
      ierr = MatGetLocalSize(newmat,&rows,&cols);CHKERRQ(ierr);
    }
    if (M != rows ||  N != cols) SETERRQ4(PETSC_COMM_SELF,PETSC_ERR_FILE_UNEXPECTED,"Matrix in file of different length (%d, %d) than the input matrix (%d, %d)",M,N,rows,cols);
  }

  /* read in row lengths */
  ierr = PetscMalloc1(M+extra_rows,&rowlengths);CHKERRQ(ierr);
  ierr = PetscBinaryRead(fd,rowlengths,M,PETSC_INT);CHKERRQ(ierr);
  for (i=0; i<extra_rows; i++) rowlengths[M+i] = 1;

  /* read in column indices */
  ierr = PetscMalloc1(nz+extra_rows,&jj);CHKERRQ(ierr);
  ierr = PetscBinaryRead(fd,jj,nz,PETSC_INT);CHKERRQ(ierr);
  for (i=0; i<extra_rows; i++) jj[nz+i] = M+i;

  /* loop over row lengths determining block row lengths */
  ierr     = PetscCalloc1(mbs,&browlengths);CHKERRQ(ierr);
  ierr     = PetscMalloc2(mbs,&mask,mbs,&masked);CHKERRQ(ierr);
  ierr     = PetscMemzero(mask,mbs*sizeof(PetscInt));CHKERRQ(ierr);
  rowcount = 0;
  nzcount  = 0;
  for (i=0; i<mbs; i++) {
    nmask = 0;
    for (j=0; j<bs; j++) {
      kmax = rowlengths[rowcount];
      for (k=0; k<kmax; k++) {
        tmp = jj[nzcount++]/bs;
        if (!mask[tmp]) {masked[nmask++] = tmp; mask[tmp] = 1;}
      }
      rowcount++;
    }
    browlengths[i] += nmask;
    /* zero out the mask elements we set */
    for (j=0; j<nmask; j++) mask[masked[j]] = 0;
  }

  /* Do preallocation  */
  ierr = MatSeqBAIJSetPreallocation_SeqBAIJ(newmat,bs,0,browlengths);CHKERRQ(ierr);
  a    = (Mat_SeqBAIJ*)newmat->data;

  /* set matrix "i" values */
  a->i[0] = 0;
  for (i=1; i<= mbs; i++) {
    a->i[i]      = a->i[i-1] + browlengths[i-1];
    a->ilen[i-1] = browlengths[i-1];
  }
  a->nz = 0;
  for (i=0; i<mbs; i++) a->nz += browlengths[i];

  /* read in nonzero values */
  ierr = PetscMalloc1(nz+extra_rows,&aa);CHKERRQ(ierr);
  ierr = PetscBinaryRead(fd,aa,nz,PETSC_SCALAR);CHKERRQ(ierr);
  for (i=0; i<extra_rows; i++) aa[nz+i] = 1.0;

  /* set "a" and "j" values into matrix */
  nzcount = 0; jcount = 0;
  for (i=0; i<mbs; i++) {
    nzcountb = nzcount;
    nmask    = 0;
    for (j=0; j<bs; j++) {
      kmax = rowlengths[i*bs+j];
      for (k=0; k<kmax; k++) {
        tmp = jj[nzcount++]/bs;
        if (!mask[tmp]) { masked[nmask++] = tmp; mask[tmp] = 1;}
      }
    }
    /* sort the masked values */
    ierr = PetscSortInt(nmask,masked);CHKERRQ(ierr);

    /* set "j" values into matrix */
    maskcount = 1;
    for (j=0; j<nmask; j++) {
      a->j[jcount++]  = masked[j];
      mask[masked[j]] = maskcount++;
    }
    /* set "a" values into matrix */
    ishift = bs2*a->i[i];
    for (j=0; j<bs; j++) {
      kmax = rowlengths[i*bs+j];
      for (k=0; k<kmax; k++) {
        tmp       = jj[nzcountb]/bs;
        block     = mask[tmp] - 1;
        point     = jj[nzcountb] - bs*tmp;
        idx       = ishift + bs2*block + j + bs*point;
        a->a[idx] = (MatScalar)aa[nzcountb++];
      }
    }
    /* zero out the mask elements we set */
    for (j=0; j<nmask; j++) mask[masked[j]] = 0;
  }
  if (jcount != a->nz) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_FILE_UNEXPECTED,"Bad binary matrix");

  ierr = PetscFree(rowlengths);CHKERRQ(ierr);
  ierr = PetscFree(browlengths);CHKERRQ(ierr);
  ierr = PetscFree(aa);CHKERRQ(ierr);
  ierr = PetscFree(jj);CHKERRQ(ierr);
  ierr = PetscFree2(mask,masked);CHKERRQ(ierr);

  ierr = MatAssemblyBegin(newmat,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(newmat,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatCreateSeqBAIJ"
/*@C
   MatCreateSeqBAIJ - Creates a sparse matrix in block AIJ (block
   compressed row) format.  For good matrix assembly performance the
   user should preallocate the matrix storage by setting the parameter nz
   (or the array nnz).  By setting these parameters accurately, performance
   during matrix assembly can be increased by more than a factor of 50.

   Collective on MPI_Comm

   Input Parameters:
+  comm - MPI communicator, set to PETSC_COMM_SELF
.  bs - size of block, the blocks are ALWAYS square. One can use MatSetBlockSizes() to set a different row and column blocksize but the row
          blocksize always defines the size of the blocks. The column blocksize sets the blocksize of the vectors obtained with MatCreateVecs()
.  m - number of rows
.  n - number of columns
.  nz - number of nonzero blocks  per block row (same for all rows)
-  nnz - array containing the number of nonzero blocks in the various block rows
         (possibly different for each block row) or NULL

   Output Parameter:
.  A - the matrix

   It is recommended that one use the MatCreate(), MatSetType() and/or MatSetFromOptions(),
   MatXXXXSetPreallocation() paradgm instead of this routine directly.
   [MatXXXXSetPreallocation() is, for example, MatSeqAIJSetPreallocation]

   Options Database Keys:
.   -mat_no_unroll - uses code that does not unroll the loops in the
                     block calculations (much slower)
.    -mat_block_size - size of the blocks to use

   Level: intermediate

   Notes:
   The number of rows and columns must be divisible by blocksize.

   If the nnz parameter is given then the nz parameter is ignored

   A nonzero block is any block that as 1 or more nonzeros in it

   The block AIJ format is fully compatible with standard Fortran 77
   storage.  That is, the stored row and column indices can begin at
   either one (as in Fortran) or zero.  See the users' manual for details.

   Specify the preallocated storage with either nz or nnz (not both).
   Set nz=PETSC_DEFAULT and nnz=NULL for PETSc to control dynamic memory
   allocation.  See Users-Manual: ch_mat for details.
   matrices.

.seealso: MatCreate(), MatCreateSeqAIJ(), MatSetValues(), MatCreateBAIJ()
@*/
PetscErrorCode  MatCreateSeqBAIJ(MPI_Comm comm,PetscInt bs,PetscInt m,PetscInt n,PetscInt nz,const PetscInt nnz[],Mat *A)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MatCreate(comm,A);CHKERRQ(ierr);
  ierr = MatSetSizes(*A,m,n,m,n);CHKERRQ(ierr);
  ierr = MatSetType(*A,MATSEQBAIJ);CHKERRQ(ierr);
  ierr = MatSeqBAIJSetPreallocation_SeqBAIJ(*A,bs,nz,(PetscInt*)nnz);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSeqBAIJSetPreallocation"
/*@C
   MatSeqBAIJSetPreallocation - Sets the block size and expected nonzeros
   per row in the matrix. For good matrix assembly performance the
   user should preallocate the matrix storage by setting the parameter nz
   (or the array nnz).  By setting these parameters accurately, performance
   during matrix assembly can be increased by more than a factor of 50.

   Collective on MPI_Comm

   Input Parameters:
+  B - the matrix
.  bs - size of block, the blocks are ALWAYS square. One can use MatSetBlockSizes() to set a different row and column blocksize but the row
          blocksize always defines the size of the blocks. The column blocksize sets the blocksize of the vectors obtained with MatCreateVecs()
.  nz - number of block nonzeros per block row (same for all rows)
-  nnz - array containing the number of block nonzeros in the various block rows
         (possibly different for each block row) or NULL

   Options Database Keys:
.   -mat_no_unroll - uses code that does not unroll the loops in the
                     block calculations (much slower)
.   -mat_block_size - size of the blocks to use

   Level: intermediate

   Notes:
   If the nnz parameter is given then the nz parameter is ignored

   You can call MatGetInfo() to get information on how effective the preallocation was;
   for example the fields mallocs,nz_allocated,nz_used,nz_unneeded;
   You can also run with the option -info and look for messages with the string
   malloc in them to see if additional memory allocation was needed.

   The block AIJ format is fully compatible with standard Fortran 77
   storage.  That is, the stored row and column indices can begin at
   either one (as in Fortran) or zero.  See the users' manual for details.

   Specify the preallocated storage with either nz or nnz (not both).
   Set nz=PETSC_DEFAULT and nnz=NULL for PETSc to control dynamic memory
   allocation.  See Users-Manual: ch_mat for details.

.seealso: MatCreate(), MatCreateSeqAIJ(), MatSetValues(), MatCreateBAIJ(), MatGetInfo()
@*/
PetscErrorCode  MatSeqBAIJSetPreallocation(Mat B,PetscInt bs,PetscInt nz,const PetscInt nnz[])
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(B,MAT_CLASSID,1);
  PetscValidType(B,1);
  PetscValidLogicalCollectiveInt(B,bs,2);
  ierr = PetscTryMethod(B,"MatSeqBAIJSetPreallocation_C",(Mat,PetscInt,PetscInt,const PetscInt[]),(B,bs,nz,nnz));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatSeqBAIJSetPreallocationCSR"
/*@C
   MatSeqBAIJSetPreallocationCSR - Allocates memory for a sparse sequential matrix in AIJ format
   (the default sequential PETSc format).

   Collective on MPI_Comm

   Input Parameters:
+  B - the matrix
.  i - the indices into j for the start of each local row (starts with zero)
.  j - the column indices for each local row (starts with zero) these must be sorted for each row
-  v - optional values in the matrix

   Level: developer

   Notes:
   The order of the entries in values is specified by the MatOption MAT_ROW_ORIENTED.  For example, C programs
   may want to use the default MAT_ROW_ORIENTED=PETSC_TRUE and use an array v[nnz][bs][bs] where the second index is
   over rows within a block and the last index is over columns within a block row.  Fortran programs will likely set
   MAT_ROW_ORIENTED=PETSC_FALSE and use a Fortran array v(bs,bs,nnz) in which the first index is over rows within a
   block column and the second index is over columns within a block.

.keywords: matrix, aij, compressed row, sparse

.seealso: MatCreate(), MatCreateSeqBAIJ(), MatSetValues(), MatSeqBAIJSetPreallocation(), MATSEQBAIJ
@*/
PetscErrorCode  MatSeqBAIJSetPreallocationCSR(Mat B,PetscInt bs,const PetscInt i[],const PetscInt j[], const PetscScalar v[])
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(B,MAT_CLASSID,1);
  PetscValidType(B,1);
  PetscValidLogicalCollectiveInt(B,bs,2);
  ierr = PetscTryMethod(B,"MatSeqBAIJSetPreallocationCSR_C",(Mat,PetscInt,const PetscInt[],const PetscInt[],const PetscScalar[]),(B,bs,i,j,v));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "MatCreateSeqBAIJWithArrays"
/*@
     MatCreateSeqBAIJWithArrays - Creates an sequential BAIJ matrix using matrix elements provided by the user.

     Collective on MPI_Comm

   Input Parameters:
+  comm - must be an MPI communicator of size 1
.  bs - size of block
.  m - number of rows
.  n - number of columns
.  i - row indices
.  j - column indices
-  a - matrix values

   Output Parameter:
.  mat - the matrix

   Level: advanced

   Notes:
       The i, j, and a arrays are not copied by this routine, the user must free these arrays
    once the matrix is destroyed

       You cannot set new nonzero locations into this matrix, that will generate an error.

       The i and j indices are 0 based

       When block size is greater than 1 the matrix values must be stored using the BAIJ storage format (see the BAIJ code to determine this).

      The order of the entries in values is the same as the block compressed sparse row storage format; that is, it is
      the same as a three dimensional array in Fortran values(bs,bs,nnz) that contains the first column of the first
      block, followed by the second column of the first block etc etc.  That is, the blocks are contiguous in memory
      with column-major ordering within blocks.

.seealso: MatCreate(), MatCreateBAIJ(), MatCreateSeqBAIJ()

@*/
PetscErrorCode  MatCreateSeqBAIJWithArrays(MPI_Comm comm,PetscInt bs,PetscInt m,PetscInt n,PetscInt *i,PetscInt *j,PetscScalar *a,Mat *mat)
{
  PetscErrorCode ierr;
  PetscInt       ii;
  Mat_SeqBAIJ    *baij;

  PetscFunctionBegin;
  if (bs != 1) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_SUP,"block size %D > 1 is not supported yet",bs);
  if (i[0]) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"i (row indices) must start with 0");

  ierr = MatCreate(comm,mat);CHKERRQ(ierr);
  ierr = MatSetSizes(*mat,m,n,m,n);CHKERRQ(ierr);
  ierr = MatSetType(*mat,MATSEQBAIJ);CHKERRQ(ierr);
  ierr = MatSeqBAIJSetPreallocation_SeqBAIJ(*mat,bs,MAT_SKIP_ALLOCATION,0);CHKERRQ(ierr);
  baij = (Mat_SeqBAIJ*)(*mat)->data;
  ierr = PetscMalloc2(m,&baij->imax,m,&baij->ilen);CHKERRQ(ierr);
  ierr = PetscLogObjectMemory((PetscObject)*mat,2*m*sizeof(PetscInt));CHKERRQ(ierr);

  baij->i = i;
  baij->j = j;
  baij->a = a;

  baij->singlemalloc = PETSC_FALSE;
  baij->nonew        = -1;             /*this indicates that inserting a new value in the matrix that generates a new nonzero is an error*/
  baij->free_a       = PETSC_FALSE;
  baij->free_ij      = PETSC_FALSE;

  for (ii=0; ii<m; ii++) {
    baij->ilen[ii] = baij->imax[ii] = i[ii+1] - i[ii];
#if defined(PETSC_USE_DEBUG)
    if (i[ii+1] - i[ii] < 0) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Negative row length in i (row indices) row = %d length = %d",ii,i[ii+1] - i[ii]);
#endif
  }
#if defined(PETSC_USE_DEBUG)
  for (ii=0; ii<baij->i[m]; ii++) {
    if (j[ii] < 0) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Negative column index at location = %d index = %d",ii,j[ii]);
    if (j[ii] > n - 1) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Column index to large at location = %d index = %d",ii,j[ii]);
  }
#endif

  ierr = MatAssemblyBegin(*mat,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(*mat,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "MatCreateMPIMatConcatenateSeqMat_SeqBAIJ"
PetscErrorCode MatCreateMPIMatConcatenateSeqMat_SeqBAIJ(MPI_Comm comm,Mat inmat,PetscInt n,MatReuse scall,Mat *outmat)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MatCreateMPIMatConcatenateSeqMat_MPIBAIJ(comm,inmat,n,scall,outmat);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
