#include <petsc/private/dmpleximpl.h> /*I "petscdmplex.h" I*/
#include <petsc/private/tsimpl.h>     /*I "petscts.h" I*/
#include <petsc/private/snesimpl.h>
#include <petscds.h>
#include <petscfv.h>

#undef __FUNCT__
#define __FUNCT__ "DMPlexTSGetGeometryFVM"
/*@
  DMPlexTSGetGeometryFVM - Return precomputed geometric data

  Input Parameter:
. dm - The DM

  Output Parameters:
+ facegeom - The values precomputed from face geometry
. cellgeom - The values precomputed from cell geometry
- minRadius - The minimum radius over the mesh of an inscribed sphere in a cell

  Level: developer

.seealso: DMPlexTSSetRHSFunctionLocal()
@*/
PetscErrorCode DMPlexTSGetGeometryFVM(DM dm, Vec *facegeom, Vec *cellgeom, PetscReal *minRadius)
{
  DMTS           dmts;
  PetscObject    obj;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  ierr = DMGetDMTS(dm, &dmts);CHKERRQ(ierr);
  ierr = PetscObjectQuery((PetscObject) dmts, "DMPlexTS_facegeom_fvm", &obj);CHKERRQ(ierr);
  if (!obj) {
    Vec cellgeom, facegeom;

    ierr = DMPlexComputeGeometryFVM(dm, &cellgeom, &facegeom);CHKERRQ(ierr);
    ierr = PetscObjectCompose((PetscObject) dmts, "DMPlexTS_facegeom_fvm", (PetscObject) facegeom);CHKERRQ(ierr);
    ierr = PetscObjectCompose((PetscObject) dmts, "DMPlexTS_cellgeom_fvm", (PetscObject) cellgeom);CHKERRQ(ierr);
    ierr = VecDestroy(&facegeom);CHKERRQ(ierr);
    ierr = VecDestroy(&cellgeom);CHKERRQ(ierr);
  }
  if (facegeom) {PetscValidPointer(facegeom, 2); ierr = PetscObjectQuery((PetscObject) dmts, "DMPlexTS_facegeom_fvm", (PetscObject *) facegeom);CHKERRQ(ierr);}
  if (cellgeom) {PetscValidPointer(cellgeom, 3); ierr = PetscObjectQuery((PetscObject) dmts, "DMPlexTS_cellgeom_fvm", (PetscObject *) cellgeom);CHKERRQ(ierr);}
  if (minRadius) {ierr = DMPlexGetMinRadius(dm, minRadius);CHKERRQ(ierr);}
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMPlexTSGetGradientDM"
/*@C
  DMPlexTSGetGradientDM - Return gradient data layout

  Input Parameters:
+ dm - The DM
- fv - The PetscFV

  Output Parameter:
. dmGrad - The layout for gradient values

  Level: developer

.seealso: DMPlexTSGetGeometryFVM(), DMPlexTSSetRHSFunctionLocal()
@*/
PetscErrorCode DMPlexTSGetGradientDM(DM dm, PetscFV fv, DM *dmGrad)
{
  DMTS           dmts;
  PetscObject    obj;
  PetscBool      computeGradients;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  PetscValidHeaderSpecific(fv,PETSCFV_CLASSID,2);
  PetscValidPointer(dmGrad,3);
  ierr = PetscFVGetComputeGradients(fv, &computeGradients);CHKERRQ(ierr);
  if (!computeGradients) {*dmGrad = NULL; PetscFunctionReturn(0);}
  ierr = DMGetDMTS(dm, &dmts);CHKERRQ(ierr);
  ierr = PetscObjectQuery((PetscObject) dmts, "DMPlexTS_dmgrad_fvm", &obj);CHKERRQ(ierr);
  if (!obj) {
    DM  dmGrad;
    Vec faceGeometry, cellGeometry;

    ierr = DMPlexTSGetGeometryFVM(dm, &faceGeometry, &cellGeometry, NULL);CHKERRQ(ierr);
    ierr = DMPlexComputeGradientFVM(dm, fv, faceGeometry, cellGeometry, &dmGrad);CHKERRQ(ierr);
    ierr = PetscObjectCompose((PetscObject) dmts, "DMPlexTS_dmgrad_fvm", (PetscObject) dmGrad);CHKERRQ(ierr);
    ierr = DMDestroy(&dmGrad);CHKERRQ(ierr);
  }
  ierr = PetscObjectQuery((PetscObject) dmts, "DMPlexTS_dmgrad_fvm", (PetscObject *) dmGrad);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMPlexTSComputeRHSFunctionFVM"
/*@
  DMPlexTSComputeRHSFunctionFVM - Form the local forcing F from the local input X using pointwise functions specified by the user

  Input Parameters:
+ dm - The mesh
. t - The time
. locX  - Local solution
- user - The user context

  Output Parameter:
. F  - Global output vector

  Level: developer

.seealso: DMPlexComputeJacobianActionFEM()
@*/
PetscErrorCode DMPlexTSComputeRHSFunctionFVM(DM dm, PetscReal time, Vec locX, Vec F, void *user)
{
  Vec            locF;
  PetscInt       cStart, cEnd, cEndInterior;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = DMPlexGetHeightStratum(dm, 0, &cStart, &cEnd);CHKERRQ(ierr);
  ierr = DMPlexGetHybridBounds(dm, &cEndInterior, NULL, NULL, NULL);CHKERRQ(ierr);
  cEnd = cEndInterior < 0 ? cEnd : cEndInterior;
  ierr = DMGetLocalVector(dm, &locF);CHKERRQ(ierr);
  ierr = VecZeroEntries(locF);CHKERRQ(ierr);
  ierr = DMPlexComputeResidual_Internal(dm, cStart, cEnd, time, locX, NULL, locF, user);CHKERRQ(ierr);
  ierr = DMLocalToGlobalBegin(dm, locF, INSERT_VALUES, F);CHKERRQ(ierr);
  ierr = DMLocalToGlobalEnd(dm, locF, INSERT_VALUES, F);CHKERRQ(ierr);
  ierr = DMRestoreLocalVector(dm, &locF);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMPlexTSComputeIFunctionFEM"
/*@
  DMPlexTSComputeIFunctionFEM - Form the local residual F from the local input X using pointwise functions specified by the user

  Input Parameters:
+ dm - The mesh
. t - The time
. locX  - Local solution
. locX_t - Local solution time derivative, or NULL
- user - The user context

  Output Parameter:
. locF  - Local output vector

  Level: developer

.seealso: DMPlexComputeJacobianActionFEM()
@*/
PetscErrorCode DMPlexTSComputeIFunctionFEM(DM dm, PetscReal time, Vec locX, Vec locX_t, Vec locF, void *user)
{
  PetscInt       cStart, cEnd, cEndInterior;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = DMPlexGetHeightStratum(dm, 0, &cStart, &cEnd);CHKERRQ(ierr);
  ierr = DMPlexGetHybridBounds(dm, &cEndInterior, NULL, NULL, NULL);CHKERRQ(ierr);
  cEnd = cEndInterior < 0 ? cEnd : cEndInterior;
  ierr = DMPlexComputeResidual_Internal(dm, cStart, cEnd, time, locX, locX_t, locF, user);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMTSCheckFromOptions"
PetscErrorCode DMTSCheckFromOptions(TS ts, Vec u, PetscErrorCode (**exactFuncs)(PetscInt dim, PetscReal time, const PetscReal x[], PetscInt Nf, PetscScalar *u, void *ctx), void **ctxs)
{
  DM             dm;
  SNES           snes;
  Vec            sol;
  PetscBool      check;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscOptionsHasName(((PetscObject)ts)->options,((PetscObject)ts)->prefix, "-dmts_check", &check);CHKERRQ(ierr);
  if (!check) PetscFunctionReturn(0);
  ierr = VecDuplicate(u, &sol);CHKERRQ(ierr);
  ierr = TSSetSolution(ts, sol);CHKERRQ(ierr);
  ierr = TSGetDM(ts, &dm);CHKERRQ(ierr);
  ierr = TSSetUp(ts);CHKERRQ(ierr);
  ierr = TSGetSNES(ts, &snes);CHKERRQ(ierr);
  ierr = SNESSetSolution(snes, sol);CHKERRQ(ierr);
  ierr = DMSNESCheckFromOptions_Internal(snes, dm, u, sol, exactFuncs, ctxs);CHKERRQ(ierr);
  ierr = VecDestroy(&sol);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
