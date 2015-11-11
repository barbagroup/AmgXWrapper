
/*
   Defines the BLAS based vector operations. Code shared by parallel
  and sequential vectors.
*/

#include <../src/vec/vec/impls/dvecimpl.h>          /*I "petscvec.h" I*/
#include <petscblaslapack.h>

#undef __FUNCT__
#define __FUNCT__ "VecDot_Seq"
PetscErrorCode VecDot_Seq(Vec xin,Vec yin,PetscScalar *z)
{
  const PetscScalar *ya,*xa;
  PetscBLASInt      one = 1,bn;
  PetscErrorCode    ierr;

  PetscFunctionBegin;
  ierr = PetscBLASIntCast(xin->map->n,&bn);CHKERRQ(ierr);
  ierr = VecGetArrayRead(xin,&xa);CHKERRQ(ierr);
  ierr = VecGetArrayRead(yin,&ya);CHKERRQ(ierr);
  /* arguments ya, xa are reversed because BLAS complex conjugates the first argument, PETSc the second */
  PetscStackCallBLAS("BLASdot",*z   = BLASdot_(&bn,ya,&one,xa,&one));
  ierr = VecRestoreArrayRead(xin,&xa);CHKERRQ(ierr);
  ierr = VecRestoreArrayRead(yin,&ya);CHKERRQ(ierr);
  if (xin->map->n > 0) {
    ierr = PetscLogFlops(2.0*xin->map->n-1);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecTDot_Seq"
PetscErrorCode VecTDot_Seq(Vec xin,Vec yin,PetscScalar *z)
{
  const PetscScalar *ya,*xa;
  PetscBLASInt      one = 1,bn;
  PetscErrorCode    ierr;

  PetscFunctionBegin;
  ierr = PetscBLASIntCast(xin->map->n,&bn);CHKERRQ(ierr);
  ierr = VecGetArrayRead(xin,&xa);CHKERRQ(ierr);
  ierr = VecGetArrayRead(yin,&ya);CHKERRQ(ierr);
  PetscStackCallBLAS("BLASdot",*z   = BLASdotu_(&bn,xa,&one,ya,&one));
  ierr = VecRestoreArrayRead(xin,&xa);CHKERRQ(ierr);
  ierr = VecRestoreArrayRead(yin,&ya);CHKERRQ(ierr);
  if (xin->map->n > 0) {
    ierr = PetscLogFlops(2.0*xin->map->n-1);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecScale_Seq"
PetscErrorCode VecScale_Seq(Vec xin, PetscScalar alpha)
{
  PetscErrorCode ierr;
  PetscBLASInt   one = 1,bn;

  PetscFunctionBegin;
  ierr = PetscBLASIntCast(xin->map->n,&bn);CHKERRQ(ierr);
  if (alpha == (PetscScalar)0.0) {
    ierr = VecSet_Seq(xin,alpha);CHKERRQ(ierr);
  } else if (alpha != (PetscScalar)1.0) {
    PetscScalar a = alpha,*xarray;
    ierr = VecGetArray(xin,&xarray);CHKERRQ(ierr);
    PetscStackCallBLAS("BLASscal",BLASscal_(&bn,&a,xarray,&one));
    ierr = VecRestoreArray(xin,&xarray);CHKERRQ(ierr);
  }
  ierr = PetscLogFlops(xin->map->n);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecAXPY_Seq"
PetscErrorCode VecAXPY_Seq(Vec yin,PetscScalar alpha,Vec xin)
{
  PetscErrorCode    ierr;
  const PetscScalar *xarray;
  PetscScalar       *yarray;
  PetscBLASInt      one = 1,bn;

  PetscFunctionBegin;
  ierr = PetscBLASIntCast(yin->map->n,&bn);CHKERRQ(ierr);
  /* assume that the BLAS handles alpha == 1.0 efficiently since we have no fast code for it */
  if (alpha != (PetscScalar)0.0) {
    ierr = VecGetArrayRead(xin,&xarray);CHKERRQ(ierr);
    ierr = VecGetArray(yin,&yarray);CHKERRQ(ierr);
    PetscStackCallBLAS("BLASaxpy",BLASaxpy_(&bn,&alpha,xarray,&one,yarray,&one));
    ierr = VecRestoreArrayRead(xin,&xarray);CHKERRQ(ierr);
    ierr = VecRestoreArray(yin,&yarray);CHKERRQ(ierr);
    ierr = PetscLogFlops(2.0*yin->map->n);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecAXPBY_Seq"
PetscErrorCode VecAXPBY_Seq(Vec yin,PetscScalar alpha,PetscScalar beta,Vec xin)
{
  PetscErrorCode    ierr;
  PetscInt          n = yin->map->n,i;
  const PetscScalar *xx;
  PetscScalar       *yy,a = alpha,b = beta;

  PetscFunctionBegin;
  if (a == (PetscScalar)0.0) {
    ierr = VecScale_Seq(yin,beta);CHKERRQ(ierr);
  } else if (b == (PetscScalar)1.0) {
    ierr = VecAXPY_Seq(yin,alpha,xin);CHKERRQ(ierr);
  } else if (a == (PetscScalar)1.0) {
    ierr = VecAYPX_Seq(yin,beta,xin);CHKERRQ(ierr);
  } else if (b == (PetscScalar)0.0) {
    ierr = VecGetArrayRead(xin,&xx);CHKERRQ(ierr);
    ierr = VecGetArray(yin,(PetscScalar**)&yy);CHKERRQ(ierr);

    for (i=0; i<n; i++) yy[i] = a*xx[i];

    ierr = VecRestoreArrayRead(xin,&xx);CHKERRQ(ierr);
    ierr = VecRestoreArray(yin,(PetscScalar**)&yy);CHKERRQ(ierr);
    ierr = PetscLogFlops(xin->map->n);CHKERRQ(ierr);
  } else {
    ierr = VecGetArrayRead(xin,&xx);CHKERRQ(ierr);
    ierr = VecGetArray(yin,(PetscScalar**)&yy);CHKERRQ(ierr);

    for (i=0; i<n; i++) yy[i] = a*xx[i] + b*yy[i];

    ierr = VecRestoreArrayRead(xin,&xx);CHKERRQ(ierr);
    ierr = VecRestoreArray(yin,(PetscScalar**)&yy);CHKERRQ(ierr);
    ierr = PetscLogFlops(3.0*xin->map->n);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecAXPBYPCZ_Seq"
PetscErrorCode VecAXPBYPCZ_Seq(Vec zin,PetscScalar alpha,PetscScalar beta,PetscScalar gamma,Vec xin,Vec yin)
{
  PetscErrorCode    ierr;
  PetscInt          n = zin->map->n,i;
  const PetscScalar *yy,*xx;
  PetscScalar       *zz;

  PetscFunctionBegin;
  ierr = VecGetArrayRead(xin,&xx);CHKERRQ(ierr);
  ierr = VecGetArrayRead(yin,&yy);CHKERRQ(ierr);
  ierr = VecGetArray(zin,&zz);CHKERRQ(ierr);
  if (alpha == (PetscScalar)1.0) {
    for (i=0; i<n; i++) zz[i] = xx[i] + beta*yy[i] + gamma*zz[i];
    ierr = PetscLogFlops(4.0*n);CHKERRQ(ierr);
  } else if (gamma == (PetscScalar)1.0) {
    for (i=0; i<n; i++) zz[i] = alpha*xx[i] + beta*yy[i] + zz[i];
    ierr = PetscLogFlops(4.0*n);CHKERRQ(ierr);
  } else if (gamma == (PetscScalar)0.0) {
    for (i=0; i<n; i++) zz[i] = alpha*xx[i] + beta*yy[i];
    ierr = PetscLogFlops(3.0*n);CHKERRQ(ierr);
  } else {
    for (i=0; i<n; i++) zz[i] = alpha*xx[i] + beta*yy[i] + gamma*zz[i];
    ierr = PetscLogFlops(5.0*n);CHKERRQ(ierr);
  }
  ierr = VecRestoreArrayRead(xin,&xx);CHKERRQ(ierr);
  ierr = VecRestoreArrayRead(yin,&yy);CHKERRQ(ierr);
  ierr = VecRestoreArray(zin,&zz);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
