
#include <../src/dm/impls/composite/packimpl.h>       /*I  "petscdmcomposite.h"  I*/
#include <petsc/private/isimpl.h>
#include <petscds.h>

#undef __FUNCT__
#define __FUNCT__ "DMCompositeSetCoupling"
/*@C
    DMCompositeSetCoupling - Sets user provided routines that compute the coupling between the
      separate components (DMs) in a DMto build the correct matrix nonzero structure.


    Logically Collective on MPI_Comm

    Input Parameter:
+   dm - the composite object
-   formcouplelocations - routine to set the nonzero locations in the matrix

    Level: advanced

    Notes: See DMSetApplicationContext() and DMGetApplicationContext() for how to get user information into
        this routine

@*/
PetscErrorCode  DMCompositeSetCoupling(DM dm,PetscErrorCode (*FormCoupleLocations)(DM,Mat,PetscInt*,PetscInt*,PetscInt,PetscInt,PetscInt,PetscInt))
{
  DM_Composite *com = (DM_Composite*)dm->data;

  PetscFunctionBegin;
  com->FormCoupleLocations = FormCoupleLocations;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMDestroy_Composite"
PetscErrorCode  DMDestroy_Composite(DM dm)
{
  PetscErrorCode         ierr;
  struct DMCompositeLink *next, *prev;
  DM_Composite           *com = (DM_Composite*)dm->data;

  PetscFunctionBegin;
  next = com->next;
  while (next) {
    prev = next;
    next = next->next;
    ierr = DMDestroy(&prev->dm);CHKERRQ(ierr);
    ierr = PetscFree(prev->grstarts);CHKERRQ(ierr);
    ierr = PetscFree(prev);CHKERRQ(ierr);
  }
  /* This was originally freed in DMDestroy(), but that prevents reference counting of backend objects */
  ierr = PetscFree(com);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMView_Composite"
PetscErrorCode  DMView_Composite(DM dm,PetscViewer v)
{
  PetscErrorCode ierr;
  PetscBool      iascii;
  DM_Composite   *com = (DM_Composite*)dm->data;

  PetscFunctionBegin;
  ierr = PetscObjectTypeCompare((PetscObject)v,PETSCVIEWERASCII,&iascii);CHKERRQ(ierr);
  if (iascii) {
    struct DMCompositeLink *lnk = com->next;
    PetscInt               i;

    ierr = PetscViewerASCIIPrintf(v,"DM (%s)\n",((PetscObject)dm)->prefix ? ((PetscObject)dm)->prefix : "no prefix");CHKERRQ(ierr);
    ierr = PetscViewerASCIIPrintf(v,"  contains %D DMs\n",com->nDM);CHKERRQ(ierr);
    ierr = PetscViewerASCIIPushTab(v);CHKERRQ(ierr);
    for (i=0; lnk; lnk=lnk->next,i++) {
      ierr = PetscViewerASCIIPrintf(v,"Link %D: DM of type %s\n",i,((PetscObject)lnk->dm)->type_name);CHKERRQ(ierr);
      ierr = PetscViewerASCIIPushTab(v);CHKERRQ(ierr);
      ierr = DMView(lnk->dm,v);CHKERRQ(ierr);
      ierr = PetscViewerASCIIPopTab(v);CHKERRQ(ierr);
    }
    ierr = PetscViewerASCIIPopTab(v);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

/* --------------------------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "DMSetUp_Composite"
PetscErrorCode  DMSetUp_Composite(DM dm)
{
  PetscErrorCode         ierr;
  PetscInt               nprev = 0;
  PetscMPIInt            rank,size;
  DM_Composite           *com  = (DM_Composite*)dm->data;
  struct DMCompositeLink *next = com->next;
  PetscLayout            map;

  PetscFunctionBegin;
  if (com->setup) SETERRQ(PetscObjectComm((PetscObject)dm),PETSC_ERR_ARG_WRONGSTATE,"Packer has already been setup");
  ierr = PetscLayoutCreate(PetscObjectComm((PetscObject)dm),&map);CHKERRQ(ierr);
  ierr = PetscLayoutSetLocalSize(map,com->n);CHKERRQ(ierr);
  ierr = PetscLayoutSetSize(map,PETSC_DETERMINE);CHKERRQ(ierr);
  ierr = PetscLayoutSetBlockSize(map,1);CHKERRQ(ierr);
  ierr = PetscLayoutSetUp(map);CHKERRQ(ierr);
  ierr = PetscLayoutGetSize(map,&com->N);CHKERRQ(ierr);
  ierr = PetscLayoutGetRange(map,&com->rstart,NULL);CHKERRQ(ierr);
  ierr = PetscLayoutDestroy(&map);CHKERRQ(ierr);

  /* now set the rstart for each linked vector */
  ierr = MPI_Comm_rank(PetscObjectComm((PetscObject)dm),&rank);CHKERRQ(ierr);
  ierr = MPI_Comm_size(PetscObjectComm((PetscObject)dm),&size);CHKERRQ(ierr);
  while (next) {
    next->rstart  = nprev;
    nprev        += next->n;
    next->grstart = com->rstart + next->rstart;
    ierr          = PetscMalloc1(size,&next->grstarts);CHKERRQ(ierr);
    ierr          = MPI_Allgather(&next->grstart,1,MPIU_INT,next->grstarts,1,MPIU_INT,PetscObjectComm((PetscObject)dm));CHKERRQ(ierr);
    next          = next->next;
  }
  com->setup = PETSC_TRUE;
  PetscFunctionReturn(0);
}

/* ----------------------------------------------------------------------------------*/

#undef __FUNCT__
#define __FUNCT__ "DMCompositeGetNumberDM"
/*@
    DMCompositeGetNumberDM - Get's the number of DM objects in the DMComposite
       representation.

    Not Collective

    Input Parameter:
.    dm - the packer object

    Output Parameter:
.     nDM - the number of DMs

    Level: beginner

@*/
PetscErrorCode  DMCompositeGetNumberDM(DM dm,PetscInt *nDM)
{
  DM_Composite *com = (DM_Composite*)dm->data;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  *nDM = com->nDM;
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "DMCompositeGetAccess"
/*@C
    DMCompositeGetAccess - Allows one to access the individual packed vectors in their global
       representation.

    Collective on DMComposite

    Input Parameters:
+    dm - the packer object
-    gvec - the global vector

    Output Parameters:
.    Vec* ... - the packed parallel vectors, NULL for those that are not needed

    Notes: Use DMCompositeRestoreAccess() to return the vectors when you no longer need them

    Fortran Notes:

    Fortran callers must use numbered versions of this routine, e.g., DMCompositeGetAccess4(dm,gvec,vec1,vec2,vec3,vec4)
    or use the alternative interface DMCompositeGetAccessArray().

    Level: advanced

.seealso: DMCompositeGetEntries(), DMCompositeScatter()
@*/
PetscErrorCode  DMCompositeGetAccess(DM dm,Vec gvec,...)
{
  va_list                Argp;
  PetscErrorCode         ierr;
  struct DMCompositeLink *next;
  DM_Composite           *com = (DM_Composite*)dm->data;
  PetscInt               readonly;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  PetscValidHeaderSpecific(gvec,VEC_CLASSID,2);
  next = com->next;
  if (!com->setup) {
    ierr = DMSetUp(dm);CHKERRQ(ierr);
  }

  ierr = VecLockGet(gvec,&readonly);CHKERRQ(ierr);
  /* loop over packed objects, handling one at at time */
  va_start(Argp,gvec);
  while (next) {
    Vec *vec;
    vec = va_arg(Argp, Vec*);
    if (vec) {
      ierr = DMGetGlobalVector(next->dm,vec);CHKERRQ(ierr);
      if (readonly) {
        const PetscScalar *array;
        ierr = VecGetArrayRead(gvec,&array);CHKERRQ(ierr);
        ierr = VecPlaceArray(*vec,array+next->rstart);CHKERRQ(ierr);
        ierr = VecLockPush(*vec);CHKERRQ(ierr);
        ierr = VecRestoreArrayRead(gvec,&array);CHKERRQ(ierr);
      } else {
        PetscScalar *array;
        ierr = VecGetArray(gvec,&array);CHKERRQ(ierr);
        ierr = VecPlaceArray(*vec,array+next->rstart);CHKERRQ(ierr);
        ierr = VecRestoreArray(gvec,&array);CHKERRQ(ierr);
      }
    }
    next = next->next;
  }
  va_end(Argp);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMCompositeGetAccessArray"
/*@C
    DMCompositeGetAccessArray - Allows one to access the individual packed vectors in their global
       representation.

    Collective on DMComposite

    Input Parameters:
+    dm - the packer object
.    pvec - packed vector
.    nwanted - number of vectors wanted
-    wanted - sorted array of vectors wanted, or NULL to get all vectors

    Output Parameters:
.    vecs - array of requested global vectors (must be allocated)

    Notes: Use DMCompositeRestoreAccessArray() to return the vectors when you no longer need them

    Level: advanced

.seealso: DMCompositeGetAccess(), DMCompositeGetEntries(), DMCompositeScatter(), DMCompositeGather()
@*/
PetscErrorCode  DMCompositeGetAccessArray(DM dm,Vec pvec,PetscInt nwanted,const PetscInt *wanted,Vec *vecs)
{
  PetscErrorCode         ierr;
  struct DMCompositeLink *link;
  PetscInt               i,wnum;
  DM_Composite           *com = (DM_Composite*)dm->data;
  PetscInt               readonly;
  
  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  PetscValidHeaderSpecific(pvec,VEC_CLASSID,2);
  if (!com->setup) {
    ierr = DMSetUp(dm);CHKERRQ(ierr);
  }

  ierr = VecLockGet(pvec,&readonly);CHKERRQ(ierr);
  for (i=0,wnum=0,link=com->next; link && wnum<nwanted; i++,link=link->next) {
    if (!wanted || i == wanted[wnum]) {
      Vec v;
      ierr = DMGetGlobalVector(link->dm,&v);CHKERRQ(ierr);
      if (readonly) {
        const PetscScalar *array;
        ierr = VecGetArrayRead(pvec,&array);CHKERRQ(ierr);
        ierr = VecPlaceArray(v,array+link->rstart);CHKERRQ(ierr);
        ierr = VecLockPush(v);CHKERRQ(ierr);
        ierr = VecRestoreArrayRead(pvec,&array);CHKERRQ(ierr);
      } else {
        PetscScalar *array;
        ierr = VecGetArray(pvec,&array);CHKERRQ(ierr);
        ierr = VecPlaceArray(v,array+link->rstart);CHKERRQ(ierr);
        ierr = VecRestoreArray(pvec,&array);CHKERRQ(ierr);
      }
      vecs[wnum++] = v;
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMCompositeRestoreAccess"
/*@C
    DMCompositeRestoreAccess - Returns the vectors obtained with DMCompositeGetAccess()
       representation.

    Collective on DMComposite

    Input Parameters:
+    dm - the packer object
.    gvec - the global vector
-    Vec* ... - the individual parallel vectors, NULL for those that are not needed

    Level: advanced

.seealso  DMCompositeAddDM(), DMCreateGlobalVector(),
         DMCompositeGather(), DMCompositeCreate(), DMCompositeGetISLocalToGlobalMappings(), DMCompositeScatter(),
         DMCompositeRestoreAccess(), DMCompositeGetAccess()

@*/
PetscErrorCode  DMCompositeRestoreAccess(DM dm,Vec gvec,...)
{
  va_list                Argp;
  PetscErrorCode         ierr;
  struct DMCompositeLink *next;
  DM_Composite           *com = (DM_Composite*)dm->data;
  PetscInt               readonly;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  PetscValidHeaderSpecific(gvec,VEC_CLASSID,2);
  next = com->next;
  if (!com->setup) {
    ierr = DMSetUp(dm);CHKERRQ(ierr);
  }

  ierr = VecLockGet(gvec,&readonly);CHKERRQ(ierr);
  /* loop over packed objects, handling one at at time */
  va_start(Argp,gvec);
  while (next) {
    Vec *vec;
    vec = va_arg(Argp, Vec*);
    if (vec) {
      ierr = VecResetArray(*vec);CHKERRQ(ierr);
      if (readonly) {
        ierr = VecLockPop(*vec);CHKERRQ(ierr);
      }
      ierr = DMRestoreGlobalVector(next->dm,vec);CHKERRQ(ierr);
    }
    next = next->next;
  }
  va_end(Argp);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMCompositeRestoreAccessArray"
/*@C
    DMCompositeRestoreAccessArray - Returns the vectors obtained with DMCompositeGetAccessArray()

    Collective on DMComposite

    Input Parameters:
+    dm - the packer object
.    pvec - packed vector
.    nwanted - number of vectors wanted
.    wanted - sorted array of vectors wanted, or NULL to get all vectors
-    vecs - array of global vectors to return

    Level: advanced

.seealso: DMCompositeRestoreAccess(), DMCompositeRestoreEntries(), DMCompositeScatter(), DMCompositeGather()
@*/
PetscErrorCode  DMCompositeRestoreAccessArray(DM dm,Vec pvec,PetscInt nwanted,const PetscInt *wanted,Vec *vecs)
{
  PetscErrorCode         ierr;
  struct DMCompositeLink *link;
  PetscInt               i,wnum;
  DM_Composite           *com = (DM_Composite*)dm->data;
  PetscInt               readonly;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  PetscValidHeaderSpecific(pvec,VEC_CLASSID,2);
  if (!com->setup) {
    ierr = DMSetUp(dm);CHKERRQ(ierr);
  }

  ierr = VecLockGet(pvec,&readonly);CHKERRQ(ierr);
  for (i=0,wnum=0,link=com->next; link && wnum<nwanted; i++,link=link->next) {
    if (!wanted || i == wanted[wnum]) {
      ierr = VecResetArray(vecs[wnum]);CHKERRQ(ierr);
      if (readonly) {
        ierr = VecLockPop(vecs[wnum]);CHKERRQ(ierr);
      }
      ierr = DMRestoreGlobalVector(link->dm,&vecs[wnum]);CHKERRQ(ierr);
      wnum++;
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMCompositeScatter"
/*@C
    DMCompositeScatter - Scatters from a global packed vector into its individual local vectors

    Collective on DMComposite

    Input Parameters:
+    dm - the packer object
.    gvec - the global vector
-    Vec ... - the individual sequential vectors, NULL for those that are not needed

    Level: advanced

    Notes:
    DMCompositeScatterArray() is a non-variadic alternative that is often more convenient for library callers and is
    accessible from Fortran.

.seealso DMDestroy(), DMCompositeAddDM(), DMCreateGlobalVector(),
         DMCompositeGather(), DMCompositeCreate(), DMCompositeGetISLocalToGlobalMappings(), DMCompositeGetAccess(),
         DMCompositeGetLocalVectors(), DMCompositeRestoreLocalVectors(), DMCompositeGetEntries()
         DMCompositeScatterArray()

@*/
PetscErrorCode  DMCompositeScatter(DM dm,Vec gvec,...)
{
  va_list                Argp;
  PetscErrorCode         ierr;
  struct DMCompositeLink *next;
  PetscInt               cnt;
  DM_Composite           *com = (DM_Composite*)dm->data;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  PetscValidHeaderSpecific(gvec,VEC_CLASSID,2);
  if (!com->setup) {
    ierr = DMSetUp(dm);CHKERRQ(ierr);
  }

  /* loop over packed objects, handling one at at time */
  va_start(Argp,gvec);
  for (cnt=3,next=com->next; next; cnt++,next=next->next) {
    Vec local;
    local = va_arg(Argp, Vec);
    if (local) {
      Vec               global;
      const PetscScalar *array;
      PetscValidHeaderSpecific(local,VEC_CLASSID,cnt);
      ierr = DMGetGlobalVector(next->dm,&global);CHKERRQ(ierr);
      ierr = VecGetArrayRead(gvec,&array);CHKERRQ(ierr);
      ierr = VecPlaceArray(global,array+next->rstart);CHKERRQ(ierr);
      ierr = DMGlobalToLocalBegin(next->dm,global,INSERT_VALUES,local);CHKERRQ(ierr);
      ierr = DMGlobalToLocalEnd(next->dm,global,INSERT_VALUES,local);CHKERRQ(ierr);
      ierr = VecRestoreArrayRead(gvec,&array);CHKERRQ(ierr);
      ierr = VecResetArray(global);CHKERRQ(ierr);
      ierr = DMRestoreGlobalVector(next->dm,&global);CHKERRQ(ierr);
    }
  }
  va_end(Argp);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMCompositeScatterArray"
/*@
    DMCompositeScatterArray - Scatters from a global packed vector into its individual local vectors

    Collective on DMComposite

    Input Parameters:
+    dm - the packer object
.    gvec - the global vector
.    lvecs - array of local vectors, NULL for any that are not needed

    Level: advanced

    Note:
    This is a non-variadic alternative to DMCompositeScatter()

.seealso DMDestroy(), DMCompositeAddDM(), DMCreateGlobalVector()
         DMCompositeGather(), DMCompositeCreate(), DMCompositeGetISLocalToGlobalMappings(), DMCompositeGetAccess(),
         DMCompositeGetLocalVectors(), DMCompositeRestoreLocalVectors(), DMCompositeGetEntries()

@*/
PetscErrorCode  DMCompositeScatterArray(DM dm,Vec gvec,Vec *lvecs)
{
  PetscErrorCode         ierr;
  struct DMCompositeLink *next;
  PetscInt               i;
  DM_Composite           *com = (DM_Composite*)dm->data;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  PetscValidHeaderSpecific(gvec,VEC_CLASSID,2);
  if (!com->setup) {
    ierr = DMSetUp(dm);CHKERRQ(ierr);
  }

  /* loop over packed objects, handling one at at time */
  for (i=0,next=com->next; next; next=next->next,i++) {
    if (lvecs[i]) {
      Vec         global;
      const PetscScalar *array;
      PetscValidHeaderSpecific(lvecs[i],VEC_CLASSID,3);
      ierr = DMGetGlobalVector(next->dm,&global);CHKERRQ(ierr);
      ierr = VecGetArrayRead(gvec,&array);CHKERRQ(ierr);
      ierr = VecPlaceArray(global,(PetscScalar*)array+next->rstart);CHKERRQ(ierr);
      ierr = DMGlobalToLocalBegin(next->dm,global,INSERT_VALUES,lvecs[i]);CHKERRQ(ierr);
      ierr = DMGlobalToLocalEnd(next->dm,global,INSERT_VALUES,lvecs[i]);CHKERRQ(ierr);
      ierr = VecRestoreArrayRead(gvec,&array);CHKERRQ(ierr);
      ierr = VecResetArray(global);CHKERRQ(ierr);
      ierr = DMRestoreGlobalVector(next->dm,&global);CHKERRQ(ierr);
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMCompositeGather"
/*@C
    DMCompositeGather - Gathers into a global packed vector from its individual local vectors

    Collective on DMComposite

    Input Parameter:
+    dm - the packer object
.    gvec - the global vector
.    imode - INSERT_VALUES or ADD_VALUES
-    Vec ... - the individual sequential vectors, NULL for any that are not needed

    Level: advanced

.seealso DMDestroy(), DMCompositeAddDM(), DMCreateGlobalVector(),
         DMCompositeScatter(), DMCompositeCreate(), DMCompositeGetISLocalToGlobalMappings(), DMCompositeGetAccess(),
         DMCompositeGetLocalVectors(), DMCompositeRestoreLocalVectors(), DMCompositeGetEntries()

@*/
PetscErrorCode  DMCompositeGather(DM dm,Vec gvec,InsertMode imode,...)
{
  va_list                Argp;
  PetscErrorCode         ierr;
  struct DMCompositeLink *next;
  DM_Composite           *com = (DM_Composite*)dm->data;
  PetscInt               cnt;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  PetscValidHeaderSpecific(gvec,VEC_CLASSID,2);
  if (!com->setup) {
    ierr = DMSetUp(dm);CHKERRQ(ierr);
  }

  /* loop over packed objects, handling one at at time */
  va_start(Argp,imode);
  for (cnt=3,next=com->next; next; cnt++,next=next->next) {
    Vec local;
    local = va_arg(Argp, Vec);
    if (local) {
      PetscScalar *array;
      Vec         global;
      PetscValidHeaderSpecific(local,VEC_CLASSID,cnt);
      ierr = DMGetGlobalVector(next->dm,&global);CHKERRQ(ierr);
      ierr = VecGetArray(gvec,&array);CHKERRQ(ierr);
      ierr = VecPlaceArray(global,array+next->rstart);CHKERRQ(ierr);
      ierr = DMLocalToGlobalBegin(next->dm,local,imode,global);CHKERRQ(ierr);
      ierr = DMLocalToGlobalEnd(next->dm,local,imode,global);CHKERRQ(ierr);
      ierr = VecRestoreArray(gvec,&array);CHKERRQ(ierr);
      ierr = VecResetArray(global);CHKERRQ(ierr);
      ierr = DMRestoreGlobalVector(next->dm,&global);CHKERRQ(ierr);
    }
  }
  va_end(Argp);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMCompositeGatherArray"
/*@
    DMCompositeGatherArray - Gathers into a global packed vector from its individual local vectors

    Collective on DMComposite

    Input Parameter:
+    dm - the packer object
.    gvec - the global vector
.    imode - INSERT_VALUES or ADD_VALUES
-    lvecs - the individual sequential vectors, NULL for any that are not needed

    Level: advanced

    Notes:
    This is a non-variadic alternative to DMCompositeGather().

.seealso DMDestroy(), DMCompositeAddDM(), DMCreateGlobalVector(),
         DMCompositeScatter(), DMCompositeCreate(), DMCompositeGetISLocalToGlobalMappings(), DMCompositeGetAccess(),
         DMCompositeGetLocalVectors(), DMCompositeRestoreLocalVectors(), DMCompositeGetEntries(),
@*/
PetscErrorCode  DMCompositeGatherArray(DM dm,Vec gvec,InsertMode imode,Vec *lvecs)
{
  PetscErrorCode         ierr;
  struct DMCompositeLink *next;
  DM_Composite           *com = (DM_Composite*)dm->data;
  PetscInt               i;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  PetscValidHeaderSpecific(gvec,VEC_CLASSID,2);
  if (!com->setup) {
    ierr = DMSetUp(dm);CHKERRQ(ierr);
  }

  /* loop over packed objects, handling one at at time */
  for (next=com->next,i=0; next; next=next->next,i++) {
    if (lvecs[i]) {
      PetscScalar *array;
      Vec         global;
      PetscValidHeaderSpecific(lvecs[i],VEC_CLASSID,3);
      ierr = DMGetGlobalVector(next->dm,&global);CHKERRQ(ierr);
      ierr = VecGetArray(gvec,&array);CHKERRQ(ierr);
      ierr = VecPlaceArray(global,array+next->rstart);CHKERRQ(ierr);
      ierr = DMLocalToGlobalBegin(next->dm,lvecs[i],imode,global);CHKERRQ(ierr);
      ierr = DMLocalToGlobalEnd(next->dm,lvecs[i],imode,global);CHKERRQ(ierr);
      ierr = VecRestoreArray(gvec,&array);CHKERRQ(ierr);
      ierr = VecResetArray(global);CHKERRQ(ierr);
      ierr = DMRestoreGlobalVector(next->dm,&global);CHKERRQ(ierr);
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMCompositeAddDM"
/*@C
    DMCompositeAddDM - adds a DM  vector to a DMComposite

    Collective on DMComposite

    Input Parameter:
+    dm - the packer object
-    dm - the DM object, if the DM is a da you will need to caste it with a (DM)

    Level: advanced

.seealso DMDestroy(), DMCompositeGather(), DMCompositeAddDM(), DMCreateGlobalVector(),
         DMCompositeScatter(), DMCompositeCreate(), DMCompositeGetISLocalToGlobalMappings(), DMCompositeGetAccess(),
         DMCompositeGetLocalVectors(), DMCompositeRestoreLocalVectors(), DMCompositeGetEntries()

@*/
PetscErrorCode  DMCompositeAddDM(DM dmc,DM dm)
{
  PetscErrorCode         ierr;
  PetscInt               n,nlocal;
  struct DMCompositeLink *mine,*next;
  Vec                    global,local;
  DM_Composite           *com = (DM_Composite*)dmc->data;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dmc,DM_CLASSID,1);
  PetscValidHeaderSpecific(dm,DM_CLASSID,2);
  next = com->next;
  if (com->setup) SETERRQ(PetscObjectComm((PetscObject)dmc),PETSC_ERR_ARG_WRONGSTATE,"Cannot add a DM once you have used the DMComposite");

  /* create new link */
  ierr = PetscNew(&mine);CHKERRQ(ierr);
  ierr = PetscObjectReference((PetscObject)dm);CHKERRQ(ierr);
  ierr = DMGetGlobalVector(dm,&global);CHKERRQ(ierr);
  ierr = VecGetLocalSize(global,&n);CHKERRQ(ierr);
  ierr = DMRestoreGlobalVector(dm,&global);CHKERRQ(ierr);
  ierr = DMGetLocalVector(dm,&local);CHKERRQ(ierr);
  ierr = VecGetSize(local,&nlocal);CHKERRQ(ierr);
  ierr = DMRestoreLocalVector(dm,&local);CHKERRQ(ierr);

  mine->n      = n;
  mine->nlocal = nlocal;
  mine->dm     = dm;
  mine->next   = NULL;
  com->n      += n;

  /* add to end of list */
  if (!next) com->next = mine;
  else {
    while (next->next) next = next->next;
    next->next = mine;
  }
  com->nDM++;
  com->nmine++;
  PetscFunctionReturn(0);
}

#include <petscdraw.h>
PETSC_EXTERN PetscErrorCode  VecView_MPI(Vec,PetscViewer);
#undef __FUNCT__
#define __FUNCT__ "VecView_DMComposite"
PetscErrorCode  VecView_DMComposite(Vec gvec,PetscViewer viewer)
{
  DM                     dm;
  PetscErrorCode         ierr;
  struct DMCompositeLink *next;
  PetscBool              isdraw;
  DM_Composite           *com;

  PetscFunctionBegin;
  ierr = VecGetDM(gvec, &dm);CHKERRQ(ierr);
  if (!dm) SETERRQ(PetscObjectComm((PetscObject)gvec),PETSC_ERR_ARG_WRONG,"Vector not generated from a DMComposite");
  com  = (DM_Composite*)dm->data;
  next = com->next;

  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERDRAW,&isdraw);CHKERRQ(ierr);
  if (!isdraw) {
    /* do I really want to call this? */
    ierr = VecView_MPI(gvec,viewer);CHKERRQ(ierr);
  } else {
    PetscInt cnt = 0;

    /* loop over packed objects, handling one at at time */
    while (next) {
      Vec         vec;
      PetscScalar *array;
      PetscInt    bs;

      /* Should use VecGetSubVector() eventually, but would need to forward the DM for that to work */
      ierr = DMGetGlobalVector(next->dm,&vec);CHKERRQ(ierr);
      ierr = VecGetArray(gvec,&array);CHKERRQ(ierr);
      ierr = VecPlaceArray(vec,array+next->rstart);CHKERRQ(ierr);
      ierr = VecRestoreArray(gvec,&array);CHKERRQ(ierr);
      ierr = VecView(vec,viewer);CHKERRQ(ierr);
      ierr = VecGetBlockSize(vec,&bs);CHKERRQ(ierr);
      ierr = VecResetArray(vec);CHKERRQ(ierr);
      ierr = DMRestoreGlobalVector(next->dm,&vec);CHKERRQ(ierr);
      ierr = PetscViewerDrawBaseAdd(viewer,bs);CHKERRQ(ierr);
      cnt += bs;
      next = next->next;
    }
    ierr = PetscViewerDrawBaseAdd(viewer,-cnt);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMCreateGlobalVector_Composite"
PetscErrorCode  DMCreateGlobalVector_Composite(DM dm,Vec *gvec)
{
  PetscErrorCode ierr;
  DM_Composite   *com = (DM_Composite*)dm->data;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  ierr = DMSetUp(dm);CHKERRQ(ierr);
  ierr = VecCreateMPI(PetscObjectComm((PetscObject)dm),com->n,com->N,gvec);CHKERRQ(ierr);
  ierr = VecSetDM(*gvec, dm);CHKERRQ(ierr);
  ierr = VecSetOperation(*gvec,VECOP_VIEW,(void (*)(void))VecView_DMComposite);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMCreateLocalVector_Composite"
PetscErrorCode  DMCreateLocalVector_Composite(DM dm,Vec *lvec)
{
  PetscErrorCode ierr;
  DM_Composite   *com = (DM_Composite*)dm->data;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  if (!com->setup) {
    ierr = DMSetUp(dm);CHKERRQ(ierr);
  }
  ierr = VecCreateSeq(PetscObjectComm((PetscObject)dm),com->nghost,lvec);CHKERRQ(ierr);
  ierr = VecSetDM(*lvec, dm);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMCompositeGetISLocalToGlobalMappings"
/*@C
    DMCompositeGetISLocalToGlobalMappings - gets an ISLocalToGlobalMapping for each DM in the DMComposite, maps to the composite global space

    Collective on DM

    Input Parameter:
.    dm - the packer object

    Output Parameters:
.    ltogs - the individual mappings for each packed vector. Note that this includes
           all the ghost points that individual ghosted DMDA's may have.

    Level: advanced

    Notes:
       Each entry of ltogs should be destroyed with ISLocalToGlobalMappingDestroy(), the ltogs array should be freed with PetscFree().

.seealso DMDestroy(), DMCompositeAddDM(), DMCreateGlobalVector(),
         DMCompositeGather(), DMCompositeCreate(), DMCompositeGetAccess(), DMCompositeScatter(),
         DMCompositeGetLocalVectors(), DMCompositeRestoreLocalVectors(),DMCompositeGetEntries()

@*/
PetscErrorCode  DMCompositeGetISLocalToGlobalMappings(DM dm,ISLocalToGlobalMapping **ltogs)
{
  PetscErrorCode         ierr;
  PetscInt               i,*idx,n,cnt;
  struct DMCompositeLink *next;
  PetscMPIInt            rank;
  DM_Composite           *com = (DM_Composite*)dm->data;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  ierr = DMSetUp(dm);CHKERRQ(ierr);
  ierr = PetscMalloc1(com->nDM,ltogs);CHKERRQ(ierr);
  next = com->next;
  ierr = MPI_Comm_rank(PetscObjectComm((PetscObject)dm),&rank);CHKERRQ(ierr);

  /* loop over packed objects, handling one at at time */
  cnt = 0;
  while (next) {
    ISLocalToGlobalMapping ltog;
    PetscMPIInt            size;
    const PetscInt         *suboff,*indices;
    Vec                    global;

    /* Get sub-DM global indices for each local dof */
    ierr = DMGetLocalToGlobalMapping(next->dm,&ltog);CHKERRQ(ierr);
    ierr = ISLocalToGlobalMappingGetSize(ltog,&n);CHKERRQ(ierr);
    ierr = ISLocalToGlobalMappingGetIndices(ltog,&indices);CHKERRQ(ierr);
    ierr = PetscMalloc1(n,&idx);CHKERRQ(ierr);

    /* Get the offsets for the sub-DM global vector */
    ierr = DMGetGlobalVector(next->dm,&global);CHKERRQ(ierr);
    ierr = VecGetOwnershipRanges(global,&suboff);CHKERRQ(ierr);
    ierr = MPI_Comm_size(PetscObjectComm((PetscObject)global),&size);CHKERRQ(ierr);

    /* Shift the sub-DM definition of the global space to the composite global space */
    for (i=0; i<n; i++) {
      PetscInt subi = indices[i],lo = 0,hi = size,t;
      /* Binary search to find which rank owns subi */
      while (hi-lo > 1) {
        t = lo + (hi-lo)/2;
        if (suboff[t] > subi) hi = t;
        else                  lo = t;
      }
      idx[i] = subi - suboff[lo] + next->grstarts[lo];
    }
    ierr = ISLocalToGlobalMappingRestoreIndices(ltog,&indices);CHKERRQ(ierr);
    ierr = ISLocalToGlobalMappingCreate(PetscObjectComm((PetscObject)dm),1,n,idx,PETSC_OWN_POINTER,&(*ltogs)[cnt]);CHKERRQ(ierr);
    ierr = DMRestoreGlobalVector(next->dm,&global);CHKERRQ(ierr);
    next = next->next;
    cnt++;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMCompositeGetLocalISs"
/*@C
   DMCompositeGetLocalISs - Gets index sets for each component of a composite local vector

   Not Collective

   Input Arguments:
. dm - composite DM

   Output Arguments:
. is - array of serial index sets for each each component of the DMComposite

   Level: intermediate

   Notes:
   At present, a composite local vector does not normally exist.  This function is used to provide index sets for
   MatGetLocalSubMatrix().  In the future, the scatters for each entry in the DMComposite may be be merged into a single
   scatter to a composite local vector.  The user should not typically need to know which is being done.

   To get the composite global indices at all local points (including ghosts), use DMCompositeGetISLocalToGlobalMappings().

   To get index sets for pieces of the composite global vector, use DMCompositeGetGlobalISs().

   Each returned IS should be destroyed with ISDestroy(), the array should be freed with PetscFree().

.seealso: DMCompositeGetGlobalISs(), DMCompositeGetISLocalToGlobalMappings(), MatGetLocalSubMatrix(), MatCreateLocalRef()
@*/
PetscErrorCode  DMCompositeGetLocalISs(DM dm,IS **is)
{
  PetscErrorCode         ierr;
  DM_Composite           *com = (DM_Composite*)dm->data;
  struct DMCompositeLink *link;
  PetscInt               cnt,start;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  PetscValidPointer(is,2);
  ierr = PetscMalloc1(com->nmine,is);CHKERRQ(ierr);
  for (cnt=0,start=0,link=com->next; link; start+=link->nlocal,cnt++,link=link->next) {
    PetscInt bs;
    ierr = ISCreateStride(PETSC_COMM_SELF,link->nlocal,start,1,&(*is)[cnt]);CHKERRQ(ierr);
    ierr = DMGetBlockSize(link->dm,&bs);CHKERRQ(ierr);
    ierr = ISSetBlockSize((*is)[cnt],bs);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMCompositeGetGlobalISs"
/*@C
    DMCompositeGetGlobalISs - Gets the index sets for each composed object

    Collective on DMComposite

    Input Parameter:
.    dm - the packer object

    Output Parameters:
.    is - the array of index sets

    Level: advanced

    Notes:
       The is entries should be destroyed with ISDestroy(), the is array should be freed with PetscFree()

       These could be used to extract a subset of vector entries for a "multi-physics" preconditioner

       Use DMCompositeGetLocalISs() for index sets in the packed local numbering, and
       DMCompositeGetISLocalToGlobalMappings() for to map local sub-DM (including ghost) indices to packed global
       indices.

    Fortran Notes:

       The output argument 'is' must be an allocated array of sufficient length, which can be learned using DMCompositeGetNumberDM().

.seealso DMDestroy(), DMCompositeAddDM(), DMCreateGlobalVector(),
         DMCompositeGather(), DMCompositeCreate(), DMCompositeGetAccess(), DMCompositeScatter(),
         DMCompositeGetLocalVectors(), DMCompositeRestoreLocalVectors(),DMCompositeGetEntries()

@*/
PetscErrorCode  DMCompositeGetGlobalISs(DM dm,IS *is[])
{
  PetscErrorCode         ierr;
  PetscInt               cnt = 0;
  struct DMCompositeLink *next;
  PetscMPIInt            rank;
  DM_Composite           *com = (DM_Composite*)dm->data;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  ierr = PetscMalloc1(com->nDM,is);CHKERRQ(ierr);
  next = com->next;
  ierr = MPI_Comm_rank(PetscObjectComm((PetscObject)dm),&rank);CHKERRQ(ierr);

  /* loop over packed objects, handling one at at time */
  while (next) {
    ierr = ISCreateStride(PetscObjectComm((PetscObject)dm),next->n,next->grstart,1,&(*is)[cnt]);CHKERRQ(ierr);
    if (dm->prob) {
      MatNullSpace space;
      Mat          pmat;
      PetscObject  disc;
      PetscInt     Nf;

      ierr = PetscDSGetNumFields(dm->prob, &Nf);CHKERRQ(ierr);
      if (cnt < Nf) {
        ierr = PetscDSGetDiscretization(dm->prob, cnt, &disc);CHKERRQ(ierr);
        ierr = PetscObjectQuery(disc, "nullspace", (PetscObject*) &space);CHKERRQ(ierr);
        if (space) {ierr = PetscObjectCompose((PetscObject) (*is)[cnt], "nullspace", (PetscObject) space);CHKERRQ(ierr);}
        ierr = PetscObjectQuery(disc, "nearnullspace", (PetscObject*) &space);CHKERRQ(ierr);
        if (space) {ierr = PetscObjectCompose((PetscObject) (*is)[cnt], "nearnullspace", (PetscObject) space);CHKERRQ(ierr);}
        ierr = PetscObjectQuery(disc, "pmat", (PetscObject*) &pmat);CHKERRQ(ierr);
        if (pmat) {ierr = PetscObjectCompose((PetscObject) (*is)[cnt], "pmat", (PetscObject) pmat);CHKERRQ(ierr);}
      }
    }
    cnt++;
    next = next->next;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMCreateFieldIS_Composite"
PetscErrorCode DMCreateFieldIS_Composite(DM dm, PetscInt *numFields,char ***fieldNames, IS **fields)
{
  PetscInt       nDM;
  DM             *dms;
  PetscInt       i;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = DMCompositeGetNumberDM(dm, &nDM);CHKERRQ(ierr);
  if (numFields) *numFields = nDM;
  ierr = DMCompositeGetGlobalISs(dm, fields);CHKERRQ(ierr);
  if (fieldNames) {
    ierr = PetscMalloc1(nDM, &dms);CHKERRQ(ierr);
    ierr = PetscMalloc1(nDM, fieldNames);CHKERRQ(ierr);
    ierr = DMCompositeGetEntriesArray(dm, dms);CHKERRQ(ierr);
    for (i=0; i<nDM; i++) {
      char       buf[256];
      const char *splitname;

      /* Split naming precedence: object name, prefix, number */
      splitname = ((PetscObject) dm)->name;
      if (!splitname) {
        ierr = PetscObjectGetOptionsPrefix((PetscObject)dms[i],&splitname);CHKERRQ(ierr);
        if (splitname) {
          size_t len;
          ierr                 = PetscStrncpy(buf,splitname,sizeof(buf));CHKERRQ(ierr);
          buf[sizeof(buf) - 1] = 0;
          ierr                 = PetscStrlen(buf,&len);CHKERRQ(ierr);
          if (buf[len-1] == '_') buf[len-1] = 0; /* Remove trailing underscore if it was used */
          splitname = buf;
        }
      }
      if (!splitname) {
        ierr      = PetscSNPrintf(buf,sizeof(buf),"%D",i);CHKERRQ(ierr);
        splitname = buf;
      }
      ierr = PetscStrallocpy(splitname,&(*fieldNames)[i]);CHKERRQ(ierr);
    }
    ierr = PetscFree(dms);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

/*
 This could take over from DMCreateFieldIS(), as it is more general,
 making DMCreateFieldIS() a special case -- calling with dmlist == NULL;
 At this point it's probably best to be less intrusive, however.
 */
#undef __FUNCT__
#define __FUNCT__ "DMCreateFieldDecomposition_Composite"
PetscErrorCode DMCreateFieldDecomposition_Composite(DM dm, PetscInt *len,char ***namelist, IS **islist, DM **dmlist)
{
  PetscInt       nDM;
  PetscInt       i;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = DMCreateFieldIS_Composite(dm, len, namelist, islist);CHKERRQ(ierr);
  if (dmlist) {
    ierr = DMCompositeGetNumberDM(dm, &nDM);CHKERRQ(ierr);
    ierr = PetscMalloc1(nDM, dmlist);CHKERRQ(ierr);
    ierr = DMCompositeGetEntriesArray(dm, *dmlist);CHKERRQ(ierr);
    for (i=0; i<nDM; i++) {
      ierr = PetscObjectReference((PetscObject)((*dmlist)[i]));CHKERRQ(ierr);
    }
  }
  PetscFunctionReturn(0);
}



/* -------------------------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "DMCompositeGetLocalVectors"
/*@C
    DMCompositeGetLocalVectors - Gets local vectors for each part of a DMComposite.
       Use DMCompositeRestoreLocalVectors() to return them.

    Not Collective

    Input Parameter:
.    dm - the packer object

    Output Parameter:
.   Vec ... - the individual sequential Vecs

    Level: advanced

.seealso DMDestroy(), DMCompositeAddDM(), DMCreateGlobalVector(),
         DMCompositeGather(), DMCompositeCreate(), DMCompositeGetISLocalToGlobalMappings(), DMCompositeGetAccess(),
         DMCompositeRestoreLocalVectors(), DMCompositeScatter(), DMCompositeGetEntries()

@*/
PetscErrorCode  DMCompositeGetLocalVectors(DM dm,...)
{
  va_list                Argp;
  PetscErrorCode         ierr;
  struct DMCompositeLink *next;
  DM_Composite           *com = (DM_Composite*)dm->data;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  next = com->next;
  /* loop over packed objects, handling one at at time */
  va_start(Argp,dm);
  while (next) {
    Vec *vec;
    vec = va_arg(Argp, Vec*);
    if (vec) {ierr = DMGetLocalVector(next->dm,vec);CHKERRQ(ierr);}
    next = next->next;
  }
  va_end(Argp);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMCompositeRestoreLocalVectors"
/*@C
    DMCompositeRestoreLocalVectors - Restores local vectors for each part of a DMComposite.

    Not Collective

    Input Parameter:
.    dm - the packer object

    Output Parameter:
.   Vec ... - the individual sequential Vecs

    Level: advanced

.seealso DMDestroy(), DMCompositeAddDM(), DMCreateGlobalVector(),
         DMCompositeGather(), DMCompositeCreate(), DMCompositeGetISLocalToGlobalMappings(), DMCompositeGetAccess(),
         DMCompositeGetLocalVectors(), DMCompositeScatter(), DMCompositeGetEntries()

@*/
PetscErrorCode  DMCompositeRestoreLocalVectors(DM dm,...)
{
  va_list                Argp;
  PetscErrorCode         ierr;
  struct DMCompositeLink *next;
  DM_Composite           *com = (DM_Composite*)dm->data;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  next = com->next;
  /* loop over packed objects, handling one at at time */
  va_start(Argp,dm);
  while (next) {
    Vec *vec;
    vec = va_arg(Argp, Vec*);
    if (vec) {ierr = DMRestoreLocalVector(next->dm,vec);CHKERRQ(ierr);}
    next = next->next;
  }
  va_end(Argp);
  PetscFunctionReturn(0);
}

/* -------------------------------------------------------------------------------------*/
#undef __FUNCT__
#define __FUNCT__ "DMCompositeGetEntries"
/*@C
    DMCompositeGetEntries - Gets the DM for each entry in a DMComposite.

    Not Collective

    Input Parameter:
.    dm - the packer object

    Output Parameter:
.   DM ... - the individual entries (DMs)

    Level: advanced

.seealso DMDestroy(), DMCompositeAddDM(), DMCreateGlobalVector(), DMCompositeGetEntriesArray()
         DMCompositeGather(), DMCompositeCreate(), DMCompositeGetISLocalToGlobalMappings(), DMCompositeGetAccess(),
         DMCompositeRestoreLocalVectors(), DMCompositeGetLocalVectors(),  DMCompositeScatter(),
         DMCompositeGetLocalVectors(), DMCompositeRestoreLocalVectors()

@*/
PetscErrorCode  DMCompositeGetEntries(DM dm,...)
{
  va_list                Argp;
  struct DMCompositeLink *next;
  DM_Composite           *com = (DM_Composite*)dm->data;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  next = com->next;
  /* loop over packed objects, handling one at at time */
  va_start(Argp,dm);
  while (next) {
    DM *dmn;
    dmn = va_arg(Argp, DM*);
    if (dmn) *dmn = next->dm;
    next = next->next;
  }
  va_end(Argp);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMCompositeGetEntriesArray"
/*@C
    DMCompositeGetEntriesArray - Gets the DM for each entry in a DMComposite.

    Not Collective

    Input Parameter:
.    dm - the packer object

    Output Parameter:
.    dms - array of sufficient length (see DMCompositeGetNumberDM()) to hold the individual DMs

    Level: advanced

.seealso DMDestroy(), DMCompositeAddDM(), DMCreateGlobalVector(), DMCompositeGetEntries()
         DMCompositeGather(), DMCompositeCreate(), DMCompositeGetISLocalToGlobalMappings(), DMCompositeGetAccess(),
         DMCompositeRestoreLocalVectors(), DMCompositeGetLocalVectors(),  DMCompositeScatter(),
         DMCompositeGetLocalVectors(), DMCompositeRestoreLocalVectors()

@*/
PetscErrorCode DMCompositeGetEntriesArray(DM dm,DM dms[])
{
  struct DMCompositeLink *next;
  DM_Composite           *com = (DM_Composite*)dm->data;
  PetscInt               i;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  /* loop over packed objects, handling one at at time */
  for (next=com->next,i=0; next; next=next->next,i++) dms[i] = next->dm;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMRefine_Composite"
PetscErrorCode  DMRefine_Composite(DM dmi,MPI_Comm comm,DM *fine)
{
  PetscErrorCode         ierr;
  struct DMCompositeLink *next;
  DM_Composite           *com = (DM_Composite*)dmi->data;
  DM                     dm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dmi,DM_CLASSID,1);
  if (comm == MPI_COMM_NULL) {
    ierr = PetscObjectGetComm((PetscObject)dmi,&comm);CHKERRQ(ierr);
  }
  ierr = DMSetUp(dmi);CHKERRQ(ierr);
  next = com->next;
  ierr = DMCompositeCreate(comm,fine);CHKERRQ(ierr);

  /* loop over packed objects, handling one at at time */
  while (next) {
    ierr = DMRefine(next->dm,comm,&dm);CHKERRQ(ierr);
    ierr = DMCompositeAddDM(*fine,dm);CHKERRQ(ierr);
    ierr = PetscObjectDereference((PetscObject)dm);CHKERRQ(ierr);
    next = next->next;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMCoarsen_Composite"
PetscErrorCode  DMCoarsen_Composite(DM dmi,MPI_Comm comm,DM *fine)
{
  PetscErrorCode         ierr;
  struct DMCompositeLink *next;
  DM_Composite           *com = (DM_Composite*)dmi->data;
  DM                     dm;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dmi,DM_CLASSID,1);
  ierr = DMSetUp(dmi);CHKERRQ(ierr);
  if (comm == MPI_COMM_NULL) {
    ierr = PetscObjectGetComm((PetscObject)dmi,&comm);CHKERRQ(ierr);
  }
  next = com->next;
  ierr = DMCompositeCreate(comm,fine);CHKERRQ(ierr);

  /* loop over packed objects, handling one at at time */
  while (next) {
    ierr = DMCoarsen(next->dm,comm,&dm);CHKERRQ(ierr);
    ierr = DMCompositeAddDM(*fine,dm);CHKERRQ(ierr);
    ierr = PetscObjectDereference((PetscObject)dm);CHKERRQ(ierr);
    next = next->next;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMCreateInterpolation_Composite"
PetscErrorCode  DMCreateInterpolation_Composite(DM coarse,DM fine,Mat *A,Vec *v)
{
  PetscErrorCode         ierr;
  PetscInt               m,n,M,N,nDM,i;
  struct DMCompositeLink *nextc;
  struct DMCompositeLink *nextf;
  Vec                    gcoarse,gfine,*vecs;
  DM_Composite           *comcoarse = (DM_Composite*)coarse->data;
  DM_Composite           *comfine   = (DM_Composite*)fine->data;
  Mat                    *mats;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(coarse,DM_CLASSID,1);
  PetscValidHeaderSpecific(fine,DM_CLASSID,2);
  ierr = DMSetUp(coarse);CHKERRQ(ierr);
  ierr = DMSetUp(fine);CHKERRQ(ierr);
  /* use global vectors only for determining matrix layout */
  ierr = DMGetGlobalVector(coarse,&gcoarse);CHKERRQ(ierr);
  ierr = DMGetGlobalVector(fine,&gfine);CHKERRQ(ierr);
  ierr = VecGetLocalSize(gcoarse,&n);CHKERRQ(ierr);
  ierr = VecGetLocalSize(gfine,&m);CHKERRQ(ierr);
  ierr = VecGetSize(gcoarse,&N);CHKERRQ(ierr);
  ierr = VecGetSize(gfine,&M);CHKERRQ(ierr);
  ierr = DMRestoreGlobalVector(coarse,&gcoarse);CHKERRQ(ierr);
  ierr = DMRestoreGlobalVector(fine,&gfine);CHKERRQ(ierr);

  nDM = comfine->nDM;
  if (nDM != comcoarse->nDM) SETERRQ2(PetscObjectComm((PetscObject)fine),PETSC_ERR_ARG_INCOMP,"Fine DMComposite has %D entries, but coarse has %D",nDM,comcoarse->nDM);
  ierr = PetscCalloc1(nDM*nDM,&mats);CHKERRQ(ierr);
  if (v) {
    ierr = PetscCalloc1(nDM,&vecs);CHKERRQ(ierr);
  }

  /* loop over packed objects, handling one at at time */
  for (nextc=comcoarse->next,nextf=comfine->next,i=0; nextc; nextc=nextc->next,nextf=nextf->next,i++) {
    if (!v) {
      ierr = DMCreateInterpolation(nextc->dm,nextf->dm,&mats[i*nDM+i],NULL);CHKERRQ(ierr);
    } else {
      ierr = DMCreateInterpolation(nextc->dm,nextf->dm,&mats[i*nDM+i],&vecs[i]);CHKERRQ(ierr);
    }
  }
  ierr = MatCreateNest(PetscObjectComm((PetscObject)fine),nDM,NULL,nDM,NULL,mats,A);CHKERRQ(ierr);
  if (v) {
    ierr = VecCreateNest(PetscObjectComm((PetscObject)fine),nDM,NULL,vecs,v);CHKERRQ(ierr);
  }
  for (i=0; i<nDM*nDM; i++) {ierr = MatDestroy(&mats[i]);CHKERRQ(ierr);}
  ierr = PetscFree(mats);CHKERRQ(ierr);
  if (v) {
    for (i=0; i<nDM; i++) {ierr = VecDestroy(&vecs[i]);CHKERRQ(ierr);}
    ierr = PetscFree(vecs);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMGetLocalToGlobalMapping_Composite"
static PetscErrorCode DMGetLocalToGlobalMapping_Composite(DM dm)
{
  DM_Composite           *com = (DM_Composite*)dm->data;
  ISLocalToGlobalMapping *ltogs;
  PetscInt               i;
  PetscErrorCode         ierr;

  PetscFunctionBegin;
  /* Set the ISLocalToGlobalMapping on the new matrix */
  ierr = DMCompositeGetISLocalToGlobalMappings(dm,&ltogs);CHKERRQ(ierr);
  ierr = ISLocalToGlobalMappingConcatenate(PetscObjectComm((PetscObject)dm),com->nDM,ltogs,&dm->ltogmap);CHKERRQ(ierr);
  for (i=0; i<com->nDM; i++) {ierr = ISLocalToGlobalMappingDestroy(&ltogs[i]);CHKERRQ(ierr);}
  ierr = PetscFree(ltogs);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "DMCreateColoring_Composite"
PetscErrorCode  DMCreateColoring_Composite(DM dm,ISColoringType ctype,ISColoring *coloring)
{
  PetscErrorCode  ierr;
  PetscInt        n,i,cnt;
  ISColoringValue *colors;
  PetscBool       dense  = PETSC_FALSE;
  ISColoringValue maxcol = 0;
  DM_Composite    *com   = (DM_Composite*)dm->data;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  if (ctype == IS_COLORING_GHOSTED) SETERRQ(PetscObjectComm((PetscObject)dm),PETSC_ERR_SUP,"Only global coloring supported");
  else if (ctype == IS_COLORING_GLOBAL) {
    n = com->n;
  } else SETERRQ(PetscObjectComm((PetscObject)dm),PETSC_ERR_ARG_OUTOFRANGE,"Unknown ISColoringType");
  ierr = PetscMalloc1(n,&colors);CHKERRQ(ierr); /* freed in ISColoringDestroy() */

  ierr = PetscOptionsGetBool(((PetscObject)dm)->options,((PetscObject)dm)->prefix,"-dmcomposite_dense_jacobian",&dense,NULL);CHKERRQ(ierr);
  if (dense) {
    for (i=0; i<n; i++) {
      colors[i] = (ISColoringValue)(com->rstart + i);
    }
    maxcol = com->N;
  } else {
    struct DMCompositeLink *next = com->next;
    PetscMPIInt            rank;

    ierr = MPI_Comm_rank(PetscObjectComm((PetscObject)dm),&rank);CHKERRQ(ierr);
    cnt  = 0;
    while (next) {
      ISColoring lcoloring;

      ierr = DMCreateColoring(next->dm,IS_COLORING_GLOBAL,&lcoloring);CHKERRQ(ierr);
      for (i=0; i<lcoloring->N; i++) {
        colors[cnt++] = maxcol + lcoloring->colors[i];
      }
      maxcol += lcoloring->n;
      ierr    = ISColoringDestroy(&lcoloring);CHKERRQ(ierr);
      next    = next->next;
    }
  }
  ierr = ISColoringCreate(PetscObjectComm((PetscObject)dm),maxcol,n,colors,PETSC_OWN_POINTER,coloring);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMGlobalToLocalBegin_Composite"
PetscErrorCode  DMGlobalToLocalBegin_Composite(DM dm,Vec gvec,InsertMode mode,Vec lvec)
{
  PetscErrorCode         ierr;
  struct DMCompositeLink *next;
  PetscInt               cnt = 3;
  PetscMPIInt            rank;
  PetscScalar            *garray,*larray;
  DM_Composite           *com = (DM_Composite*)dm->data;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  PetscValidHeaderSpecific(gvec,VEC_CLASSID,2);
  next = com->next;
  if (!com->setup) {
    ierr = DMSetUp(dm);CHKERRQ(ierr);
  }
  ierr = MPI_Comm_rank(PetscObjectComm((PetscObject)dm),&rank);CHKERRQ(ierr);
  ierr = VecGetArray(gvec,&garray);CHKERRQ(ierr);
  ierr = VecGetArray(lvec,&larray);CHKERRQ(ierr);

  /* loop over packed objects, handling one at at time */
  while (next) {
    Vec      local,global;
    PetscInt N;

    ierr = DMGetGlobalVector(next->dm,&global);CHKERRQ(ierr);
    ierr = VecGetLocalSize(global,&N);CHKERRQ(ierr);
    ierr = VecPlaceArray(global,garray);CHKERRQ(ierr);
    ierr = DMGetLocalVector(next->dm,&local);CHKERRQ(ierr);
    ierr = VecPlaceArray(local,larray);CHKERRQ(ierr);
    ierr = DMGlobalToLocalBegin(next->dm,global,mode,local);CHKERRQ(ierr);
    ierr = DMGlobalToLocalEnd(next->dm,global,mode,local);CHKERRQ(ierr);
    ierr = VecResetArray(global);CHKERRQ(ierr);
    ierr = VecResetArray(local);CHKERRQ(ierr);
    ierr = DMRestoreGlobalVector(next->dm,&global);CHKERRQ(ierr);
    ierr = DMRestoreGlobalVector(next->dm,&local);CHKERRQ(ierr);
    cnt++;
    larray += next->nlocal;
    next    = next->next;
  }

  ierr = VecRestoreArray(gvec,NULL);CHKERRQ(ierr);
  ierr = VecRestoreArray(lvec,NULL);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMGlobalToLocalEnd_Composite"
PetscErrorCode  DMGlobalToLocalEnd_Composite(DM dm,Vec gvec,InsertMode mode,Vec lvec)
{
  PetscFunctionBegin;
  PetscFunctionReturn(0);
}

/*MC
   DMCOMPOSITE = "composite" - A DM object that is used to manage data for a collection of DMs

  Level: intermediate

.seealso: DMType, DM, DMDACreate(), DMCreate(), DMSetType(), DMCompositeCreate()
M*/


#undef __FUNCT__
#define __FUNCT__ "DMCreate_Composite"
PETSC_EXTERN PetscErrorCode DMCreate_Composite(DM p)
{
  PetscErrorCode ierr;
  DM_Composite   *com;

  PetscFunctionBegin;
  ierr      = PetscNewLog(p,&com);CHKERRQ(ierr);
  p->data   = com;
  ierr      = PetscObjectChangeTypeName((PetscObject)p,"DMComposite");CHKERRQ(ierr);
  com->n    = 0;
  com->next = NULL;
  com->nDM  = 0;

  p->ops->createglobalvector              = DMCreateGlobalVector_Composite;
  p->ops->createlocalvector               = DMCreateLocalVector_Composite;
  p->ops->getlocaltoglobalmapping         = DMGetLocalToGlobalMapping_Composite;
  p->ops->createfieldis                   = DMCreateFieldIS_Composite;
  p->ops->createfielddecomposition        = DMCreateFieldDecomposition_Composite;
  p->ops->refine                          = DMRefine_Composite;
  p->ops->coarsen                         = DMCoarsen_Composite;
  p->ops->createinterpolation             = DMCreateInterpolation_Composite;
  p->ops->creatematrix                    = DMCreateMatrix_Composite;
  p->ops->getcoloring                     = DMCreateColoring_Composite;
  p->ops->globaltolocalbegin              = DMGlobalToLocalBegin_Composite;
  p->ops->globaltolocalend                = DMGlobalToLocalEnd_Composite;
  p->ops->destroy                         = DMDestroy_Composite;
  p->ops->view                            = DMView_Composite;
  p->ops->setup                           = DMSetUp_Composite;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMCompositeCreate"
/*@C
    DMCompositeCreate - Creates a vector packer, used to generate "composite"
      vectors made up of several subvectors.

    Collective on MPI_Comm

    Input Parameter:
.   comm - the processors that will share the global vector

    Output Parameters:
.   packer - the packer object

    Level: advanced

.seealso DMDestroy(), DMCompositeAddDM(), DMCompositeScatter(), DMCOMPOSITE,DMCreate()
         DMCompositeGather(), DMCreateGlobalVector(), DMCompositeGetISLocalToGlobalMappings(), DMCompositeGetAccess()
         DMCompositeGetLocalVectors(), DMCompositeRestoreLocalVectors(), DMCompositeGetEntries()

@*/
PetscErrorCode  DMCompositeCreate(MPI_Comm comm,DM *packer)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidPointer(packer,2);
  ierr = DMCreate(comm,packer);CHKERRQ(ierr);
  ierr = DMSetType(*packer,DMCOMPOSITE);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
