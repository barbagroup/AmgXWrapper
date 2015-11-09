#include <petsc/private/vecimpl.h>    /*I   "petscvec.h"  I*/

#undef __FUNCT__
#define __FUNCT__ "VecWhichEqual"
/*@
  VecWhichEqual - Creates an index set containing the indices
             where the vectors Vec1 and Vec2 have identical elements.

  Collective on Vec

  Input Parameters:
. Vec1, Vec2 - the two vectors to compare

  OutputParameter:
. S - The index set containing the indices i where vec1[i] == vec2[i]

  Level: advanced
@*/
PetscErrorCode VecWhichEqual(Vec Vec1, Vec Vec2, IS * S)
{
  PetscErrorCode  ierr;
  PetscInt        i,n_same = 0;
  PetscInt        n,low,high,low2,high2;
  PetscInt        *same = NULL;
  PetscScalar     *v1,*v2;
  MPI_Comm        comm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(Vec1,VEC_CLASSID,1);
  PetscValidHeaderSpecific(Vec2,VEC_CLASSID,2);
  PetscCheckSameComm(Vec1,1,Vec2,2);

  ierr = VecGetOwnershipRange(Vec1, &low, &high);CHKERRQ(ierr);
  ierr = VecGetOwnershipRange(Vec2, &low2, &high2);CHKERRQ(ierr);
  if ( low != low2 || high != high2 ) SETERRQ(PETSC_COMM_SELF,1,"Vectors must have identical layout");

  ierr = VecGetLocalSize(Vec1,&n);CHKERRQ(ierr);
  if (n>0){
    if (Vec1 == Vec2){
      ierr = VecGetArray(Vec1,&v1);CHKERRQ(ierr);
      v2=v1;
    } else {
      ierr = VecGetArray(Vec1,&v1);CHKERRQ(ierr);
      ierr = VecGetArray(Vec2,&v2);CHKERRQ(ierr);
    }

    ierr = PetscMalloc1( n,&same );CHKERRQ(ierr);

    for (i=0; i<n; i++){
      if (v1[i] == v2[i]) {same[n_same]=low+i; n_same++;}
    }

    if (Vec1 == Vec2){
      ierr = VecRestoreArray(Vec1,&v1);CHKERRQ(ierr);
    } else {
      ierr = VecRestoreArray(Vec1,&v1);CHKERRQ(ierr);
      ierr = VecRestoreArray(Vec2,&v2);CHKERRQ(ierr);
    }
  }
  ierr = PetscObjectGetComm((PetscObject)Vec1,&comm);CHKERRQ(ierr);
  ierr = ISCreateGeneral(comm,n_same,same,PETSC_OWN_POINTER,S);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecWhichLessThan"
/*@
  VecWhichLessThan - Creates an index set containing the indices
  where the vectors Vec1 < Vec2

  Collective on S

  Input Parameters:
. Vec1, Vec2 - the two vectors to compare

  OutputParameter:
. S - The index set containing the indices i where vec1[i] < vec2[i]

  Level: advanced
@*/
PetscErrorCode VecWhichLessThan(Vec Vec1, Vec Vec2, IS * S)
{
  PetscErrorCode ierr;
  PetscInt       i;
  PetscInt       n,low,high,low2,high2,n_lt=0;
  PetscInt       *lt = NULL;
  PetscScalar    *v1,*v2;
  MPI_Comm       comm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(Vec1,VEC_CLASSID,1);
  PetscValidHeaderSpecific(Vec2,VEC_CLASSID,2);
  PetscCheckSameComm(Vec1,1,Vec2,2);

  ierr = VecGetOwnershipRange(Vec1, &low, &high);CHKERRQ(ierr);
  ierr = VecGetOwnershipRange(Vec2, &low2, &high2);CHKERRQ(ierr);
  if ( low != low2 || high != high2 ) SETERRQ(PETSC_COMM_SELF,1,"Vectors must haveidentical layout");

  ierr = VecGetLocalSize(Vec1,&n);CHKERRQ(ierr);
  if (n>0){
    if (Vec1 == Vec2){
      ierr = VecGetArray(Vec1,&v1);CHKERRQ(ierr);
      v2=v1;
    } else {
      ierr = VecGetArray(Vec1,&v1);CHKERRQ(ierr);
      ierr = VecGetArray(Vec2,&v2);CHKERRQ(ierr);
    }
    ierr = PetscMalloc1(n,&lt );CHKERRQ(ierr);

    for (i=0; i<n; i++){
      if (PetscRealPart(v1[i]) < PetscRealPart(v2[i])) {lt[n_lt]=low+i; n_lt++;}
    }

    if (Vec1 == Vec2){
      ierr = VecRestoreArray(Vec1,&v1);CHKERRQ(ierr);
    } else {
      ierr = VecRestoreArray(Vec1,&v1);CHKERRQ(ierr);
      ierr = VecRestoreArray(Vec2,&v2);CHKERRQ(ierr);
    }
  }
  ierr = PetscObjectGetComm((PetscObject)Vec1,&comm);CHKERRQ(ierr);
  ierr = ISCreateGeneral(comm,n_lt,lt,PETSC_OWN_POINTER,S);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecWhichGreaterThan"
/*@
  VecWhichGreaterThan - Creates an index set containing the indices
  where the vectors Vec1 > Vec2

  Collective on S

  Input Parameters:
. Vec1, Vec2 - the two vectors to compare

  OutputParameter:
. S - The index set containing the indices i where vec1[i] > vec2[i]

  Level: advanced
@*/
PetscErrorCode VecWhichGreaterThan(Vec Vec1, Vec Vec2, IS * S)
{
  PetscErrorCode ierr;
  PetscInt       n,low,high,low2,high2,n_gt=0,i;
  PetscInt       *gt=NULL;
  PetscScalar    *v1,*v2;
  MPI_Comm       comm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(Vec1,VEC_CLASSID,1);
  PetscValidHeaderSpecific(Vec2,VEC_CLASSID,2);
  PetscCheckSameComm(Vec1,1,Vec2,2);

  ierr = VecGetOwnershipRange(Vec1, &low, &high);CHKERRQ(ierr);
  ierr = VecGetOwnershipRange(Vec2, &low2, &high2);CHKERRQ(ierr);
  if ( low != low2 || high != high2 ) SETERRQ(PETSC_COMM_SELF,1,"Vectors must be have identical layout");

  ierr = VecGetLocalSize(Vec1,&n);CHKERRQ(ierr);

  if (n>0){

    if (Vec1 == Vec2){
      ierr = VecGetArray(Vec1,&v1);CHKERRQ(ierr);
      v2=v1;
    } else {
      ierr = VecGetArray(Vec1,&v1);CHKERRQ(ierr);
      ierr = VecGetArray(Vec2,&v2);CHKERRQ(ierr);
    }

    ierr = PetscMalloc1(n, &gt );CHKERRQ(ierr);

    for (i=0; i<n; i++){
      if (PetscRealPart(v1[i]) > PetscRealPart(v2[i])) {gt[n_gt]=low+i; n_gt++;}
    }

    if (Vec1 == Vec2){
      ierr = VecRestoreArray(Vec1,&v1);CHKERRQ(ierr);
    } else {
      ierr = VecRestoreArray(Vec1,&v1);CHKERRQ(ierr);
      ierr = VecRestoreArray(Vec2,&v2);CHKERRQ(ierr);
    }
  }
  ierr = PetscObjectGetComm((PetscObject)Vec1,&comm);CHKERRQ(ierr);
  ierr = ISCreateGeneral(comm,n_gt,gt,PETSC_OWN_POINTER,S);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecWhichBetween"
/*@
  VecWhichBetween - Creates an index set containing the indices
               where  VecLow < V < VecHigh

  Collective on S

  Input Parameters:
+ VecLow - lower bound
. V - Vector to compare
- VecHigh - higher bound

  OutputParameter:
. S - The index set containing the indices i where veclow[i] < v[i] < vechigh[i]

  Level: advanced
@*/
PetscErrorCode VecWhichBetween(Vec VecLow, Vec V, Vec VecHigh, IS *S)
{

  PetscErrorCode ierr;
  PetscInt       n,low,high,low2,high2,low3,high3,n_vm=0;
  PetscInt       *vm,i;
  PetscScalar    *v1,*v2,*vmiddle;
  MPI_Comm       comm;

  PetscValidHeaderSpecific(V,VEC_CLASSID,2);
  PetscCheckSameComm(V,2,VecLow,1); PetscCheckSameComm(V,2,VecHigh,3);

  ierr = VecGetOwnershipRange(VecLow, &low, &high);CHKERRQ(ierr);
  ierr = VecGetOwnershipRange(VecHigh, &low2, &high2);CHKERRQ(ierr);
  ierr = VecGetOwnershipRange(V, &low3, &high3);CHKERRQ(ierr);
  if ( low!=low2 || high!=high2 || low!=low3 || high!=high3) SETERRQ(PETSC_COMM_SELF,1,"Vectors must have identical layout");

  ierr = VecGetLocalSize(VecLow,&n);CHKERRQ(ierr);
  if (n>0){
    ierr = VecGetArray(VecLow,&v1);CHKERRQ(ierr);
    if (VecLow != VecHigh){
      ierr = VecGetArray(VecHigh,&v2);CHKERRQ(ierr);
    } else {
      v2=v1;
    }
    if ( V != VecLow && V != VecHigh){
      ierr = VecGetArray(V,&vmiddle);CHKERRQ(ierr);
    } else if ( V==VecLow ){
      vmiddle=v1;
    } else {
      vmiddle =v2;
    }

    ierr = PetscMalloc1(n, &vm );CHKERRQ(ierr);

    for (i=0; i<n; i++){
      if (PetscRealPart(v1[i]) < PetscRealPart(vmiddle[i]) && PetscRealPart(vmiddle[i]) < PetscRealPart(v2[i])) {vm[n_vm]=low+i; n_vm++;}
    }

    ierr = VecRestoreArray(VecLow,&v1);CHKERRQ(ierr);
    if (VecLow != VecHigh){
      ierr = VecRestoreArray(VecHigh,&v2);CHKERRQ(ierr);
    }
    if ( V != VecLow && V != VecHigh){
      ierr = VecRestoreArray(V,&vmiddle);CHKERRQ(ierr);
    }
  }
  ierr = PetscObjectGetComm((PetscObject)V,&comm);CHKERRQ(ierr);
  ierr = ISCreateGeneral(comm,n_vm,vm,PETSC_OWN_POINTER,S);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "VecWhichBetweenOrEqual"
/*@
  VecWhichBetweenOrEqual - Creates an index set containing the indices
  where  VecLow <= V <= VecHigh

  Collective on S

  Input Parameters:
+ VecLow - lower bound
. V - Vector to compare
- VecHigh - higher bound

  OutputParameter:
. S - The index set containing the indices i where veclow[i] <= v[i] <= vechigh[i]

  Level: advanced
@*/

PetscErrorCode VecWhichBetweenOrEqual(Vec VecLow, Vec V, Vec VecHigh, IS * S)
{
  PetscErrorCode ierr;
  PetscInt       n,low,high,low2,high2,low3,high3,n_vm=0,i;
  PetscInt       *vm = NULL;
  PetscScalar    *v1,*v2,*vmiddle;
  MPI_Comm       comm;

  PetscValidHeaderSpecific(V,VEC_CLASSID,2);
  PetscCheckSameComm(V,2,VecLow,1); PetscCheckSameComm(V,2,VecHigh,3);

  ierr = VecGetOwnershipRange(VecLow, &low, &high);CHKERRQ(ierr);
  ierr = VecGetOwnershipRange(VecHigh, &low2, &high2);CHKERRQ(ierr);
  ierr = VecGetOwnershipRange(V, &low3, &high3);CHKERRQ(ierr);
  if ( low!=low2 || high!=high2 || low!=low3 || high!=high3 ) SETERRQ(PETSC_COMM_SELF,1,"Vectors must have identical layout");

  ierr = VecGetLocalSize(VecLow,&n);CHKERRQ(ierr);

  if (n>0){
    ierr = VecGetArray(VecLow,&v1);CHKERRQ(ierr);
    if (VecLow != VecHigh){
      ierr = VecGetArray(VecHigh,&v2);CHKERRQ(ierr);
    } else {
      v2=v1;
    }
    if ( V != VecLow && V != VecHigh){
      ierr = VecGetArray(V,&vmiddle);CHKERRQ(ierr);
    } else if ( V==VecLow ){
      vmiddle=v1;
    } else {
      vmiddle =v2;
    }

    ierr = PetscMalloc1(n, &vm );CHKERRQ(ierr);

    for (i=0; i<n; i++){
      if (PetscRealPart(v1[i]) <= PetscRealPart(vmiddle[i]) && PetscRealPart(vmiddle[i]) <= PetscRealPart(v2[i])) {vm[n_vm]=low+i; n_vm++;}
    }

    ierr = VecRestoreArray(VecLow,&v1);CHKERRQ(ierr);
    if (VecLow != VecHigh){
      ierr = VecRestoreArray(VecHigh,&v2);CHKERRQ(ierr);
    }
    if ( V != VecLow && V != VecHigh){
      ierr = VecRestoreArray(V,&vmiddle);CHKERRQ(ierr);
    }
  }
  ierr = PetscObjectGetComm((PetscObject)V,&comm);CHKERRQ(ierr);
  ierr = ISCreateGeneral(comm,n_vm,vm,PETSC_OWN_POINTER,S);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecISAXPY"
/*@
  VecISAXPY - Adds a reduced vector to the appropriate elements of a full-space vector. 
                  vfull[is[i]] += alpha*vreduced[i]

  Input Parameters:
+ vfull - the full-space vector
. vreduced - the reduced-space vector
- is - the index set for the reduced space

  Output Parameters:
. vfull - the sum of the full-space vector and reduced-space vector

  Level: advanced

.seealso:  VecAXPY()
@*/
PetscErrorCode VecISAXPY(Vec vfull, IS is, PetscScalar alpha,Vec vreduced)
{
  PetscInt       nfull,nreduced;
  MPI_Comm       comm;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(vfull,VEC_CLASSID,1);
  PetscValidHeaderSpecific(vreduced,VEC_CLASSID,2);
  PetscValidHeaderSpecific(is,IS_CLASSID,3);
  ierr = VecGetSize(vfull,&nfull);CHKERRQ(ierr);
  ierr = VecGetSize(vreduced,&nreduced);CHKERRQ(ierr);

  if (nfull == nreduced) { /* Also takes care of masked vectors */
    ierr = VecAXPY(vfull,alpha,vreduced);CHKERRQ(ierr);
  } else {
    PetscScalar      *y;
    const PetscScalar *x;
    PetscInt          i,n,m,rstart;
    const PetscInt    *id;

    ierr = PetscObjectGetComm((PetscObject)vfull,&comm);CHKERRQ(ierr);
    ierr = VecGetArray(vfull,&y);CHKERRQ(ierr);
    ierr = VecGetArrayRead(vreduced,&x);CHKERRQ(ierr);
    ierr = ISGetIndices(is,&id);CHKERRQ(ierr);
    ierr = ISGetLocalSize(is,&n);CHKERRQ(ierr);
    ierr = VecGetLocalSize(vreduced,&m);CHKERRQ(ierr);
    if (m != n) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SUP,"IS local length not equal to Vec local length");
    ierr = VecGetOwnershipRange(vfull,&rstart,NULL);CHKERRQ(ierr);
    y -= rstart;
    if (alpha == 1.0) {
      for (i=0; i<n; i++) {
        y[id[i]] += x[i];
      }
    } else {
      for (i=0; i<n; i++) {
        y[id[i]] += alpha*x[i];
      }
    }
    y += rstart;
    ierr = ISRestoreIndices(is,&id);CHKERRQ(ierr);
    ierr = VecRestoreArray(vfull,&y);CHKERRQ(ierr);
    ierr = VecRestoreArrayRead(vreduced,&x);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "ISComplementVec"
/*@
   ISComplementVec - Creates the complement of the index set relative to a layout defined by a Vec

   Collective on IS

   Input Parameter:
+  S -  a PETSc IS
-  V - the reference vector space

   Output Parameter:
.  T -  the complement of S

.seealso ISCreateGeneral()

   Level: advanced
@*/
PetscErrorCode ISComplementVec(IS S, Vec V, IS *T)
{
  PetscErrorCode ierr;
  PetscInt       start, end;

  PetscFunctionBegin;
  ierr = VecGetOwnershipRange(V,&start,&end);CHKERRQ(ierr);
  ierr = ISComplement(S,start,end,T);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecISSet"
/*@
   VecISSet - Sets the elements of a vector, specified by an index set, to a constant

   Input Parameter:
+  V - the vector
.  S -  the locations in the vector
-  c - the constant

.seealso VecSet()

   Level: advanced
@*/
PetscErrorCode VecISSet(Vec V,IS S, PetscScalar c)
{
  PetscErrorCode ierr;
  PetscInt       nloc,low,high,i;
  const PetscInt *s;
  PetscScalar    *v;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(V,VEC_CLASSID,1);
  PetscValidHeaderSpecific(S,IS_CLASSID,2);
  PetscValidType(V,3);
  PetscCheckSameComm(V,3,S,1);

  ierr = VecGetOwnershipRange(V, &low, &high);CHKERRQ(ierr);
  ierr = ISGetLocalSize(S,&nloc);CHKERRQ(ierr);
  ierr = ISGetIndices(S, &s);CHKERRQ(ierr);
  ierr = VecGetArray(V,&v);CHKERRQ(ierr);
  for (i=0; i<nloc; i++){
    v[s[i]-low] = c;
  }
  ierr = ISRestoreIndices(S, &s);CHKERRQ(ierr);
  ierr = VecRestoreArray(V,&v);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#if !defined(PETSC_USE_COMPLEX)
#undef __FUNCT__
#define __FUNCT__ "VecBoundGradientProjection"
/*@C
  VecBoundGradientProjection - Projects  vector according to this definition.
  If XL[i] < X[i] < XU[i], then GP[i] = G[i];
  If X[i]<=XL[i], then GP[i] = min(G[i],0);
  If X[i]>=XU[i], then GP[i] = max(G[i],0);

  Input Parameters:
+ G - current gradient vector
. X - current solution vector
. XL - lower bounds
- XU - upper bounds

  Output Parameter:
. GP - gradient projection vector

  Level: advanced
C@*/
PetscErrorCode VecBoundGradientProjection(Vec G, Vec X, Vec XL, Vec XU, Vec GP)
{

  PetscErrorCode ierr;
  PetscInt       n,i;
  PetscReal      *xptr,*xlptr,*xuptr,*gptr,*gpptr;
  PetscReal      xval,gpval;

  /* Project variables at the lower and upper bound */
  PetscFunctionBegin;
  PetscValidHeaderSpecific(G,VEC_CLASSID,1);
  PetscValidHeaderSpecific(X,VEC_CLASSID,2);
  PetscValidHeaderSpecific(XL,VEC_CLASSID,3);
  PetscValidHeaderSpecific(XU,VEC_CLASSID,4);
  PetscValidHeaderSpecific(GP,VEC_CLASSID,5);

  ierr = VecGetLocalSize(X,&n);CHKERRQ(ierr);

  ierr=VecGetArray(X,&xptr);CHKERRQ(ierr);
  ierr=VecGetArray(XL,&xlptr);CHKERRQ(ierr);
  ierr=VecGetArray(XU,&xuptr);CHKERRQ(ierr);
  ierr=VecGetArray(G,&gptr);CHKERRQ(ierr);
  if (G!=GP){
    ierr=VecGetArray(GP,&gpptr);CHKERRQ(ierr);
  } else { gpptr=gptr; }

  for (i=0; i<n; ++i){
    gpval = gptr[i]; xval = xptr[i];

    if (gpval>0 && xval<=xlptr[i]){
      gpval = 0;
    } else if (gpval<0 && xval>=xuptr[i]){
      gpval = 0;
    }
    gpptr[i] = gpval;
  }

  ierr=VecRestoreArray(X,&xptr);CHKERRQ(ierr);
  ierr=VecRestoreArray(XL,&xlptr);CHKERRQ(ierr);
  ierr=VecRestoreArray(XU,&xuptr);CHKERRQ(ierr);
  ierr=VecRestoreArray(G,&gptr);CHKERRQ(ierr);
  if (G!=GP){
    ierr=VecRestoreArray(GP,&gpptr);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}
#endif

#undef __FUNCT__
#define __FUNCT__ "VecStepMaxBounded"
/*@
     VecStepMaxBounded - See below

     Collective on Vec

     Input Parameters:
+      X  - vector with no negative entries
.      XL - lower bounds
.      XU - upper bounds
-      DX  - step direction, can have negative, positive or zero entries

     Output Parameter:
.     stepmax -   minimum value so that X[i] + stepmax*DX[i] <= XL[i]  or  XU[i] <= X[i] + stepmax*DX[i]

@*/
PetscErrorCode VecStepMaxBounded(Vec X, Vec DX, Vec XL, Vec XU, PetscReal *stepmax)
{
  PetscErrorCode ierr;
  PetscInt       i,nn;
  PetscScalar    *xx,*dx,*xl,*xu;
  PetscReal      localmax=0;
  MPI_Comm       comm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(X,VEC_CLASSID,2);
  PetscValidHeaderSpecific(DX,VEC_CLASSID,5);
  PetscValidHeaderSpecific(XL,VEC_CLASSID,3);
  PetscValidHeaderSpecific(XU,VEC_CLASSID,4);

  ierr = VecGetArray(X,&xx);CHKERRQ(ierr);
  ierr = VecGetArray(XL,&xl);CHKERRQ(ierr);
  ierr = VecGetArray(XU,&xu);CHKERRQ(ierr);
  ierr = VecGetArray(DX,&dx);CHKERRQ(ierr);
  ierr = VecGetLocalSize(X,&nn);CHKERRQ(ierr);
  for (i=0;i<nn;i++){
    if (PetscRealPart(dx[i]) > 0){
      localmax=PetscMax(localmax,PetscRealPart((xu[i]-xx[i])/dx[i]));
    } else if (PetscRealPart(dx[i])<0){
      localmax=PetscMax(localmax,PetscRealPart((xl[i]-xx[i])/dx[i]));
    }
  }
  ierr = VecRestoreArray(X,&xx);CHKERRQ(ierr);
  ierr = VecRestoreArray(XL,&xl);CHKERRQ(ierr);
  ierr = VecRestoreArray(XU,&xu);CHKERRQ(ierr);
  ierr = VecRestoreArray(DX,&dx);CHKERRQ(ierr);
  ierr = PetscObjectGetComm((PetscObject)X,&comm);CHKERRQ(ierr);
  ierr = MPIU_Allreduce(&localmax,stepmax,1,MPIU_REAL,MPIU_MAX,comm);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecStepBoundInfo"
/*@
     VecStepBoundInfo - See below

     Collective on Vec

     Input Parameters:
+      X  - vector with no negative entries
.      XL - lower bounds
.      XU - upper bounds
-      DX  - step direction, can have negative, positive or zero entries

     Output Parameter:
+     boundmin -  maximum value so that   XL[i] <= X[i] + boundmax*DX[i] <= XU[i]
.     wolfemin -
-     boundmax -   minimum value so that X[i] + boundmax*DX[i] <= XL[i]  or  XU[i] <= X[i] + boundmax*DX[i]

  Level: advanced
@*/
PetscErrorCode VecStepBoundInfo(Vec X, Vec DX, Vec XL, Vec XU, PetscReal *boundmin, PetscReal *wolfemin, PetscReal *boundmax)
{
  PetscErrorCode ierr;
  PetscInt       n,i;
  PetscScalar    *x,*xl,*xu,*dx;
  PetscReal      t;
  PetscReal      localmin=PETSC_INFINITY,localwolfemin=PETSC_INFINITY,localmax=-1;
  MPI_Comm       comm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(X,VEC_CLASSID,1);
  PetscValidHeaderSpecific(XL,VEC_CLASSID,2);
  PetscValidHeaderSpecific(XU,VEC_CLASSID,3);
  PetscValidHeaderSpecific(DX,VEC_CLASSID,4);

  ierr=VecGetArray(X,&x);CHKERRQ(ierr);
  ierr=VecGetArray(XL,&xl);CHKERRQ(ierr);
  ierr=VecGetArray(XU,&xu);CHKERRQ(ierr);
  ierr=VecGetArray(DX,&dx);CHKERRQ(ierr);
  ierr = VecGetLocalSize(X,&n);CHKERRQ(ierr);
  for (i=0;i<n;i++){
    if (PetscRealPart(dx[i])>0 && PetscRealPart(xu[i]) < PETSC_INFINITY) {
      t=PetscRealPart((xu[i]-x[i])/dx[i]);
      localmin=PetscMin(t,localmin);
      if (localmin>0){
        localwolfemin = PetscMin(t,localwolfemin);
      }
      localmax = PetscMax(t,localmax);
    } else if (PetscRealPart(dx[i])<0 && PetscRealPart(xl[i]) > PETSC_NINFINITY) {
      t=PetscRealPart((xl[i]-x[i])/dx[i]);
      localmin = PetscMin(t,localmin);
      if (localmin>0){
        localwolfemin = PetscMin(t,localwolfemin);
      }
      localmax = PetscMax(t,localmax);
    }
  }

  ierr=VecRestoreArray(X,&x);CHKERRQ(ierr);
  ierr=VecRestoreArray(XL,&xl);CHKERRQ(ierr);
  ierr=VecRestoreArray(XU,&xu);CHKERRQ(ierr);
  ierr=VecRestoreArray(DX,&dx);CHKERRQ(ierr);
  ierr=PetscObjectGetComm((PetscObject)X,&comm);CHKERRQ(ierr);

  if (boundmin){
    ierr = MPIU_Allreduce(&localmin,boundmin,1,MPIU_REAL,MPIU_MIN,comm);CHKERRQ(ierr);
    ierr = PetscInfo1(X,"Step Bound Info: Closest Bound: %g \n",(double)*boundmin);CHKERRQ(ierr);
  }
  if (wolfemin){
    ierr = MPIU_Allreduce(&localwolfemin,wolfemin,1,MPIU_REAL,MPIU_MIN,comm);CHKERRQ(ierr);
    ierr = PetscInfo1(X,"Step Bound Info: Wolfe: %g \n",(double)*wolfemin);CHKERRQ(ierr);
  }
  if (boundmax) {
    ierr = MPIU_Allreduce(&localmax,boundmax,1,MPIU_REAL,MPIU_MAX,comm);CHKERRQ(ierr);
    if (*boundmax < 0) *boundmax=PETSC_INFINITY;
    ierr = PetscInfo1(X,"Step Bound Info: Max: %g \n",(double)*boundmax);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecStepMax"
/*@
     VecStepMax - Returns the largest value so that x[i] + step*DX[i] >= 0 for all i

     Collective on Vec

     Input Parameters:
+      X  - vector with no negative entries
-      DX  - a step direction, can have negative, positive or zero entries

     Output Parameter:
.    step - largest value such that x[i] + step*DX[i] >= 0 for all i

  Level: advanced
 @*/
PetscErrorCode VecStepMax(Vec X, Vec DX, PetscReal *step)
{
  PetscErrorCode ierr;
  PetscInt       i, nn;
  PetscReal      stepmax=PETSC_INFINITY;
  PetscScalar    *xx, *dx;
  MPI_Comm       comm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(X,VEC_CLASSID,1);
  PetscValidHeaderSpecific(DX,VEC_CLASSID,2);

  ierr = VecGetLocalSize(X,&nn);CHKERRQ(ierr);
  ierr = VecGetArray(X,&xx);CHKERRQ(ierr);
  ierr = VecGetArray(DX,&dx);CHKERRQ(ierr);
  for (i=0;i<nn;i++){
    if (PetscRealPart(xx[i]) < 0) SETERRQ(PETSC_COMM_SELF,1,"Vector must be positive");
    else if (PetscRealPart(dx[i])<0) stepmax=PetscMin(stepmax,PetscRealPart(-xx[i]/dx[i]));
  }
  ierr = VecRestoreArray(X,&xx);CHKERRQ(ierr);
  ierr = VecRestoreArray(DX,&dx);CHKERRQ(ierr);
  ierr = PetscObjectGetComm((PetscObject)X,&comm);CHKERRQ(ierr);
  ierr = MPIU_Allreduce(&stepmax,step,1,MPIU_REAL,MPIU_MIN,comm);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecPow"
/*@
  VecPow - Replaces each component of a vector by x_i^p

  Logically Collective on v

  Input Parameter:
+ v - the vector
- p - the exponent to use on each element

  Output Parameter:
. v - the vector

  Level: intermediate

@*/
PetscErrorCode VecPow(Vec v, PetscScalar p)
{
  PetscErrorCode ierr;
  PetscInt       n,i;
  PetscScalar    *v1;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(v, VEC_CLASSID, 1);

  ierr = VecGetArray(v, &v1);CHKERRQ(ierr);
  ierr = VecGetLocalSize(v, &n);CHKERRQ(ierr);

  if (1.0 == p) {
  } else if (-1.0 == p) {
    for (i = 0; i < n; ++i){
      v1[i] = 1.0 / v1[i];
    }
  } else if (0.0 == p) {
    for (i = 0; i < n; ++i){
      /*  Not-a-number left alone
          Infinity set to one  */
      if (v1[i] == v1[i]) {
        v1[i] = 1.0;
      }
    }
  } else if (0.5 == p) {
    for (i = 0; i < n; ++i) {
      if (PetscRealPart(v1[i]) >= 0) {
        v1[i] = PetscSqrtScalar(v1[i]);
      } else {
        v1[i] = PETSC_INFINITY;
      }
    }
  } else if (-0.5 == p) {
    for (i = 0; i < n; ++i) {
      if (PetscRealPart(v1[i]) >= 0) {
        v1[i] = 1.0 / PetscSqrtScalar(v1[i]);
      } else {
        v1[i] = PETSC_INFINITY;
      }
    }
  } else if (2.0 == p) {
    for (i = 0; i < n; ++i){
      v1[i] *= v1[i];
    }
  } else if (-2.0 == p) {
    for (i = 0; i < n; ++i){
      v1[i] = 1.0 / (v1[i] * v1[i]);
    }
  } else {
    for (i = 0; i < n; ++i) {
      if (PetscRealPart(v1[i]) >= 0) {
        v1[i] = PetscPowScalar(v1[i], p);
      } else {
        v1[i] = PETSC_INFINITY;
      }
    }
  }
  ierr = VecRestoreArray(v,&v1);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecMedian"
/*@
  VecMedian - Computes the componentwise median of three vectors
  and stores the result in this vector.  Used primarily for projecting
  a vector within upper and lower bounds.

  Logically Collective

  Input Parameters:
. Vec1, Vec2, Vec3 - The three vectors

  Output Parameter:
. VMedian - The median vector

  Level: advanced
@*/
PetscErrorCode VecMedian(Vec Vec1, Vec Vec2, Vec Vec3, Vec VMedian)
{
  PetscErrorCode ierr;
  PetscInt       i,n,low1,low2,low3,low4,high1,high2,high3,high4;
  PetscScalar    *v1,*v2,*v3,*vmed;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(Vec1,VEC_CLASSID,1);
  PetscValidHeaderSpecific(Vec2,VEC_CLASSID,2);
  PetscValidHeaderSpecific(Vec3,VEC_CLASSID,3);
  PetscValidHeaderSpecific(VMedian,VEC_CLASSID,4);

  if (Vec1==Vec2 || Vec1==Vec3){
    ierr=VecCopy(Vec1,VMedian);CHKERRQ(ierr);
    PetscFunctionReturn(0);
  }
  if (Vec2==Vec3){
    ierr=VecCopy(Vec2,VMedian);CHKERRQ(ierr);
    PetscFunctionReturn(0);
  }

  PetscValidType(Vec1,1);
  PetscValidType(Vec2,2);
  PetscValidType(VMedian,4);
  PetscCheckSameType(Vec1,1,Vec2,2); PetscCheckSameType(Vec1,1,VMedian,4);
  PetscCheckSameComm(Vec1,1,Vec2,2); PetscCheckSameComm(Vec1,1,VMedian,4);

  ierr = VecGetOwnershipRange(Vec1, &low1, &high1);CHKERRQ(ierr);
  ierr = VecGetOwnershipRange(Vec2, &low2, &high2);CHKERRQ(ierr);
  ierr = VecGetOwnershipRange(Vec3, &low3, &high3);CHKERRQ(ierr);
  ierr = VecGetOwnershipRange(VMedian, &low4, &high4);CHKERRQ(ierr);
  if ( low1!= low2 || low1!= low3 || low1!= low4 || high1!= high2 || high1!= high3 || high1!= high4) SETERRQ(PETSC_COMM_SELF,1,"InCompatible vector local lengths");

  ierr = VecGetArray(Vec1,&v1);CHKERRQ(ierr);
  ierr = VecGetArray(Vec2,&v2);CHKERRQ(ierr);
  ierr = VecGetArray(Vec3,&v3);CHKERRQ(ierr);

  if ( VMedian != Vec1 && VMedian != Vec2 && VMedian != Vec3){
    ierr = VecGetArray(VMedian,&vmed);CHKERRQ(ierr);
  } else if ( VMedian==Vec1 ){
    vmed=v1;
  } else if ( VMedian==Vec2 ){
    vmed=v2;
  } else {
    vmed=v3;
  }

  ierr=VecGetLocalSize(Vec1,&n);CHKERRQ(ierr);

  for (i=0;i<n;i++){
    vmed[i]=PetscMax(PetscMax(PetscMin(PetscRealPart(v1[i]),PetscRealPart(v2[i])),PetscMin(PetscRealPart(v1[i]),PetscRealPart(v3[i]))),PetscMin(PetscRealPart(v2[i]),PetscRealPart(v3[i])));
  }

  ierr = VecRestoreArray(Vec1,&v1);CHKERRQ(ierr);
  ierr = VecRestoreArray(Vec2,&v2);CHKERRQ(ierr);
  ierr = VecRestoreArray(Vec3,&v3);CHKERRQ(ierr);

  if (VMedian!=Vec1 && VMedian != Vec2 && VMedian != Vec3){
    ierr = VecRestoreArray(VMedian,&vmed);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}
