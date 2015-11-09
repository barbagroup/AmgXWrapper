
/*
    This file contains routines for interfacing to random number generators.
    This provides more than just an interface to some system random number
    generator:

    Numbers can be shuffled for use as random tuples

    Multiple random number generators may be used

    We are still not sure what interface we want here.  There should be
    one to reinitialize and set the seed.
 */

#include <../src/sys/classes/random/randomimpl.h>                              /*I "petscsys.h" I*/

#undef __FUNCT__
#define __FUNCT__ "PetscRandomGetValue"
/*@
   PetscRandomGetValue - Generates a random number.  Call this after first calling
   PetscRandomCreate().

   Not Collective

   Intput Parameter:
.  r  - the random number generator context

   Output Parameter:
.  val - the value

   Level: intermediate

   Notes:
   Use VecSetRandom() to set the elements of a vector to random numbers.

   When PETSc is compiled for complex numbers this returns a complex number with random real and complex parts.
   Use PetscGetValueReal() to get a random real number.

   To get a complex number with only a random real part, first call PetscRandomSetInterval() with a equal
   low and high imaginary part. Similarly to get a complex number with only a random imaginary part call
   PetscRandomSetInterval() with a equal low and high real part.

   Example of Usage:
.vb
      PetscRandomCreate(PETSC_COMM_WORLD,&r);
      PetscRandomGetValue(r,&value1);
      PetscRandomGetValue(r,&value2);
      PetscRandomGetValue(r,&value3);
      PetscRandomDestroy(&r);
.ve

   Concepts: random numbers^getting

.seealso: PetscRandomCreate(), PetscRandomDestroy(), VecSetRandom(), PetscRandomGetValueReal()
@*/
PetscErrorCode  PetscRandomGetValue(PetscRandom r,PetscScalar *val)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(r,PETSC_RANDOM_CLASSID,1);
  PetscValidIntPointer(val,2);
  PetscValidType(r,1);

  ierr = (*r->ops->getvalue)(r,val);CHKERRQ(ierr);
  ierr = PetscObjectStateIncrease((PetscObject)r);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscRandomGetValueReal"
/*@
   PetscRandomGetValueReal - Generates a purely real random number.  Call this after first calling
   PetscRandomCreate().

   Not Collective

   Intput Parameter:
.  r  - the random number generator context

   Output Parameter:
.  val - the value

   Level: intermediate

   Notes:
   Use VecSetRandom() to set the elements of a vector to random numbers.

   Example of Usage:
.vb
      PetscRandomCreate(PETSC_COMM_WORLD,&r);
      PetscRandomGetValueReal(r,&value1);
      PetscRandomGetValueReal(r,&value2);
      PetscRandomGetValueReal(r,&value3);
      PetscRandomDestroy(&r);
.ve

   Concepts: random numbers^getting

.seealso: PetscRandomCreate(), PetscRandomDestroy(), VecSetRandom(), PetscRandomGetValue()
@*/
PetscErrorCode  PetscRandomGetValueReal(PetscRandom r,PetscReal *val)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(r,PETSC_RANDOM_CLASSID,1);
  PetscValidIntPointer(val,2);
  PetscValidType(r,1);

  ierr = (*r->ops->getvaluereal)(r,val);CHKERRQ(ierr);
  ierr = PetscObjectStateIncrease((PetscObject)r);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscRandomGetInterval"
/*@
   PetscRandomGetInterval - Gets the interval over which the random numbers
   will be randomly distributed.  By default, this interval is [0,1).

   Not collective

   Input Parameters:
.  r  - the random number generator context

   Output Parameters:
+  low - The lower bound of the interval
-  high - The upper bound of the interval

   Level: intermediate

   Concepts: random numbers^range

.seealso: PetscRandomCreate(), PetscRandomSetInterval()
@*/
PetscErrorCode  PetscRandomGetInterval(PetscRandom r,PetscScalar *low,PetscScalar *high)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(r,PETSC_RANDOM_CLASSID,1);
  if (low) {
    PetscValidScalarPointer(low,2);
    *low = r->low;
  }
  if (high) {
    PetscValidScalarPointer(high,3);
    *high = r->low+r->width;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscRandomSetInterval"
/*@
   PetscRandomSetInterval - Sets the interval over which the random numbers
   will be randomly distributed.  By default, this interval is [0,1).

   Not collective

   Input Parameters:
+  r  - the random number generator context
.  low - The lower bound of the interval
-  high - The upper bound of the interval

   Level: intermediate

   Notes: for complex numbers either the real part or the imaginary part of high must be greater than its low part; or both of them can be greater.
    If the real or imaginary part of low and high are the same then that value is always returned in the real or imaginary part.

   Concepts: random numbers^range

.seealso: PetscRandomCreate(), PetscRandomGetInterval()
@*/
PetscErrorCode  PetscRandomSetInterval(PetscRandom r,PetscScalar low,PetscScalar high)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(r,PETSC_RANDOM_CLASSID,1);
#if defined(PETSC_USE_COMPLEX)
  if (PetscRealPart(low) > PetscRealPart(high))           SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"only low <= high");
  if (PetscImaginaryPart(low) > PetscImaginaryPart(high)) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"only low <= high");
#else
  if (low >= high) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"only low <= high: Instead %g %g",(double)low,(double)high);
#endif
  r->low   = low;
  r->width = high-low;
  r->iset  = PETSC_TRUE;
  PetscFunctionReturn(0);
}
