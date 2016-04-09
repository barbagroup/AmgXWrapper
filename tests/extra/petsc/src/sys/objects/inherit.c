
/*
     Provides utility routines for manipulating any type of PETSc object.
*/
#include <petsc/private/petscimpl.h>  /*I   "petscsys.h"    I*/
#include <petscviewer.h>

#if defined(PETSC_USE_LOG)
PetscObject *PetscObjects      = 0;
PetscInt    PetscObjectsCounts = 0, PetscObjectsMaxCounts = 0;
PetscBool   PetscObjectsLog    = PETSC_FALSE;
#endif

extern PetscErrorCode PetscObjectGetComm_Petsc(PetscObject,MPI_Comm*);
extern PetscErrorCode PetscObjectCompose_Petsc(PetscObject,const char[],PetscObject);
extern PetscErrorCode PetscObjectQuery_Petsc(PetscObject,const char[],PetscObject*);
extern PetscErrorCode PetscObjectComposeFunction_Petsc(PetscObject,const char[],void (*)(void));
extern PetscErrorCode PetscObjectQueryFunction_Petsc(PetscObject,const char[],void (**)(void));

#undef __FUNCT__
#define __FUNCT__ "PetscHeaderCreate_Private"
/*
   PetscHeaderCreate_Private - Creates a base PETSc object header and fills
   in the default values.  Called by the macro PetscHeaderCreate().
*/
PetscErrorCode  PetscHeaderCreate_Private(PetscObject h,PetscClassId classid,const char class_name[],const char descr[],const char mansec[],
                                          MPI_Comm comm,PetscObjectDestroyFunction destroy,PetscObjectViewFunction view)
{
  static PetscInt idcnt = 1;
  PetscErrorCode  ierr;
#if defined(PETSC_USE_LOG)
  PetscObject     *newPetscObjects;
  PetscInt         newPetscObjectsMaxCounts,i;
#endif

  PetscFunctionBegin;
  h->classid               = classid;
  h->type                  = 0;
  h->class_name            = (char*)class_name;
  h->description           = (char*)descr;
  h->mansec                = (char*)mansec;
  h->prefix                = 0;
  h->refct                 = 1;
#if defined(PETSC_HAVE_SAWS)
  h->amsmem                = PETSC_FALSE;
#endif
  h->id                    = idcnt++;
  h->parentid              = 0;
  h->qlist                 = 0;
  h->olist                 = 0;
  h->precision             = (PetscPrecision) sizeof(PetscReal);
  h->bops->destroy         = destroy;
  h->bops->view            = view;
  h->bops->getcomm         = PetscObjectGetComm_Petsc;
  h->bops->compose         = PetscObjectCompose_Petsc;
  h->bops->query           = PetscObjectQuery_Petsc;
  h->bops->composefunction = PetscObjectComposeFunction_Petsc;
  h->bops->queryfunction   = PetscObjectQueryFunction_Petsc;

  ierr = PetscCommDuplicate(comm,&h->comm,&h->tag);CHKERRQ(ierr);

#if defined(PETSC_USE_LOG)
  /* Keep a record of object created */
  if (PetscObjectsLog) {
    PetscObjectsCounts++;
    for (i=0; i<PetscObjectsMaxCounts; i++) {
      if (!PetscObjects[i]) {
        PetscObjects[i] = h;
        PetscFunctionReturn(0);
      }
    }
    /* Need to increase the space for storing PETSc objects */
    if (!PetscObjectsMaxCounts) newPetscObjectsMaxCounts = 100;
    else                        newPetscObjectsMaxCounts = 2*PetscObjectsMaxCounts;
    ierr = PetscMalloc1(newPetscObjectsMaxCounts,&newPetscObjects);CHKERRQ(ierr);
    ierr = PetscMemcpy(newPetscObjects,PetscObjects,PetscObjectsMaxCounts*sizeof(PetscObject));CHKERRQ(ierr);
    ierr = PetscMemzero(newPetscObjects+PetscObjectsMaxCounts,(newPetscObjectsMaxCounts - PetscObjectsMaxCounts)*sizeof(PetscObject));CHKERRQ(ierr);
    ierr = PetscFree(PetscObjects);CHKERRQ(ierr);

    PetscObjects                        = newPetscObjects;
    PetscObjects[PetscObjectsMaxCounts] = h;
    PetscObjectsMaxCounts               = newPetscObjectsMaxCounts;
  }
#endif
  PetscFunctionReturn(0);
}

extern PetscBool      PetscMemoryCollectMaximumUsage;
extern PetscLogDouble PetscMemoryMaximumUsage;

#undef __FUNCT__
#define __FUNCT__ "PetscHeaderDestroy_Private"
/*
    PetscHeaderDestroy_Private - Destroys a base PETSc object header. Called by
    the macro PetscHeaderDestroy().
*/
PetscErrorCode  PetscHeaderDestroy_Private(PetscObject h)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeader(h,1);
  ierr = PetscLogObjectDestroy(h);CHKERRQ(ierr);
  ierr = PetscComposedQuantitiesDestroy(h);CHKERRQ(ierr);
  if (PetscMemoryCollectMaximumUsage) {
    PetscLogDouble usage;
    ierr = PetscMemoryGetCurrentUsage(&usage);CHKERRQ(ierr);
    if (usage > PetscMemoryMaximumUsage) PetscMemoryMaximumUsage = usage;
  }
  /* first destroy things that could execute arbitrary code */
  if (h->python_destroy) {
    void           *python_context = h->python_context;
    PetscErrorCode (*python_destroy)(void*) = h->python_destroy;
    h->python_context = 0;
    h->python_destroy = 0;

    ierr = (*python_destroy)(python_context);CHKERRQ(ierr);
  }
  ierr = PetscObjectDestroyOptionsHandlers(h);CHKERRQ(ierr);
  ierr = PetscObjectListDestroy(&h->olist);CHKERRQ(ierr);
  ierr = PetscCommDestroy(&h->comm);CHKERRQ(ierr);
  /* next destroy other things */
  h->classid = PETSCFREEDHEADER;

  ierr = PetscFunctionListDestroy(&h->qlist);CHKERRQ(ierr);
  ierr = PetscFree(h->type_name);CHKERRQ(ierr);
  ierr = PetscFree(h->name);CHKERRQ(ierr);
  ierr = PetscFree(h->prefix);CHKERRQ(ierr);
  ierr = PetscFree(h->fortran_func_pointers);CHKERRQ(ierr);
  ierr = PetscFree(h->fortrancallback[PETSC_FORTRAN_CALLBACK_CLASS]);CHKERRQ(ierr);
  ierr = PetscFree(h->fortrancallback[PETSC_FORTRAN_CALLBACK_SUBTYPE]);CHKERRQ(ierr);

#if defined(PETSC_USE_LOG)
  if (PetscObjectsLog) {
    PetscInt i;
    /* Record object removal from list of all objects */
    for (i=0; i<PetscObjectsMaxCounts; i++) {
      if (PetscObjects[i] == h) {
        PetscObjects[i] = 0;
        PetscObjectsCounts--;
        break;
      }
    }
    if (!PetscObjectsCounts) {
      ierr = PetscFree(PetscObjects);CHKERRQ(ierr);
      PetscObjectsMaxCounts = 0;
    }
  }
#endif
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscObjectCopyFortranFunctionPointers"
/*@C
   PetscObjectCopyFortranFunctionPointers - Copy function pointers to another object

   Logically Collective on PetscObject

   Input Parameter:
+  src - source object
-  dest - destination object

   Level: developer

   Note:
   Both objects must have the same class.
@*/
PetscErrorCode PetscObjectCopyFortranFunctionPointers(PetscObject src,PetscObject dest)
{
  PetscErrorCode ierr;
  PetscInt       cbtype,numcb[PETSC_FORTRAN_CALLBACK_MAXTYPE];

  PetscFunctionBegin;
  PetscValidHeader(src,1);
  PetscValidHeader(dest,2);
  if (src->classid != dest->classid) SETERRQ(src->comm,PETSC_ERR_ARG_INCOMP,"Objects must be of the same class");

  ierr = PetscFree(dest->fortran_func_pointers);CHKERRQ(ierr);
  ierr = PetscMalloc(src->num_fortran_func_pointers*sizeof(void(*)(void)),&dest->fortran_func_pointers);CHKERRQ(ierr);
  ierr = PetscMemcpy(dest->fortran_func_pointers,src->fortran_func_pointers,src->num_fortran_func_pointers*sizeof(void(*)(void)));CHKERRQ(ierr);

  dest->num_fortran_func_pointers = src->num_fortran_func_pointers;

  ierr = PetscFortranCallbackGetSizes(src->classid,&numcb[PETSC_FORTRAN_CALLBACK_CLASS],&numcb[PETSC_FORTRAN_CALLBACK_SUBTYPE]);CHKERRQ(ierr);
  for (cbtype=PETSC_FORTRAN_CALLBACK_CLASS; cbtype<PETSC_FORTRAN_CALLBACK_MAXTYPE; cbtype++) {
    ierr = PetscFree(dest->fortrancallback[cbtype]);CHKERRQ(ierr);
    ierr = PetscCalloc1(numcb[cbtype],&dest->fortrancallback[cbtype]);CHKERRQ(ierr);
    ierr = PetscMemcpy(dest->fortrancallback[cbtype],src->fortrancallback[cbtype],src->num_fortrancallback[cbtype]*sizeof(PetscFortranCallback));CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscObjectSetFortranCallback"
/*@C
   PetscObjectSetFortranCallback - set fortran callback function pointer and context

   Logically Collective

   Input Arguments:
+  obj - object on which to set callback
.  cbtype - callback type (class or subtype)
.  cid - address of callback Id, updated if not yet initialized (zero)
.  func - Fortran function
-  ctx - Fortran context

   Level: developer

.seealso: PetscObjectGetFortranCallback()
@*/
PetscErrorCode PetscObjectSetFortranCallback(PetscObject obj,PetscFortranCallbackType cbtype,PetscFortranCallbackId *cid,void (*func)(void),void *ctx)
{
  PetscErrorCode ierr;
  const char     *subtype = NULL;

  PetscFunctionBegin;
  PetscValidHeader(obj,1);
  if (cbtype == PETSC_FORTRAN_CALLBACK_SUBTYPE) subtype = obj->type_name;
  if (!*cid) {ierr = PetscFortranCallbackRegister(obj->classid,subtype,cid);CHKERRQ(ierr);}
  if (*cid >= PETSC_SMALLEST_FORTRAN_CALLBACK+obj->num_fortrancallback[cbtype]) {
    PetscInt             oldnum = obj->num_fortrancallback[cbtype],newnum = PetscMax(1,2*oldnum);
    PetscFortranCallback *callback;
    ierr = PetscMalloc1(newnum,&callback);CHKERRQ(ierr);
    ierr = PetscMemcpy(callback,obj->fortrancallback[cbtype],oldnum*sizeof(*obj->fortrancallback[cbtype]));CHKERRQ(ierr);
    ierr = PetscFree(obj->fortrancallback[cbtype]);CHKERRQ(ierr);

    obj->fortrancallback[cbtype] = callback;
    obj->num_fortrancallback[cbtype] = newnum;
  }
  obj->fortrancallback[cbtype][*cid-PETSC_SMALLEST_FORTRAN_CALLBACK].func = func;
  obj->fortrancallback[cbtype][*cid-PETSC_SMALLEST_FORTRAN_CALLBACK].ctx = ctx;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscObjectGetFortranCallback"
/*@C
   PetscObjectGetFortranCallback - get fortran callback function pointer and context

   Logically Collective

   Input Arguments:
+  obj - object on which to get callback
.  cbtype - callback type
-  cid - address of callback Id

   Output Arguments:
+  func - Fortran function (or NULL if not needed)
-  ctx - Fortran context (or NULL if not needed)

   Level: developer

.seealso: PetscObjectSetFortranCallback()
@*/
PetscErrorCode PetscObjectGetFortranCallback(PetscObject obj,PetscFortranCallbackType cbtype,PetscFortranCallbackId cid,void (**func)(void),void **ctx)
{
  PetscFortranCallback *cb;

  PetscFunctionBegin;
  PetscValidHeader(obj,1);
  if (PetscUnlikely(cid < PETSC_SMALLEST_FORTRAN_CALLBACK)) SETERRQ(obj->comm,PETSC_ERR_ARG_CORRUPT,"Fortran callback Id invalid");
  if (PetscUnlikely(cid >= PETSC_SMALLEST_FORTRAN_CALLBACK+obj->num_fortrancallback[cbtype])) SETERRQ(obj->comm,PETSC_ERR_ARG_CORRUPT,"Fortran callback not set on this object");
  cb = &obj->fortrancallback[cbtype][cid-PETSC_SMALLEST_FORTRAN_CALLBACK];
  if (func) *func = cb->func;
  if (ctx) *ctx = cb->ctx;
  PetscFunctionReturn(0);
}

#if defined(PETSC_USE_LOG)
#undef __FUNCT__
#define __FUNCT__ "PetscObjectsDump"
/*@C
   PetscObjectsDump - Prints the currently existing objects.

   Logically Collective on PetscViewer

   Input Parameter:
+  fd - file pointer
-  all - by default only tries to display objects created explicitly by the user, if all is PETSC_TRUE then lists all outstanding objects

   Options Database:
.  -objects_dump <all>

   Level: advanced

   Concepts: options database^printing

@*/
PetscErrorCode  PetscObjectsDump(FILE *fd,PetscBool all)
{
  PetscErrorCode ierr;
  PetscInt       i;
#if defined(PETSC_USE_DEBUG)
  PetscInt       j,k=0;
#endif
  PetscObject    h;

  PetscFunctionBegin;
  if (PetscObjectsCounts) {
    ierr = PetscFPrintf(PETSC_COMM_WORLD,fd,"The following objects were never freed\n");CHKERRQ(ierr);
    ierr = PetscFPrintf(PETSC_COMM_WORLD,fd,"-----------------------------------------\n");CHKERRQ(ierr);
    for (i=0; i<PetscObjectsMaxCounts; i++) {
      if ((h = PetscObjects[i])) {
        ierr = PetscObjectName(h);CHKERRQ(ierr);
        {
#if defined(PETSC_USE_DEBUG)
        PetscStack *stack = 0;
        char       *create,*rclass;

        /* if the PETSc function the user calls is not a create then this object was NOT directly created by them */
        ierr = PetscMallocGetStack(h,&stack);CHKERRQ(ierr);
        if (stack) {
          k = stack->currentsize-2;
          if (!all) {
            k = 0;
            while (!stack->petscroutine[k]) k++;
            ierr = PetscStrstr(stack->function[k],"Create",&create);CHKERRQ(ierr);
            if (!create) {
              ierr = PetscStrstr(stack->function[k],"Get",&create);CHKERRQ(ierr);
            }
            ierr = PetscStrstr(stack->function[k],h->class_name,&rclass);CHKERRQ(ierr);
            if (!create) continue;
            if (!rclass) continue;
          }
        }
#endif

        ierr = PetscFPrintf(PETSC_COMM_WORLD,fd,"[%d] %s %s %s\n",PetscGlobalRank,h->class_name,h->type_name,h->name);CHKERRQ(ierr);

#if defined(PETSC_USE_DEBUG)
        ierr = PetscMallocGetStack(h,&stack);CHKERRQ(ierr);
        if (stack) {
          for (j=k; j>=0; j--) {
            fprintf(fd,"      [%d]  %s() in %s\n",PetscGlobalRank,stack->function[j],stack->file[j]);
          }
        }
#endif
        }
      }
    }
  }
  PetscFunctionReturn(0);
}
#endif

#if defined(PETSC_USE_LOG)

#undef __FUNCT__
#define __FUNCT__ "PetscObjectsView"
/*@C
   PetscObjectsView - Prints the currently existing objects.

   Logically Collective on PetscViewer

   Input Parameter:
.  viewer - must be an PETSCVIEWERASCII viewer

   Level: advanced

   Concepts: options database^printing

@*/
PetscErrorCode  PetscObjectsView(PetscViewer viewer)
{
  PetscErrorCode ierr;
  PetscBool      isascii;
  FILE           *fd;

  PetscFunctionBegin;
  if (!viewer) viewer = PETSC_VIEWER_STDOUT_WORLD;
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERASCII,&isascii);CHKERRQ(ierr);
  if (!isascii) SETERRQ(PetscObjectComm((PetscObject)viewer),PETSC_ERR_SUP,"Only supports ASCII viewer");
  ierr = PetscViewerASCIIGetPointer(viewer,&fd);CHKERRQ(ierr);
  ierr = PetscObjectsDump(fd,PETSC_TRUE);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscObjectsGetObject"
/*@C
   PetscObjectsGetObject - Get a pointer to a named object

   Not collective

   Input Parameter:
.  name - the name of an object

   Output Parameter:
.   obj - the object or null if there is no object

   Level: advanced

   Concepts: options database^printing

@*/
PetscErrorCode  PetscObjectsGetObject(const char *name,PetscObject *obj,char **classname)
{
  PetscErrorCode ierr;
  PetscInt       i;
  PetscObject    h;
  PetscBool      flg;

  PetscFunctionBegin;
  *obj = NULL;
  for (i=0; i<PetscObjectsMaxCounts; i++) {
    if ((h = PetscObjects[i])) {
      ierr = PetscObjectName(h);CHKERRQ(ierr);
      ierr = PetscStrcmp(h->name,name,&flg);CHKERRQ(ierr);
      if (flg) {
        *obj = h;
        if (classname) *classname = h->class_name;
        PetscFunctionReturn(0);
      }
    }
  }
  PetscFunctionReturn(0);
}
#endif

#undef __FUNCT__
#define __FUNCT__ "PetscObjectAddOptionsHandler"
/*@C
    PetscObjectAddOptionsHandler - Adds an additional function to check for options when XXXSetFromOptions() is called.

    Not Collective

    Input Parameter:
+   obj - the PETSc object
.   handle - function that checks for options
.   destroy - function to destroy context if provided
-   ctx - optional context for check function

    Level: developer


.seealso: KSPSetFromOptions(), PCSetFromOptions(), SNESSetFromOptions(), PetscObjectProcessOptionsHandlers(), PetscObjectDestroyOptionsHandlers()

@*/
PetscErrorCode PetscObjectAddOptionsHandler(PetscObject obj,PetscErrorCode (*handle)(PetscOptionItems*,PetscObject,void*),PetscErrorCode (*destroy)(PetscObject,void*),void *ctx)
{
  PetscFunctionBegin;
  PetscValidHeader(obj,1);
  if (obj->noptionhandler >= PETSC_MAX_OPTIONS_HANDLER) SETERRQ(obj->comm,PETSC_ERR_ARG_OUTOFRANGE,"To many options handlers added");
  obj->optionhandler[obj->noptionhandler] = handle;
  obj->optiondestroy[obj->noptionhandler] = destroy;
  obj->optionctx[obj->noptionhandler++]   = ctx;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscObjectProcessOptionsHandlers"
/*@C
    PetscObjectProcessOptionsHandlers - Calls all the options handlers attached to an object

    Not Collective

    Input Parameter:
.   obj - the PETSc object

    Level: developer


.seealso: KSPSetFromOptions(), PCSetFromOptions(), SNESSetFromOptions(), PetscObjectAddOptionsHandler(), PetscObjectDestroyOptionsHandlers()

@*/
PetscErrorCode  PetscObjectProcessOptionsHandlers(PetscOptionItems *PetscOptionsObject,PetscObject obj)
{
  PetscInt       i;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeader(obj,1);
  for (i=0; i<obj->noptionhandler; i++) {
    ierr = (*obj->optionhandler[i])(PetscOptionsObject,obj,obj->optionctx[i]);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscObjectDestroyOptionsHandlers"
/*@C
    PetscObjectDestroyOptionsHandlers - Destroys all the option handlers attached to an object

    Not Collective

    Input Parameter:
.   obj - the PETSc object

    Level: developer


.seealso: KSPSetFromOptions(), PCSetFromOptions(), SNESSetFromOptions(), PetscObjectAddOptionsHandler(), PetscObjectProcessOptionsHandlers()

@*/
PetscErrorCode  PetscObjectDestroyOptionsHandlers(PetscObject obj)
{
  PetscInt       i;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeader(obj,1);
  for (i=0; i<obj->noptionhandler; i++) {
    if (obj->optiondestroy[i]) {
      ierr = (*obj->optiondestroy[i])(obj,obj->optionctx[i]);CHKERRQ(ierr);
    }
  }
  obj->noptionhandler = 0;
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "PetscObjectReference"
/*@
   PetscObjectReference - Indicates to any PetscObject that it is being
   referenced by another PetscObject. This increases the reference
   count for that object by one.

   Logically Collective on PetscObject

   Input Parameter:
.  obj - the PETSc object. This must be cast with (PetscObject), for example,
         PetscObjectReference((PetscObject)mat);

   Level: advanced

.seealso: PetscObjectCompose(), PetscObjectDereference()
@*/
PetscErrorCode  PetscObjectReference(PetscObject obj)
{
  PetscFunctionBegin;
  if (!obj) PetscFunctionReturn(0);
  PetscValidHeader(obj,1);
  obj->refct++;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscObjectGetReference"
/*@
   PetscObjectGetReference - Gets the current reference count for
   any PETSc object.

   Not Collective

   Input Parameter:
.  obj - the PETSc object; this must be cast with (PetscObject), for example,
         PetscObjectGetReference((PetscObject)mat,&cnt);

   Output Parameter:
.  cnt - the reference count

   Level: advanced

.seealso: PetscObjectCompose(), PetscObjectDereference(), PetscObjectReference()
@*/
PetscErrorCode  PetscObjectGetReference(PetscObject obj,PetscInt *cnt)
{
  PetscFunctionBegin;
  PetscValidHeader(obj,1);
  PetscValidIntPointer(cnt,2);
  *cnt = obj->refct;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscObjectDereference"
/*@
   PetscObjectDereference - Indicates to any PetscObject that it is being
   referenced by one less PetscObject. This decreases the reference
   count for that object by one.

   Collective on PetscObject if reference reaches 0 otherwise Logically Collective

   Input Parameter:
.  obj - the PETSc object; this must be cast with (PetscObject), for example,
         PetscObjectDereference((PetscObject)mat);

   Notes: PetscObjectDestroy(PetscObject *obj)  sets the obj pointer to null after the call, this routine does not.

   Level: advanced

.seealso: PetscObjectCompose(), PetscObjectReference()
@*/
PetscErrorCode  PetscObjectDereference(PetscObject obj)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (!obj) PetscFunctionReturn(0);
  PetscValidHeader(obj,1);
  if (obj->bops->destroy) {
    ierr = (*obj->bops->destroy)(&obj);CHKERRQ(ierr);
  } else if (!--obj->refct) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SUP,"This PETSc object does not have a generic destroy routine");
  PetscFunctionReturn(0);
}

/* ----------------------------------------------------------------------- */
/*
     The following routines are the versions private to the PETSc object
     data structures.
*/
#undef __FUNCT__
#define __FUNCT__ "PetscObjectGetComm_Petsc"
PetscErrorCode PetscObjectGetComm_Petsc(PetscObject obj,MPI_Comm *comm)
{
  PetscFunctionBegin;
  PetscValidHeader(obj,1);
  *comm = obj->comm;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscObjectRemoveReference"
PetscErrorCode PetscObjectRemoveReference(PetscObject obj,const char name[])
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeader(obj,1);
  ierr = PetscObjectListRemoveReference(&obj->olist,name);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscObjectCompose_Petsc"
PetscErrorCode PetscObjectCompose_Petsc(PetscObject obj,const char name[],PetscObject ptr)
{
  PetscErrorCode ierr;
  char           *tname;
  PetscBool      skipreference;

  PetscFunctionBegin;
  if (ptr) {
    ierr = PetscObjectListReverseFind(ptr->olist,obj,&tname,&skipreference);CHKERRQ(ierr);
    if (tname && !skipreference) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_INCOMP,"An object cannot be composed with an object that was composed with it");
  }
  ierr = PetscObjectListAdd(&obj->olist,name,ptr);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscObjectQuery_Petsc"
PetscErrorCode PetscObjectQuery_Petsc(PetscObject obj,const char name[],PetscObject *ptr)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeader(obj,1);
  ierr = PetscObjectListFind(obj->olist,name,ptr);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscObjectComposeFunction_Petsc"
PetscErrorCode PetscObjectComposeFunction_Petsc(PetscObject obj,const char name[],void (*ptr)(void))
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeader(obj,1);
  ierr = PetscFunctionListAdd(&obj->qlist,name,ptr);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscObjectQueryFunction_Petsc"
PetscErrorCode PetscObjectQueryFunction_Petsc(PetscObject obj,const char name[],void (**ptr)(void))
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeader(obj,1);
  ierr = PetscFunctionListFind(obj->qlist,name,ptr);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscObjectCompose"
/*@C
   PetscObjectCompose - Associates another PETSc object with a given PETSc object.

   Not Collective

   Input Parameters:
+  obj - the PETSc object; this must be cast with (PetscObject), for example,
         PetscObjectCompose((PetscObject)mat,...);
.  name - name associated with the child object
-  ptr - the other PETSc object to associate with the PETSc object; this must also be
         cast with (PetscObject)

   Level: advanced

   Notes:
   The second objects reference count is automatically increased by one when it is
   composed.

   Replaces any previous object that had the same name.

   If ptr is null and name has previously been composed using an object, then that
   entry is removed from the obj.

   PetscObjectCompose() can be used with any PETSc object (such as
   Mat, Vec, KSP, SNES, etc.) or any user-provided object.  See
   PetscContainerCreate() for info on how to create an object from a
   user-provided pointer that may then be composed with PETSc objects.

   Concepts: objects^composing
   Concepts: composing objects

.seealso: PetscObjectQuery(), PetscContainerCreate()
@*/
PetscErrorCode  PetscObjectCompose(PetscObject obj,const char name[],PetscObject ptr)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeader(obj,1);
  PetscValidCharPointer(name,2);
  if (ptr) PetscValidHeader(ptr,3);
  ierr = (*obj->bops->compose)(obj,name,ptr);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscObjectSetPrecision"
/*@C
   PetscObjectSetPrecision - sets the precision used within a given object.

   Collective on the PetscObject

   Input Parameters:
+  obj - the PETSc object; this must be cast with (PetscObject), for example,
         PetscObjectCompose((PetscObject)mat,...);
-  precision - the precision

   Level: advanced

.seealso: PetscObjectQuery(), PetscContainerCreate()
@*/
PetscErrorCode  PetscObjectSetPrecision(PetscObject obj,PetscPrecision precision)
{
  PetscFunctionBegin;
  PetscValidHeader(obj,1);
  obj->precision = precision;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscObjectQuery"
/*@C
   PetscObjectQuery  - Gets a PETSc object associated with a given object.

   Not Collective

   Input Parameters:
+  obj - the PETSc object
         Thus must be cast with a (PetscObject), for example,
         PetscObjectCompose((PetscObject)mat,...);
.  name - name associated with child object
-  ptr - the other PETSc object associated with the PETSc object, this must be
         cast with (PetscObject*)

   Level: advanced

   The reference count of neither object is increased in this call

   Concepts: objects^composing
   Concepts: composing objects
   Concepts: objects^querying
   Concepts: querying objects

.seealso: PetscObjectCompose()
@*/
PetscErrorCode  PetscObjectQuery(PetscObject obj,const char name[],PetscObject *ptr)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeader(obj,1);
  PetscValidCharPointer(name,2);
  PetscValidPointer(ptr,3);
  ierr = (*obj->bops->query)(obj,name,ptr);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/*MC
   PetscObjectComposeFunction - Associates a function with a given PETSc object.

    Synopsis:
    #include <petscsys.h>
    PetscErrorCode PetscObjectComposeFunction(PetscObject obj,const char name[],void (*fptr)(void))

   Logically Collective on PetscObject

   Input Parameters:
+  obj - the PETSc object; this must be cast with a (PetscObject), for example,
         PetscObjectCompose((PetscObject)mat,...);
.  name - name associated with the child function
.  fname - name of the function
-  fptr - function pointer

   Level: advanced

   Notes:
   To remove a registered routine, pass in NULL for fptr().

   PetscObjectComposeFunction() can be used with any PETSc object (such as
   Mat, Vec, KSP, SNES, etc.) or any user-provided object.

   Concepts: objects^composing functions
   Concepts: composing functions
   Concepts: functions^querying
   Concepts: objects^querying
   Concepts: querying objects

.seealso: PetscObjectQueryFunction(), PetscContainerCreate()
M*/

#undef __FUNCT__
#define __FUNCT__ "PetscObjectComposeFunction_Private"
PetscErrorCode  PetscObjectComposeFunction_Private(PetscObject obj,const char name[],void (*fptr)(void))
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeader(obj,1);
  PetscValidCharPointer(name,2);
  ierr = (*obj->bops->composefunction)(obj,name,fptr);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/*MC
   PetscObjectQueryFunction - Gets a function associated with a given object.

    Synopsis:
    #include <petscsys.h>
    PetscErrorCode PetscObjectQueryFunction(PetscObject obj,const char name[],void (**fptr)(void))

   Logically Collective on PetscObject

   Input Parameters:
+  obj - the PETSc object; this must be cast with (PetscObject), for example,
         PetscObjectQueryFunction((PetscObject)ksp,...);
-  name - name associated with the child function

   Output Parameter:
.  fptr - function pointer

   Level: advanced

   Concepts: objects^composing functions
   Concepts: composing functions
   Concepts: functions^querying
   Concepts: objects^querying
   Concepts: querying objects

.seealso: PetscObjectComposeFunction(), PetscFunctionListFind()
M*/
#undef __FUNCT__
#define __FUNCT__ "PetscObjectQueryFunction_Private"
PETSC_EXTERN PetscErrorCode PetscObjectQueryFunction_Private(PetscObject obj,const char name[],void (**ptr)(void))
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeader(obj,1);
  PetscValidCharPointer(name,2);
  ierr = (*obj->bops->queryfunction)(obj,name,ptr);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

struct _p_PetscContainer {
  PETSCHEADER(int);
  void           *ptr;
  PetscErrorCode (*userdestroy)(void*);
};

#undef __FUNCT__
#define __FUNCT__ "PetscContainerGetPointer"
/*@C
   PetscContainerGetPointer - Gets the pointer value contained in the container.

   Not Collective

   Input Parameter:
.  obj - the object created with PetscContainerCreate()

   Output Parameter:
.  ptr - the pointer value

   Level: advanced

.seealso: PetscContainerCreate(), PetscContainerDestroy(),
          PetscContainerSetPointer()
@*/
PetscErrorCode  PetscContainerGetPointer(PetscContainer obj,void **ptr)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(obj,PETSC_CONTAINER_CLASSID,1);
  PetscValidPointer(ptr,2);
  *ptr = obj->ptr;
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "PetscContainerSetPointer"
/*@C
   PetscContainerSetPointer - Sets the pointer value contained in the container.

   Logically Collective on PetscContainer

   Input Parameters:
+  obj - the object created with PetscContainerCreate()
-  ptr - the pointer value

   Level: advanced

.seealso: PetscContainerCreate(), PetscContainerDestroy(),
          PetscContainerGetPointer()
@*/
PetscErrorCode  PetscContainerSetPointer(PetscContainer obj,void *ptr)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(obj,PETSC_CONTAINER_CLASSID,1);
  if (ptr) PetscValidPointer(ptr,2);
  obj->ptr = ptr;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscContainerDestroy"
/*@C
   PetscContainerDestroy - Destroys a PETSc container object.

   Collective on PetscContainer

   Input Parameter:
.  obj - an object that was created with PetscContainerCreate()

   Level: advanced

.seealso: PetscContainerCreate(), PetscContainerSetUserDestroy()
@*/
PetscErrorCode  PetscContainerDestroy(PetscContainer *obj)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (!*obj) PetscFunctionReturn(0);
  PetscValidHeaderSpecific(*obj,PETSC_CONTAINER_CLASSID,1);
  if (--((PetscObject)(*obj))->refct > 0) {*obj = 0; PetscFunctionReturn(0);}
  if ((*obj)->userdestroy) (*(*obj)->userdestroy)((*obj)->ptr);
  ierr = PetscHeaderDestroy(obj);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscContainerSetUserDestroy"
/*@C
   PetscContainerSetUserDestroy - Sets name of the user destroy function.

   Logically Collective on PetscContainer

   Input Parameter:
+  obj - an object that was created with PetscContainerCreate()
-  des - name of the user destroy function

   Level: advanced

.seealso: PetscContainerDestroy()
@*/
PetscErrorCode  PetscContainerSetUserDestroy(PetscContainer obj, PetscErrorCode (*des)(void*))
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(obj,PETSC_CONTAINER_CLASSID,1);
  obj->userdestroy = des;
  PetscFunctionReturn(0);
}

PetscClassId PETSC_CONTAINER_CLASSID;

#undef __FUNCT__
#define __FUNCT__ "PetscContainerCreate"
/*@C
   PetscContainerCreate - Creates a PETSc object that has room to hold
   a single pointer. This allows one to attach any type of data (accessible
   through a pointer) with the PetscObjectCompose() function to a PetscObject.
   The data item itself is attached by a call to PetscContainerSetPointer().

   Collective on MPI_Comm

   Input Parameters:
.  comm - MPI communicator that shares the object

   Output Parameters:
.  container - the container created

   Level: advanced

.seealso: PetscContainerDestroy(), PetscContainerSetPointer(), PetscContainerGetPointer()
@*/
PetscErrorCode  PetscContainerCreate(MPI_Comm comm,PetscContainer *container)
{
  PetscErrorCode ierr;
  PetscContainer contain;

  PetscFunctionBegin;
  PetscValidPointer(container,2);
  ierr = PetscSysInitializePackage();CHKERRQ(ierr);
  ierr = PetscHeaderCreate(contain,PETSC_CONTAINER_CLASSID,"PetscContainer","Container","Sys",comm,PetscContainerDestroy,NULL);CHKERRQ(ierr);
  *container = contain;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscObjectSetFromOptions"
/*@
   PetscObjectSetFromOptions - Sets generic parameters from user options.

   Collective on obj

   Input Parameter:
.  obj - the PetscObjcet

   Options Database Keys:

   Notes:
   We have no generic options at present, so this does nothing

   Level: beginner

.keywords: set, options, database
.seealso: PetscObjectSetOptionsPrefix(), PetscObjectGetOptionsPrefix()
@*/
PetscErrorCode  PetscObjectSetFromOptions(PetscObject obj)
{
  PetscFunctionBegin;
  PetscValidHeader(obj,1);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscObjectSetUp"
/*@
   PetscObjectSetUp - Sets up the internal data structures for the later use.

   Collective on PetscObject

   Input Parameters:
.  obj - the PetscObject

   Notes:
   This does nothing at present.

   Level: advanced

.keywords: setup
.seealso: PetscObjectDestroy()
@*/
PetscErrorCode  PetscObjectSetUp(PetscObject obj)
{
  PetscFunctionBegin;
  PetscValidHeader(obj,1);
  PetscFunctionReturn(0);
}
