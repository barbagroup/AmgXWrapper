/*
    The PF mathematical functions interface routines, callable by users.
*/
#include <../src/vec/pf/pfimpl.h>            /*I "petscpf.h" I*/

PetscClassId      PF_CLASSID          = 0;
PetscFunctionList PFList              = NULL;   /* list of all registered PD functions */
PetscBool         PFRegisterAllCalled = PETSC_FALSE;

#undef __FUNCT__
#define __FUNCT__ "PFSet"
/*@C
   PFSet - Sets the C/C++/Fortran functions to be used by the PF function

   Collective on PF

   Input Parameter:
+  pf - the function context
.  apply - function to apply to an array
.  applyvec - function to apply to a Vec
.  view - function that prints information about the PF
.  destroy - function to free the private function context
-  ctx - private function context

   Level: beginner

.keywords: PF, setting

.seealso: PFCreate(), PFDestroy(), PFSetType(), PFApply(), PFApplyVec()
@*/
PetscErrorCode  PFSet(PF pf,PetscErrorCode (*apply)(void*,PetscInt,const PetscScalar*,PetscScalar*),PetscErrorCode (*applyvec)(void*,Vec,Vec),PetscErrorCode (*view)(void*,PetscViewer),PetscErrorCode (*destroy)(void*),void*ctx)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(pf,PF_CLASSID,1);
  pf->data          = ctx;
  pf->ops->destroy  = destroy;
  pf->ops->apply    = apply;
  pf->ops->applyvec = applyvec;
  pf->ops->view     = view;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PFDestroy"
/*@C
   PFDestroy - Destroys PF context that was created with PFCreate().

   Collective on PF

   Input Parameter:
.  pf - the function context

   Level: beginner

.keywords: PF, destroy

.seealso: PFCreate(), PFSet(), PFSetType()
@*/
PetscErrorCode  PFDestroy(PF *pf)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (!*pf) PetscFunctionReturn(0);
  PetscValidHeaderSpecific((*pf),PF_CLASSID,1);
  if (--((PetscObject)(*pf))->refct > 0) PetscFunctionReturn(0);

  ierr = PFViewFromOptions(*pf,NULL,"-pf_view");CHKERRQ(ierr);
  /* if memory was published with SAWs then destroy it */
  ierr = PetscObjectSAWsViewOff((PetscObject)*pf);CHKERRQ(ierr);

  if ((*pf)->ops->destroy) {ierr =  (*(*pf)->ops->destroy)((*pf)->data);CHKERRQ(ierr);}
  ierr = PetscHeaderDestroy(pf);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PFCreate"
/*@C
   PFCreate - Creates a mathematical function context.

   Collective on MPI_Comm

   Input Parameter:
+  comm - MPI communicator
.  dimin - dimension of the space you are mapping from
-  dimout - dimension of the space you are mapping to

   Output Parameter:
.  pf - the function context

   Level: developer

.keywords: PF, create, context

.seealso: PFSet(), PFApply(), PFDestroy(), PFApplyVec()
@*/
PetscErrorCode  PFCreate(MPI_Comm comm,PetscInt dimin,PetscInt dimout,PF *pf)
{
  PF             newpf;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidPointer(pf,1);
  *pf = NULL;
  ierr = PFInitializePackage();CHKERRQ(ierr);

  ierr = PetscHeaderCreate(newpf,PF_CLASSID,"PF","Mathematical functions","Vec",comm,PFDestroy,PFView);CHKERRQ(ierr);
  newpf->data          = 0;
  newpf->ops->destroy  = 0;
  newpf->ops->apply    = 0;
  newpf->ops->applyvec = 0;
  newpf->ops->view     = 0;
  newpf->dimin         = dimin;
  newpf->dimout        = dimout;

  *pf                  = newpf;
  PetscFunctionReturn(0);

}

/* -------------------------------------------------------------------------------*/

#undef __FUNCT__
#define __FUNCT__ "PFApplyVec"
/*@
   PFApplyVec - Applies the mathematical function to a vector

   Collective on PF

   Input Parameters:
+  pf - the function context
-  x - input vector (or NULL for the vector (0,1, .... N-1)

   Output Parameter:
.  y - output vector

   Level: beginner

.keywords: PF, apply

.seealso: PFApply(), PFCreate(), PFDestroy(), PFSetType(), PFSet()
@*/
PetscErrorCode  PFApplyVec(PF pf,Vec x,Vec y)
{
  PetscErrorCode ierr;
  PetscInt       i,rstart,rend,n,p;
  PetscBool      nox = PETSC_FALSE;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(pf,PF_CLASSID,1);
  PetscValidHeaderSpecific(y,VEC_CLASSID,3);
  if (x) {
    PetscValidHeaderSpecific(x,VEC_CLASSID,2);
    if (x == y) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_IDN,"x and y must be different vectors");
  } else {
    PetscScalar *xx;
    PetscInt    lsize;

    ierr  = VecGetLocalSize(y,&lsize);CHKERRQ(ierr);
    lsize = pf->dimin*lsize/pf->dimout;
    ierr  = VecCreateMPI(PetscObjectComm((PetscObject)y),lsize,PETSC_DETERMINE,&x);CHKERRQ(ierr);
    nox   = PETSC_TRUE;
    ierr  = VecGetOwnershipRange(x,&rstart,&rend);CHKERRQ(ierr);
    ierr  = VecGetArray(x,&xx);CHKERRQ(ierr);
    for (i=rstart; i<rend; i++) xx[i-rstart] = (PetscScalar)i;
    ierr = VecRestoreArray(x,&xx);CHKERRQ(ierr);
  }

  ierr = VecGetLocalSize(x,&n);CHKERRQ(ierr);
  ierr = VecGetLocalSize(y,&p);CHKERRQ(ierr);
  if ((pf->dimin*(n/pf->dimin)) != n) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_SIZ,"Local input vector length %D not divisible by dimin %D of function",n,pf->dimin);
  if ((pf->dimout*(p/pf->dimout)) != p) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_SIZ,"Local output vector length %D not divisible by dimout %D of function",p,pf->dimout);
  if ((n/pf->dimin) != (p/pf->dimout)) SETERRQ4(PETSC_COMM_SELF,PETSC_ERR_ARG_SIZ,"Local vector lengths %D %D are wrong for dimin and dimout %D %D of function",n,p,pf->dimin,pf->dimout);

  if (pf->ops->applyvec) {
    ierr = (*pf->ops->applyvec)(pf->data,x,y);CHKERRQ(ierr);
  } else {
    PetscScalar *xx,*yy;

    ierr = VecGetLocalSize(x,&n);CHKERRQ(ierr);
    n    = n/pf->dimin;
    ierr = VecGetArray(x,&xx);CHKERRQ(ierr);
    ierr = VecGetArray(y,&yy);CHKERRQ(ierr);
    if (!pf->ops->apply) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONGSTATE,"No function has been provided for this PF");
    ierr = (*pf->ops->apply)(pf->data,n,xx,yy);CHKERRQ(ierr);
    ierr = VecRestoreArray(x,&xx);CHKERRQ(ierr);
    ierr = VecRestoreArray(y,&yy);CHKERRQ(ierr);
  }
  if (nox) {
    ierr = VecDestroy(&x);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PFApply"
/*@
   PFApply - Applies the mathematical function to an array of values.

   Collective on PF

   Input Parameters:
+  pf - the function context
.  n - number of pointwise function evaluations to perform, each pointwise function evaluation
       is a function of dimin variables and computes dimout variables where dimin and dimout are defined
       in the call to PFCreate()
-  x - input array

   Output Parameter:
.  y - output array

   Level: beginner

   Notes:

.keywords: PF, apply

.seealso: PFApplyVec(), PFCreate(), PFDestroy(), PFSetType(), PFSet()
@*/
PetscErrorCode  PFApply(PF pf,PetscInt n,const PetscScalar *x,PetscScalar *y)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(pf,PF_CLASSID,1);
  PetscValidScalarPointer(x,2);
  PetscValidScalarPointer(y,3);
  if (x == y) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_IDN,"x and y must be different arrays");
  if (!pf->ops->apply) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONGSTATE,"No function has been provided for this PF");

  ierr = (*pf->ops->apply)(pf->data,n,x,y);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PFView"
/*@
   PFView - Prints information about a mathematical function

   Collective on PF unless PetscViewer is PETSC_VIEWER_STDOUT_SELF

   Input Parameters:
+  PF - the PF context
-  viewer - optional visualization context

   Note:
   The available visualization contexts include
+     PETSC_VIEWER_STDOUT_SELF - standard output (default)
-     PETSC_VIEWER_STDOUT_WORLD - synchronized standard
         output where only the first processor opens
         the file.  All other processors send their
         data to the first processor to print.

   The user can open an alternative visualization contexts with
   PetscViewerASCIIOpen() (output to a specified file).

   Level: developer

.keywords: PF, view

.seealso: PetscViewerCreate(), PetscViewerASCIIOpen()
@*/
PetscErrorCode  PFView(PF pf,PetscViewer viewer)
{
  PetscErrorCode    ierr;
  PetscBool         iascii;
  PetscViewerFormat format;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(pf,PF_CLASSID,1);
  if (!viewer) {
    ierr = PetscViewerASCIIGetStdout(PetscObjectComm((PetscObject)pf),&viewer);CHKERRQ(ierr);
  }
  PetscValidHeaderSpecific(viewer,PETSC_VIEWER_CLASSID,2);
  PetscCheckSameComm(pf,1,viewer,2);

  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERASCII,&iascii);CHKERRQ(ierr);
  if (iascii) {
    ierr = PetscViewerGetFormat(viewer,&format);CHKERRQ(ierr);
    ierr = PetscObjectPrintClassNamePrefixType((PetscObject)pf,viewer);CHKERRQ(ierr);
    if (pf->ops->view) {
      ierr = PetscViewerASCIIPushTab(viewer);CHKERRQ(ierr);
      ierr = (*pf->ops->view)(pf->data,viewer);CHKERRQ(ierr);
      ierr = PetscViewerASCIIPopTab(viewer);CHKERRQ(ierr);
    }
  }
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "PFRegister"
/*@C
   PFRegister - Adds a method to the mathematical function package.

   Not collective

   Input Parameters:
+  name_solver - name of a new user-defined solver
-  routine_create - routine to create method context

   Notes:
   PFRegister() may be called multiple times to add several user-defined functions

   Sample usage:
.vb
   PFRegister("my_function",MyFunctionSetCreate);
.ve

   Then, your solver can be chosen with the procedural interface via
$     PFSetType(pf,"my_function")
   or at runtime via the option
$     -pf_type my_function

   Level: advanced

.keywords: PF, register

.seealso: PFRegisterAll(), PFRegisterDestroy(), PFRegister()
@*/
PetscErrorCode  PFRegister(const char sname[],PetscErrorCode (*function)(PF,void*))
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscFunctionListAdd(&PFList,sname,function);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PFGetType"
/*@C
   PFGetType - Gets the PF method type and name (as a string) from the PF
   context.

   Not Collective

   Input Parameter:
.  pf - the function context

   Output Parameter:
.  type - name of function

   Level: intermediate

.keywords: PF, get, method, name, type

.seealso: PFSetType()

@*/
PetscErrorCode  PFGetType(PF pf,PFType *type)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(pf,PF_CLASSID,1);
  PetscValidPointer(type,2);
  *type = ((PetscObject)pf)->type_name;
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "PFSetType"
/*@C
   PFSetType - Builds PF for a particular function

   Collective on PF

   Input Parameter:
+  pf - the function context.
.  type - a known method
-  ctx - optional type dependent context

   Options Database Key:
.  -pf_type <type> - Sets PF type


  Notes:
  See "petsc/include/petscpf.h" for available methods (for instance,
  PFCONSTANT)

  Level: intermediate

.keywords: PF, set, method, type

.seealso: PFSet(), PFRegister(), PFCreate(), DMDACreatePF()

@*/
PetscErrorCode  PFSetType(PF pf,PFType type,void *ctx)
{
  PetscErrorCode ierr,(*r)(PF,void*);
  PetscBool      match;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(pf,PF_CLASSID,1);
  PetscValidCharPointer(type,2);

  ierr = PetscObjectTypeCompare((PetscObject)pf,type,&match);CHKERRQ(ierr);
  if (match) PetscFunctionReturn(0);

  if (pf->ops->destroy) {ierr =  (*pf->ops->destroy)(pf);CHKERRQ(ierr);}
  pf->data = 0;

  /* Determine the PFCreateXXX routine for a particular function */
  ierr = PetscFunctionListFind(PFList,type,&r);CHKERRQ(ierr);
  if (!r) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_ARG_UNKNOWN_TYPE,"Unable to find requested PF type %s",type);
  pf->ops->destroy  = 0;
  pf->ops->view     = 0;
  pf->ops->apply    = 0;
  pf->ops->applyvec = 0;

  /* Call the PFCreateXXX routine for this particular function */
  ierr = (*r)(pf,ctx);CHKERRQ(ierr);

  ierr = PetscObjectChangeTypeName((PetscObject)pf,type);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PFSetFromOptions"
/*@
   PFSetFromOptions - Sets PF options from the options database.

   Collective on PF

   Input Parameters:
.  pf - the mathematical function context

   Options Database Keys:

   Notes:
   To see all options, run your program with the -help option
   or consult the users manual.

   Level: intermediate

.keywords: PF, set, from, options, database

.seealso:
@*/
PetscErrorCode  PFSetFromOptions(PF pf)
{
  PetscErrorCode ierr;
  char           type[256];
  PetscBool      flg;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(pf,PF_CLASSID,1);

  ierr = PetscObjectOptionsBegin((PetscObject)pf);CHKERRQ(ierr);
  ierr = PetscOptionsFList("-pf_type","Type of function","PFSetType",PFList,0,type,256,&flg);CHKERRQ(ierr);
  if (flg) {
    ierr = PFSetType(pf,type,NULL);CHKERRQ(ierr);
  }
  if (pf->ops->setfromoptions) {
    ierr = (*pf->ops->setfromoptions)(PetscOptionsObject,pf);CHKERRQ(ierr);
  }

  /* process any options handlers added with PetscObjectAddOptionsHandler() */
  ierr = PetscObjectProcessOptionsHandlers(PetscOptionsObject,(PetscObject)pf);CHKERRQ(ierr);
  ierr = PetscOptionsEnd();CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

static PetscBool PFPackageInitialized = PETSC_FALSE;
#undef __FUNCT__
#define __FUNCT__ "PFFinalizePackage"
/*@C
  PFFinalizePackage - This function destroys everything in the Petsc interface to Mathematica. It is
  called from PetscFinalize().

  Level: developer

.keywords: Petsc, destroy, package, mathematica
.seealso: PetscFinalize()
@*/
PetscErrorCode  PFFinalizePackage(void)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscFunctionListDestroy(&PFList);CHKERRQ(ierr);
  PFPackageInitialized = PETSC_FALSE;
  PFRegisterAllCalled  = PETSC_FALSE;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PFInitializePackage"
/*@C
  PFInitializePackage - This function initializes everything in the PF package. It is called
  from PetscDLLibraryRegister() when using dynamic libraries, and on the first call to PFCreate()
  when using static libraries.

  Level: developer

.keywords: Vec, initialize, package
.seealso: PetscInitialize()
@*/
PetscErrorCode  PFInitializePackage(void)
{
  char           logList[256];
  char           *className;
  PetscBool      opt;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (PFPackageInitialized) PetscFunctionReturn(0);
  PFPackageInitialized = PETSC_TRUE;
  /* Register Classes */
  ierr = PetscClassIdRegister("PointFunction",&PF_CLASSID);CHKERRQ(ierr);
  /* Register Constructors */
  ierr = PFRegisterAll();CHKERRQ(ierr);
  /* Process info exclusions */
  ierr = PetscOptionsGetString(NULL,NULL, "-info_exclude", logList, 256, &opt);CHKERRQ(ierr);
  if (opt) {
    ierr = PetscStrstr(logList, "pf", &className);CHKERRQ(ierr);
    if (className) {
      ierr = PetscInfoDeactivateClass(PF_CLASSID);CHKERRQ(ierr);
    }
  }
  /* Process summary exclusions */
  ierr = PetscOptionsGetString(NULL,NULL, "-log_summary_exclude", logList, 256, &opt);CHKERRQ(ierr);
  if (opt) {
    ierr = PetscStrstr(logList, "pf", &className);CHKERRQ(ierr);
    if (className) {
      ierr = PetscLogEventDeactivateClass(PF_CLASSID);CHKERRQ(ierr);
    }
  }
  ierr = PetscRegisterFinalize(PFFinalizePackage);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}









