
/*
     Provides the interface functions for vector operations that have PetscScalar/PetscReal in the signature
   These are the vector functions the user calls.
*/
#include <petsc/private/vecimpl.h>       /*I  "petscvec.h"   I*/
static PetscInt VecGetSubVectorSavedStateId = -1;

#define PetscCheckSameSizeVec(x,y) \
  if ((x)->map->N != (y)->map->N) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_INCOMP,"Incompatible vector global lengths %d != %d", (x)->map->N, (y)->map->N); \
  if ((x)->map->n != (y)->map->n) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_INCOMP,"Incompatible vector local lengths %d != %d", (x)->map->n, (y)->map->n);

#undef __FUNCT__
#define __FUNCT__ "VecValidValues"
PETSC_EXTERN PetscErrorCode VecValidValues(Vec vec,PetscInt argnum,PetscBool begin)
{
#if defined(PETSC_USE_DEBUG)
  PetscErrorCode    ierr;
  PetscInt          n,i;
  const PetscScalar *x;

  PetscFunctionBegin;
#if defined(PETSC_HAVE_CUSP)
  if ((vec->petscnative || vec->ops->getarray) && (vec->valid_GPU_array == PETSC_CUSP_CPU || vec->valid_GPU_array == PETSC_CUSP_BOTH)) {
#else
  if (vec->petscnative || vec->ops->getarray) {
#endif
    ierr = VecGetLocalSize(vec,&n);CHKERRQ(ierr);
    ierr = VecGetArrayRead(vec,&x);CHKERRQ(ierr);
    for (i=0; i<n; i++) {
      if (begin) {
        if (PetscIsInfOrNanScalar(x[i])) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_FP,"Vec entry at local location %D is not-a-number or infinite at beginning of function: Parameter number %D",i,argnum);
      } else {
        if (PetscIsInfOrNanScalar(x[i])) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_FP,"Vec entry at local location %D is not-a-number or infinite at end of function: Parameter number %D",i,argnum);
      }
    }
    ierr = VecRestoreArrayRead(vec,&x);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
#else
  return 0;
#endif
}

#undef __FUNCT__
#define __FUNCT__ "VecMaxPointwiseDivide"
/*@
   VecMaxPointwiseDivide - Computes the maximum of the componentwise division max = max_i abs(x_i/y_i).

   Logically Collective on Vec

   Input Parameters:
.  x, y  - the vectors

   Output Parameter:
.  max - the result

   Level: advanced

   Notes: x and y may be the same vector
          if a particular y_i is zero, it is treated as 1 in the above formula

.seealso: VecPointwiseDivide(), VecPointwiseMult(), VecPointwiseMax(), VecPointwiseMin(), VecPointwiseMaxAbs()
@*/
PetscErrorCode  VecMaxPointwiseDivide(Vec x,Vec y,PetscReal *max)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(x,VEC_CLASSID,1);
  PetscValidHeaderSpecific(y,VEC_CLASSID,2);
  PetscValidRealPointer(max,3);
  PetscValidType(x,1);
  PetscValidType(y,2);
  PetscCheckSameTypeAndComm(x,1,y,2);
  PetscCheckSameSizeVec(x,y);

  ierr = (*x->ops->maxpointwisedivide)(x,y,max);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecDot"
/*@
   VecDot - Computes the vector dot product.

   Collective on Vec

   Input Parameters:
.  x, y - the vectors

   Output Parameter:
.  val - the dot product

   Performance Issues:
$    per-processor memory bandwidth
$    interprocessor latency
$    work load inbalance that causes certain processes to arrive much earlier than others

   Notes for Users of Complex Numbers:
   For complex vectors, VecDot() computes
$     val = (x,y) = y^H x,
   where y^H denotes the conjugate transpose of y. Note that this corresponds to the usual "mathematicians" complex
   inner product where the SECOND argument gets the complex conjugate. Since the BLASdot() complex conjugates the first
   first argument we call the BLASdot() with the arguments reversed.

   Use VecTDot() for the indefinite form
$     val = (x,y) = y^T x,
   where y^T denotes the transpose of y.

   Level: intermediate

   Concepts: inner product
   Concepts: vector^inner product

.seealso: VecMDot(), VecTDot(), VecNorm(), VecDotBegin(), VecDotEnd(), VecDotRealPart()
@*/
PetscErrorCode  VecDot(Vec x,Vec y,PetscScalar *val)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(x,VEC_CLASSID,1);
  PetscValidHeaderSpecific(y,VEC_CLASSID,2);
  PetscValidScalarPointer(val,3);
  PetscValidType(x,1);
  PetscValidType(y,2);
  PetscCheckSameTypeAndComm(x,1,y,2);
  PetscCheckSameSizeVec(x,y);

  ierr = PetscLogEventBarrierBegin(VEC_DotBarrier,x,y,0,0,PetscObjectComm((PetscObject)x));CHKERRQ(ierr);
  ierr = (*x->ops->dot)(x,y,val);CHKERRQ(ierr);
  ierr = PetscLogEventBarrierEnd(VEC_DotBarrier,x,y,0,0,PetscObjectComm((PetscObject)x));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecDotRealPart"
/*@
   VecDotRealPart - Computes the real part of the vector dot product.

   Collective on Vec

   Input Parameters:
.  x, y - the vectors

   Output Parameter:
.  val - the real part of the dot product;

   Performance Issues:
$    per-processor memory bandwidth
$    interprocessor latency
$    work load inbalance that causes certain processes to arrive much earlier than others

   Notes for Users of Complex Numbers:
     See VecDot() for more details on the definition of the dot product for complex numbers

     For real numbers this returns the same value as VecDot()

     For complex numbers in C^n (that is a vector of n components with a complex number for each component) this is equal to the usual real dot product on the
     the space R^{2n} (that is a vector of 2n components with the real or imaginary part of the complex numbers for components)

   Developer Note: This is not currently optimized to compute only the real part of the dot product.

   Level: intermediate

   Concepts: inner product
   Concepts: vector^inner product

.seealso: VecMDot(), VecTDot(), VecNorm(), VecDotBegin(), VecDotEnd(), VecDot(), VecDotNorm2()
@*/
PetscErrorCode  VecDotRealPart(Vec x,Vec y,PetscReal *val)
{
  PetscErrorCode ierr;
  PetscScalar    fdot;

  PetscFunctionBegin;
  ierr = VecDot(x,y,&fdot);CHKERRQ(ierr);
  *val = PetscRealPart(fdot);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecNorm"
/*@
   VecNorm  - Computes the vector norm.

   Collective on Vec

   Input Parameters:
+  x - the vector
-  type - one of NORM_1, NORM_2, NORM_INFINITY.  Also available
          NORM_1_AND_2, which computes both norms and stores them
          in a two element array.

   Output Parameter:
.  val - the norm

   Notes:
$     NORM_1 denotes sum_i |x_i|
$     NORM_2 denotes sqrt(sum_i (x_i)^2)
$     NORM_INFINITY denotes max_i |x_i|

      For complex numbers NORM_1 will return the traditional 1 norm of the 2 norm of the complex numbers; that is the 1
      norm of the absolutely values of the complex entries. In PETSc 3.6 and earlier releases it returned the 1 norm of
      the 1 norm of the complex entries (what is returned by the BLAS routine asum()). Both are valid norms but most
      people expect the former.

   Level: intermediate

   Performance Issues:
$    per-processor memory bandwidth
$    interprocessor latency
$    work load inbalance that causes certain processes to arrive much earlier than others

   Concepts: norm
   Concepts: vector^norm

.seealso: VecDot(), VecTDot(), VecNorm(), VecDotBegin(), VecDotEnd(), VecNormAvailable(),
          VecNormBegin(), VecNormEnd()

@*/
PetscErrorCode  VecNorm(Vec x,NormType type,PetscReal *val)
{
  PetscBool      flg;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(x,VEC_CLASSID,1);
  PetscValidRealPointer(val,3);
  PetscValidType(x,1);
  if (((PetscObject)x)->precision != sizeof(PetscReal)) SETERRQ(PetscObjectComm((PetscObject)x),PETSC_ERR_SUP,"Wrong precision of input argument");

  /*
   * Cached data?
   */
  if (type!=NORM_1_AND_2) {
    ierr = PetscObjectComposedDataGetReal((PetscObject)x,NormIds[type],*val,flg);CHKERRQ(ierr);
    if (flg) PetscFunctionReturn(0);
  }
  ierr = PetscLogEventBarrierBegin(VEC_NormBarrier,x,0,0,0,PetscObjectComm((PetscObject)x));CHKERRQ(ierr);
  ierr = (*x->ops->norm)(x,type,val);CHKERRQ(ierr);
  ierr = PetscLogEventBarrierEnd(VEC_NormBarrier,x,0,0,0,PetscObjectComm((PetscObject)x));CHKERRQ(ierr);

  if (type!=NORM_1_AND_2) {
    ierr = PetscObjectComposedDataSetReal((PetscObject)x,NormIds[type],*val);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecNormAvailable"
/*@
   VecNormAvailable  - Returns the vector norm if it is already known.

   Not Collective

   Input Parameters:
+  x - the vector
-  type - one of NORM_1, NORM_2, NORM_INFINITY.  Also available
          NORM_1_AND_2, which computes both norms and stores them
          in a two element array.

   Output Parameter:
+  available - PETSC_TRUE if the val returned is valid
-  val - the norm

   Notes:
$     NORM_1 denotes sum_i |x_i|
$     NORM_2 denotes sqrt(sum_i (x_i)^2)
$     NORM_INFINITY denotes max_i |x_i|

   Level: intermediate

   Performance Issues:
$    per-processor memory bandwidth
$    interprocessor latency
$    work load inbalance that causes certain processes to arrive much earlier than others

   Compile Option:
   PETSC_HAVE_SLOW_BLAS_NORM2 will cause a C (loop unrolled) version of the norm to be used, rather
 than the BLAS. This should probably only be used when one is using the FORTRAN BLAS routines
 (as opposed to vendor provided) because the FORTRAN BLAS NRM2() routine is very slow.

   Concepts: norm
   Concepts: vector^norm

.seealso: VecDot(), VecTDot(), VecNorm(), VecDotBegin(), VecDotEnd(), VecNorm()
          VecNormBegin(), VecNormEnd()

@*/
PetscErrorCode  VecNormAvailable(Vec x,NormType type,PetscBool  *available,PetscReal *val)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(x,VEC_CLASSID,1);
  PetscValidRealPointer(val,3);
  PetscValidType(x,1);

  *available = PETSC_FALSE;
  if (type!=NORM_1_AND_2) {
    ierr = PetscObjectComposedDataGetReal((PetscObject)x,NormIds[type],*val,*available);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecNormalize"
/*@
   VecNormalize - Normalizes a vector by 2-norm.

   Collective on Vec

   Input Parameters:
+  x - the vector

   Output Parameter:
.  x - the normalized vector
-  val - the vector norm before normalization

   Level: intermediate

   Concepts: vector^normalizing
   Concepts: normalizing^vector

@*/
PetscErrorCode  VecNormalize(Vec x,PetscReal *val)
{
  PetscErrorCode ierr;
  PetscReal      norm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(x,VEC_CLASSID,1);
  PetscValidType(x,1);
  ierr = PetscLogEventBegin(VEC_Normalize,x,0,0,0);CHKERRQ(ierr);
  ierr = VecNorm(x,NORM_2,&norm);CHKERRQ(ierr);
  if (norm == 0.0) {
    ierr = PetscInfo(x,"Vector of zero norm can not be normalized; Returning only the zero norm\n");CHKERRQ(ierr);
  } else if (norm != 1.0) {
    PetscScalar tmp = 1.0/norm;
    ierr = VecScale(x,tmp);CHKERRQ(ierr);
  }
  if (val) *val = norm;
  ierr = PetscLogEventEnd(VEC_Normalize,x,0,0,0);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecMax"
/*@C
   VecMax - Determines the maximum vector component and its location.

   Collective on Vec

   Input Parameter:
.  x - the vector

   Output Parameters:
+  val - the maximum component
-  p - the location of val (pass NULL if you don't want this)

   Notes:
   Returns the value PETSC_MIN_REAL and p = -1 if the vector is of length 0.

   Returns the smallest index with the maximum value
   Level: intermediate

   Concepts: maximum^of vector
   Concepts: vector^maximum value

.seealso: VecNorm(), VecMin()
@*/
PetscErrorCode  VecMax(Vec x,PetscInt *p,PetscReal *val)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(x,VEC_CLASSID,1);
  PetscValidScalarPointer(val,3);
  PetscValidType(x,1);
  ierr = PetscLogEventBegin(VEC_Max,x,0,0,0);CHKERRQ(ierr);
  ierr = (*x->ops->max)(x,p,val);CHKERRQ(ierr);
  ierr = PetscLogEventEnd(VEC_Max,x,0,0,0);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecMin"
/*@
   VecMin - Determines the minimum vector component and its location.

   Collective on Vec

   Input Parameters:
.  x - the vector

   Output Parameter:
+  val - the minimum component
-  p - the location of val (pass NULL if you don't want this location)

   Level: intermediate

   Notes:
   Returns the value PETSC_MAX_REAL and p = -1 if the vector is of length 0.

   This returns the smallest index with the minumum value

   Concepts: minimum^of vector
   Concepts: vector^minimum entry

.seealso: VecMax()
@*/
PetscErrorCode  VecMin(Vec x,PetscInt *p,PetscReal *val)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(x,VEC_CLASSID,1);
  PetscValidScalarPointer(val,3);
  PetscValidType(x,1);
  ierr = PetscLogEventBegin(VEC_Min,x,0,0,0);CHKERRQ(ierr);
  ierr = (*x->ops->min)(x,p,val);CHKERRQ(ierr);
  ierr = PetscLogEventEnd(VEC_Min,x,0,0,0);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecTDot"
/*@
   VecTDot - Computes an indefinite vector dot product. That is, this
   routine does NOT use the complex conjugate.

   Collective on Vec

   Input Parameters:
.  x, y - the vectors

   Output Parameter:
.  val - the dot product

   Notes for Users of Complex Numbers:
   For complex vectors, VecTDot() computes the indefinite form
$     val = (x,y) = y^T x,
   where y^T denotes the transpose of y.

   Use VecDot() for the inner product
$     val = (x,y) = y^H x,
   where y^H denotes the conjugate transpose of y.

   Level: intermediate

   Concepts: inner product^non-Hermitian
   Concepts: vector^inner product
   Concepts: non-Hermitian inner product

.seealso: VecDot(), VecMTDot()
@*/
PetscErrorCode  VecTDot(Vec x,Vec y,PetscScalar *val)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(x,VEC_CLASSID,1);
  PetscValidHeaderSpecific(y,VEC_CLASSID,2);
  PetscValidScalarPointer(val,3);
  PetscValidType(x,1);
  PetscValidType(y,2);
  PetscCheckSameTypeAndComm(x,1,y,2);
  PetscCheckSameSizeVec(x,y);

  ierr = PetscLogEventBegin(VEC_TDot,x,y,0,0);CHKERRQ(ierr);
  ierr = (*x->ops->tdot)(x,y,val);CHKERRQ(ierr);
  ierr = PetscLogEventEnd(VEC_TDot,x,y,0,0);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecScale"
/*@
   VecScale - Scales a vector.

   Not collective on Vec

   Input Parameters:
+  x - the vector
-  alpha - the scalar

   Output Parameter:
.  x - the scaled vector

   Note:
   For a vector with n components, VecScale() computes
$      x[i] = alpha * x[i], for i=1,...,n.

   Level: intermediate

   Concepts: vector^scaling
   Concepts: scaling^vector

@*/
PetscErrorCode  VecScale(Vec x, PetscScalar alpha)
{
  PetscReal      norms[4] = {0.0,0.0,0.0, 0.0};
  PetscBool      flgs[4];
  PetscErrorCode ierr;
  PetscInt       i;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(x,VEC_CLASSID,1);
  PetscValidType(x,1);
  if (x->stash.insertmode != NOT_SET_VALUES) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONGSTATE,"Not for unassembled vector");
  ierr = PetscLogEventBegin(VEC_Scale,x,0,0,0);CHKERRQ(ierr);
  if (alpha != (PetscScalar)1.0) {
    /* get current stashed norms */
    for (i=0; i<4; i++) {
      ierr = PetscObjectComposedDataGetReal((PetscObject)x,NormIds[i],norms[i],flgs[i]);CHKERRQ(ierr);
    }
    ierr = (*x->ops->scale)(x,alpha);CHKERRQ(ierr);
    ierr = PetscObjectStateIncrease((PetscObject)x);CHKERRQ(ierr);
    /* put the scaled stashed norms back into the Vec */
    for (i=0; i<4; i++) {
      if (flgs[i]) {
        ierr = PetscObjectComposedDataSetReal((PetscObject)x,NormIds[i],PetscAbsScalar(alpha)*norms[i]);CHKERRQ(ierr);
      }
    }
  }
  ierr = PetscLogEventEnd(VEC_Scale,x,0,0,0);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecSet"
/*@
   VecSet - Sets all components of a vector to a single scalar value.

   Logically Collective on Vec

   Input Parameters:
+  x  - the vector
-  alpha - the scalar

   Output Parameter:
.  x  - the vector

   Note:
   For a vector of dimension n, VecSet() computes
$     x[i] = alpha, for i=1,...,n,
   so that all vector entries then equal the identical
   scalar value, alpha.  Use the more general routine
   VecSetValues() to set different vector entries.

   You CANNOT call this after you have called VecSetValues() but before you call
   VecAssemblyBegin/End().

   Level: beginner

.seealso VecSetValues(), VecSetValuesBlocked(), VecSetRandom()

   Concepts: vector^setting to constant

@*/
PetscErrorCode  VecSet(Vec x,PetscScalar alpha)
{
  PetscReal      val;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(x,VEC_CLASSID,1);
  PetscValidType(x,1);
  if (x->stash.insertmode != NOT_SET_VALUES) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONGSTATE,"You cannot call this after you have called VecSetValues() but\n before you have called VecAssemblyBegin/End()");
  PetscValidLogicalCollectiveScalar(x,alpha,2);
  VecLocked(x,1);

  ierr = PetscLogEventBegin(VEC_Set,x,0,0,0);CHKERRQ(ierr);
  ierr = (*x->ops->set)(x,alpha);CHKERRQ(ierr);
  ierr = PetscLogEventEnd(VEC_Set,x,0,0,0);CHKERRQ(ierr);
  ierr = PetscObjectStateIncrease((PetscObject)x);CHKERRQ(ierr);

  /*  norms can be simply set (if |alpha|*N not too large) */
  val  = PetscAbsScalar(alpha);
  if (x->map->N == 0) {
    ierr = PetscObjectComposedDataSetReal((PetscObject)x,NormIds[NORM_1],0.0l);CHKERRQ(ierr);
    ierr = PetscObjectComposedDataSetReal((PetscObject)x,NormIds[NORM_INFINITY],0.0);CHKERRQ(ierr);
    ierr = PetscObjectComposedDataSetReal((PetscObject)x,NormIds[NORM_2],0.0);CHKERRQ(ierr);
    ierr = PetscObjectComposedDataSetReal((PetscObject)x,NormIds[NORM_FROBENIUS],0.0);CHKERRQ(ierr);
  } else if (val > PETSC_MAX_REAL/x->map->N) {
    ierr = PetscObjectComposedDataSetReal((PetscObject)x,NormIds[NORM_INFINITY],val);CHKERRQ(ierr);
  } else {
    ierr = PetscObjectComposedDataSetReal((PetscObject)x,NormIds[NORM_1],x->map->N * val);CHKERRQ(ierr);
    ierr = PetscObjectComposedDataSetReal((PetscObject)x,NormIds[NORM_INFINITY],val);CHKERRQ(ierr);
    val  = PetscSqrtReal((PetscReal)x->map->N) * val;
    ierr = PetscObjectComposedDataSetReal((PetscObject)x,NormIds[NORM_2],val);CHKERRQ(ierr);
    ierr = PetscObjectComposedDataSetReal((PetscObject)x,NormIds[NORM_FROBENIUS],val);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "VecAXPY"
/*@
   VecAXPY - Computes y = alpha x + y.

   Logically Collective on Vec

   Input Parameters:
+  alpha - the scalar
-  x, y  - the vectors

   Output Parameter:
.  y - output vector

   Level: intermediate

   Notes: x and y MUST be different vectors

   Concepts: vector^BLAS
   Concepts: BLAS

.seealso: VecAYPX(), VecMAXPY(), VecWAXPY()
@*/
PetscErrorCode  VecAXPY(Vec y,PetscScalar alpha,Vec x)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(x,VEC_CLASSID,3);
  PetscValidHeaderSpecific(y,VEC_CLASSID,1);
  PetscValidType(x,3);
  PetscValidType(y,1);
  PetscCheckSameTypeAndComm(x,3,y,1);
  PetscCheckSameSizeVec(x,y);
  if (x == y) SETERRQ(PetscObjectComm((PetscObject)x),PETSC_ERR_ARG_IDN,"x and y cannot be the same vector");
  PetscValidLogicalCollectiveScalar(y,alpha,2);
  VecLocked(y,1);

  ierr = VecLockPush(x);CHKERRQ(ierr);
  ierr = PetscLogEventBegin(VEC_AXPY,x,y,0,0);CHKERRQ(ierr);
  ierr = (*y->ops->axpy)(y,alpha,x);CHKERRQ(ierr);
  ierr = PetscLogEventEnd(VEC_AXPY,x,y,0,0);CHKERRQ(ierr);
  ierr = VecLockPop(x);CHKERRQ(ierr);
  ierr = PetscObjectStateIncrease((PetscObject)y);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecAXPBY"
/*@
   VecAXPBY - Computes y = alpha x + beta y.

   Logically Collective on Vec

   Input Parameters:
+  alpha,beta - the scalars
-  x, y  - the vectors

   Output Parameter:
.  y - output vector

   Level: intermediate

   Notes: x and y MUST be different vectors

   Concepts: BLAS
   Concepts: vector^BLAS

.seealso: VecAYPX(), VecMAXPY(), VecWAXPY(), VecAXPY()
@*/
PetscErrorCode  VecAXPBY(Vec y,PetscScalar alpha,PetscScalar beta,Vec x)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(x,VEC_CLASSID,4);
  PetscValidHeaderSpecific(y,VEC_CLASSID,1);
  PetscValidType(x,4);
  PetscValidType(y,1);
  PetscCheckSameTypeAndComm(x,4,y,1);
  PetscCheckSameSizeVec(x,y);
  if (x == y) SETERRQ(PetscObjectComm((PetscObject)x),PETSC_ERR_ARG_IDN,"x and y cannot be the same vector");
  PetscValidLogicalCollectiveScalar(y,alpha,2);
  PetscValidLogicalCollectiveScalar(y,beta,3);

  ierr = PetscLogEventBegin(VEC_AXPY,x,y,0,0);CHKERRQ(ierr);
  ierr = (*y->ops->axpby)(y,alpha,beta,x);CHKERRQ(ierr);
  ierr = PetscLogEventEnd(VEC_AXPY,x,y,0,0);CHKERRQ(ierr);
  ierr = PetscObjectStateIncrease((PetscObject)y);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecAXPBYPCZ"
/*@
   VecAXPBYPCZ - Computes z = alpha x + beta y + gamma z

   Logically Collective on Vec

   Input Parameters:
+  alpha,beta, gamma - the scalars
-  x, y, z  - the vectors

   Output Parameter:
.  z - output vector

   Level: intermediate

   Notes: x, y and z must be different vectors

   Developer Note:   alpha = 1 or gamma = 1 or gamma = 0.0 are handled as special cases

   Concepts: BLAS
   Concepts: vector^BLAS

.seealso: VecAYPX(), VecMAXPY(), VecWAXPY(), VecAXPY()
@*/
PetscErrorCode  VecAXPBYPCZ(Vec z,PetscScalar alpha,PetscScalar beta,PetscScalar gamma,Vec x,Vec y)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(x,VEC_CLASSID,5);
  PetscValidHeaderSpecific(y,VEC_CLASSID,6);
  PetscValidHeaderSpecific(z,VEC_CLASSID,1);
  PetscValidType(x,5);
  PetscValidType(y,6);
  PetscValidType(z,1);
  PetscCheckSameTypeAndComm(x,5,y,6);
  PetscCheckSameTypeAndComm(x,5,z,1);
  PetscCheckSameSizeVec(x,y);
  PetscCheckSameSizeVec(x,z);
  if (x == y || x == z) SETERRQ(PetscObjectComm((PetscObject)x),PETSC_ERR_ARG_IDN,"x, y, and z must be different vectors");
  if (y == z) SETERRQ(PetscObjectComm((PetscObject)y),PETSC_ERR_ARG_IDN,"x, y, and z must be different vectors");
  PetscValidLogicalCollectiveScalar(z,alpha,2);
  PetscValidLogicalCollectiveScalar(z,beta,3);
  PetscValidLogicalCollectiveScalar(z,gamma,4);

  ierr = PetscLogEventBegin(VEC_AXPBYPCZ,x,y,z,0);CHKERRQ(ierr);
  ierr = (*y->ops->axpbypcz)(z,alpha,beta,gamma,x,y);CHKERRQ(ierr);
  ierr = PetscLogEventEnd(VEC_AXPBYPCZ,x,y,z,0);CHKERRQ(ierr);
  ierr = PetscObjectStateIncrease((PetscObject)z);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecAYPX"
/*@
   VecAYPX - Computes y = x + alpha y.

   Logically Collective on Vec

   Input Parameters:
+  alpha - the scalar
-  x, y  - the vectors

   Output Parameter:
.  y - output vector

   Level: intermediate

   Notes: x and y MUST be different vectors

   Concepts: vector^BLAS
   Concepts: BLAS

.seealso: VecAXPY(), VecWAXPY()
@*/
PetscErrorCode  VecAYPX(Vec y,PetscScalar alpha,Vec x)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(x,VEC_CLASSID,3);
  PetscValidHeaderSpecific(y,VEC_CLASSID,1);
  PetscValidType(x,3);
  PetscValidType(y,1);
  if (x == y) SETERRQ(PetscObjectComm((PetscObject)x),PETSC_ERR_ARG_IDN,"x and y must be different vectors");
  PetscValidLogicalCollectiveScalar(y,alpha,2);

  ierr = PetscLogEventBegin(VEC_AYPX,x,y,0,0);CHKERRQ(ierr);
  ierr =  (*y->ops->aypx)(y,alpha,x);CHKERRQ(ierr);
  ierr = PetscLogEventEnd(VEC_AYPX,x,y,0,0);CHKERRQ(ierr);
  ierr = PetscObjectStateIncrease((PetscObject)y);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "VecWAXPY"
/*@
   VecWAXPY - Computes w = alpha x + y.

   Logically Collective on Vec

   Input Parameters:
+  alpha - the scalar
-  x, y  - the vectors

   Output Parameter:
.  w - the result

   Level: intermediate

   Notes: w cannot be either x or y, but x and y can be the same

   Concepts: vector^BLAS
   Concepts: BLAS

.seealso: VecAXPY(), VecAYPX(), VecAXPBY()
@*/
PetscErrorCode  VecWAXPY(Vec w,PetscScalar alpha,Vec x,Vec y)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(w,VEC_CLASSID,1);
  PetscValidHeaderSpecific(x,VEC_CLASSID,3);
  PetscValidHeaderSpecific(y,VEC_CLASSID,4);
  PetscValidType(w,1);
  PetscValidType(x,3);
  PetscValidType(y,4);
  PetscCheckSameTypeAndComm(x,3,y,4);
  PetscCheckSameTypeAndComm(y,4,w,1);
  PetscCheckSameSizeVec(x,y);
  PetscCheckSameSizeVec(x,w);
  if (w == y) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SUP,"Result vector w cannot be same as input vector y, suggest VecAXPY()");
  if (w == x) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SUP,"Result vector w cannot be same as input vector x, suggest VecAYPX()");
  PetscValidLogicalCollectiveScalar(y,alpha,2);

  ierr = PetscLogEventBegin(VEC_WAXPY,x,y,w,0);CHKERRQ(ierr);
  ierr =  (*w->ops->waxpy)(w,alpha,x,y);CHKERRQ(ierr);
  ierr = PetscLogEventEnd(VEC_WAXPY,x,y,w,0);CHKERRQ(ierr);
  ierr = PetscObjectStateIncrease((PetscObject)w);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "VecSetValues"
/*@
   VecSetValues - Inserts or adds values into certain locations of a vector.

   Not Collective

   Input Parameters:
+  x - vector to insert in
.  ni - number of elements to add
.  ix - indices where to add
.  y - array of values
-  iora - either INSERT_VALUES or ADD_VALUES, where
   ADD_VALUES adds values to any existing entries, and
   INSERT_VALUES replaces existing entries with new values

   Notes:
   VecSetValues() sets x[ix[i]] = y[i], for i=0,...,ni-1.

   Calls to VecSetValues() with the INSERT_VALUES and ADD_VALUES
   options cannot be mixed without intervening calls to the assembly
   routines.

   These values may be cached, so VecAssemblyBegin() and VecAssemblyEnd()
   MUST be called after all calls to VecSetValues() have been completed.

   VecSetValues() uses 0-based indices in Fortran as well as in C.

   If you call VecSetOption(x, VEC_IGNORE_NEGATIVE_INDICES,PETSC_TRUE),
   negative indices may be passed in ix. These rows are
   simply ignored. This allows easily inserting element load matrices
   with homogeneous Dirchlet boundary conditions that you don't want represented
   in the vector.

   Level: beginner

   Concepts: vector^setting values

.seealso:  VecAssemblyBegin(), VecAssemblyEnd(), VecSetValuesLocal(),
           VecSetValue(), VecSetValuesBlocked(), InsertMode, INSERT_VALUES, ADD_VALUES, VecGetValues()
@*/
PetscErrorCode  VecSetValues(Vec x,PetscInt ni,const PetscInt ix[],const PetscScalar y[],InsertMode iora)
{
  PetscErrorCode ierr;

  PetscFunctionBeginHot;
  PetscValidHeaderSpecific(x,VEC_CLASSID,1);
  if (!ni) PetscFunctionReturn(0);
  PetscValidIntPointer(ix,3);
  PetscValidScalarPointer(y,4);
  PetscValidType(x,1);
  ierr = PetscLogEventBegin(VEC_SetValues,x,0,0,0);CHKERRQ(ierr);
  ierr = (*x->ops->setvalues)(x,ni,ix,y,iora);CHKERRQ(ierr);
  ierr = PetscLogEventEnd(VEC_SetValues,x,0,0,0);CHKERRQ(ierr);
  ierr = PetscObjectStateIncrease((PetscObject)x);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecGetValues"
/*@
   VecGetValues - Gets values from certain locations of a vector. Currently
          can only get values on the same processor

    Not Collective

   Input Parameters:
+  x - vector to get values from
.  ni - number of elements to get
-  ix - indices where to get them from (in global 1d numbering)

   Output Parameter:
.   y - array of values

   Notes:
   The user provides the allocated array y; it is NOT allocated in this routine

   VecGetValues() gets y[i] = x[ix[i]], for i=0,...,ni-1.

   VecAssemblyBegin() and VecAssemblyEnd()  MUST be called before calling this

   VecGetValues() uses 0-based indices in Fortran as well as in C.

   If you call VecSetOption(x, VEC_IGNORE_NEGATIVE_INDICES,PETSC_TRUE),
   negative indices may be passed in ix. These rows are
   simply ignored.

   Level: beginner

   Concepts: vector^getting values

.seealso:  VecAssemblyBegin(), VecAssemblyEnd(), VecGetValuesLocal(),
           VecGetValuesBlocked(), InsertMode, INSERT_VALUES, ADD_VALUES, VecSetValues()
@*/
PetscErrorCode  VecGetValues(Vec x,PetscInt ni,const PetscInt ix[],PetscScalar y[])
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(x,VEC_CLASSID,1);
  if (!ni) PetscFunctionReturn(0);
  PetscValidIntPointer(ix,3);
  PetscValidScalarPointer(y,4);
  PetscValidType(x,1);
  ierr = (*x->ops->getvalues)(x,ni,ix,y);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecSetValuesBlocked"
/*@
   VecSetValuesBlocked - Inserts or adds blocks of values into certain locations of a vector.

   Not Collective

   Input Parameters:
+  x - vector to insert in
.  ni - number of blocks to add
.  ix - indices where to add in block count, rather than element count
.  y - array of values
-  iora - either INSERT_VALUES or ADD_VALUES, where
   ADD_VALUES adds values to any existing entries, and
   INSERT_VALUES replaces existing entries with new values

   Notes:
   VecSetValuesBlocked() sets x[bs*ix[i]+j] = y[bs*i+j],
   for j=0,...,bs-1, for i=0,...,ni-1. where bs was set with VecSetBlockSize().

   Calls to VecSetValuesBlocked() with the INSERT_VALUES and ADD_VALUES
   options cannot be mixed without intervening calls to the assembly
   routines.

   These values may be cached, so VecAssemblyBegin() and VecAssemblyEnd()
   MUST be called after all calls to VecSetValuesBlocked() have been completed.

   VecSetValuesBlocked() uses 0-based indices in Fortran as well as in C.

   Negative indices may be passed in ix, these rows are
   simply ignored. This allows easily inserting element load matrices
   with homogeneous Dirchlet boundary conditions that you don't want represented
   in the vector.

   Level: intermediate

   Concepts: vector^setting values blocked

.seealso:  VecAssemblyBegin(), VecAssemblyEnd(), VecSetValuesBlockedLocal(),
           VecSetValues()
@*/
PetscErrorCode  VecSetValuesBlocked(Vec x,PetscInt ni,const PetscInt ix[],const PetscScalar y[],InsertMode iora)
{
  PetscErrorCode ierr;

  PetscFunctionBeginHot;
  PetscValidHeaderSpecific(x,VEC_CLASSID,1);
  PetscValidIntPointer(ix,3);
  PetscValidScalarPointer(y,4);
  PetscValidType(x,1);
  ierr = PetscLogEventBegin(VEC_SetValues,x,0,0,0);CHKERRQ(ierr);
  ierr = (*x->ops->setvaluesblocked)(x,ni,ix,y,iora);CHKERRQ(ierr);
  ierr = PetscLogEventEnd(VEC_SetValues,x,0,0,0);CHKERRQ(ierr);
  ierr = PetscObjectStateIncrease((PetscObject)x);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "VecSetValuesLocal"
/*@
   VecSetValuesLocal - Inserts or adds values into certain locations of a vector,
   using a local ordering of the nodes.

   Not Collective

   Input Parameters:
+  x - vector to insert in
.  ni - number of elements to add
.  ix - indices where to add
.  y - array of values
-  iora - either INSERT_VALUES or ADD_VALUES, where
   ADD_VALUES adds values to any existing entries, and
   INSERT_VALUES replaces existing entries with new values

   Level: intermediate

   Notes:
   VecSetValuesLocal() sets x[ix[i]] = y[i], for i=0,...,ni-1.

   Calls to VecSetValues() with the INSERT_VALUES and ADD_VALUES
   options cannot be mixed without intervening calls to the assembly
   routines.

   These values may be cached, so VecAssemblyBegin() and VecAssemblyEnd()
   MUST be called after all calls to VecSetValuesLocal() have been completed.

   VecSetValuesLocal() uses 0-based indices in Fortran as well as in C.

   Concepts: vector^setting values with local numbering

.seealso:  VecAssemblyBegin(), VecAssemblyEnd(), VecSetValues(), VecSetLocalToGlobalMapping(),
           VecSetValuesBlockedLocal()
@*/
PetscErrorCode  VecSetValuesLocal(Vec x,PetscInt ni,const PetscInt ix[],const PetscScalar y[],InsertMode iora)
{
  PetscErrorCode ierr;
  PetscInt       lixp[128],*lix = lixp;

  PetscFunctionBeginHot;
  PetscValidHeaderSpecific(x,VEC_CLASSID,1);
  if (!ni) PetscFunctionReturn(0);
  PetscValidIntPointer(ix,3);
  PetscValidScalarPointer(y,4);
  PetscValidType(x,1);

  ierr = PetscLogEventBegin(VEC_SetValues,x,0,0,0);CHKERRQ(ierr);
  if (!x->ops->setvalueslocal) {
    if (!x->map->mapping) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONGSTATE,"Local to global never set with VecSetLocalToGlobalMapping()");
    if (ni > 128) {
      ierr = PetscMalloc1(ni,&lix);CHKERRQ(ierr);
    }
    ierr = ISLocalToGlobalMappingApply(x->map->mapping,ni,(PetscInt*)ix,lix);CHKERRQ(ierr);
    ierr = (*x->ops->setvalues)(x,ni,lix,y,iora);CHKERRQ(ierr);
    if (ni > 128) {
      ierr = PetscFree(lix);CHKERRQ(ierr);
    }
  } else {
    ierr = (*x->ops->setvalueslocal)(x,ni,ix,y,iora);CHKERRQ(ierr);
  }
  ierr = PetscLogEventEnd(VEC_SetValues,x,0,0,0);CHKERRQ(ierr);
  ierr = PetscObjectStateIncrease((PetscObject)x);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecSetValuesBlockedLocal"
/*@
   VecSetValuesBlockedLocal - Inserts or adds values into certain locations of a vector,
   using a local ordering of the nodes.

   Not Collective

   Input Parameters:
+  x - vector to insert in
.  ni - number of blocks to add
.  ix - indices where to add in block count, not element count
.  y - array of values
-  iora - either INSERT_VALUES or ADD_VALUES, where
   ADD_VALUES adds values to any existing entries, and
   INSERT_VALUES replaces existing entries with new values

   Level: intermediate

   Notes:
   VecSetValuesBlockedLocal() sets x[bs*ix[i]+j] = y[bs*i+j],
   for j=0,..bs-1, for i=0,...,ni-1, where bs has been set with VecSetBlockSize().

   Calls to VecSetValuesBlockedLocal() with the INSERT_VALUES and ADD_VALUES
   options cannot be mixed without intervening calls to the assembly
   routines.

   These values may be cached, so VecAssemblyBegin() and VecAssemblyEnd()
   MUST be called after all calls to VecSetValuesBlockedLocal() have been completed.

   VecSetValuesBlockedLocal() uses 0-based indices in Fortran as well as in C.


   Concepts: vector^setting values blocked with local numbering

.seealso:  VecAssemblyBegin(), VecAssemblyEnd(), VecSetValues(), VecSetValuesBlocked(),
           VecSetLocalToGlobalMapping()
@*/
PetscErrorCode  VecSetValuesBlockedLocal(Vec x,PetscInt ni,const PetscInt ix[],const PetscScalar y[],InsertMode iora)
{
  PetscErrorCode ierr;
  PetscInt       lixp[128],*lix = lixp;

  PetscFunctionBeginHot;
  PetscValidHeaderSpecific(x,VEC_CLASSID,1);
  PetscValidIntPointer(ix,3);
  PetscValidScalarPointer(y,4);
  PetscValidType(x,1);
  if (ni > 128) {
    ierr = PetscMalloc1(ni,&lix);CHKERRQ(ierr);
  }

  ierr = PetscLogEventBegin(VEC_SetValues,x,0,0,0);CHKERRQ(ierr);
  ierr = ISLocalToGlobalMappingApplyBlock(x->map->mapping,ni,(PetscInt*)ix,lix);CHKERRQ(ierr);
  ierr = (*x->ops->setvaluesblocked)(x,ni,lix,y,iora);CHKERRQ(ierr);
  ierr = PetscLogEventEnd(VEC_SetValues,x,0,0,0);CHKERRQ(ierr);
  if (ni > 128) {
    ierr = PetscFree(lix);CHKERRQ(ierr);
  }
  ierr = PetscObjectStateIncrease((PetscObject)x);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecMTDot"
/*@
   VecMTDot - Computes indefinite vector multiple dot products.
   That is, it does NOT use the complex conjugate.

   Collective on Vec

   Input Parameters:
+  x - one vector
.  nv - number of vectors
-  y - array of vectors.  Note that vectors are pointers

   Output Parameter:
.  val - array of the dot products

   Notes for Users of Complex Numbers:
   For complex vectors, VecMTDot() computes the indefinite form
$      val = (x,y) = y^T x,
   where y^T denotes the transpose of y.

   Use VecMDot() for the inner product
$      val = (x,y) = y^H x,
   where y^H denotes the conjugate transpose of y.

   Level: intermediate

   Concepts: inner product^multiple
   Concepts: vector^multiple inner products

.seealso: VecMDot(), VecTDot()
@*/
PetscErrorCode  VecMTDot(Vec x,PetscInt nv,const Vec y[],PetscScalar val[])
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(x,VEC_CLASSID,1);
  PetscValidPointer(y,3);
  PetscValidHeaderSpecific(*y,VEC_CLASSID,3);
  PetscValidScalarPointer(val,4);
  PetscValidType(x,2);
  PetscValidType(*y,3);
  PetscCheckSameTypeAndComm(x,2,*y,3);
  PetscCheckSameSizeVec(x,*y);

  ierr = PetscLogEventBegin(VEC_MTDot,x,*y,0,0);CHKERRQ(ierr);
  ierr = (*x->ops->mtdot)(x,nv,y,val);CHKERRQ(ierr);
  ierr = PetscLogEventEnd(VEC_MTDot,x,*y,0,0);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecMDot"
/*@
   VecMDot - Computes vector multiple dot products.

   Collective on Vec

   Input Parameters:
+  x - one vector
.  nv - number of vectors
-  y - array of vectors.

   Output Parameter:
.  val - array of the dot products (does not allocate the array)

   Notes for Users of Complex Numbers:
   For complex vectors, VecMDot() computes
$     val = (x,y) = y^H x,
   where y^H denotes the conjugate transpose of y.

   Use VecMTDot() for the indefinite form
$     val = (x,y) = y^T x,
   where y^T denotes the transpose of y.

   Level: intermediate

   Concepts: inner product^multiple
   Concepts: vector^multiple inner products

.seealso: VecMTDot(), VecDot()
@*/
PetscErrorCode  VecMDot(Vec x,PetscInt nv,const Vec y[],PetscScalar val[])
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(x,VEC_CLASSID,1);
  if (!nv) PetscFunctionReturn(0);
  if (nv < 0) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Number of vectors (given %D) cannot be negative",nv);
  PetscValidPointer(y,3);
  PetscValidHeaderSpecific(*y,VEC_CLASSID,3);
  PetscValidScalarPointer(val,4);
  PetscValidType(x,2);
  PetscValidType(*y,3);
  PetscCheckSameTypeAndComm(x,2,*y,3);
  PetscCheckSameSizeVec(x,*y);

  ierr = PetscLogEventBarrierBegin(VEC_MDotBarrier,x,*y,0,0,PetscObjectComm((PetscObject)x));CHKERRQ(ierr);
  ierr = (*x->ops->mdot)(x,nv,y,val);CHKERRQ(ierr);
  ierr = PetscLogEventBarrierEnd(VEC_MDotBarrier,x,*y,0,0,PetscObjectComm((PetscObject)x));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecMAXPY"
/*@
   VecMAXPY - Computes y = y + sum alpha[j] x[j]

   Logically Collective on Vec

   Input Parameters:
+  nv - number of scalars and x-vectors
.  alpha - array of scalars
.  y - one vector
-  x - array of vectors

   Level: intermediate

   Notes: y cannot be any of the x vectors

   Concepts: BLAS

.seealso: VecAXPY(), VecWAXPY(), VecAYPX()
@*/
PetscErrorCode  VecMAXPY(Vec y,PetscInt nv,const PetscScalar alpha[],Vec x[])
{
  PetscErrorCode ierr;
  PetscInt       i;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(y,VEC_CLASSID,1);
  if (!nv) PetscFunctionReturn(0);
  if (nv < 0) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Number of vectors (given %D) cannot be negative",nv);
  PetscValidScalarPointer(alpha,3);
  PetscValidPointer(x,4);
  PetscValidHeaderSpecific(*x,VEC_CLASSID,4);
  PetscValidType(y,1);
  PetscValidType(*x,4);
  PetscCheckSameTypeAndComm(y,1,*x,4);
  PetscCheckSameSizeVec(y,*x);
  for (i=0; i<nv; i++) PetscValidLogicalCollectiveScalar(y,alpha[i],3);

  ierr = PetscLogEventBegin(VEC_MAXPY,*x,y,0,0);CHKERRQ(ierr);
  ierr = (*y->ops->maxpy)(y,nv,alpha,x);CHKERRQ(ierr);
  ierr = PetscLogEventEnd(VEC_MAXPY,*x,y,0,0);CHKERRQ(ierr);
  ierr = PetscObjectStateIncrease((PetscObject)y);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecGetSubVector"
/*@
   VecGetSubVector - Gets a vector representing part of another vector

   Collective on IS (and Vec if nonlocal entries are needed)

   Input Arguments:
+ X - vector from which to extract a subvector
- is - index set representing portion of X to extract

   Output Arguments:
. Y - subvector corresponding to is

   Level: advanced

   Notes:
   The subvector Y should be returned with VecRestoreSubVector().

   This function may return a subvector without making a copy, therefore it is not safe to use the original vector while
   modifying the subvector.  Other non-overlapping subvectors can still be obtained from X using this function.

.seealso: MatGetSubMatrix()
@*/
PetscErrorCode  VecGetSubVector(Vec X,IS is,Vec *Y)
{
  PetscErrorCode   ierr;
  Vec              Z;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(X,VEC_CLASSID,1);
  PetscValidHeaderSpecific(is,IS_CLASSID,2);
  PetscValidPointer(Y,3);
  if (X->ops->getsubvector) {
    ierr = (*X->ops->getsubvector)(X,is,&Z);CHKERRQ(ierr);
  } else {                      /* Default implementation currently does no caching */
    PetscInt  gstart,gend,start;
    PetscBool contiguous,gcontiguous;
    ierr = VecGetOwnershipRange(X,&gstart,&gend);CHKERRQ(ierr);
    ierr = ISContiguousLocal(is,gstart,gend,&start,&contiguous);CHKERRQ(ierr);
    ierr = MPIU_Allreduce(&contiguous,&gcontiguous,1,MPIU_BOOL,MPI_LAND,PetscObjectComm((PetscObject)is));CHKERRQ(ierr);
    if (gcontiguous) {          /* We can do a no-copy implementation */
      PetscInt    n,N,bs;
      PetscMPIInt size;
      PetscInt    state;

      ierr = ISGetLocalSize(is,&n);CHKERRQ(ierr);
      ierr = VecGetBlockSize(X,&bs);CHKERRQ(ierr);
      if (n%bs || bs == 1) bs = -1; /* Do not decide block size if we do not have to */
      ierr = MPI_Comm_size(PetscObjectComm((PetscObject)X),&size);CHKERRQ(ierr);
      ierr = VecLockGet(X,&state);CHKERRQ(ierr);
      if (state) {
        const PetscScalar *x;
        ierr = VecGetArrayRead(X,&x);CHKERRQ(ierr);
        if (size == 1) {
          ierr = VecCreateSeqWithArray(PetscObjectComm((PetscObject)X),bs,n,(PetscScalar*)x+start,&Z);CHKERRQ(ierr);
        } else {
          ierr = ISGetSize(is,&N);CHKERRQ(ierr);
          ierr = VecCreateMPIWithArray(PetscObjectComm((PetscObject)X),bs,n,N,(PetscScalar*)x+start,&Z);CHKERRQ(ierr);
        }
        ierr = VecRestoreArrayRead(X,&x);CHKERRQ(ierr);
        ierr = VecLockPush(Z);CHKERRQ(ierr);
      } else {
        PetscScalar *x;
        ierr = VecGetArray(X,&x);CHKERRQ(ierr);
        if (size == 1) {
          ierr = VecCreateSeqWithArray(PetscObjectComm((PetscObject)X),bs,n,x+start,&Z);CHKERRQ(ierr);
        } else {
          ierr = ISGetSize(is,&N);CHKERRQ(ierr);
          ierr = VecCreateMPIWithArray(PetscObjectComm((PetscObject)X),bs,n,N,x+start,&Z);CHKERRQ(ierr);
        }
        ierr = VecRestoreArray(X,&x);CHKERRQ(ierr);
      }
    } else {                    /* Have to create a scatter and do a copy */
      VecScatter scatter;
      PetscInt   n,N;
      ierr = ISGetLocalSize(is,&n);CHKERRQ(ierr);
      ierr = ISGetSize(is,&N);CHKERRQ(ierr);
      ierr = VecCreate(PetscObjectComm((PetscObject)is),&Z);CHKERRQ(ierr);
      ierr = VecSetSizes(Z,n,N);CHKERRQ(ierr);
      ierr = VecSetType(Z,((PetscObject)X)->type_name);CHKERRQ(ierr);
      ierr = VecScatterCreate(X,is,Z,NULL,&scatter);CHKERRQ(ierr);
      ierr = VecScatterBegin(scatter,X,Z,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
      ierr = VecScatterEnd(scatter,X,Z,INSERT_VALUES,SCATTER_FORWARD);CHKERRQ(ierr);
      ierr = PetscObjectCompose((PetscObject)Z,"VecGetSubVector_Scatter",(PetscObject)scatter);CHKERRQ(ierr);
      ierr = VecScatterDestroy(&scatter);CHKERRQ(ierr);
    }
  }
  /* Record the state when the subvector was gotten so we know whether its values need to be put back */
  if (VecGetSubVectorSavedStateId < 0) {ierr = PetscObjectComposedDataRegister(&VecGetSubVectorSavedStateId);CHKERRQ(ierr);}
  ierr = PetscObjectComposedDataSetInt((PetscObject)Z,VecGetSubVectorSavedStateId,1);CHKERRQ(ierr);
  *Y   = Z;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecRestoreSubVector"
/*@
   VecRestoreSubVector - Restores a subvector extracted using VecGetSubVector()

   Collective on IS (and Vec if nonlocal entries need to be written)

   Input Arguments:
+ X - vector from which subvector was obtained
. is - index set representing the subset of X
- Y - subvector being restored

   Level: advanced

.seealso: VecGetSubVector()
@*/
PetscErrorCode  VecRestoreSubVector(Vec X,IS is,Vec *Y)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(X,VEC_CLASSID,1);
  PetscValidHeaderSpecific(is,IS_CLASSID,2);
  PetscValidPointer(Y,3);
  PetscValidHeaderSpecific(*Y,VEC_CLASSID,3);
  if (X->ops->restoresubvector) {
    ierr = (*X->ops->restoresubvector)(X,is,Y);CHKERRQ(ierr);
  } else {
    PETSC_UNUSED PetscObjectState dummystate = 0;
    PetscBool valid;
    ierr = PetscObjectComposedDataGetInt((PetscObject)*Y,VecGetSubVectorSavedStateId,dummystate,valid);CHKERRQ(ierr);
    if (!valid) {
      VecScatter scatter;

      ierr = PetscObjectQuery((PetscObject)*Y,"VecGetSubVector_Scatter",(PetscObject*)&scatter);CHKERRQ(ierr);
      if (scatter) {
        ierr = VecScatterBegin(scatter,*Y,X,INSERT_VALUES,SCATTER_REVERSE);CHKERRQ(ierr);
        ierr = VecScatterEnd(scatter,*Y,X,INSERT_VALUES,SCATTER_REVERSE);CHKERRQ(ierr);
      }
    }
    ierr = VecDestroy(Y);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

/*@C
   VecGetLocalVectorRead - Maps the local portion of a vector into a
   vector.  You must call VecRestoreLocalVectorRead() when the local
   vector is no longer needed.

   Not collective.

   Input parameter:
.  v - The vector for which the local vector is desired.

   Output parameter:
.  w - Upon exit this contains the local vector.

   Level: beginner
   
   Notes:
   This function is similar to VecGetArrayRead() which maps the local
   portion into a raw pointer.  VecGetLocalVectorRead() is usually
   almost as efficient as VecGetArrayRead() but in certain circumstances
   VecGetLocalVectorRead() can be much more efficient than
   VecGetArrayRead().  This is because the construction of a contiguous
   array representing the vector data required by VecGetArrayRead() can
   be an expensive operation for certain vector types.  For example, for
   GPU vectors VecGetArrayRead() requires that the data between device
   and host is synchronized.  

   Unlike VecGetLocalVector(), this routine is not collective and
   preserves cached information.

.seealso: VecRestoreLocalVectorRead(), VecGetLocalVector(), VecGetArrayRead(), VecGetArray()
@*/
#undef __FUNCT__
#define __FUNCT__ "VecGetLocalVectorRead"
PetscErrorCode VecGetLocalVectorRead(Vec v,Vec w)
{
  PetscErrorCode ierr;
  PetscScalar    *a;
  PetscInt       m1,m2;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(v,VEC_CLASSID,1);
  PetscValidHeaderSpecific(w,VEC_CLASSID,2);
  ierr = VecGetLocalSize(v,&m1);CHKERRQ(ierr);
  ierr = VecGetLocalSize(w,&m2);CHKERRQ(ierr);
  if (m1 != m2) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_INCOMP,"Vectors of different local sizes.");
  if (v->ops->getlocalvectorread) {
    ierr = (*v->ops->getlocalvectorread)(v,w);CHKERRQ(ierr);
  } else {
    ierr = VecGetArrayRead(v,(const PetscScalar**)&a);CHKERRQ(ierr);
    ierr = VecPlaceArray(w,a);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

/*@C
   VecRestoreLocalVectorRead - Unmaps the local portion of a vector
   previously mapped into a vector using VecGetLocalVectorRead().

   Not collective.

   Input parameter:
.  v - The local portion of this vector was previously mapped into w using VecGetLocalVectorRead().
.  w - The vector into which the local portion of v was mapped.

   Level: beginner

.seealso: VecGetLocalVectorRead(), VecGetLocalVector(), VecGetArrayRead(), VecGetArray()
@*/
#undef __FUNCT__
#define __FUNCT__ "VecRestoreLocalVectorRead"
PetscErrorCode VecRestoreLocalVectorRead(Vec v,Vec w)
{
  PetscErrorCode ierr;
  PetscScalar    *a;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(v,VEC_CLASSID,1);
  PetscValidHeaderSpecific(w,VEC_CLASSID,2);
  if (v->ops->restorelocalvectorread) {
    ierr = (*v->ops->restorelocalvectorread)(v,w);CHKERRQ(ierr);
  } else {
    ierr = VecGetArrayRead(w,(const PetscScalar**)&a);CHKERRQ(ierr);
    ierr = VecRestoreArrayRead(v,(const PetscScalar**)&a);CHKERRQ(ierr);
    ierr = VecResetArray(w);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

/*@C
   VecGetLocalVector - Maps the local portion of a vector into a
   vector.

   Collective on v, not collective on w.

   Input parameter:
.  v - The vector for which the local vector is desired.

   Output parameter:
.  w - Upon exit this contains the local vector.

   Level: beginner
   
   Notes:
   This function is similar to VecGetArray() which maps the local
   portion into a raw pointer.  VecGetLocalVector() is usually about as
   efficient as VecGetArray() but in certain circumstances
   VecGetLocalVector() can be much more efficient than VecGetArray().
   This is because the construction of a contiguous array representing
   the vector data required by VecGetArray() can be an expensive
   operation for certain vector types.  For example, for GPU vectors
   VecGetArray() requires that the data between device and host is
   synchronized.

.seealso: VecRestoreLocalVector(), VecGetLocalVectorRead(), VecGetArrayRead(), VecGetArray()
@*/
#undef __FUNCT__
#define __FUNCT__ "VecGetLocalVector"
PetscErrorCode VecGetLocalVector(Vec v,Vec w)
{
  PetscErrorCode ierr;
  PetscScalar    *a;
  PetscInt       m1,m2;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(v,VEC_CLASSID,1);
  PetscValidHeaderSpecific(w,VEC_CLASSID,2);
  ierr = VecGetLocalSize(v,&m1);CHKERRQ(ierr);
  ierr = VecGetLocalSize(w,&m2);CHKERRQ(ierr);
  if (m1 != m2) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_INCOMP,"Vectors of different local sizes.");
  if (v->ops->getlocalvector) {
    ierr = (*v->ops->getlocalvector)(v,w);CHKERRQ(ierr);
  } else {
    ierr = VecGetArray(v,&a);CHKERRQ(ierr);
    ierr = VecPlaceArray(w,a);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

/*@C
   VecRestoreLocalVector - Unmaps the local portion of a vector
   previously mapped into a vector using VecGetLocalVector().

   Logically collective.

   Input parameter:
.  v - The local portion of this vector was previously mapped into w using VecGetLocalVector().
.  w - The vector into which the local portion of v was mapped.

   Level: beginner

.seealso: VecGetLocalVector(), VecGetLocalVectorRead(), VecRestoreLocalVectorRead(), LocalVectorRead(), VecGetArrayRead(), VecGetArray()
@*/
#undef __FUNCT__
#define __FUNCT__ "VecRestoreLocalVector"
PetscErrorCode VecRestoreLocalVector(Vec v,Vec w)
{
  PetscErrorCode ierr;
  PetscScalar    *a;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(v,VEC_CLASSID,1);
  PetscValidHeaderSpecific(w,VEC_CLASSID,2);
  if (v->ops->restorelocalvector) {
    ierr = (*v->ops->restorelocalvector)(v,w);CHKERRQ(ierr);
  } else {
    ierr = VecGetArray(w,&a);CHKERRQ(ierr);
    ierr = VecRestoreArray(v,&a);CHKERRQ(ierr);
    ierr = VecResetArray(w);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecGetArray"
/*@C
   VecGetArray - Returns a pointer to a contiguous array that contains this
   processor's portion of the vector data. For the standard PETSc
   vectors, VecGetArray() returns a pointer to the local data array and
   does not use any copies. If the underlying vector data is not stored
   in a contiquous array this routine will copy the data to a contiquous
   array and return a pointer to that. You MUST call VecRestoreArray()
   when you no longer need access to the array.

   Logically Collective on Vec

   Input Parameter:
.  x - the vector

   Output Parameter:
.  a - location to put pointer to the array

   Fortran Note:
   This routine is used differently from Fortran 77
$    Vec         x
$    PetscScalar x_array(1)
$    PetscOffset i_x
$    PetscErrorCode ierr
$       call VecGetArray(x,x_array,i_x,ierr)
$
$   Access first local entry in vector with
$      value = x_array(i_x + 1)
$
$      ...... other code
$       call VecRestoreArray(x,x_array,i_x,ierr)
   For Fortran 90 see VecGetArrayF90()

   See the Fortran chapter of the users manual and
   petsc/src/snes/examples/tutorials/ex5f.F for details.

   Level: beginner

   Concepts: vector^accessing local values

.seealso: VecRestoreArray(), VecGetArrayRead(), VecGetArrays(), VecGetArrayF90(), VecGetArrayReadF90(), VecPlaceArray(), VecGetArray2d()
@*/
PetscErrorCode VecGetArray(Vec x,PetscScalar **a)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(x,VEC_CLASSID,1);
  VecLocked(x,1);
  if (x->petscnative) {
#if defined(PETSC_HAVE_CUSP)
    if (x->valid_GPU_array == PETSC_CUSP_GPU) {
      ierr = VecCUSPCopyFromGPU(x);CHKERRQ(ierr);
    } else if (x->valid_GPU_array == PETSC_CUSP_UNALLOCATED) {
      ierr = VecCUSPAllocateCheckHost(x); CHKERRQ(ierr);
    }
#endif
#if defined(PETSC_HAVE_VIENNACL)
    if (x->valid_GPU_array == PETSC_VIENNACL_GPU) {
      ierr = VecViennaCLCopyFromGPU(x);CHKERRQ(ierr);
    }
#endif
    *a = *((PetscScalar**)x->data);
  } else {
    ierr = (*x->ops->getarray)(x,a);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecGetArrayRead"
/*@C
   VecGetArrayRead - Get read-only pointer to contiguous array containing this processor's portion of the vector data.

   Not Collective

   Input Parameters:
.  x - the vector

   Output Parameter:
.  a - the array

   Level: beginner

   Notes:
   The array must be returned using a matching call to VecRestoreArrayRead().

   Unlike VecGetArray(), this routine is not collective and preserves cached information like vector norms.

   Standard PETSc vectors use contiguous storage so that this routine does not perform a copy.  Other vector
   implementations may require a copy, but must such implementations should cache the contiguous representation so that
   only one copy is performed when this routine is called multiple times in sequence.

.seealso: VecGetArray(), VecRestoreArray()
@*/
PetscErrorCode VecGetArrayRead(Vec x,const PetscScalar **a)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(x,VEC_CLASSID,1);
  if (x->petscnative) {
#if defined(PETSC_HAVE_CUSP)
    if (x->valid_GPU_array == PETSC_CUSP_GPU) {
      ierr = VecCUSPCopyFromGPU(x);CHKERRQ(ierr);
    }
#endif
#if defined(PETSC_HAVE_VIENNACL)
    if (x->valid_GPU_array == PETSC_VIENNACL_GPU) {
      ierr = VecViennaCLCopyFromGPU(x);CHKERRQ(ierr);
    }
#endif
    *a = *((PetscScalar **)x->data);
  } else if (x->ops->getarrayread) {
    ierr = (*x->ops->getarrayread)(x,a);CHKERRQ(ierr);
  } else {
    ierr = (*x->ops->getarray)(x,(PetscScalar**)a);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecGetArrays"
/*@C
   VecGetArrays - Returns a pointer to the arrays in a set of vectors
   that were created by a call to VecDuplicateVecs().  You MUST call
   VecRestoreArrays() when you no longer need access to the array.

   Logically Collective on Vec

   Input Parameter:
+  x - the vectors
-  n - the number of vectors

   Output Parameter:
.  a - location to put pointer to the array

   Fortran Note:
   This routine is not supported in Fortran.

   Level: intermediate

.seealso: VecGetArray(), VecRestoreArrays()
@*/
PetscErrorCode  VecGetArrays(const Vec x[],PetscInt n,PetscScalar **a[])
{
  PetscErrorCode ierr;
  PetscInt       i;
  PetscScalar    **q;

  PetscFunctionBegin;
  PetscValidPointer(x,1);
  PetscValidHeaderSpecific(*x,VEC_CLASSID,1);
  PetscValidPointer(a,3);
  if (n <= 0) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Must get at least one array n = %D",n);
  ierr = PetscMalloc1(n,&q);CHKERRQ(ierr);
  for (i=0; i<n; ++i) {
    ierr = VecGetArray(x[i],&q[i]);CHKERRQ(ierr);
  }
  *a = q;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecRestoreArrays"
/*@C
   VecRestoreArrays - Restores a group of vectors after VecGetArrays()
   has been called.

   Logically Collective on Vec

   Input Parameters:
+  x - the vector
.  n - the number of vectors
-  a - location of pointer to arrays obtained from VecGetArrays()

   Notes:
   For regular PETSc vectors this routine does not involve any copies. For
   any special vectors that do not store local vector data in a contiguous
   array, this routine will copy the data back into the underlying
   vector data structure from the arrays obtained with VecGetArrays().

   Fortran Note:
   This routine is not supported in Fortran.

   Level: intermediate

.seealso: VecGetArrays(), VecRestoreArray()
@*/
PetscErrorCode  VecRestoreArrays(const Vec x[],PetscInt n,PetscScalar **a[])
{
  PetscErrorCode ierr;
  PetscInt       i;
  PetscScalar    **q = *a;

  PetscFunctionBegin;
  PetscValidPointer(x,1);
  PetscValidHeaderSpecific(*x,VEC_CLASSID,1);
  PetscValidPointer(a,3);

  for (i=0; i<n; ++i) {
    ierr = VecRestoreArray(x[i],&q[i]);CHKERRQ(ierr);
  }
  ierr = PetscFree(q);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecRestoreArray"
/*@C
   VecRestoreArray - Restores a vector after VecGetArray() has been called.

   Logically Collective on Vec

   Input Parameters:
+  x - the vector
-  a - location of pointer to array obtained from VecGetArray()

   Level: beginner

   Notes:
   For regular PETSc vectors this routine does not involve any copies. For
   any special vectors that do not store local vector data in a contiguous
   array, this routine will copy the data back into the underlying
   vector data structure from the array obtained with VecGetArray().

   This routine actually zeros out the a pointer. This is to prevent accidental
   us of the array after it has been restored. If you pass null for a it will
   not zero the array pointer a.

   Fortran Note:
   This routine is used differently from Fortran 77
$    Vec         x
$    PetscScalar x_array(1)
$    PetscOffset i_x
$    PetscErrorCode ierr
$       call VecGetArray(x,x_array,i_x,ierr)
$
$   Access first local entry in vector with
$      value = x_array(i_x + 1)
$
$      ...... other code
$       call VecRestoreArray(x,x_array,i_x,ierr)

   See the Fortran chapter of the users manual and
   petsc/src/snes/examples/tutorials/ex5f.F for details.
   For Fortran 90 see VecRestoreArrayF90()

.seealso: VecGetArray(), VecRestoreArrayRead(), VecRestoreArrays(), VecRestoreArrayF90(), VecRestoreArrayReadF90(), VecPlaceArray(), VecRestoreArray2d()
@*/
PetscErrorCode VecRestoreArray(Vec x,PetscScalar **a)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(x,VEC_CLASSID,1);
  if (x->petscnative) {
#if defined(PETSC_HAVE_CUSP)
    x->valid_GPU_array = PETSC_CUSP_CPU;
#endif
#if defined(PETSC_HAVE_VIENNACL)
    x->valid_GPU_array = PETSC_VIENNACL_CPU;
#endif
  } else {
    ierr = (*x->ops->restorearray)(x,a);CHKERRQ(ierr);
  }
  if (a) *a = NULL;
  ierr = PetscObjectStateIncrease((PetscObject)x);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecRestoreArrayRead"
/*@C
   VecRestoreArrayRead - Restore array obtained with VecGetArrayRead()

   Not Collective

   Input Parameters:
+  vec - the vector
-  array - the array

   Level: beginner

.seealso: VecGetArray(), VecRestoreArray()
@*/
PetscErrorCode VecRestoreArrayRead(Vec x,const PetscScalar **a)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(x,VEC_CLASSID,1);
  if (x->petscnative) {
#if defined(PETSC_HAVE_VIENNACL)
    x->valid_GPU_array = PETSC_VIENNACL_CPU;
#endif
  } else if (x->ops->restorearrayread) {
    ierr = (*x->ops->restorearrayread)(x,a);CHKERRQ(ierr);
  } else {
    ierr = (*x->ops->restorearray)(x,(PetscScalar**)a);CHKERRQ(ierr);
  }
  if (a) *a = NULL;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecPlaceArray"
/*@
   VecPlaceArray - Allows one to replace the array in a vector with an
   array provided by the user. This is useful to avoid copying an array
   into a vector.

   Not Collective

   Input Parameters:
+  vec - the vector
-  array - the array

   Notes:
   You can return to the original array with a call to VecResetArray()

   Level: developer

.seealso: VecGetArray(), VecRestoreArray(), VecReplaceArray(), VecResetArray()

@*/
PetscErrorCode  VecPlaceArray(Vec vec,const PetscScalar array[])
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(vec,VEC_CLASSID,1);
  PetscValidType(vec,1);
  if (array) PetscValidScalarPointer(array,2);
  if (vec->ops->placearray) {
    ierr = (*vec->ops->placearray)(vec,array);CHKERRQ(ierr);
  } else SETERRQ(PetscObjectComm((PetscObject)vec),PETSC_ERR_SUP,"Cannot place array in this type of vector");
  ierr = PetscObjectStateIncrease((PetscObject)vec);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecReplaceArray"
/*@C
   VecReplaceArray - Allows one to replace the array in a vector with an
   array provided by the user. This is useful to avoid copying an array
   into a vector.

   Not Collective

   Input Parameters:
+  vec - the vector
-  array - the array

   Notes:
   This permanently replaces the array and frees the memory associated
   with the old array.

   The memory passed in MUST be obtained with PetscMalloc() and CANNOT be
   freed by the user. It will be freed when the vector is destroy.

   Not supported from Fortran

   Level: developer

.seealso: VecGetArray(), VecRestoreArray(), VecPlaceArray(), VecResetArray()

@*/
PetscErrorCode  VecReplaceArray(Vec vec,const PetscScalar array[])
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(vec,VEC_CLASSID,1);
  PetscValidType(vec,1);
  if (vec->ops->replacearray) {
    ierr = (*vec->ops->replacearray)(vec,array);CHKERRQ(ierr);
  } else SETERRQ(PetscObjectComm((PetscObject)vec),PETSC_ERR_SUP,"Cannot replace array in this type of vector");
  ierr = PetscObjectStateIncrease((PetscObject)vec);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/*MC
    VecDuplicateVecsF90 - Creates several vectors of the same type as an existing vector
    and makes them accessible via a Fortran90 pointer.

    Synopsis:
    VecDuplicateVecsF90(Vec x,PetscInt n,{Vec, pointer :: y(:)},integer ierr)

    Collective on Vec

    Input Parameters:
+   x - a vector to mimic
-   n - the number of vectors to obtain

    Output Parameters:
+   y - Fortran90 pointer to the array of vectors
-   ierr - error code

    Example of Usage:
.vb
    Vec x
    Vec, pointer :: y(:)
    ....
    call VecDuplicateVecsF90(x,2,y,ierr)
    call VecSet(y(2),alpha,ierr)
    call VecSet(y(2),alpha,ierr)
    ....
    call VecDestroyVecsF90(2,y,ierr)
.ve

    Notes:
    Not yet supported for all F90 compilers

    Use VecDestroyVecsF90() to free the space.

    Level: beginner

.seealso:  VecDestroyVecsF90(), VecDuplicateVecs()

M*/

/*MC
    VecRestoreArrayF90 - Restores a vector to a usable state after a call to
    VecGetArrayF90().

    Synopsis:
    VecRestoreArrayF90(Vec x,{Scalar, pointer :: xx_v(:)},integer ierr)

    Logically Collective on Vec

    Input Parameters:
+   x - vector
-   xx_v - the Fortran90 pointer to the array

    Output Parameter:
.   ierr - error code

    Example of Usage:
.vb
    PetscScalar, pointer :: xx_v(:)
    ....
    call VecGetArrayF90(x,xx_v,ierr)
    xx_v(3) = a
    call VecRestoreArrayF90(x,xx_v,ierr)
.ve

    Level: beginner

.seealso:  VecGetArrayF90(), VecGetArray(), VecRestoreArray(), UsingFortran, VecRestoreArrayReadF90()

M*/

/*MC
    VecDestroyVecsF90 - Frees a block of vectors obtained with VecDuplicateVecsF90().

    Synopsis:
    VecDestroyVecsF90(PetscInt n,{Vec, pointer :: x(:)},PetscErrorCode ierr)

    Collective on Vec

    Input Parameters:
+   n - the number of vectors previously obtained
-   x - pointer to array of vector pointers

    Output Parameter:
.   ierr - error code

    Notes:
    Not yet supported for all F90 compilers

    Level: beginner

.seealso:  VecDestroyVecs(), VecDuplicateVecsF90()

M*/

/*MC
    VecGetArrayF90 - Accesses a vector array from Fortran90. For default PETSc
    vectors, VecGetArrayF90() returns a pointer to the local data array. Otherwise,
    this routine is implementation dependent. You MUST call VecRestoreArrayF90()
    when you no longer need access to the array.

    Synopsis:
    VecGetArrayF90(Vec x,{Scalar, pointer :: xx_v(:)},integer ierr)

    Logically Collective on Vec

    Input Parameter:
.   x - vector

    Output Parameters:
+   xx_v - the Fortran90 pointer to the array
-   ierr - error code

    Example of Usage:
.vb
    PetscScalar, pointer :: xx_v(:)
    ....
    call VecGetArrayF90(x,xx_v,ierr)
    xx_v(3) = a
    call VecRestoreArrayF90(x,xx_v,ierr)
.ve

    If you ONLY intend to read entries from the array and not change any entries you should use VecGetArrayReadF90().

    Level: beginner

.seealso:  VecRestoreArrayF90(), VecGetArray(), VecRestoreArray(), VecGetArrayReadF90(), UsingFortran

M*/

 /*MC
    VecGetArrayReadF90 - Accesses a read only array from Fortran90. For default PETSc
    vectors, VecGetArrayF90() returns a pointer to the local data array. Otherwise,
    this routine is implementation dependent. You MUST call VecRestoreArrayReadF90()
    when you no longer need access to the array.

    Synopsis:
    VecGetArrayReadF90(Vec x,{Scalar, pointer :: xx_v(:)},integer ierr)

    Logically Collective on Vec

    Input Parameter:
.   x - vector

    Output Parameters:
+   xx_v - the Fortran90 pointer to the array
-   ierr - error code

    Example of Usage:
.vb
    PetscScalar, pointer :: xx_v(:)
    ....
    call VecGetArrayReadF90(x,xx_v,ierr)
    a = xx_v(3)
    call VecRestoreArrayReadF90(x,xx_v,ierr)
.ve

    If you intend to write entries into the array you must use VecGetArrayF90().

    Level: beginner

.seealso:  VecRestoreArrayReadF90(), VecGetArray(), VecRestoreArray(), VecGetArrayRead(), VecRestoreArrayRead(), VecGetArrayF90(), UsingFortran

M*/

/*MC
    VecRestoreArrayReadF90 - Restores a readonly vector to a usable state after a call to
    VecGetArrayReadF90().

    Synopsis:
    VecRestoreArrayReadF90(Vec x,{Scalar, pointer :: xx_v(:)},integer ierr)

    Logically Collective on Vec

    Input Parameters:
+   x - vector
-   xx_v - the Fortran90 pointer to the array

    Output Parameter:
.   ierr - error code

    Example of Usage:
.vb
    PetscScalar, pointer :: xx_v(:)
    ....
    call VecGetArrayReadF90(x,xx_v,ierr)
    a = xx_v(3)
    call VecRestoreArrayReadF90(x,xx_v,ierr)
.ve

    Level: beginner

.seealso:  VecGetArrayReadF90(), VecGetArray(), VecRestoreArray(), VecGetArrayRead(), VecRestoreArrayRead(),UsingFortran, VecRestoreArrayF90()

M*/

#undef __FUNCT__
#define __FUNCT__ "VecGetArray2d"
/*@C
   VecGetArray2d - Returns a pointer to a 2d contiguous array that contains this
   processor's portion of the vector data.  You MUST call VecRestoreArray2d()
   when you no longer need access to the array.

   Logically Collective

   Input Parameter:
+  x - the vector
.  m - first dimension of two dimensional array
.  n - second dimension of two dimensional array
.  mstart - first index you will use in first coordinate direction (often 0)
-  nstart - first index in the second coordinate direction (often 0)

   Output Parameter:
.  a - location to put pointer to the array

   Level: developer

  Notes:
   For a vector obtained from DMCreateLocalVector() mstart and nstart are likely
   obtained from the corner indices obtained from DMDAGetGhostCorners() while for
   DMCreateGlobalVector() they are the corner indices from DMDAGetCorners(). In both cases
   the arguments from DMDAGet[Ghost]Corners() are reversed in the call to VecGetArray2d().

   For standard PETSc vectors this is an inexpensive call; it does not copy the vector values.

   Concepts: vector^accessing local values as 2d array

.seealso: VecGetArray(), VecRestoreArray(), VecGetArrays(), VecGetArrayF90(), VecPlaceArray(),
          VecRestoreArray2d(), DMDAVecGetArray(), DMDAVecRestoreArray(), VecGetArray3d(), VecRestoreArray3d(),
          VecGetArray1d(), VecRestoreArray1d(), VecGetArray4d(), VecRestoreArray4d()
@*/
PetscErrorCode  VecGetArray2d(Vec x,PetscInt m,PetscInt n,PetscInt mstart,PetscInt nstart,PetscScalar **a[])
{
  PetscErrorCode ierr;
  PetscInt       i,N;
  PetscScalar    *aa;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(x,VEC_CLASSID,1);
  PetscValidPointer(a,6);
  PetscValidType(x,1);
  ierr = VecGetLocalSize(x,&N);CHKERRQ(ierr);
  if (m*n != N) SETERRQ3(PETSC_COMM_SELF,PETSC_ERR_ARG_INCOMP,"Local array size %D does not match 2d array dimensions %D by %D",N,m,n);
  ierr = VecGetArray(x,&aa);CHKERRQ(ierr);

  ierr = PetscMalloc1(m,a);CHKERRQ(ierr);
  for (i=0; i<m; i++) (*a)[i] = aa + i*n - nstart;
  *a -= mstart;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecRestoreArray2d"
/*@C
   VecRestoreArray2d - Restores a vector after VecGetArray2d() has been called.

   Logically Collective

   Input Parameters:
+  x - the vector
.  m - first dimension of two dimensional array
.  n - second dimension of the two dimensional array
.  mstart - first index you will use in first coordinate direction (often 0)
.  nstart - first index in the second coordinate direction (often 0)
-  a - location of pointer to array obtained from VecGetArray2d()

   Level: developer

   Notes:
   For regular PETSc vectors this routine does not involve any copies. For
   any special vectors that do not store local vector data in a contiguous
   array, this routine will copy the data back into the underlying
   vector data structure from the array obtained with VecGetArray().

   This routine actually zeros out the a pointer.

.seealso: VecGetArray(), VecRestoreArray(), VecRestoreArrays(), VecRestoreArrayF90(), VecPlaceArray(),
          VecGetArray2d(), VecGetArray3d(), VecRestoreArray3d(), DMDAVecGetArray(), DMDAVecRestoreArray()
          VecGetArray1d(), VecRestoreArray1d(), VecGetArray4d(), VecRestoreArray4d()
@*/
PetscErrorCode  VecRestoreArray2d(Vec x,PetscInt m,PetscInt n,PetscInt mstart,PetscInt nstart,PetscScalar **a[])
{
  PetscErrorCode ierr;
  void           *dummy;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(x,VEC_CLASSID,1);
  PetscValidPointer(a,6);
  PetscValidType(x,1);
  dummy = (void*)(*a + mstart);
  ierr  = PetscFree(dummy);CHKERRQ(ierr);
  ierr  = VecRestoreArray(x,NULL);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecGetArray1d"
/*@C
   VecGetArray1d - Returns a pointer to a 1d contiguous array that contains this
   processor's portion of the vector data.  You MUST call VecRestoreArray1d()
   when you no longer need access to the array.

   Logically Collective

   Input Parameter:
+  x - the vector
.  m - first dimension of two dimensional array
-  mstart - first index you will use in first coordinate direction (often 0)

   Output Parameter:
.  a - location to put pointer to the array

   Level: developer

  Notes:
   For a vector obtained from DMCreateLocalVector() mstart are likely
   obtained from the corner indices obtained from DMDAGetGhostCorners() while for
   DMCreateGlobalVector() they are the corner indices from DMDAGetCorners().

   For standard PETSc vectors this is an inexpensive call; it does not copy the vector values.

.seealso: VecGetArray(), VecRestoreArray(), VecGetArrays(), VecGetArrayF90(), VecPlaceArray(),
          VecRestoreArray2d(), DMDAVecGetArray(), DMDAVecRestoreArray(), VecGetArray3d(), VecRestoreArray3d(),
          VecGetArray2d(), VecRestoreArray1d(), VecGetArray4d(), VecRestoreArray4d()
@*/
PetscErrorCode  VecGetArray1d(Vec x,PetscInt m,PetscInt mstart,PetscScalar *a[])
{
  PetscErrorCode ierr;
  PetscInt       N;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(x,VEC_CLASSID,1);
  PetscValidPointer(a,4);
  PetscValidType(x,1);
  ierr = VecGetLocalSize(x,&N);CHKERRQ(ierr);
  if (m != N) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Local array size %D does not match 1d array dimensions %D",N,m);
  ierr = VecGetArray(x,a);CHKERRQ(ierr);
  *a  -= mstart;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecRestoreArray1d"
/*@C
   VecRestoreArray1d - Restores a vector after VecGetArray1d() has been called.

   Logically Collective

   Input Parameters:
+  x - the vector
.  m - first dimension of two dimensional array
.  mstart - first index you will use in first coordinate direction (often 0)
-  a - location of pointer to array obtained from VecGetArray21()

   Level: developer

   Notes:
   For regular PETSc vectors this routine does not involve any copies. For
   any special vectors that do not store local vector data in a contiguous
   array, this routine will copy the data back into the underlying
   vector data structure from the array obtained with VecGetArray1d().

   This routine actually zeros out the a pointer.

   Concepts: vector^accessing local values as 1d array

.seealso: VecGetArray(), VecRestoreArray(), VecRestoreArrays(), VecRestoreArrayF90(), VecPlaceArray(),
          VecGetArray2d(), VecGetArray3d(), VecRestoreArray3d(), DMDAVecGetArray(), DMDAVecRestoreArray()
          VecGetArray1d(), VecRestoreArray2d(), VecGetArray4d(), VecRestoreArray4d()
@*/
PetscErrorCode  VecRestoreArray1d(Vec x,PetscInt m,PetscInt mstart,PetscScalar *a[])
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(x,VEC_CLASSID,1);
  PetscValidType(x,1);
  ierr = VecRestoreArray(x,NULL);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "VecGetArray3d"
/*@C
   VecGetArray3d - Returns a pointer to a 3d contiguous array that contains this
   processor's portion of the vector data.  You MUST call VecRestoreArray3d()
   when you no longer need access to the array.

   Logically Collective

   Input Parameter:
+  x - the vector
.  m - first dimension of three dimensional array
.  n - second dimension of three dimensional array
.  p - third dimension of three dimensional array
.  mstart - first index you will use in first coordinate direction (often 0)
.  nstart - first index in the second coordinate direction (often 0)
-  pstart - first index in the third coordinate direction (often 0)

   Output Parameter:
.  a - location to put pointer to the array

   Level: developer

  Notes:
   For a vector obtained from DMCreateLocalVector() mstart, nstart, and pstart are likely
   obtained from the corner indices obtained from DMDAGetGhostCorners() while for
   DMCreateGlobalVector() they are the corner indices from DMDAGetCorners(). In both cases
   the arguments from DMDAGet[Ghost]Corners() are reversed in the call to VecGetArray3d().

   For standard PETSc vectors this is an inexpensive call; it does not copy the vector values.

   Concepts: vector^accessing local values as 3d array

.seealso: VecGetArray(), VecRestoreArray(), VecGetArrays(), VecGetArrayF90(), VecPlaceArray(),
          VecRestoreArray2d(), DMDAVecGetarray(), DMDAVecRestoreArray(), VecGetArray3d(), VecRestoreArray3d(),
          VecGetArray1d(), VecRestoreArray1d(), VecGetArray4d(), VecRestoreArray4d()
@*/
PetscErrorCode  VecGetArray3d(Vec x,PetscInt m,PetscInt n,PetscInt p,PetscInt mstart,PetscInt nstart,PetscInt pstart,PetscScalar ***a[])
{
  PetscErrorCode ierr;
  PetscInt       i,N,j;
  PetscScalar    *aa,**b;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(x,VEC_CLASSID,1);
  PetscValidPointer(a,8);
  PetscValidType(x,1);
  ierr = VecGetLocalSize(x,&N);CHKERRQ(ierr);
  if (m*n*p != N) SETERRQ4(PETSC_COMM_SELF,PETSC_ERR_ARG_INCOMP,"Local array size %D does not match 3d array dimensions %D by %D by %D",N,m,n,p);
  ierr = VecGetArray(x,&aa);CHKERRQ(ierr);

  ierr = PetscMalloc1(m*sizeof(PetscScalar**)+m*n,a);CHKERRQ(ierr);
  b    = (PetscScalar**)((*a) + m);
  for (i=0; i<m; i++) (*a)[i] = b + i*n - nstart;
  for (i=0; i<m; i++)
    for (j=0; j<n; j++)
      b[i*n+j] = aa + i*n*p + j*p - pstart;

  *a -= mstart;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecRestoreArray3d"
/*@C
   VecRestoreArray3d - Restores a vector after VecGetArray3d() has been called.

   Logically Collective

   Input Parameters:
+  x - the vector
.  m - first dimension of three dimensional array
.  n - second dimension of the three dimensional array
.  p - third dimension of the three dimensional array
.  mstart - first index you will use in first coordinate direction (often 0)
.  nstart - first index in the second coordinate direction (often 0)
.  pstart - first index in the third coordinate direction (often 0)
-  a - location of pointer to array obtained from VecGetArray3d()

   Level: developer

   Notes:
   For regular PETSc vectors this routine does not involve any copies. For
   any special vectors that do not store local vector data in a contiguous
   array, this routine will copy the data back into the underlying
   vector data structure from the array obtained with VecGetArray().

   This routine actually zeros out the a pointer.

.seealso: VecGetArray(), VecRestoreArray(), VecRestoreArrays(), VecRestoreArrayF90(), VecPlaceArray(),
          VecGetArray2d(), VecGetArray3d(), VecRestoreArray3d(), DMDAVecGetArray(), DMDAVecRestoreArray()
          VecGetArray1d(), VecRestoreArray1d(), VecGetArray4d(), VecRestoreArray4d(), VecGet
@*/
PetscErrorCode  VecRestoreArray3d(Vec x,PetscInt m,PetscInt n,PetscInt p,PetscInt mstart,PetscInt nstart,PetscInt pstart,PetscScalar ***a[])
{
  PetscErrorCode ierr;
  void           *dummy;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(x,VEC_CLASSID,1);
  PetscValidPointer(a,8);
  PetscValidType(x,1);
  dummy = (void*)(*a + mstart);
  ierr  = PetscFree(dummy);CHKERRQ(ierr);
  ierr  = VecRestoreArray(x,NULL);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecGetArray4d"
/*@C
   VecGetArray4d - Returns a pointer to a 4d contiguous array that contains this
   processor's portion of the vector data.  You MUST call VecRestoreArray4d()
   when you no longer need access to the array.

   Logically Collective

   Input Parameter:
+  x - the vector
.  m - first dimension of four dimensional array
.  n - second dimension of four dimensional array
.  p - third dimension of four dimensional array
.  q - fourth dimension of four dimensional array
.  mstart - first index you will use in first coordinate direction (often 0)
.  nstart - first index in the second coordinate direction (often 0)
.  pstart - first index in the third coordinate direction (often 0)
-  qstart - first index in the fourth coordinate direction (often 0)

   Output Parameter:
.  a - location to put pointer to the array

   Level: beginner

  Notes:
   For a vector obtained from DMCreateLocalVector() mstart, nstart, and pstart are likely
   obtained from the corner indices obtained from DMDAGetGhostCorners() while for
   DMCreateGlobalVector() they are the corner indices from DMDAGetCorners(). In both cases
   the arguments from DMDAGet[Ghost]Corners() are reversed in the call to VecGetArray3d().

   For standard PETSc vectors this is an inexpensive call; it does not copy the vector values.

   Concepts: vector^accessing local values as 3d array

.seealso: VecGetArray(), VecRestoreArray(), VecGetArrays(), VecGetArrayF90(), VecPlaceArray(),
          VecRestoreArray2d(), DMDAVecGetarray(), DMDAVecRestoreArray(), VecGetArray3d(), VecRestoreArray3d(),
          VecGetArray1d(), VecRestoreArray1d(), VecGetArray4d(), VecRestoreArray4d()
@*/
PetscErrorCode  VecGetArray4d(Vec x,PetscInt m,PetscInt n,PetscInt p,PetscInt q,PetscInt mstart,PetscInt nstart,PetscInt pstart,PetscInt qstart,PetscScalar ****a[])
{
  PetscErrorCode ierr;
  PetscInt       i,N,j,k;
  PetscScalar    *aa,***b,**c;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(x,VEC_CLASSID,1);
  PetscValidPointer(a,10);
  PetscValidType(x,1);
  ierr = VecGetLocalSize(x,&N);CHKERRQ(ierr);
  if (m*n*p*q != N) SETERRQ5(PETSC_COMM_SELF,PETSC_ERR_ARG_INCOMP,"Local array size %D does not match 4d array dimensions %D by %D by %D by %D",N,m,n,p,q);
  ierr = VecGetArray(x,&aa);CHKERRQ(ierr);

  ierr = PetscMalloc1(m*sizeof(PetscScalar***)+m*n*sizeof(PetscScalar**)+m*n*p,a);CHKERRQ(ierr);
  b    = (PetscScalar***)((*a) + m);
  c    = (PetscScalar**)(b + m*n);
  for (i=0; i<m; i++) (*a)[i] = b + i*n - nstart;
  for (i=0; i<m; i++)
    for (j=0; j<n; j++)
      b[i*n+j] = c + i*n*p + j*p - pstart;
  for (i=0; i<m; i++)
    for (j=0; j<n; j++)
      for (k=0; k<p; k++)
        c[i*n*p+j*p+k] = aa + i*n*p*q + j*p*q + k*q - qstart;
  *a -= mstart;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecRestoreArray4d"
/*@C
   VecRestoreArray4d - Restores a vector after VecGetArray3d() has been called.

   Logically Collective

   Input Parameters:
+  x - the vector
.  m - first dimension of four dimensional array
.  n - second dimension of the four dimensional array
.  p - third dimension of the four dimensional array
.  q - fourth dimension of the four dimensional array
.  mstart - first index you will use in first coordinate direction (often 0)
.  nstart - first index in the second coordinate direction (often 0)
.  pstart - first index in the third coordinate direction (often 0)
.  qstart - first index in the fourth coordinate direction (often 0)
-  a - location of pointer to array obtained from VecGetArray4d()

   Level: beginner

   Notes:
   For regular PETSc vectors this routine does not involve any copies. For
   any special vectors that do not store local vector data in a contiguous
   array, this routine will copy the data back into the underlying
   vector data structure from the array obtained with VecGetArray().

   This routine actually zeros out the a pointer.

.seealso: VecGetArray(), VecRestoreArray(), VecRestoreArrays(), VecRestoreArrayF90(), VecPlaceArray(),
          VecGetArray2d(), VecGetArray3d(), VecRestoreArray3d(), DMDAVecGetArray(), DMDAVecRestoreArray()
          VecGetArray1d(), VecRestoreArray1d(), VecGetArray4d(), VecRestoreArray4d(), VecGet
@*/
PetscErrorCode  VecRestoreArray4d(Vec x,PetscInt m,PetscInt n,PetscInt p,PetscInt q,PetscInt mstart,PetscInt nstart,PetscInt pstart,PetscInt qstart,PetscScalar ****a[])
{
  PetscErrorCode ierr;
  void           *dummy;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(x,VEC_CLASSID,1);
  PetscValidPointer(a,8);
  PetscValidType(x,1);
  dummy = (void*)(*a + mstart);
  ierr  = PetscFree(dummy);CHKERRQ(ierr);
  ierr  = VecRestoreArray(x,NULL);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

 #undef __FUNCT__
#define __FUNCT__ "VecGetArray2dRead"
/*@C
   VecGetArray2dRead - Returns a pointer to a 2d contiguous array that contains this
   processor's portion of the vector data.  You MUST call VecRestoreArray2dRead()
   when you no longer need access to the array.

   Logically Collective

   Input Parameter:
+  x - the vector
.  m - first dimension of two dimensional array
.  n - second dimension of two dimensional array
.  mstart - first index you will use in first coordinate direction (often 0)
-  nstart - first index in the second coordinate direction (often 0)

   Output Parameter:
.  a - location to put pointer to the array

   Level: developer

  Notes:
   For a vector obtained from DMCreateLocalVector() mstart and nstart are likely
   obtained from the corner indices obtained from DMDAGetGhostCorners() while for
   DMCreateGlobalVector() they are the corner indices from DMDAGetCorners(). In both cases
   the arguments from DMDAGet[Ghost]Corners() are reversed in the call to VecGetArray2d().

   For standard PETSc vectors this is an inexpensive call; it does not copy the vector values.

   Concepts: vector^accessing local values as 2d array

.seealso: VecGetArray(), VecRestoreArray(), VecGetArrays(), VecGetArrayF90(), VecPlaceArray(),
          VecRestoreArray2d(), DMDAVecGetArray(), DMDAVecRestoreArray(), VecGetArray3d(), VecRestoreArray3d(),
          VecGetArray1d(), VecRestoreArray1d(), VecGetArray4d(), VecRestoreArray4d()
@*/
PetscErrorCode  VecGetArray2dRead(Vec x,PetscInt m,PetscInt n,PetscInt mstart,PetscInt nstart,PetscScalar **a[])
{
  PetscErrorCode    ierr;
  PetscInt          i,N;
  const PetscScalar *aa;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(x,VEC_CLASSID,1);
  PetscValidPointer(a,6);
  PetscValidType(x,1);
  ierr = VecGetLocalSize(x,&N);CHKERRQ(ierr);
  if (m*n != N) SETERRQ3(PETSC_COMM_SELF,PETSC_ERR_ARG_INCOMP,"Local array size %D does not match 2d array dimensions %D by %D",N,m,n);
  ierr = VecGetArrayRead(x,&aa);CHKERRQ(ierr);

  ierr = PetscMalloc1(m,a);CHKERRQ(ierr);
  for (i=0; i<m; i++) (*a)[i] = (PetscScalar*) aa + i*n - nstart;
  *a -= mstart;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecRestoreArray2dRead"
/*@C
   VecRestoreArray2dRead - Restores a vector after VecGetArray2dRead() has been called.

   Logically Collective

   Input Parameters:
+  x - the vector
.  m - first dimension of two dimensional array
.  n - second dimension of the two dimensional array
.  mstart - first index you will use in first coordinate direction (often 0)
.  nstart - first index in the second coordinate direction (often 0)
-  a - location of pointer to array obtained from VecGetArray2d()

   Level: developer

   Notes:
   For regular PETSc vectors this routine does not involve any copies. For
   any special vectors that do not store local vector data in a contiguous
   array, this routine will copy the data back into the underlying
   vector data structure from the array obtained with VecGetArray().

   This routine actually zeros out the a pointer.

.seealso: VecGetArray(), VecRestoreArray(), VecRestoreArrays(), VecRestoreArrayF90(), VecPlaceArray(),
          VecGetArray2d(), VecGetArray3d(), VecRestoreArray3d(), DMDAVecGetArray(), DMDAVecRestoreArray()
          VecGetArray1d(), VecRestoreArray1d(), VecGetArray4d(), VecRestoreArray4d()
@*/
PetscErrorCode  VecRestoreArray2dRead(Vec x,PetscInt m,PetscInt n,PetscInt mstart,PetscInt nstart,PetscScalar **a[])
{
  PetscErrorCode ierr;
  void           *dummy;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(x,VEC_CLASSID,1);
  PetscValidPointer(a,6);
  PetscValidType(x,1);
  dummy = (void*)(*a + mstart);
  ierr  = PetscFree(dummy);CHKERRQ(ierr);
  ierr  = VecRestoreArrayRead(x,NULL);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecGetArray1dRead"
/*@C
   VecGetArray1dRead - Returns a pointer to a 1d contiguous array that contains this
   processor's portion of the vector data.  You MUST call VecRestoreArray1dRead()
   when you no longer need access to the array.

   Logically Collective

   Input Parameter:
+  x - the vector
.  m - first dimension of two dimensional array
-  mstart - first index you will use in first coordinate direction (often 0)

   Output Parameter:
.  a - location to put pointer to the array

   Level: developer

  Notes:
   For a vector obtained from DMCreateLocalVector() mstart are likely
   obtained from the corner indices obtained from DMDAGetGhostCorners() while for
   DMCreateGlobalVector() they are the corner indices from DMDAGetCorners().

   For standard PETSc vectors this is an inexpensive call; it does not copy the vector values.

.seealso: VecGetArray(), VecRestoreArray(), VecGetArrays(), VecGetArrayF90(), VecPlaceArray(),
          VecRestoreArray2d(), DMDAVecGetArray(), DMDAVecRestoreArray(), VecGetArray3d(), VecRestoreArray3d(),
          VecGetArray2d(), VecRestoreArray1d(), VecGetArray4d(), VecRestoreArray4d()
@*/
PetscErrorCode  VecGetArray1dRead(Vec x,PetscInt m,PetscInt mstart,PetscScalar *a[])
{
  PetscErrorCode ierr;
  PetscInt       N;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(x,VEC_CLASSID,1);
  PetscValidPointer(a,4);
  PetscValidType(x,1);
  ierr = VecGetLocalSize(x,&N);CHKERRQ(ierr);
  if (m != N) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"Local array size %D does not match 1d array dimensions %D",N,m);
  ierr = VecGetArrayRead(x,(const PetscScalar**)a);CHKERRQ(ierr);
  *a  -= mstart;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecRestoreArray1dRead"
/*@C
   VecRestoreArray1dRead - Restores a vector after VecGetArray1dRead() has been called.

   Logically Collective

   Input Parameters:
+  x - the vector
.  m - first dimension of two dimensional array
.  mstart - first index you will use in first coordinate direction (often 0)
-  a - location of pointer to array obtained from VecGetArray21()

   Level: developer

   Notes:
   For regular PETSc vectors this routine does not involve any copies. For
   any special vectors that do not store local vector data in a contiguous
   array, this routine will copy the data back into the underlying
   vector data structure from the array obtained with VecGetArray1dRead().

   This routine actually zeros out the a pointer.

   Concepts: vector^accessing local values as 1d array

.seealso: VecGetArray(), VecRestoreArray(), VecRestoreArrays(), VecRestoreArrayF90(), VecPlaceArray(),
          VecGetArray2d(), VecGetArray3d(), VecRestoreArray3d(), DMDAVecGetArray(), DMDAVecRestoreArray()
          VecGetArray1d(), VecRestoreArray2d(), VecGetArray4d(), VecRestoreArray4d()
@*/
PetscErrorCode  VecRestoreArray1dRead(Vec x,PetscInt m,PetscInt mstart,PetscScalar *a[])
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(x,VEC_CLASSID,1);
  PetscValidType(x,1);
  ierr = VecRestoreArrayRead(x,NULL);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "VecGetArray3dRead"
/*@C
   VecGetArray3dRead - Returns a pointer to a 3d contiguous array that contains this
   processor's portion of the vector data.  You MUST call VecRestoreArray3dRead()
   when you no longer need access to the array.

   Logically Collective

   Input Parameter:
+  x - the vector
.  m - first dimension of three dimensional array
.  n - second dimension of three dimensional array
.  p - third dimension of three dimensional array
.  mstart - first index you will use in first coordinate direction (often 0)
.  nstart - first index in the second coordinate direction (often 0)
-  pstart - first index in the third coordinate direction (often 0)

   Output Parameter:
.  a - location to put pointer to the array

   Level: developer

  Notes:
   For a vector obtained from DMCreateLocalVector() mstart, nstart, and pstart are likely
   obtained from the corner indices obtained from DMDAGetGhostCorners() while for
   DMCreateGlobalVector() they are the corner indices from DMDAGetCorners(). In both cases
   the arguments from DMDAGet[Ghost]Corners() are reversed in the call to VecGetArray3dRead().

   For standard PETSc vectors this is an inexpensive call; it does not copy the vector values.

   Concepts: vector^accessing local values as 3d array

.seealso: VecGetArray(), VecRestoreArray(), VecGetArrays(), VecGetArrayF90(), VecPlaceArray(),
          VecRestoreArray2d(), DMDAVecGetarray(), DMDAVecRestoreArray(), VecGetArray3d(), VecRestoreArray3d(),
          VecGetArray1d(), VecRestoreArray1d(), VecGetArray4d(), VecRestoreArray4d()
@*/
PetscErrorCode  VecGetArray3dRead(Vec x,PetscInt m,PetscInt n,PetscInt p,PetscInt mstart,PetscInt nstart,PetscInt pstart,PetscScalar ***a[])
{
  PetscErrorCode    ierr;
  PetscInt          i,N,j;
  const PetscScalar *aa;
  PetscScalar       **b;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(x,VEC_CLASSID,1);
  PetscValidPointer(a,8);
  PetscValidType(x,1);
  ierr = VecGetLocalSize(x,&N);CHKERRQ(ierr);
  if (m*n*p != N) SETERRQ4(PETSC_COMM_SELF,PETSC_ERR_ARG_INCOMP,"Local array size %D does not match 3d array dimensions %D by %D by %D",N,m,n,p);
  ierr = VecGetArrayRead(x,&aa);CHKERRQ(ierr);

  ierr = PetscMalloc1(m*sizeof(PetscScalar**)+m*n,a);CHKERRQ(ierr);
  b    = (PetscScalar**)((*a) + m);
  for (i=0; i<m; i++) (*a)[i] = b + i*n - nstart;
  for (i=0; i<m; i++)
    for (j=0; j<n; j++)
      b[i*n+j] = (PetscScalar *)aa + i*n*p + j*p - pstart;

  *a -= mstart;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecRestoreArray3dRead"
/*@C
   VecRestoreArray3dRead - Restores a vector after VecGetArray3dRead() has been called.

   Logically Collective

   Input Parameters:
+  x - the vector
.  m - first dimension of three dimensional array
.  n - second dimension of the three dimensional array
.  p - third dimension of the three dimensional array
.  mstart - first index you will use in first coordinate direction (often 0)
.  nstart - first index in the second coordinate direction (often 0)
.  pstart - first index in the third coordinate direction (often 0)
-  a - location of pointer to array obtained from VecGetArray3dRead()

   Level: developer

   Notes:
   For regular PETSc vectors this routine does not involve any copies. For
   any special vectors that do not store local vector data in a contiguous
   array, this routine will copy the data back into the underlying
   vector data structure from the array obtained with VecGetArray().

   This routine actually zeros out the a pointer.

.seealso: VecGetArray(), VecRestoreArray(), VecRestoreArrays(), VecRestoreArrayF90(), VecPlaceArray(),
          VecGetArray2d(), VecGetArray3d(), VecRestoreArray3d(), DMDAVecGetArray(), DMDAVecRestoreArray()
          VecGetArray1d(), VecRestoreArray1d(), VecGetArray4d(), VecRestoreArray4d(), VecGet
@*/
PetscErrorCode  VecRestoreArray3dRead(Vec x,PetscInt m,PetscInt n,PetscInt p,PetscInt mstart,PetscInt nstart,PetscInt pstart,PetscScalar ***a[])
{
  PetscErrorCode ierr;
  void           *dummy;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(x,VEC_CLASSID,1);
  PetscValidPointer(a,8);
  PetscValidType(x,1);
  dummy = (void*)(*a + mstart);
  ierr  = PetscFree(dummy);CHKERRQ(ierr);
  ierr  = VecRestoreArrayRead(x,NULL);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecGetArray4dRead"
/*@C
   VecGetArray4dRead - Returns a pointer to a 4d contiguous array that contains this
   processor's portion of the vector data.  You MUST call VecRestoreArray4dRead()
   when you no longer need access to the array.

   Logically Collective

   Input Parameter:
+  x - the vector
.  m - first dimension of four dimensional array
.  n - second dimension of four dimensional array
.  p - third dimension of four dimensional array
.  q - fourth dimension of four dimensional array
.  mstart - first index you will use in first coordinate direction (often 0)
.  nstart - first index in the second coordinate direction (often 0)
.  pstart - first index in the third coordinate direction (often 0)
-  qstart - first index in the fourth coordinate direction (often 0)

   Output Parameter:
.  a - location to put pointer to the array

   Level: beginner

  Notes:
   For a vector obtained from DMCreateLocalVector() mstart, nstart, and pstart are likely
   obtained from the corner indices obtained from DMDAGetGhostCorners() while for
   DMCreateGlobalVector() they are the corner indices from DMDAGetCorners(). In both cases
   the arguments from DMDAGet[Ghost]Corners() are reversed in the call to VecGetArray3d().

   For standard PETSc vectors this is an inexpensive call; it does not copy the vector values.

   Concepts: vector^accessing local values as 3d array

.seealso: VecGetArray(), VecRestoreArray(), VecGetArrays(), VecGetArrayF90(), VecPlaceArray(),
          VecRestoreArray2d(), DMDAVecGetarray(), DMDAVecRestoreArray(), VecGetArray3d(), VecRestoreArray3d(),
          VecGetArray1d(), VecRestoreArray1d(), VecGetArray4d(), VecRestoreArray4d()
@*/
PetscErrorCode  VecGetArray4dRead(Vec x,PetscInt m,PetscInt n,PetscInt p,PetscInt q,PetscInt mstart,PetscInt nstart,PetscInt pstart,PetscInt qstart,PetscScalar ****a[])
{
  PetscErrorCode    ierr;
  PetscInt          i,N,j,k;
  const PetscScalar *aa;
  PetscScalar       ***b,**c;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(x,VEC_CLASSID,1);
  PetscValidPointer(a,10);
  PetscValidType(x,1);
  ierr = VecGetLocalSize(x,&N);CHKERRQ(ierr);
  if (m*n*p*q != N) SETERRQ5(PETSC_COMM_SELF,PETSC_ERR_ARG_INCOMP,"Local array size %D does not match 4d array dimensions %D by %D by %D by %D",N,m,n,p,q);
  ierr = VecGetArrayRead(x,&aa);CHKERRQ(ierr);

  ierr = PetscMalloc1(m*sizeof(PetscScalar***)+m*n*sizeof(PetscScalar**)+m*n*p,a);CHKERRQ(ierr);
  b    = (PetscScalar***)((*a) + m);
  c    = (PetscScalar**)(b + m*n);
  for (i=0; i<m; i++) (*a)[i] = b + i*n - nstart;
  for (i=0; i<m; i++)
    for (j=0; j<n; j++)
      b[i*n+j] = c + i*n*p + j*p - pstart;
  for (i=0; i<m; i++)
    for (j=0; j<n; j++)
      for (k=0; k<p; k++)
        c[i*n*p+j*p+k] = (PetscScalar*) aa + i*n*p*q + j*p*q + k*q - qstart;
  *a -= mstart;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecRestoreArray4dRead"
/*@C
   VecRestoreArray4dRead - Restores a vector after VecGetArray3d() has been called.

   Logically Collective

   Input Parameters:
+  x - the vector
.  m - first dimension of four dimensional array
.  n - second dimension of the four dimensional array
.  p - third dimension of the four dimensional array
.  q - fourth dimension of the four dimensional array
.  mstart - first index you will use in first coordinate direction (often 0)
.  nstart - first index in the second coordinate direction (often 0)
.  pstart - first index in the third coordinate direction (often 0)
.  qstart - first index in the fourth coordinate direction (often 0)
-  a - location of pointer to array obtained from VecGetArray4dRead()

   Level: beginner

   Notes:
   For regular PETSc vectors this routine does not involve any copies. For
   any special vectors that do not store local vector data in a contiguous
   array, this routine will copy the data back into the underlying
   vector data structure from the array obtained with VecGetArray().

   This routine actually zeros out the a pointer.

.seealso: VecGetArray(), VecRestoreArray(), VecRestoreArrays(), VecRestoreArrayF90(), VecPlaceArray(),
          VecGetArray2d(), VecGetArray3d(), VecRestoreArray3d(), DMDAVecGetArray(), DMDAVecRestoreArray()
          VecGetArray1d(), VecRestoreArray1d(), VecGetArray4d(), VecRestoreArray4d(), VecGet
@*/
PetscErrorCode  VecRestoreArray4dRead(Vec x,PetscInt m,PetscInt n,PetscInt p,PetscInt q,PetscInt mstart,PetscInt nstart,PetscInt pstart,PetscInt qstart,PetscScalar ****a[])
{
  PetscErrorCode ierr;
  void           *dummy;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(x,VEC_CLASSID,1);
  PetscValidPointer(a,8);
  PetscValidType(x,1);
  dummy = (void*)(*a + mstart);
  ierr  = PetscFree(dummy);CHKERRQ(ierr);
  ierr  = VecRestoreArrayRead(x,NULL);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#if defined(PETSC_USE_DEBUG)

#undef __FUNCT__
#define __FUNCT__ "VecLockGet"
/*@
   VecLockGet  - Gets the current lock status of a vector

   Logically Collective on Vec

   Input Parameter:
.  x - the vector

   Output Parameter:
.  state - greater than zero indicates the vector is still locked

   Level: beginner

   Concepts: vector^accessing local values

.seealso: VecRestoreArray(), VecGetArrayRead(), VecLockPush(), VecLockGet()
@*/
PetscErrorCode VecLockGet(Vec x,PetscInt *state)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(x,VEC_CLASSID,1);
  *state = x->lock;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecLockPush"
/*@
   VecLockPush  - Lock a vector from writing

   Logically Collective on Vec

   Input Parameter:
.  x - the vector

   Notes: If this is set then calls to VecGetArray() or VecSetValues() or any other routines that change the vectors values will fail.

    Call VecLockPop() to remove the latest lock

   Level: beginner

   Concepts: vector^accessing local values

.seealso: VecRestoreArray(), VecGetArrayRead(), VecLockPop(), VecLockGet()
@*/
PetscErrorCode VecLockPush(Vec x)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(x,VEC_CLASSID,1);
  x->lock++;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecLockPop"
/*@
   VecLockPop  - Unlock a vector from writing

   Logically Collective on Vec

   Input Parameter:
.  x - the vector

   Level: beginner

   Concepts: vector^accessing local values

.seealso: VecRestoreArray(), VecGetArrayRead(), VecLockPush(), VecLockGet()
@*/
PetscErrorCode VecLockPop(Vec x)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(x,VEC_CLASSID,1);
  x->lock--;
  if (x->lock < 0) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONGSTATE,"Vector has been unlocked too many times");
  PetscFunctionReturn(0);
}

#endif
