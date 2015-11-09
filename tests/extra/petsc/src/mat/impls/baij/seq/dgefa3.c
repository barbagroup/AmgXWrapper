
/*
     Inverts 3 by 3 matrix using partial pivoting.

       Used by the sparse factorization routines in
     src/mat/impls/baij/seq


       This is a combination of the Linpack routines
    dgefa() and dgedi() specialized for a size of 3.

*/
#include <petscsys.h>

#undef __FUNCT__
#define __FUNCT__ "PetscKernel_A_gets_inverse_A_3"
PETSC_EXTERN PetscErrorCode PetscKernel_A_gets_inverse_A_3(MatScalar *a,PetscReal shift)
{
  PetscInt  i__2,i__3,kp1,j,k,l,ll,i,ipvt[3],kb,k3;
  PetscInt  k4,j3;
  MatScalar *aa,*ax,*ay,work[9],stmp;
  MatReal   tmp,max;

/*     gaussian elimination with partial pivoting */

  PetscFunctionBegin;
  shift = .333*shift*(1.e-12 + PetscAbsScalar(a[0]) + PetscAbsScalar(a[4]) + PetscAbsScalar(a[8]));
  /* Parameter adjustments */
  a -= 4;

  for (k = 1; k <= 2; ++k) {
    kp1 = k + 1;
    k3  = 3*k;
    k4  = k3 + k;
/*        find l = pivot index */

    i__2 = 4 - k;
    aa   = &a[k4];
    max  = PetscAbsScalar(aa[0]);
    l    = 1;
    for (ll=1; ll<i__2; ll++) {
      tmp = PetscAbsScalar(aa[ll]);
      if (tmp > max) { max = tmp; l = ll+1;}
    }
    l        += k - 1;
    ipvt[k-1] = l;

    if (a[l + k3] == 0.0) {
      if (shift == 0.0) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_MAT_LU_ZRPVT,"Zero pivot, row %D",k-1);
      else {
        /* Shift is applied to single diagonal entry */
        a[l + k3] = shift;
      }
    }
/*           interchange if necessary */

    if (l != k) {
      stmp      = a[l + k3];
      a[l + k3] = a[k4];
      a[k4]     = stmp;
    }

/*           compute multipliers */

    stmp = -1. / a[k4];
    i__2 = 3 - k;
    aa   = &a[1 + k4];
    for (ll=0; ll<i__2; ll++) aa[ll] *= stmp;

/*           row elimination with column indexing */

    ax = &a[k4+1];
    for (j = kp1; j <= 3; ++j) {
      j3   = 3*j;
      stmp = a[l + j3];
      if (l != k) {
        a[l + j3] = a[k + j3];
        a[k + j3] = stmp;
      }

      i__3 = 3 - k;
      ay   = &a[1+k+j3];
      for (ll=0; ll<i__3; ll++) ay[ll] += stmp*ax[ll];
    }
  }
  ipvt[2] = 3;
  if (a[12] == 0.0) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_MAT_LU_ZRPVT,"Zero pivot, row %D",2);

  /*
       Now form the inverse
  */

  /*     compute inverse(u) */

  for (k = 1; k <= 3; ++k) {
    k3    = 3*k;
    k4    = k3 + k;
    a[k4] = 1.0 / a[k4];
    stmp  = -a[k4];
    i__2  = k - 1;
    aa    = &a[k3 + 1];
    for (ll=0; ll<i__2; ll++) aa[ll] *= stmp;
    kp1 = k + 1;
    if (3 < kp1) continue;
    ax = aa;
    for (j = kp1; j <= 3; ++j) {
      j3        = 3*j;
      stmp      = a[k + j3];
      a[k + j3] = 0.0;
      ay        = &a[j3 + 1];
      for (ll=0; ll<k; ll++) ay[ll] += stmp*ax[ll];
    }
  }

  /*    form inverse(u)*inverse(l) */

  for (kb = 1; kb <= 2; ++kb) {
    k   = 3 - kb;
    k3  = 3*k;
    kp1 = k + 1;
    aa  = a + k3;
    for (i = kp1; i <= 3; ++i) {
      work[i-1] = aa[i];
      aa[i]     = 0.0;
    }
    for (j = kp1; j <= 3; ++j) {
      stmp   = work[j-1];
      ax     = &a[3*j + 1];
      ay     = &a[k3 + 1];
      ay[0] += stmp*ax[0];
      ay[1] += stmp*ax[1];
      ay[2] += stmp*ax[2];
    }
    l = ipvt[k-1];
    if (l != k) {
      ax   = &a[k3 + 1];
      ay   = &a[3*l + 1];
      stmp = ax[0]; ax[0] = ay[0]; ay[0] = stmp;
      stmp = ax[1]; ax[1] = ay[1]; ay[1] = stmp;
      stmp = ax[2]; ax[2] = ay[2]; ay[2] = stmp;
    }
  }
  PetscFunctionReturn(0);
}


