
#include <petscsys.h> /*I "petscsys.h" I*/

#if defined(PETSC_HAVE_SSE)

#include PETSC_HAVE_SSE
#define SSE_FEATURE_FLAG 0x2000000 /* Mask for bit 25 (from bit 0) */

#undef __FUNCT__
#define __FUNCT__ "PetscSSEHardwareTest"
PetscErrorCode  PetscSSEHardwareTest(PetscBool  *flag)
{
  PetscErrorCode ierr;
  char           *vendor;
  char           Intel[13]="GenuineIntel";
  char           AMD[13]  ="AuthenticAMD";

  PetscFunctionBegin;
  ierr = PetscMalloc1(13,&vendor);CHKERRQ(ierr);
  strcpy(vendor,"************");
  CPUID_GET_VENDOR(vendor);
  if (!strcmp(vendor,Intel) || !strcmp(vendor,AMD)) {
    /* Both Intel and AMD use bit 25 of CPUID_FEATURES */
    /* to denote availability of SSE Support */
    unsigned long myeax,myebx,myecx,myedx;
    CPUID(CPUID_FEATURES,&myeax,&myebx,&myecx,&myedx);
    if (myedx & SSE_FEATURE_FLAG) *flag = PETSC_TRUE;
    else *flag = PETSC_FALSE;
  }
  ierr = PetscFree(vendor);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#if defined(PETSC_HAVE_FORK)
#include <signal.h>
/*
   Early versions of the Linux kernel disables SSE hardware because
   it does not know how to preserve the SSE state at a context switch.
   To detect this feature, try an sse instruction in another process.
   If it works, great!  If not, an illegal instruction signal will be thrown,
   so catch it and return an error code.
*/
#define PetscSSEOSEnabledTest(arg) PetscSSEOSEnabledTest_Linux(arg)

static void PetscSSEDisabledHandler(int sig)
{
  signal(SIGILL,SIG_IGN);
  exit(-1);
}

#undef __FUNCT__
#define __FUNCT__ "PetscSSEOSEnabledTest_Linux"
PetscErrorCode  PetscSSEOSEnabledTest_Linux(PetscBool  *flag)
{
  int status, pid = 0;

  PetscFunctionBegin;
  signal(SIGILL,PetscSSEDisabledHandler);
  pid = fork();
  if (pid==0) {
    SSE_SCOPE_BEGIN;
    XOR_PS(XMM0,XMM0);
    SSE_SCOPE_END;
    exit(0);
  } else wait(&status);
  if (!status) *flag = PETSC_TRUE;
  else *flag = PETSC_FALSE;
  PetscFunctionReturn(0);
}

#else
/*
   Windows 95/98/NT4 should have a Windows Update/Service Patch which enables this hardware.
   Windows ME/2000 doesn't disable SSE Hardware
*/
#define PetscSSEOSEnabledTest(arg) PetscSSEOSEnabledTest_TRUE(arg)
#endif

#undef __FUNCT__
#define __FUNCT__ "PetscSSEOSEnabledTest_TRUE"
PetscErrorCode  PetscSSEOSEnabledTest_TRUE(PetscBool  *flag)
{
  PetscFunctionBegin;
  if (flag) *flag = PETSC_TRUE;
  PetscFunctionReturn(0);
}

#else  /* Not defined PETSC_HAVE_SSE */

#define PetscSSEHardwareTest(arg) PetscSSEEnabledTest_FALSE(arg)
#define PetscSSEOSEnabledTest(arg) PetscSSEEnabledTest_FALSE(arg)

#undef __FUNCT__
#define __FUNCT__ "PetscSSEEnabledTest_FALSE"
PetscErrorCode  PetscSSEEnabledTest_FALSE(PetscBool  *flag)
{
  PetscFunctionBegin;
  if (flag) *flag = PETSC_FALSE;
  PetscFunctionReturn(0);
}

#endif /* defined PETSC_HAVE_SSE */

#undef __FUNCT__
#define __FUNCT__ "PetscSSEIsEnabled"
/*@C
     PetscSSEIsEnabled - Determines if Intel Streaming SIMD Extensions (SSE) to the x86 instruction
     set can be used.  Some operating systems do not allow the use of these instructions despite
     hardware availability.

     Collective on MPI_Comm

     Input Parameter:
.    comm - the MPI Communicator

     Output Parameters:
.    lflag - Local Flag:  PETSC_TRUE if enabled in this process
.    gflag - Global Flag: PETSC_TRUE if enabled for all processes in comm

     Notes:
     NULL can be specified for lflag or gflag if either of these values are not desired.

     Options Database Keys:
.    -disable_sse - Disable use of hand tuned Intel SSE implementations

     Level: developer
@*/
static PetscBool petsc_sse_local_is_untested  = PETSC_TRUE;
static PetscBool petsc_sse_enabled_local      = PETSC_FALSE;
static PetscBool petsc_sse_global_is_untested = PETSC_TRUE;
static PetscBool petsc_sse_enabled_global     = PETSC_FALSE;
PetscErrorCode  PetscSSEIsEnabled(MPI_Comm comm,PetscBool  *lflag,PetscBool  *gflag)
{
  PetscErrorCode ierr;
  PetscBool      disabled_option;

  PetscFunctionBegin;
  if (petsc_sse_local_is_untested && petsc_sse_global_is_untested) {
    disabled_option = PETSC_FALSE;

    ierr = PetscOptionsGetBool(NULL,NULL,"-disable_sse",&disabled_option,NULL);CHKERRQ(ierr);
    if (disabled_option) {
      petsc_sse_local_is_untested  = PETSC_FALSE;
      petsc_sse_enabled_local      = PETSC_FALSE;
      petsc_sse_global_is_untested = PETSC_FALSE;
      petsc_sse_enabled_global     = PETSC_FALSE;
    }

    if (petsc_sse_local_is_untested) {
      ierr = PetscSSEHardwareTest(&petsc_sse_enabled_local);CHKERRQ(ierr);
      if (petsc_sse_enabled_local) {
        ierr = PetscSSEOSEnabledTest(&petsc_sse_enabled_local);CHKERRQ(ierr);
      }
      petsc_sse_local_is_untested = PETSC_FALSE;
    }

    if (gflag && petsc_sse_global_is_untested) {
      ierr = MPIU_Allreduce(&petsc_sse_enabled_local,&petsc_sse_enabled_global,1,MPIU_BOOL,MPI_LAND,comm);CHKERRQ(ierr);

      petsc_sse_global_is_untested = PETSC_FALSE;
    }
  }

  if (lflag) *lflag = petsc_sse_enabled_local;
  if (gflag) *gflag = petsc_sse_enabled_global;
  PetscFunctionReturn(0);
}


