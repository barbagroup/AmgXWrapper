
/*
   This file contains routines for sorting integers and doubles with a permutation array.

   The word "register"  in this code is used to identify data that is not
   aliased.  For some compilers, this can cause the compiler to fail to
   place inner-loop variables into registers.
 */
#include <petscsys.h>                /*I  "petscsys.h"  I*/

#define SWAP(a,b,t) {t=a;a=b;b=t;}

#undef __FUNCT__
#define __FUNCT__ "PetscSortIntWithPermutation_Private"
static PetscErrorCode PetscSortIntWithPermutation_Private(const PetscInt v[],PetscInt vdx[],PetscInt right)
{
  PetscErrorCode ierr;
  PetscInt       tmp,i,vl,last;

  PetscFunctionBegin;
  if (right <= 1) {
    if (right == 1) {
      if (v[vdx[0]] > v[vdx[1]]) SWAP(vdx[0],vdx[1],tmp);
    }
    PetscFunctionReturn(0);
  }
  SWAP(vdx[0],vdx[right/2],tmp);
  vl   = v[vdx[0]];
  last = 0;
  for (i=1; i<=right; i++) {
    if (v[vdx[i]] < vl) {last++; SWAP(vdx[last],vdx[i],tmp);}
  }
  SWAP(vdx[0],vdx[last],tmp);
  ierr = PetscSortIntWithPermutation_Private(v,vdx,last-1);CHKERRQ(ierr);
  ierr = PetscSortIntWithPermutation_Private(v,vdx+last+1,right-(last+1));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscSortIntWithPermutation"
/*@
   PetscSortIntWithPermutation - Computes the permutation of values that gives
   a sorted sequence.

   Not Collective

   Input Parameters:
+  n  - number of values to sort
.  i  - values to sort
-  idx - permutation array.  Must be initialized to 0:n-1 on input.

   Level: intermediate

   Notes:
   On output i is unchanged and idx[i] is the position of the i-th smallest index in i.

   Concepts: sorting^ints with permutation

.seealso: PetscSortInt(), PetscSortRealWithPermutation(), PetscSortIntWithArray()
 @*/
PetscErrorCode  PetscSortIntWithPermutation(PetscInt n,const PetscInt i[],PetscInt idx[])
{
  PetscErrorCode ierr;
  PetscInt       j,k,tmp,ik;

  PetscFunctionBegin;
  if (n<8) {
    for (k=0; k<n; k++) {
      ik = i[idx[k]];
      for (j=k+1; j<n; j++) {
        if (ik > i[idx[j]]) {
          SWAP(idx[k],idx[j],tmp);
          ik = i[idx[k]];
        }
      }
    }
  } else {
    ierr = PetscSortIntWithPermutation_Private(i,idx,n-1);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

/* ---------------------------------------------------------------------- */

#undef __FUNCT__
#define __FUNCT__ "PetscSortRealWithPermutation_Private"
static PetscErrorCode PetscSortRealWithPermutation_Private(const PetscReal v[],PetscInt vdx[],PetscInt right)
{
  PetscReal      vl;
  PetscErrorCode ierr;
  PetscInt       tmp,i,last;

  PetscFunctionBegin;
  if (right <= 1) {
    if (right == 1) {
      if (v[vdx[0]] > v[vdx[1]]) SWAP(vdx[0],vdx[1],tmp);
    }
    PetscFunctionReturn(0);
  }
  SWAP(vdx[0],vdx[right/2],tmp);
  vl   = v[vdx[0]];
  last = 0;
  for (i=1; i<=right; i++) {
    if (v[vdx[i]] < vl) {last++; SWAP(vdx[last],vdx[i],tmp);}
  }
  SWAP(vdx[0],vdx[last],tmp);
  ierr = PetscSortRealWithPermutation_Private(v,vdx,last-1);CHKERRQ(ierr);
  ierr = PetscSortRealWithPermutation_Private(v,vdx+last+1,right-(last+1));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscSortRealWithPermutation"
/*@
   PetscSortRealWithPermutation - Computes the permutation of values that gives
   a sorted sequence.

   Not Collective

   Input Parameters:
+  n  - number of values to sort
.  i  - values to sort
-  idx - permutation array.  Must be initialized to 0:n-1 on input.

   Level: intermediate

   Notes:
   i is unchanged on output.

   Concepts: sorting^doubles with permutation

.seealso: PetscSortReal(), PetscSortIntWithPermutation()
 @*/
PetscErrorCode  PetscSortRealWithPermutation(PetscInt n,const PetscReal i[],PetscInt idx[])
{
  PetscErrorCode ierr;
  PetscInt       j,k,tmp;
  PetscReal      ik;

  PetscFunctionBegin;
  if (n<8) {
    for (k=0; k<n; k++) {
      ik = i[idx[k]];
      for (j=k+1; j<n; j++) {
        if (ik > i[idx[j]]) {
          SWAP(idx[k],idx[j],tmp);
          ik = i[idx[k]];
        }
      }
    }
  } else {
    ierr = PetscSortRealWithPermutation_Private(i,idx,n-1);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscSortStrWithPermutation_Private"
static PetscErrorCode PetscSortStrWithPermutation_Private(const char* v[],PetscInt vdx[],PetscInt right)
{
  PetscErrorCode ierr;
  PetscInt       tmp,i,last;
  PetscBool      gt;
  const char     *vl;

  PetscFunctionBegin;
  if (right <= 1) {
    if (right == 1) {
      ierr = PetscStrgrt(v[vdx[0]],v[vdx[1]],&gt);CHKERRQ(ierr);
      if (gt) SWAP(vdx[0],vdx[1],tmp);
    }
    PetscFunctionReturn(0);
  }
  SWAP(vdx[0],vdx[right/2],tmp);
  vl   = v[vdx[0]];
  last = 0;
  for (i=1; i<=right; i++) {
    ierr = PetscStrgrt(vl,v[vdx[i]],&gt);CHKERRQ(ierr);
    if (gt) {last++; SWAP(vdx[last],vdx[i],tmp);}
  }
  SWAP(vdx[0],vdx[last],tmp);
  ierr = PetscSortStrWithPermutation_Private(v,vdx,last-1);CHKERRQ(ierr);
  ierr = PetscSortStrWithPermutation_Private(v,vdx+last+1,right-(last+1));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscSortStrWithPermutation"
/*@C
   PetscSortStrWithPermutation - Computes the permutation of values that gives
   a sorted sequence.

   Not Collective

   Input Parameters:
+  n  - number of values to sort
.  i  - values to sort
-  idx - permutation array.  Must be initialized to 0:n-1 on input.

   Level: intermediate

   Notes:
   i is unchanged on output.

   Concepts: sorting^ints with permutation

.seealso: PetscSortInt(), PetscSortRealWithPermutation()
 @*/
PetscErrorCode  PetscSortStrWithPermutation(PetscInt n,const char* i[],PetscInt idx[])
{
  PetscErrorCode ierr;
  PetscInt       j,k,tmp;
  const char     *ik;
  PetscBool      gt;

  PetscFunctionBegin;
  if (n<8) {
    for (k=0; k<n; k++) {
      ik = i[idx[k]];
      for (j=k+1; j<n; j++) {
        ierr = PetscStrgrt(ik,i[idx[j]],&gt);CHKERRQ(ierr);
        if (gt) {
          SWAP(idx[k],idx[j],tmp);
          ik = i[idx[k]];
        }
      }
    }
  } else {
    ierr = PetscSortStrWithPermutation_Private(i,idx,n-1);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}
