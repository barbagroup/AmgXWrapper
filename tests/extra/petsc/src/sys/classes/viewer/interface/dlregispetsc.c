
#include <petscdraw.h>
#include <petscviewer.h>

extern PetscLogEvent PETSC_Barrier,PETSC_BuildTwoSided,PETSC_BuildTwoSidedF;

static PetscBool PetscSysPackageInitialized = PETSC_FALSE;
#undef __FUNCT__
#define __FUNCT__ "PetscSysFinalizePackage"
/*@C
  PetscSysFinalizePackage - This function destroys everything in the Petsc interface to Mathematica. It is
  called from PetscFinalize().

  Level: developer

.keywords: Petsc, destroy, package, mathematica
.seealso: PetscFinalize()
@*/
PetscErrorCode  PetscSysFinalizePackage(void)
{
  PetscFunctionBegin;
  PetscSysPackageInitialized = PETSC_FALSE;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscSysInitializePackage"
/*@C
  PetscSysInitializePackage - This function initializes everything in the main Petsc package. It is called
  from PetscDLLibraryRegister() when using dynamic libraries, and on the call to PetscInitialize()
  when using static libraries.

  Level: developer

.keywords: Petsc, initialize, package
.seealso: PetscInitialize()
@*/
PetscErrorCode  PetscSysInitializePackage(void)
{
  char           logList[256];
  char           *className;
  PetscBool      opt;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (PetscSysPackageInitialized) PetscFunctionReturn(0);
  PetscSysPackageInitialized = PETSC_TRUE;
  /* Register Classes */
  ierr = PetscClassIdRegister("Object",&PETSC_OBJECT_CLASSID);CHKERRQ(ierr);
  ierr = PetscClassIdRegister("Container",&PETSC_CONTAINER_CLASSID);CHKERRQ(ierr);

  /* Register Events */
  ierr = PetscLogEventRegister("PetscBarrier", PETSC_SMALLEST_CLASSID,&PETSC_Barrier);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("BuildTwoSided",PETSC_SMALLEST_CLASSID,&PETSC_BuildTwoSided);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("BuildTwoSidedF",PETSC_SMALLEST_CLASSID,&PETSC_BuildTwoSidedF);CHKERRQ(ierr);
  /* Process info exclusions */
  ierr = PetscOptionsGetString(NULL,NULL, "-info_exclude", logList, 256, &opt);CHKERRQ(ierr);
  if (opt) {
    ierr = PetscStrstr(logList, "null", &className);CHKERRQ(ierr);
    if (className) {
      ierr = PetscInfoDeactivateClass(0);CHKERRQ(ierr);
    }
  }
  /* Process summary exclusions */
  ierr = PetscOptionsGetString(NULL,NULL, "-log_summary_exclude", logList, 256, &opt);CHKERRQ(ierr);
  if (opt) {
    ierr = PetscStrstr(logList, "null", &className);CHKERRQ(ierr);
    if (className) {
      ierr = PetscLogEventDeactivateClass(0);CHKERRQ(ierr);
    }
  }
  ierr = PetscRegisterFinalize(PetscSysFinalizePackage);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#if defined(PETSC_HAVE_DYNAMIC_LIBRARIES)

#if defined(PETSC_USE_SINGLE_LIBRARY)
PETSC_EXTERN PetscErrorCode PetscDLLibraryRegister_petscvec(void);
PETSC_EXTERN PetscErrorCode PetscDLLibraryRegister_petscmat(void);
PETSC_EXTERN PetscErrorCode PetscDLLibraryRegister_petscdm(void);
PETSC_EXTERN PetscErrorCode PetscDLLibraryRegister_petscksp(void);
PETSC_EXTERN PetscErrorCode PetscDLLibraryRegister_petscsnes(void);
PETSC_EXTERN PetscErrorCode PetscDLLibraryRegister_petscts(void);
#endif

#undef __FUNCT__
#if defined(PETSC_USE_SINGLE_LIBRARY)
#define __FUNCT__ "PetscDLLibraryRegister_petsc"
#else
#define __FUNCT__ "PetscDLLibraryRegister_petscsys"
#endif
/*
  PetscDLLibraryRegister - This function is called when the dynamic library it is in is opened.

  This one registers all the draw and PetscViewer objects.

 */
#if defined(PETSC_USE_SINGLE_LIBRARY)
PETSC_EXTERN PetscErrorCode PetscDLLibraryRegister_petsc(void)
#else
PETSC_EXTERN PetscErrorCode PetscDLLibraryRegister_petscsys(void)
#endif
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  /*
      If we got here then PETSc was properly loaded
  */
  ierr = PetscSysInitializePackage();CHKERRQ(ierr);
  ierr = PetscDrawInitializePackage();CHKERRQ(ierr);
  ierr = PetscViewerInitializePackage();CHKERRQ(ierr);
  ierr = PetscRandomInitializePackage();CHKERRQ(ierr);

#if defined(PETSC_USE_SINGLE_LIBRARY)
  ierr = PetscDLLibraryRegister_petscvec();CHKERRQ(ierr);
  ierr = PetscDLLibraryRegister_petscmat();CHKERRQ(ierr);
  ierr = PetscDLLibraryRegister_petscdm();CHKERRQ(ierr);
  ierr = PetscDLLibraryRegister_petscksp();CHKERRQ(ierr);
  ierr = PetscDLLibraryRegister_petscsnes();CHKERRQ(ierr);
  ierr = PetscDLLibraryRegister_petscts();CHKERRQ(ierr);
#endif
  PetscFunctionReturn(0);
}
#endif  /* PETSC_HAVE_DYNAMIC_LIBRARIES */
