
#include <petsc/private/petscimpl.h>
#include <petscdm.h>     /*I "petscdm.h" I*/
#include <petscdmplex.h> /*I "petscdmplex.h" I*/
#include <petscksp.h>    /*I "petscksp.h" I*/

typedef struct _projectConstraintsCtx
{
  DM  dm;
  Vec mask;
}
projectConstraintsCtx;

#undef __FUNCT__
#define __FUNCT__ "MatMult_GlobalToLocalNormal"
PetscErrorCode MatMult_GlobalToLocalNormal(Mat CtC, Vec x, Vec y)
{
  DM                    dm;
  Vec                   local, mask;
  projectConstraintsCtx *ctx;
  PetscErrorCode        ierr;

  PetscFunctionBegin;
  ierr = MatShellGetContext(CtC,&ctx);CHKERRQ(ierr);
  dm   = ctx->dm;
  mask = ctx->mask;
  ierr = DMGetLocalVector(dm,&local);CHKERRQ(ierr);
  ierr = DMGlobalToLocalBegin(dm,x,INSERT_VALUES,local);CHKERRQ(ierr);
  ierr = DMGlobalToLocalEnd(dm,x,INSERT_VALUES,local);CHKERRQ(ierr);
  if (mask) {ierr = VecPointwiseMult(local,mask,local);CHKERRQ(ierr);}
  ierr = VecSet(y,0.);CHKERRQ(ierr);
  ierr = DMLocalToGlobalBegin(dm,local,ADD_VALUES,y);CHKERRQ(ierr);
  ierr = DMLocalToGlobalEnd(dm,local,ADD_VALUES,y);CHKERRQ(ierr);
  ierr = DMRestoreLocalVector(dm,&local);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMGlobalToLocalSolve"
/*@
  DMGlobalToLocalSolve - Solve for the global vector that is mapped to a given local vector by DMGlobalToLocalBegin()/DMGlobalToLocalEnd() with mode
  = INSERT_VALUES.  It is assumed that the sum of all the local vector sizes is greater than or equal to the global vector size, so the solution is
  a least-squares solution.  It is also assumed that DMLocalToGlobalBegin()/DMLocalToGlobalEnd() with mode = ADD_VALUES is the adjoint of the
  global-to-local map, so that the least-squares solution may be found by the normal equations.

  collective

  Input Parameters:
+ dm - The DM object
. x - The local vector
- y - The global vector: the input value of globalVec is used as an initial guess

  Output Parameters:
. y - The least-squares solution

  Level: advanced

  Note: If the DM is of type DMPLEX, then y is the solution of L' * D * L * y = L' * D * x, where D is a diagonal mask that is 1 for every point in
  the union of the closures of the local cells and 0 otherwise.  This difference is only relevant if there are anchor points that are not in the
  closure of any local cell (see DMPlexGetAnchors()/DMPlexSetAnchors()).

.seealso: DMGlobalToLocalBegin(), DMGlobalToLocalEnd(), DMLocalToGlobalBegin(), DMLocalToGlobalEnd(), DMPlexGetAnchors(), DMPlexSetAnchors()
@*/
PetscErrorCode DMGlobalToLocalSolve(DM dm, Vec x, Vec y)
{
  Mat                   CtC;
  PetscInt              n, N, cStart, cEnd, c;
  PetscBool             isPlex;
  KSP                   ksp;
  PC                    pc;
  Vec                   global, mask=NULL;
  projectConstraintsCtx ctx;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscObjectTypeCompare((PetscObject)dm,DMPLEX,&isPlex);CHKERRQ(ierr);
  if (isPlex) {
    /* mark points in the closure */
    ierr = DMGetLocalVector(dm,&mask);CHKERRQ(ierr);
    ierr = VecSet(mask,0.0);CHKERRQ(ierr);
    ierr = DMPlexGetHeightStratum(dm,0,&cStart,&cEnd);CHKERRQ(ierr);
    if (cEnd > cStart) {
      PetscScalar *ones;
      PetscInt numValues, i;

      ierr = DMPlexVecGetClosure(dm,NULL,mask,cStart,&numValues,NULL);CHKERRQ(ierr);
      ierr = PetscMalloc1(numValues,&ones);CHKERRQ(ierr);
      for (i = 0; i < numValues; i++) {
        ones[i] = 1.;
      }
      for (c = cStart; c < cEnd; c++) {
        ierr = DMPlexVecSetClosure(dm,NULL,mask,c,ones,INSERT_VALUES);CHKERRQ(ierr);
      }
      ierr = PetscFree(ones);CHKERRQ(ierr);
    }
  }
  ctx.dm   = dm;
  ctx.mask = mask;
  ierr = VecGetSize(y,&N);CHKERRQ(ierr);
  ierr = VecGetLocalSize(y,&n);CHKERRQ(ierr);
  ierr = MatCreate(PetscObjectComm((PetscObject)dm),&CtC);CHKERRQ(ierr);
  ierr = MatSetSizes(CtC,n,n,N,N);CHKERRQ(ierr);
  ierr = MatSetType(CtC,MATSHELL);CHKERRQ(ierr);
  ierr = MatSetUp(CtC);CHKERRQ(ierr);
  ierr = MatShellSetContext(CtC,&ctx);CHKERRQ(ierr);
  ierr = MatShellSetOperation(CtC,MATOP_MULT,(void(*)(void))MatMult_GlobalToLocalNormal);CHKERRQ(ierr);
  ierr = KSPCreate(PetscObjectComm((PetscObject)dm),&ksp);CHKERRQ(ierr);
  ierr = KSPSetOperators(ksp,CtC,CtC);CHKERRQ(ierr);
  ierr = KSPSetType(ksp,KSPCG);CHKERRQ(ierr);
  ierr = KSPGetPC(ksp,&pc);CHKERRQ(ierr);
  ierr = PCSetType(pc,PCNONE);CHKERRQ(ierr);
  ierr = KSPSetInitialGuessNonzero(ksp,PETSC_TRUE);CHKERRQ(ierr);
  ierr = KSPSetUp(ksp);CHKERRQ(ierr);
  ierr = DMGetGlobalVector(dm,&global);CHKERRQ(ierr);
  ierr = VecSet(global,0.);CHKERRQ(ierr);
  if (mask) {ierr = VecPointwiseMult(x,mask,x);CHKERRQ(ierr);}
  ierr = DMLocalToGlobalBegin(dm,x,ADD_VALUES,global);CHKERRQ(ierr);
  ierr = DMLocalToGlobalEnd(dm,x,ADD_VALUES,global);CHKERRQ(ierr);
  ierr = KSPSetFromOptions(ksp);CHKERRQ(ierr);
  ierr = KSPSolve(ksp,global,y);CHKERRQ(ierr);
  ierr = DMRestoreGlobalVector(dm,&global);CHKERRQ(ierr);
  /* clean up */
  if (isPlex) {
    ierr = DMRestoreLocalVector(dm,&mask);CHKERRQ(ierr);
  }
  ierr = KSPDestroy(&ksp);CHKERRQ(ierr);
  ierr = MatDestroy(&CtC);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMPlexProjectField"
/*@C
  DMPlexProjectField - This projects the given function of the fields into the function space provided.

  Input Parameters:
+ dm      - The DM
. U       - The input field vector
. funcs   - The functions to evaluate, one per field
- mode    - The insertion mode for values

  Output Parameter:
. X       - The output vector

  Level: developer

.seealso: DMPlexProjectFunction(), DMPlexComputeL2Diff()
@*/
PetscErrorCode DMPlexProjectField(DM dm, Vec U, void (**funcs)(const PetscScalar[], const PetscScalar[], const PetscScalar[], const PetscScalar[], const PetscScalar[], const PetscScalar[], const PetscReal [], PetscScalar []), InsertMode mode, Vec X)
{
  Vec            localX, localU;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm, DM_CLASSID, 1);
  ierr = DMGetLocalVector(dm, &localX);CHKERRQ(ierr);
  ierr = DMGetLocalVector(dm, &localU);CHKERRQ(ierr);
  ierr = DMGlobalToLocalBegin(dm, U, INSERT_VALUES, localU);CHKERRQ(ierr);
  ierr = DMGlobalToLocalEnd(dm, U, INSERT_VALUES, localU);CHKERRQ(ierr);
  ierr = DMPlexProjectFieldLocal(dm, localU, funcs, mode, localX);CHKERRQ(ierr);
  ierr = DMLocalToGlobalBegin(dm, localX, mode, X);CHKERRQ(ierr);
  ierr = DMLocalToGlobalEnd(dm, localX, mode, X);CHKERRQ(ierr);
  if (mode == INSERT_VALUES || mode == INSERT_ALL_VALUES || mode == INSERT_BC_VALUES) {
    Mat cMat;

    ierr = DMGetDefaultConstraints(dm, NULL, &cMat);CHKERRQ(ierr);
    if (cMat) {
      ierr = DMGlobalToLocalSolve(dm, localX, X);CHKERRQ(ierr);
    }
  }
  ierr = DMRestoreLocalVector(dm, &localX);CHKERRQ(ierr);
  ierr = DMRestoreLocalVector(dm, &localU);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

