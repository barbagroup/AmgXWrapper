
#include <petsc/private/dmdaimpl.h>
#include <petsc/private/dmpleximpl.h>
#include <petsc/private/petscdsimpl.h>
#include <petsc/private/petscfeimpl.h>
#include <petsc/private/petscfvimpl.h>

static PetscBool DMPackageInitialized = PETSC_FALSE;
#undef __FUNCT__
#define __FUNCT__ "DMFinalizePackage"
/*@C
  DMFinalizePackage - This function finalizes everything in the DM package. It is called
  from PetscFinalize().

  Level: developer

.keywords: AO, initialize, package
.seealso: PetscInitialize()
@*/
PetscErrorCode  DMFinalizePackage(void)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscFunctionListDestroy(&PetscPartitionerList);CHKERRQ(ierr);
  ierr = PetscFunctionListDestroy(&DMList);CHKERRQ(ierr);
  DMPackageInitialized = PETSC_FALSE;
  DMRegisterAllCalled  = PETSC_FALSE;
  PetscPartitionerRegisterAllCalled = PETSC_FALSE;
  PetscFunctionReturn(0);
}

#if defined(PETSC_HAVE_HYPRE)
PETSC_EXTERN PetscErrorCode MatCreate_HYPREStruct(Mat);
#endif

#undef __FUNCT__
#define __FUNCT__ "DMInitializePackage"
/*@C
  DMInitializePackage - This function initializes everything in the DM package. It is called
  from PetscDLLibraryRegister() when using dynamic libraries, and on the first call to AOCreate()
  or DMDACreate() when using static libraries.

  Level: developer

.keywords: AO, initialize, package
.seealso: PetscInitialize()
@*/
PetscErrorCode  DMInitializePackage(void)
{
  char           logList[256];
  char           *className;
  PetscBool      opt;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (DMPackageInitialized) PetscFunctionReturn(0);
  DMPackageInitialized = PETSC_TRUE;

  /* Register Classes */
  ierr = PetscClassIdRegister("Distributed Mesh",&DM_CLASSID);CHKERRQ(ierr);
  ierr = PetscClassIdRegister("GraphPartitioner",&PETSCPARTITIONER_CLASSID);CHKERRQ(ierr);

#if defined(PETSC_HAVE_HYPRE)
  ierr = MatRegister(MATHYPRESTRUCT, MatCreate_HYPREStruct);CHKERRQ(ierr);
#endif

  /* Register Constructors */
  ierr = DMRegisterAll();CHKERRQ(ierr);
  /* Register Events */
  ierr = PetscLogEventRegister("DMConvert",              DM_CLASSID,&DM_Convert);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("DMGlobalToLocal",        DM_CLASSID,&DM_GlobalToLocal);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("DMLocalToGlobal",        DM_CLASSID,&DM_LocalToGlobal);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("DMLocatePoints",         DM_CLASSID,&DM_LocatePoints);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("DMCoarsen",              DM_CLASSID,&DM_Coarsen);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("DMCreateInterpolation",  DM_CLASSID,&DM_CreateInterpolation);CHKERRQ(ierr);

  ierr = PetscLogEventRegister("DMDALocalADFunc",        DM_CLASSID,&DMDA_LocalADFunction);CHKERRQ(ierr);

  ierr = PetscLogEventRegister("Mesh Partition",         PETSCPARTITIONER_CLASSID,&PETSCPARTITIONER_Partition);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("Mesh Migration",         DM_CLASSID,&DMPLEX_Migrate);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("DMPlexInterp",           DM_CLASSID,&DMPLEX_Interpolate);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("DMPlexDistribute",       DM_CLASSID,&DMPLEX_Distribute);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("DMPlexDistCones",        DM_CLASSID,&DMPLEX_DistributeCones);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("DMPlexDistLabels",       DM_CLASSID,&DMPLEX_DistributeLabels);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("DMPlexDistribSF",        DM_CLASSID,&DMPLEX_DistributeSF);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("DMPlexDistribOL",        DM_CLASSID,&DMPLEX_DistributeOverlap);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("DMPlexDistField",        DM_CLASSID,&DMPLEX_DistributeField);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("DMPlexDistData",         DM_CLASSID,&DMPLEX_DistributeData);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("DMPlexGToNBegin",        DM_CLASSID,&DMPLEX_GlobalToNaturalBegin);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("DMPlexGToNEnd",          DM_CLASSID,&DMPLEX_GlobalToNaturalEnd);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("DMPlexNToGBegin",        DM_CLASSID,&DMPLEX_NaturalToGlobalBegin);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("DMPlexNToGEnd",          DM_CLASSID,&DMPLEX_NaturalToGlobalEnd);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("DMPlexStratify",         DM_CLASSID,&DMPLEX_Stratify);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("DMPlexPrealloc",         DM_CLASSID,&DMPLEX_Preallocate);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("DMPlexResidualFE",       DM_CLASSID,&DMPLEX_ResidualFEM);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("DMPlexJacobianFE",       DM_CLASSID,&DMPLEX_JacobianFEM);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("DMPlexInterpFE",         DM_CLASSID,&DMPLEX_InterpolatorFEM);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("DMPlexInjectorFE",       DM_CLASSID,&DMPLEX_InjectorFEM);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("DMPlexIntegralFEM",      DM_CLASSID,&DMPLEX_IntegralFEM);CHKERRQ(ierr);
  ierr = PetscLogEventRegister("DMPlexCreateGmsh",       DM_CLASSID,&DMPLEX_CreateGmsh);CHKERRQ(ierr);
  /* Process info exclusions */
  ierr = PetscOptionsGetString(NULL,NULL, "-info_exclude", logList, 256, &opt);CHKERRQ(ierr);
  if (opt) {
    ierr = PetscStrstr(logList, "da", &className);CHKERRQ(ierr);
    if (className) {
      ierr = PetscInfoDeactivateClass(DM_CLASSID);CHKERRQ(ierr);
    }
  }
  /* Process summary exclusions */
  ierr = PetscOptionsGetString(NULL,NULL, "-log_summary_exclude", logList, 256, &opt);CHKERRQ(ierr);
  if (opt) {
    ierr = PetscStrstr(logList, "da", &className);CHKERRQ(ierr);
    if (className) {
      ierr = PetscLogEventDeactivateClass(DM_CLASSID);CHKERRQ(ierr);
    }
  }
  ierr = PetscRegisterFinalize(DMFinalizePackage);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
#include <petscfe.h>

static PetscBool PetscFEPackageInitialized = PETSC_FALSE;
#undef __FUNCT__
#define __FUNCT__ "PetscFEFinalizePackage"
/*@C
  PetscFEFinalizePackage - This function finalizes everything in the PetscFE package. It is called
  from PetscFinalize().

  Level: developer

.keywords: PetscFE, initialize, package
.seealso: PetscInitialize()
@*/
PetscErrorCode PetscFEFinalizePackage(void)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscFunctionListDestroy(&PetscSpaceList);CHKERRQ(ierr);
  ierr = PetscFunctionListDestroy(&PetscDualSpaceList);CHKERRQ(ierr);
  ierr = PetscFunctionListDestroy(&PetscFEList);CHKERRQ(ierr);
  PetscFEPackageInitialized       = PETSC_FALSE;
  PetscSpaceRegisterAllCalled     = PETSC_FALSE;
  PetscDualSpaceRegisterAllCalled = PETSC_FALSE;
  PetscFERegisterAllCalled        = PETSC_FALSE;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscFEInitializePackage"
/*@C
  PetscFEInitializePackage - This function initializes everything in the FE package. It is called
  from PetscDLLibraryRegister() when using dynamic libraries, and on the first call to PetscSpaceCreate()
  when using static libraries.

  Level: developer

.keywords: PetscFE, initialize, package
.seealso: PetscInitialize()
@*/
PetscErrorCode PetscFEInitializePackage(void)
{
  char           logList[256];
  char          *className;
  PetscBool      opt;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (PetscFEPackageInitialized) PetscFunctionReturn(0);
  PetscFEPackageInitialized = PETSC_TRUE;

  /* Register Classes */
  ierr = PetscClassIdRegister("Linear Space", &PETSCSPACE_CLASSID);CHKERRQ(ierr);
  ierr = PetscClassIdRegister("Dual Space",   &PETSCDUALSPACE_CLASSID);CHKERRQ(ierr);
  ierr = PetscClassIdRegister("FE Space",     &PETSCFE_CLASSID);CHKERRQ(ierr);

  /* Register Constructors */
  ierr = PetscSpaceRegisterAll();CHKERRQ(ierr);
  ierr = PetscDualSpaceRegisterAll();CHKERRQ(ierr);
  ierr = PetscFERegisterAll();CHKERRQ(ierr);
  /* Register Events */
  /* Process info exclusions */
  ierr = PetscOptionsGetString(NULL,NULL, "-info_exclude", logList, 256, &opt);CHKERRQ(ierr);
  if (opt) {
    ierr = PetscStrstr(logList, "fe", &className);CHKERRQ(ierr);
    if (className) {ierr = PetscInfoDeactivateClass(PETSCFE_CLASSID);CHKERRQ(ierr);}
  }
  /* Process summary exclusions */
  ierr = PetscOptionsGetString(NULL,NULL, "-log_summary_exclude", logList, 256, &opt);CHKERRQ(ierr);
  if (opt) {
    ierr = PetscStrstr(logList, "fe", &className);CHKERRQ(ierr);
    if (className) {ierr = PetscLogEventDeactivateClass(PETSCFE_CLASSID);CHKERRQ(ierr);}
  }
  ierr = PetscRegisterFinalize(PetscFEFinalizePackage);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
#include <petscfv.h>

static PetscBool PetscFVPackageInitialized = PETSC_FALSE;
#undef __FUNCT__
#define __FUNCT__ "PetscFVFinalizePackage"
/*@C
  PetscFVFinalizePackage - This function finalizes everything in the PetscFV package. It is called
  from PetscFinalize().

  Level: developer

.keywords: PetscFV, initialize, package
.seealso: PetscInitialize()
@*/
PetscErrorCode PetscFVFinalizePackage(void)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscFunctionListDestroy(&PetscLimiterList);CHKERRQ(ierr);
  ierr = PetscFunctionListDestroy(&PetscFVList);CHKERRQ(ierr);
  PetscFVPackageInitialized     = PETSC_FALSE;
  PetscFVRegisterAllCalled      = PETSC_FALSE;
  PetscLimiterRegisterAllCalled = PETSC_FALSE;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscFVInitializePackage"
/*@C
  PetscFVInitializePackage - This function initializes everything in the FV package. It is called
  from PetscDLLibraryRegister() when using dynamic libraries, and on the first call to PetscFVCreate()
  when using static libraries.

  Level: developer

.keywords: PetscFV, initialize, package
.seealso: PetscInitialize()
@*/
PetscErrorCode PetscFVInitializePackage(void)
{
  char           logList[256];
  char          *className;
  PetscBool      opt;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (PetscFVPackageInitialized) PetscFunctionReturn(0);
  PetscFVPackageInitialized = PETSC_TRUE;

  /* Register Classes */
  ierr = PetscClassIdRegister("FV Space", &PETSCFV_CLASSID);CHKERRQ(ierr);
  ierr = PetscClassIdRegister("Limiter",  &PETSCLIMITER_CLASSID);CHKERRQ(ierr);

  /* Register Constructors */
  ierr = PetscFVRegisterAll();CHKERRQ(ierr);
  /* Register Events */
  /* Process info exclusions */
  ierr = PetscOptionsGetString(NULL,NULL, "-info_exclude", logList, 256, &opt);CHKERRQ(ierr);
  if (opt) {
    ierr = PetscStrstr(logList, "fv", &className);CHKERRQ(ierr);
    if (className) {ierr = PetscInfoDeactivateClass(PETSCFV_CLASSID);CHKERRQ(ierr);}
    ierr = PetscStrstr(logList, "limiter", &className);CHKERRQ(ierr);
    if (className) {ierr = PetscInfoDeactivateClass(PETSCLIMITER_CLASSID);CHKERRQ(ierr);}
  }
  /* Process summary exclusions */
  ierr = PetscOptionsGetString(NULL,NULL, "-log_summary_exclude", logList, 256, &opt);CHKERRQ(ierr);
  if (opt) {
    ierr = PetscStrstr(logList, "fv", &className);CHKERRQ(ierr);
    if (className) {ierr = PetscLogEventDeactivateClass(PETSCFV_CLASSID);CHKERRQ(ierr);}
    ierr = PetscStrstr(logList, "limiter", &className);CHKERRQ(ierr);
    if (className) {ierr = PetscLogEventDeactivateClass(PETSCLIMITER_CLASSID);CHKERRQ(ierr);}
  }
  ierr = PetscRegisterFinalize(PetscFVFinalizePackage);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
#include <petscds.h>

static PetscBool PetscDSPackageInitialized = PETSC_FALSE;
#undef __FUNCT__
#define __FUNCT__ "PetscDSFinalizePackage"
/*@C
  PetscDSFinalizePackage - This function finalizes everything in the PetscDS package. It is called
  from PetscFinalize().

  Level: developer

.keywords: PetscDS, initialize, package
.seealso: PetscInitialize()
@*/
PetscErrorCode PetscDSFinalizePackage(void)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscFunctionListDestroy(&PetscDSList);CHKERRQ(ierr);
  PetscDSPackageInitialized = PETSC_FALSE;
  PetscDSRegisterAllCalled  = PETSC_FALSE;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDSInitializePackage"
/*@C
  PetscDSInitializePackage - This function initializes everything in the DS package. It is called
  from PetscDLLibraryRegister() when using dynamic libraries, and on the first call to PetscDSCreate()
  when using static libraries.

  Level: developer

.keywords: PetscDS, initialize, package
.seealso: PetscInitialize()
@*/
PetscErrorCode PetscDSInitializePackage(void)
{
  char           logList[256];
  char          *className;
  PetscBool      opt;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (PetscDSPackageInitialized) PetscFunctionReturn(0);
  PetscDSPackageInitialized = PETSC_TRUE;

  /* Register Classes */
  ierr = PetscClassIdRegister("Discrete System", &PETSCDS_CLASSID);CHKERRQ(ierr);

  /* Register Constructors */
  ierr = PetscDSRegisterAll();CHKERRQ(ierr);
  /* Register Events */
  /* Process info exclusions */
  ierr = PetscOptionsGetString(NULL,NULL, "-info_exclude", logList, 256, &opt);CHKERRQ(ierr);
  if (opt) {
    ierr = PetscStrstr(logList, "ds", &className);CHKERRQ(ierr);
    if (className) {ierr = PetscInfoDeactivateClass(PETSCDS_CLASSID);CHKERRQ(ierr);}
  }
  /* Process summary exclusions */
  ierr = PetscOptionsGetString(NULL,NULL, "-log_summary_exclude", logList, 256, &opt);CHKERRQ(ierr);
  if (opt) {
    ierr = PetscStrstr(logList, "ds", &className);CHKERRQ(ierr);
    if (className) {ierr = PetscLogEventDeactivateClass(PETSCDS_CLASSID);CHKERRQ(ierr);}
  }
  ierr = PetscRegisterFinalize(PetscDSFinalizePackage);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#if defined(PETSC_HAVE_DYNAMIC_LIBRARIES)
#undef __FUNCT__
#define __FUNCT__ "PetscDLLibraryRegister_petscdm"
/*
  PetscDLLibraryRegister - This function is called when the dynamic library it is in is opened.

  This one registers all the mesh generators and partitioners that are in
  the basic DM library.

*/
PETSC_EXTERN PetscErrorCode PetscDLLibraryRegister_petscdm(void)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = AOInitializePackage();CHKERRQ(ierr);
  ierr = DMInitializePackage();CHKERRQ(ierr);
  ierr = PetscFEInitializePackage();CHKERRQ(ierr);
  ierr = PetscFVInitializePackage();CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#endif /* PETSC_HAVE_DYNAMIC_LIBRARIES */
