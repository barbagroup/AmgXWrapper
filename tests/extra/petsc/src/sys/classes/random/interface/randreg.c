
#include <../src/sys/classes/random/randomimpl.h>         /*I "petscsys.h" I*/

PetscFunctionList PetscRandomList              = NULL;
PetscBool         PetscRandomRegisterAllCalled = PETSC_FALSE;

#undef __FUNCT__
#define __FUNCT__ "PetscRandomSetType"
/*@C
  PetscRandomSetType - Builds a context for generating particular type of random numbers.

  Collective on PetscRandom

  Input Parameters:
+ rnd   - The random number generator context
- type - The name of the random type

  Options Database Key:
. -random_type <type> - Sets the random type; use -help for a list
                     of available types

  Notes:
  See "petsc/include/petscsys.h" for available random types (for instance, PETSCRAND48, PETSCRAND).

  Level: intermediate

.keywords: random, set, type
.seealso: PetscRandomGetType(), PetscRandomCreate()
@*/

PetscErrorCode  PetscRandomSetType(PetscRandom rnd, PetscRandomType type)
{
  PetscErrorCode (*r)(PetscRandom);
  PetscBool      match;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(rnd, PETSC_RANDOM_CLASSID,1);
  ierr = PetscObjectTypeCompare((PetscObject)rnd, type, &match);CHKERRQ(ierr);
  if (match) PetscFunctionReturn(0);

  ierr = PetscFunctionListFind(PetscRandomList,type,&r);CHKERRQ(ierr);
  if (!r) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_ARG_UNKNOWN_TYPE, "Unknown random type: %s", type);

  if (rnd->ops->destroy) {
    ierr = (*rnd->ops->destroy)(rnd);CHKERRQ(ierr);

    rnd->ops->destroy = NULL;
  }
  ierr = (*r)(rnd);CHKERRQ(ierr);
  ierr = PetscRandomSeed(rnd);CHKERRQ(ierr);

  ierr = PetscObjectChangeTypeName((PetscObject)rnd, type);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscRandomGetType"
/*@C
  PetscRandomGetType - Gets the type name (as a string) from the PetscRandom.

  Not Collective

  Input Parameter:
. rnd  - The random number generator context

  Output Parameter:
. type - The type name

  Level: intermediate

.keywords: random, get, type, name
.seealso: PetscRandomSetType(), PetscRandomCreate()
@*/
PetscErrorCode  PetscRandomGetType(PetscRandom rnd, PetscRandomType *type)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(rnd, PETSC_RANDOM_CLASSID,1);
  PetscValidPointer(type,2);
  *type = ((PetscObject)rnd)->type_name;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscRandomRegister"
/*@C
  PetscRandomRegister -  Adds a new PetscRandom component implementation

  Not Collective

  Input Parameters:
+ name        - The name of a new user-defined creation routine
- create_func - The creation routine itself

  Notes:
  PetscRandomRegister() may be called multiple times to add several user-defined randome number generators

  Sample usage:
.vb
    PetscRandomRegister("my_rand",  MyPetscRandomtorCreate);
.ve

  Then, your random type can be chosen with the procedural interface via
.vb
    PetscRandomCreate(MPI_Comm, PetscRandom *);
    PetscRandomSetType(PetscRandom,"my_random_name");
.ve
   or at runtime via the option
.vb
    -random_type my_random_name
.ve

  Notes: For an example of the code needed to interface your own random number generator see
         src/sys/random/impls/rand/rand.c

  Level: advanced

.keywords: PetscRandom, register

.seealso: PetscRandomRegisterAll(), PetscRandomRegisterDestroy(), PetscRandomRegister()
@*/
PetscErrorCode  PetscRandomRegister(const char sname[], PetscErrorCode (*function)(PetscRandom))
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscFunctionListAdd(&PetscRandomList,sname,function);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#if defined(PETSC_HAVE_RAND)
PETSC_EXTERN PetscErrorCode PetscRandomCreate_Rand(PetscRandom);
#endif
#if defined(PETSC_HAVE_DRAND48)
PETSC_EXTERN PetscErrorCode PetscRandomCreate_Rand48(PetscRandom);
#endif
#if defined(PETSC_HAVE_SPRNG)
PETSC_EXTERN PetscErrorCode PetscRandomCreate_Sprng(PetscRandom);
#endif
PETSC_EXTERN PetscErrorCode PetscRandomCreate_Rander48(PetscRandom);

#undef __FUNCT__
#define __FUNCT__ "PetscRandomRegisterAll"
/*@C
  PetscRandomRegisterAll - Registers all of the components in the PetscRandom package.

  Not Collective

  Level: advanced

.keywords: PetscRandom, register, all
.seealso:  PetscRandomRegister(), PetscRandomRegisterDestroy()
@*/
PetscErrorCode  PetscRandomRegisterAll(void)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (PetscRandomRegisterAllCalled) PetscFunctionReturn(0);
  PetscRandomRegisterAllCalled = PETSC_TRUE;
#if defined(PETSC_HAVE_RAND)
  ierr = PetscRandomRegister(PETSCRAND,  PetscRandomCreate_Rand);CHKERRQ(ierr);
#endif
#if defined(PETSC_HAVE_DRAND48)
  ierr = PetscRandomRegister(PETSCRAND48,PetscRandomCreate_Rand48);CHKERRQ(ierr);
#endif
#if defined(PETSC_HAVE_SPRNG)
  ierr = PetscRandomRegister(PETSCSPRNG, PetscRandomCreate_Sprng);CHKERRQ(ierr);
#endif
  ierr = PetscRandomRegister(PETSCRANDER48,PetscRandomCreate_Rander48);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

