
#include <petsc/private/isimpl.h>    /*I "petscis.h"  I*/

PetscFunctionList ISList              = NULL;
PetscBool         ISRegisterAllCalled = PETSC_FALSE;

#undef __FUNCT__
#define __FUNCT__ "ISCreate"
/*@
   ISCreate - Creates an index set object.

   Collective on MPI_Comm

   Input Parameters:
.  comm - the MPI communicator

   Output Parameter:
.  is - the new index set

   Notes:
   When the communicator is not MPI_COMM_SELF, the operations on IS are NOT
   conceptually the same as MPI_Group operations. The IS are then
   distributed sets of indices and thus certain operations on them are
   collective.

   Level: beginner

  Concepts: index sets^creating
  Concepts: IS^creating

.seealso: ISCreateGeneral(), ISCreateStride(), ISCreateBlock(), ISAllGather()
@*/
PetscErrorCode  ISCreate(MPI_Comm comm,IS *is)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidPointer(is,2);
  ierr = ISInitializePackage();CHKERRQ(ierr);

  ierr = PetscHeaderCreate(*is,IS_CLASSID,"IS","Index Set","IS",comm,ISDestroy,ISView);CHKERRQ(ierr);
  ierr = PetscLayoutCreate(comm, &(*is)->map);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "ISSetType"
/*@C
  ISSetType - Builds a index set, for a particular implementation.

  Collective on IS

  Input Parameters:
+ is    - The index set object
- method - The name of the index set type

  Options Database Key:
. -is_type <type> - Sets the index set type; use -help for a list of available types

  Notes:
  See "petsc/include/petscis.h" for available istor types (for instance, ISGENERAL, ISSTRIDE, or ISBLOCK).

  Use ISDuplicate() to make a duplicate

  Level: intermediate


.seealso: ISGetType(), ISCreate()
@*/
PetscErrorCode  ISSetType(IS is, ISType method)
{
  PetscErrorCode (*r)(IS);
  PetscBool      match;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(is, IS_CLASSID,1);
  ierr = PetscObjectTypeCompare((PetscObject) is, method, &match);CHKERRQ(ierr);
  if (match) PetscFunctionReturn(0);

  ierr = ISRegisterAll();CHKERRQ(ierr);
  ierr = PetscFunctionListFind(ISList,method,&r);CHKERRQ(ierr);
  if (!r) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_ARG_UNKNOWN_TYPE, "Unknown IS type: %s", method);
  if (is->ops->destroy) {
    ierr = (*is->ops->destroy)(is);CHKERRQ(ierr);
    is->ops->destroy = NULL;
  }
  ierr = (*r)(is);CHKERRQ(ierr);
  ierr = PetscObjectChangeTypeName((PetscObject)is,method);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "ISGetType"
/*@C
  ISGetType - Gets the index set type name (as a string) from the IS.

  Not Collective

  Input Parameter:
. is  - The index set

  Output Parameter:
. type - The index set type name

  Level: intermediate

.seealso: ISSetType(), ISCreate()
@*/
PetscErrorCode  ISGetType(IS is, ISType *type)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(is, IS_CLASSID,1);
  PetscValidCharPointer(type,2);
  if (!ISRegisterAllCalled) {
    ierr = ISRegisterAll();CHKERRQ(ierr);
  }
  *type = ((PetscObject)is)->type_name;
  PetscFunctionReturn(0);
}


/*--------------------------------------------------------------------------------------------------------------------*/

#undef __FUNCT__
#define __FUNCT__ "ISRegister"
/*@C
  ISRegister - Adds a new index set implementation

  Not Collective

  Input Parameters:
+ name        - The name of a new user-defined creation routine
- create_func - The creation routine itself

  Notes:
  ISRegister() may be called multiple times to add several user-defined vectors

  Sample usage:
.vb
    ISRegister("my_is_name",  MyISCreate);
.ve

  Then, your vector type can be chosen with the procedural interface via
.vb
    ISCreate(MPI_Comm, IS *);
    ISSetType(IS,"my_is_name");
.ve
   or at runtime via the option
.vb
    -is_type my_is_name
.ve

  This is no ISSetFromOptions() and the current implementations do not have a way to dynamically determine type, so
  dynamic registration of custom IS types will be of limited use to users.

  Level: developer

.keywords: IS, register
.seealso: ISRegisterAll(), ISRegisterDestroy(), ISRegister()

  Level: advanced
@*/
PetscErrorCode  ISRegister(const char sname[], PetscErrorCode (*function)(IS))
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscFunctionListAdd(&ISList,sname,function);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}


