#include <petsc/private/tsimpl.h>     /*I "petscts.h" I*/
#include <petsc/private/dmimpl.h>

#undef __FUNCT__
#define __FUNCT__ "DMTSDestroy"
static PetscErrorCode DMTSDestroy(DMTS *kdm)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (!*kdm) PetscFunctionReturn(0);
  PetscValidHeaderSpecific((*kdm),DMTS_CLASSID,1);
  if (--((PetscObject)(*kdm))->refct > 0) {*kdm = 0; PetscFunctionReturn(0);}
  if ((*kdm)->ops->destroy) {ierr = ((*kdm)->ops->destroy)(*kdm);CHKERRQ(ierr);}
  ierr = PetscHeaderDestroy(kdm);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMTSLoad"
PetscErrorCode DMTSLoad(DMTS kdm,PetscViewer viewer)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscViewerBinaryRead(viewer,&kdm->ops->ifunction,1,NULL,PETSC_FUNCTION);CHKERRQ(ierr);
  ierr = PetscViewerBinaryRead(viewer,&kdm->ops->ifunctionview,1,NULL,PETSC_FUNCTION);CHKERRQ(ierr);
  ierr = PetscViewerBinaryRead(viewer,&kdm->ops->ifunctionload,1,NULL,PETSC_FUNCTION);CHKERRQ(ierr);
  if (kdm->ops->ifunctionload) {
    ierr = (*kdm->ops->ifunctionload)(&kdm->ifunctionctx,viewer);CHKERRQ(ierr);
  }
  ierr = PetscViewerBinaryRead(viewer,&kdm->ops->ijacobian,1,NULL,PETSC_FUNCTION);CHKERRQ(ierr);
  ierr = PetscViewerBinaryRead(viewer,&kdm->ops->ijacobianview,1,NULL,PETSC_FUNCTION);CHKERRQ(ierr);
  ierr = PetscViewerBinaryRead(viewer,&kdm->ops->ijacobianload,1,NULL,PETSC_FUNCTION);CHKERRQ(ierr);
  if (kdm->ops->ijacobianload) {
    ierr = (*kdm->ops->ijacobianload)(&kdm->ijacobianctx,viewer);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMTSView"
PetscErrorCode DMTSView(DMTS kdm,PetscViewer viewer)
{
  PetscErrorCode ierr;
  PetscBool      isascii,isbinary;

  PetscFunctionBegin;
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERASCII,&isascii);CHKERRQ(ierr);
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERBINARY,&isbinary);CHKERRQ(ierr);
  if (isascii) {
#if defined(PETSC_SERIALIZE_FUNCTIONS)
    const char *fname;

    ierr = PetscFPTFind(kdm->ops->ifunction,&fname);CHKERRQ(ierr);
    if (fname) {
      ierr = PetscViewerASCIIPrintf(viewer,"  IFunction used by TS: %s\n",fname);CHKERRQ(ierr);
    }
    ierr = PetscFPTFind(kdm->ops->ijacobian,&fname);CHKERRQ(ierr);
    if (fname) {
      ierr = PetscViewerASCIIPrintf(viewer,"  IJacobian function used by TS: %s\n",fname);CHKERRQ(ierr);
    }
#endif
  } else if (isbinary) {
    struct {
      TSIFunction ifunction;
    } funcstruct;
    struct {
      PetscErrorCode (*ifunctionview)(void*,PetscViewer);
    } funcviewstruct;
    struct {
      PetscErrorCode (*ifunctionload)(void**,PetscViewer);
    } funcloadstruct;
    struct {
      TSIJacobian ijacobian;
    } jacstruct;
    struct {
      PetscErrorCode (*ijacobianview)(void*,PetscViewer);
    } jacviewstruct;
    struct {
      PetscErrorCode (*ijacobianload)(void**,PetscViewer);
    } jacloadstruct;

    funcstruct.ifunction         = kdm->ops->ifunction;
    funcviewstruct.ifunctionview = kdm->ops->ifunctionview;
    funcloadstruct.ifunctionload = kdm->ops->ifunctionload;
    ierr = PetscViewerBinaryWrite(viewer,&funcstruct,1,PETSC_FUNCTION,PETSC_FALSE);CHKERRQ(ierr);
    ierr = PetscViewerBinaryWrite(viewer,&funcviewstruct,1,PETSC_FUNCTION,PETSC_FALSE);CHKERRQ(ierr);
    ierr = PetscViewerBinaryWrite(viewer,&funcloadstruct,1,PETSC_FUNCTION,PETSC_FALSE);CHKERRQ(ierr);
    if (kdm->ops->ifunctionview) {
      ierr = (*kdm->ops->ifunctionview)(kdm->ifunctionctx,viewer);CHKERRQ(ierr);
    }
    jacstruct.ijacobian = kdm->ops->ijacobian;
    jacviewstruct.ijacobianview = kdm->ops->ijacobianview;
    jacloadstruct.ijacobianload = kdm->ops->ijacobianload;
    ierr = PetscViewerBinaryWrite(viewer,&jacstruct,1,PETSC_FUNCTION,PETSC_FALSE);CHKERRQ(ierr);
    ierr = PetscViewerBinaryWrite(viewer,&jacviewstruct,1,PETSC_FUNCTION,PETSC_FALSE);CHKERRQ(ierr);
    ierr = PetscViewerBinaryWrite(viewer,&jacloadstruct,1,PETSC_FUNCTION,PETSC_FALSE);CHKERRQ(ierr);
    if (kdm->ops->ijacobianview) {
      ierr = (*kdm->ops->ijacobianview)(kdm->ijacobianctx,viewer);CHKERRQ(ierr);
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMTSCreate"
static PetscErrorCode DMTSCreate(MPI_Comm comm,DMTS *kdm)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = TSInitializePackage();CHKERRQ(ierr);
  ierr = PetscHeaderCreate(*kdm, DMTS_CLASSID, "DMTS", "DMTS", "DMTS", comm, DMTSDestroy, DMTSView);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMCoarsenHook_DMTS"
/* Attaches the DMTS to the coarse level.
 * Under what conditions should we copy versus duplicate?
 */
static PetscErrorCode DMCoarsenHook_DMTS(DM dm,DM dmc,void *ctx)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = DMCopyDMTS(dm,dmc);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMRestrictHook_DMTS"
/* This could restrict auxiliary information to the coarse level.
 */
static PetscErrorCode DMRestrictHook_DMTS(DM dm,Mat Restrict,Vec rscale,Mat Inject,DM dmc,void *ctx)
{

  PetscFunctionBegin;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMSubDomainHook_DMTS"
static PetscErrorCode DMSubDomainHook_DMTS(DM dm,DM subdm,void *ctx)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = DMCopyDMTS(dm,subdm);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMSubDomainRestrictHook_DMTS"
/* This could restrict auxiliary information to the coarse level.
 */
static PetscErrorCode DMSubDomainRestrictHook_DMTS(DM dm,VecScatter gscat,VecScatter lscat,DM subdm,void *ctx)
{
  PetscFunctionBegin;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMTSCopy"
/*@C
   DMTSCopy - copies the information in a DMTS to another DMTS

   Not Collective

   Input Argument:
+  kdm - Original DMTS
-  nkdm - DMTS to receive the data, should have been created with DMTSCreate()

   Level: developer

.seealso: DMTSCreate(), DMTSDestroy()
@*/
PetscErrorCode DMTSCopy(DMTS kdm,DMTS nkdm)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(kdm,DMTS_CLASSID,1);
  PetscValidHeaderSpecific(nkdm,DMTS_CLASSID,2);
  nkdm->ops->rhsfunction = kdm->ops->rhsfunction;
  nkdm->ops->rhsjacobian = kdm->ops->rhsjacobian;
  nkdm->ops->ifunction   = kdm->ops->ifunction;
  nkdm->ops->ijacobian   = kdm->ops->ijacobian;
  nkdm->ops->solution    = kdm->ops->solution;
  nkdm->ops->destroy     = kdm->ops->destroy;
  nkdm->ops->duplicate   = kdm->ops->duplicate;

  nkdm->rhsfunctionctx = kdm->rhsfunctionctx;
  nkdm->rhsjacobianctx = kdm->rhsjacobianctx;
  nkdm->ifunctionctx   = kdm->ifunctionctx;
  nkdm->ijacobianctx   = kdm->ijacobianctx;
  nkdm->solutionctx    = kdm->solutionctx;

  nkdm->data = kdm->data;

  /*
  nkdm->fortran_func_pointers[0] = kdm->fortran_func_pointers[0];
  nkdm->fortran_func_pointers[1] = kdm->fortran_func_pointers[1];
  nkdm->fortran_func_pointers[2] = kdm->fortran_func_pointers[2];
  */

  /* implementation specific copy hooks */
  if (kdm->ops->duplicate) {ierr = (*kdm->ops->duplicate)(kdm,nkdm);CHKERRQ(ierr);}
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMGetDMTS"
/*@C
   DMGetDMTS - get read-only private DMTS context from a DM

   Not Collective

   Input Argument:
.  dm - DM to be used with TS

   Output Argument:
.  tsdm - private DMTS context

   Level: developer

   Notes:
   Use DMGetDMTSWrite() if write access is needed. The DMTSSetXXX API should be used wherever possible.

.seealso: DMGetDMTSWrite()
@*/
PetscErrorCode DMGetDMTS(DM dm,DMTS *tsdm)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  *tsdm = (DMTS) dm->dmts;
  if (!*tsdm) {
    ierr = PetscInfo(dm,"Creating new DMTS\n");CHKERRQ(ierr);
    ierr = DMTSCreate(PetscObjectComm((PetscObject)dm),tsdm);CHKERRQ(ierr);
    dm->dmts = (PetscObject) *tsdm;
    ierr = DMCoarsenHookAdd(dm,DMCoarsenHook_DMTS,DMRestrictHook_DMTS,NULL);CHKERRQ(ierr);
    ierr = DMSubDomainHookAdd(dm,DMSubDomainHook_DMTS,DMSubDomainRestrictHook_DMTS,NULL);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMGetDMTSWrite"
/*@C
   DMGetDMTSWrite - get write access to private DMTS context from a DM

   Not Collective

   Input Argument:
.  dm - DM to be used with TS

   Output Argument:
.  tsdm - private DMTS context

   Level: developer

.seealso: DMGetDMTS()
@*/
PetscErrorCode DMGetDMTSWrite(DM dm,DMTS *tsdm)
{
  PetscErrorCode ierr;
  DMTS           sdm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  ierr = DMGetDMTS(dm,&sdm);CHKERRQ(ierr);
  if (!sdm->originaldm) sdm->originaldm = dm;
  if (sdm->originaldm != dm) {  /* Copy on write */
    DMTS oldsdm = sdm;
    ierr     = PetscInfo(dm,"Copying DMTS due to write\n");CHKERRQ(ierr);
    ierr     = DMTSCreate(PetscObjectComm((PetscObject)dm),&sdm);CHKERRQ(ierr);
    ierr     = DMTSCopy(oldsdm,sdm);CHKERRQ(ierr);
    ierr     = DMTSDestroy((DMTS*)&dm->dmts);CHKERRQ(ierr);
    dm->dmts = (PetscObject) sdm;
  }
  *tsdm = sdm;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMCopyDMTS"
/*@C
   DMCopyDMTS - copies a DM context to a new DM

   Logically Collective

   Input Arguments:
+  dmsrc - DM to obtain context from
-  dmdest - DM to add context to

   Level: developer

   Note:
   The context is copied by reference. This function does not ensure that a context exists.

.seealso: DMGetDMTS(), TSSetDM()
@*/
PetscErrorCode DMCopyDMTS(DM dmsrc,DM dmdest)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dmsrc,DM_CLASSID,1);
  PetscValidHeaderSpecific(dmdest,DM_CLASSID,2);
  ierr         = DMTSDestroy((DMTS*)&dmdest->dmts);CHKERRQ(ierr);
  dmdest->dmts = dmsrc->dmts;
  ierr         = PetscObjectReference(dmdest->dmts);CHKERRQ(ierr);
  ierr         = DMCoarsenHookAdd(dmdest,DMCoarsenHook_DMTS,DMRestrictHook_DMTS,NULL);CHKERRQ(ierr);
  ierr         = DMSubDomainHookAdd(dmdest,DMSubDomainHook_DMTS,DMSubDomainRestrictHook_DMTS,NULL);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMTSSetIFunction"
/*@C
   DMTSSetIFunction - set TS implicit function evaluation function

   Not Collective

   Input Arguments:
+  dm - DM to be used with TS
.  func - function evaluation function, see TSSetIFunction() for calling sequence
-  ctx - context for residual evaluation

   Level: advanced

   Note:
   TSSetFunction() is normally used, but it calls this function internally because the user context is actually
   associated with the DM.  This makes the interface consistent regardless of whether the user interacts with a DM or
   not. If DM took a more central role at some later date, this could become the primary method of setting the residual.

.seealso: DMTSSetContext(), TSSetFunction(), DMTSSetJacobian()
@*/
PetscErrorCode DMTSSetIFunction(DM dm,TSIFunction func,void *ctx)
{
  PetscErrorCode ierr;
  DMTS           tsdm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  ierr = DMGetDMTSWrite(dm,&tsdm);CHKERRQ(ierr);
  if (func) tsdm->ops->ifunction = func;
  if (ctx)  tsdm->ifunctionctx = ctx;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMTSGetIFunction"
/*@C
   DMTSGetIFunction - get TS implicit residual evaluation function

   Not Collective

   Input Argument:
.  dm - DM to be used with TS

   Output Arguments:
+  func - function evaluation function, see TSSetIFunction() for calling sequence
-  ctx - context for residual evaluation

   Level: advanced

   Note:
   TSGetFunction() is normally used, but it calls this function internally because the user context is actually
   associated with the DM.

.seealso: DMTSSetContext(), DMTSSetFunction(), TSSetFunction()
@*/
PetscErrorCode DMTSGetIFunction(DM dm,TSIFunction *func,void **ctx)
{
  PetscErrorCode ierr;
  DMTS           tsdm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  ierr = DMGetDMTS(dm,&tsdm);CHKERRQ(ierr);
  if (func) *func = tsdm->ops->ifunction;
  if (ctx)  *ctx = tsdm->ifunctionctx;
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "DMTSSetRHSFunction"
/*@C
   DMTSSetRHSFunction - set TS explicit residual evaluation function

   Not Collective

   Input Arguments:
+  dm - DM to be used with TS
.  func - RHS function evaluation function, see TSSetRHSFunction() for calling sequence
-  ctx - context for residual evaluation

   Level: advanced

   Note:
   TSSetRSHFunction() is normally used, but it calls this function internally because the user context is actually
   associated with the DM.  This makes the interface consistent regardless of whether the user interacts with a DM or
   not. If DM took a more central role at some later date, this could become the primary method of setting the residual.

.seealso: DMTSSetContext(), TSSetFunction(), DMTSSetJacobian()
@*/
PetscErrorCode DMTSSetRHSFunction(DM dm,TSRHSFunction func,void *ctx)
{
  PetscErrorCode ierr;
  DMTS           tsdm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  ierr = DMGetDMTSWrite(dm,&tsdm);CHKERRQ(ierr);
  if (func) tsdm->ops->rhsfunction = func;
  if (ctx)  tsdm->rhsfunctionctx = ctx;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMTSGetSolutionFunction"
/*@C
   DMTSGetSolutionFunction - gets the TS solution evaluation function

   Not Collective

   Input Arguments:
.  dm - DM to be used with TS

   Output Parameters:
+  func - solution function evaluation function, see TSSetSolution() for calling sequence
-  ctx - context for solution evaluation

   Level: advanced

.seealso: DMTSSetContext(), TSSetFunction(), DMTSSetJacobian()
@*/
PetscErrorCode DMTSGetSolutionFunction(DM dm,TSSolutionFunction *func,void **ctx)
{
  PetscErrorCode ierr;
  DMTS           tsdm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  ierr = DMGetDMTS(dm,&tsdm);CHKERRQ(ierr);
  if (func) *func = tsdm->ops->solution;
  if (ctx)  *ctx  = tsdm->solutionctx;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMTSSetSolutionFunction"
/*@C
   DMTSSetSolutionFunction - set TS solution evaluation function

   Not Collective

   Input Arguments:
+  dm - DM to be used with TS
.  func - solution function evaluation function, see TSSetSolution() for calling sequence
-  ctx - context for solution evaluation

   Level: advanced

   Note:
   TSSetSolutionFunction() is normally used, but it calls this function internally because the user context is actually
   associated with the DM.  This makes the interface consistent regardless of whether the user interacts with a DM or
   not. If DM took a more central role at some later date, this could become the primary method of setting the residual.

.seealso: DMTSSetContext(), TSSetFunction(), DMTSSetJacobian()
@*/
PetscErrorCode DMTSSetSolutionFunction(DM dm,TSSolutionFunction func,void *ctx)
{
  PetscErrorCode ierr;
  DMTS           tsdm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  ierr = DMGetDMTSWrite(dm,&tsdm);CHKERRQ(ierr);
  if (func) tsdm->ops->solution = func;
  if (ctx)  tsdm->solutionctx   = ctx;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMTSSetForcingFunction"
/*@C
   DMTSSetForcingFunction - set TS forcing function evaluation function

   Not Collective

   Input Arguments:
+  dm - DM to be used with TS
.  f - forcing function evaluation function; see TSForcingFunction
-  ctx - context for solution evaluation

   Level: advanced

   Note:
   TSSetForcingFunction() is normally used, but it calls this function internally because the user context is actually
   associated with the DM.  This makes the interface consistent regardless of whether the user interacts with a DM or
   not. If DM took a more central role at some later date, this could become the primary method of setting the residual.

.seealso: DMTSSetContext(), TSSetFunction(), DMTSSetJacobian(), TSSetForcingFunction(), DMTSGetForcingFunction()
@*/
PetscErrorCode DMTSSetForcingFunction(DM dm,PetscErrorCode (*f)(TS,PetscReal,Vec,void*),void *ctx)
{
  PetscErrorCode ierr;
  DMTS           tsdm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  ierr = DMGetDMTSWrite(dm,&tsdm);CHKERRQ(ierr);
  if (f) tsdm->ops->forcing = f;
  if (ctx)  tsdm->forcingctx   = ctx;
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "DMTSGetForcingFunction"
/*@C
   DMTSGetForcingFunction - get TS forcing function evaluation function

   Not Collective

   Input Argument:
.   dm - DM to be used with TS

   Output Arguments:
+  f - forcing function evaluation function; see TSForcingFunction for details
-  ctx - context for solution evaluation

   Level: advanced

   Note:
   TSSetForcingFunction() is normally used, but it calls this function internally because the user context is actually
   associated with the DM.  This makes the interface consistent regardless of whether the user interacts with a DM or
   not. If DM took a more central role at some later date, this could become the primary method of setting the residual.

.seealso: DMTSSetContext(), TSSetFunction(), DMTSSetJacobian(), TSSetForcingFunction(), DMTSGetForcingFunction()
@*/
PetscErrorCode DMTSGetForcingFunction(DM dm,PetscErrorCode (**f)(TS,PetscReal,Vec,void*),void **ctx)
{
  PetscErrorCode ierr;
  DMTS           tsdm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  ierr = DMGetDMTSWrite(dm,&tsdm);CHKERRQ(ierr);
  if (f) *f = tsdm->ops->forcing;
  if (ctx) *ctx = tsdm->forcingctx;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMTSGetRHSFunction"
/*@C
   DMTSGetRHSFunction - get TS explicit residual evaluation function

   Not Collective

   Input Argument:
.  dm - DM to be used with TS

   Output Arguments:
+  func - residual evaluation function, see TSSetRHSFunction() for calling sequence
-  ctx - context for residual evaluation

   Level: advanced

   Note:
   TSGetFunction() is normally used, but it calls this function internally because the user context is actually
   associated with the DM.

.seealso: DMTSSetContext(), DMTSSetFunction(), TSSetFunction()
@*/
PetscErrorCode DMTSGetRHSFunction(DM dm,TSRHSFunction *func,void **ctx)
{
  PetscErrorCode ierr;
  DMTS           tsdm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  ierr = DMGetDMTS(dm,&tsdm);CHKERRQ(ierr);
  if (func) *func = tsdm->ops->rhsfunction;
  if (ctx)  *ctx = tsdm->rhsfunctionctx;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMTSSetIJacobian"
/*@C
   DMTSSetIJacobian - set TS Jacobian evaluation function

   Not Collective

   Input Argument:
+  dm - DM to be used with TS
.  func - Jacobian evaluation function, see TSSetIJacobian() for calling sequence
-  ctx - context for residual evaluation

   Level: advanced

   Note:
   TSSetJacobian() is normally used, but it calls this function internally because the user context is actually
   associated with the DM.  This makes the interface consistent regardless of whether the user interacts with a DM or
   not. If DM took a more central role at some later date, this could become the primary method of setting the Jacobian.

.seealso: DMTSSetContext(), TSSetFunction(), DMTSGetJacobian(), TSSetJacobian()
@*/
PetscErrorCode DMTSSetIJacobian(DM dm,TSIJacobian func,void *ctx)
{
  PetscErrorCode ierr;
  DMTS           sdm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  ierr = DMGetDMTSWrite(dm,&sdm);CHKERRQ(ierr);
  if (func) sdm->ops->ijacobian = func;
  if (ctx)  sdm->ijacobianctx   = ctx;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMTSGetIJacobian"
/*@C
   DMTSGetIJacobian - get TS Jacobian evaluation function

   Not Collective

   Input Argument:
.  dm - DM to be used with TS

   Output Arguments:
+  func - Jacobian evaluation function, see TSSetIJacobian() for calling sequence
-  ctx - context for residual evaluation

   Level: advanced

   Note:
   TSGetJacobian() is normally used, but it calls this function internally because the user context is actually
   associated with the DM.  This makes the interface consistent regardless of whether the user interacts with a DM or
   not. If DM took a more central role at some later date, this could become the primary method of setting the Jacobian.

.seealso: DMTSSetContext(), TSSetFunction(), DMTSSetJacobian()
@*/
PetscErrorCode DMTSGetIJacobian(DM dm,TSIJacobian *func,void **ctx)
{
  PetscErrorCode ierr;
  DMTS           tsdm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  ierr = DMGetDMTS(dm,&tsdm);CHKERRQ(ierr);
  if (func) *func = tsdm->ops->ijacobian;
  if (ctx)  *ctx = tsdm->ijacobianctx;
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "DMTSSetRHSJacobian"
/*@C
   DMTSSetRHSJacobian - set TS Jacobian evaluation function

   Not Collective

   Input Argument:
+  dm - DM to be used with TS
.  func - Jacobian evaluation function, see TSSetRHSJacobian() for calling sequence
-  ctx - context for residual evaluation

   Level: advanced

   Note:
   TSSetJacobian() is normally used, but it calls this function internally because the user context is actually
   associated with the DM.  This makes the interface consistent regardless of whether the user interacts with a DM or
   not. If DM took a more central role at some later date, this could become the primary method of setting the Jacobian.

.seealso: DMTSSetContext(), TSSetFunction(), DMTSGetJacobian(), TSSetJacobian()
@*/
PetscErrorCode DMTSSetRHSJacobian(DM dm,TSRHSJacobian func,void *ctx)
{
  PetscErrorCode ierr;
  DMTS           tsdm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  ierr = DMGetDMTSWrite(dm,&tsdm);CHKERRQ(ierr);
  if (func) tsdm->ops->rhsjacobian = func;
  if (ctx)  tsdm->rhsjacobianctx = ctx;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMTSGetRHSJacobian"
/*@C
   DMTSGetRHSJacobian - get TS Jacobian evaluation function

   Not Collective

   Input Argument:
.  dm - DM to be used with TS

   Output Arguments:
+  func - Jacobian evaluation function, see TSSetRHSJacobian() for calling sequence
-  ctx - context for residual evaluation

   Level: advanced

   Note:
   TSGetJacobian() is normally used, but it calls this function internally because the user context is actually
   associated with the DM.  This makes the interface consistent regardless of whether the user interacts with a DM or
   not. If DM took a more central role at some later date, this could become the primary method of setting the Jacobian.

.seealso: DMTSSetContext(), TSSetFunction(), DMTSSetJacobian()
@*/
PetscErrorCode DMTSGetRHSJacobian(DM dm,TSRHSJacobian *func,void **ctx)
{
  PetscErrorCode ierr;
  DMTS           tsdm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  ierr = DMGetDMTS(dm,&tsdm);CHKERRQ(ierr);
  if (func) *func = tsdm->ops->rhsjacobian;
  if (ctx)  *ctx = tsdm->rhsjacobianctx;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMTSSetIFunctionSerialize"
/*@C
   DMTSSetIFunctionSerialize - sets functions used to view and load a IFunction context

   Not Collective

   Input Arguments:
+  dm - DM to be used with TS
.  view - viewer function
-  load - loading function

   Level: advanced

.seealso: DMTSSetContext(), TSSetFunction(), DMTSSetJacobian()
@*/
PetscErrorCode DMTSSetIFunctionSerialize(DM dm,PetscErrorCode (*view)(void*,PetscViewer),PetscErrorCode (*load)(void**,PetscViewer))
{
  PetscErrorCode ierr;
  DMTS           tsdm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  ierr = DMGetDMTSWrite(dm,&tsdm);CHKERRQ(ierr);
  tsdm->ops->ifunctionview = view;
  tsdm->ops->ifunctionload = load;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMTSSetIJacobianSerialize"
/*@C
   DMTSSetIJacobianSerialize - sets functions used to view and load a IJacobian context

   Not Collective

   Input Arguments:
+  dm - DM to be used with TS
.  view - viewer function
-  load - loading function

   Level: advanced

.seealso: DMTSSetContext(), TSSetFunction(), DMTSSetJacobian()
@*/
PetscErrorCode DMTSSetIJacobianSerialize(DM dm,PetscErrorCode (*view)(void*,PetscViewer),PetscErrorCode (*load)(void**,PetscViewer))
{
  PetscErrorCode ierr;
  DMTS           tsdm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  ierr = DMGetDMTSWrite(dm,&tsdm);CHKERRQ(ierr);
  tsdm->ops->ijacobianview = view;
  tsdm->ops->ijacobianload = load;
  PetscFunctionReturn(0);
}
