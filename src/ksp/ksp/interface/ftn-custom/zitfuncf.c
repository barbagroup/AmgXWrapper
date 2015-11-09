#include <petsc/private/fortranimpl.h>
#include <petscksp.h>

#if defined(PETSC_HAVE_FORTRAN_CAPS)
#define kspmonitorset_             KSPMONITORSET
#define kspsetconvergencetest_     KSPSETCONVERGENCETEST
#define kspgetresidualhistory_     KSPGETRESIDUALHISTORY

#define kspconvergeddefault_         KSPCONVERGEDDEFAULT
#define kspconvergeddefaultcreate_   KSPCONVERGEDDEFAULTCREATE
#define kspconvergeddefaultdestroy_  KSPCONVERGEDDEFAULTDESTROY
#define kspconvergedskip_            KSPCONVERGEDSKIP
#define kspgmresmonitorkrylov_     KSPGMRESMONITORKRYLOV
#define kspmonitordefault_         KSPMONITORDEFAULT
#define kspmonitortrueresidualnorm_    KSPMONITORTRUERESIDUALNORM
#define kspmonitorsolution_        KSPMONITORSOLUTION
#define kspmonitorlgresidualnorm_      KSPMONITORLGRESIDUALNORM
#define kspmonitorlgtrueresidualnorm_  KSPMONITORLGTRUERESIDUALNORM
#define kspmonitorsingularvalue_   KSPMONITORSINGULARVALUE
#define kspsetcomputerhs_              KSPSETCOMPUTERHS
#define kspsetcomputeinitialguess_     KSPSETCOMPUTEINITIALGUESS
#define kspsetcomputeoperators_        KSPSETCOMPUTEOPERATORS
#define dmkspsetcomputerhs_            DMKSPSETCOMPUTERHS          /* zdmkspf.c */
#define dmkspsetcomputeinitialguess_   DMKSPSETCOMPUTEINITIALGUESS /* zdmkspf.c */
#define dmkspsetcomputeoperators_      DMKSPSETCOMPUTEOPERATORS    /* zdmkspf.c */
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE)
#define kspmonitorset_                 kspmonitorset
#define kspsetconvergencetest_         kspsetconvergencetest
#define kspgetresidualhistory_         kspgetresidualhistory
#define kspconvergeddefault_           kspconvergeddefault
#define kspconvergeddefaultcreate_     kspconvergeddefaultcreate
#define kspconvergeddefaultdestroy_    kspconvergeddefaultdestroy
#define kspconvergedskip_              kspconvergedskip
#define kspmonitorsingularvalue_       kspmonitorsingularvalue
#define kspgmresmonitorkrylov_         kspgmresmonitorkrylov
#define kspmonitordefault_             kspmonitordefault
#define kspmonitortrueresidualnorm_    kspmonitortrueresidualnorm
#define kspmonitorsolution_            kspmonitorsolution
#define kspmonitorlgresidualnorm_      kspmonitorlgresidualnorm
#define kspmonitorlgtrueresidualnorm_  kspmonitorlgtrueresidualnorm
#define kspsetcomputerhs_              kspsetcomputerhs
#define kspsetcomputeinitialguess_     kspsetcomputeinitialguess
#define kspsetcomputeoperators_        kspsetcomputeoperators
#define dmkspsetcomputerhs_            dmkspsetcomputerhs          /* zdmkspf.c */
#define dmkspsetcomputeinitialguess_   dmkspsetcomputeinitialguess /* zdmkspf.c */
#define dmkspsetcomputeoperators_      dmkspsetcomputeoperators    /* zdmkspf.c */
#endif

/* These are defined in zdmkspf.c */
PETSC_EXTERN void PETSC_STDCALL dmkspsetcomputerhs_(DM *dm,void (PETSC_STDCALL *func)(KSP*,Vec*,void*,PetscErrorCode*),void *ctx,PetscErrorCode *ierr);
PETSC_EXTERN void PETSC_STDCALL dmkspsetcomputeinitialguess_(DM *dm,void (PETSC_STDCALL *func)(KSP*,Vec*,void*,PetscErrorCode*),void *ctx,PetscErrorCode *ierr);
PETSC_EXTERN void PETSC_STDCALL dmkspsetcomputeoperators_(DM *dm,void (PETSC_STDCALL *func)(KSP*,Vec*,void*,PetscErrorCode*),void *ctx,PetscErrorCode *ierr);

/*
        These are not usually called from Fortran but allow Fortran users
   to transparently set these monitors from .F code

   functions, hence no STDCALL
*/

PETSC_EXTERN void kspconvergeddefault_(KSP *ksp,PetscInt *n,PetscReal *rnorm,KSPConvergedReason *flag,void *dummy,PetscErrorCode *ierr)
{
  CHKFORTRANNULLOBJECT(dummy);
  *ierr = KSPConvergedDefault(*ksp,*n,*rnorm,flag,dummy);
}

PETSC_EXTERN void kspconvergedskip_(KSP *ksp,PetscInt *n,PetscReal *rnorm,KSPConvergedReason *flag,void *dummy,PetscErrorCode *ierr)
{
  CHKFORTRANNULLOBJECT(dummy);
  *ierr = KSPConvergedSkip(*ksp,*n,*rnorm,flag,dummy);
}

PETSC_EXTERN void kspgmresmonitorkrylov_(KSP *ksp,PetscInt *it,PetscReal *norm,void *ctx,PetscErrorCode *ierr)
{
  *ierr = KSPGMRESMonitorKrylov(*ksp,*it,*norm,ctx);
}

PETSC_EXTERN void  kspmonitordefault_(KSP *ksp,PetscInt *it,PetscReal *norm,void *ctx,PetscErrorCode *ierr)
{
  *ierr = KSPMonitorDefault(*ksp,*it,*norm,ctx);
}

PETSC_EXTERN void  kspmonitorsingularvalue_(KSP *ksp,PetscInt *it,PetscReal *norm,void *ctx,PetscErrorCode *ierr)
{
  *ierr = KSPMonitorSingularValue(*ksp,*it,*norm,ctx);
}

PETSC_EXTERN void  kspmonitorlgresidualnorm_(KSP *ksp,PetscInt *it,PetscReal *norm,PetscObject *ctx,PetscErrorCode *ierr)
{
  *ierr = KSPMonitorLGResidualNorm(*ksp,*it,*norm,ctx);
}

PETSC_EXTERN void  kspmonitorlgtrueresidualnorm_(KSP *ksp,PetscInt *it,PetscReal *norm,PetscObject *ctx,PetscErrorCode *ierr)
{
  *ierr = KSPMonitorLGTrueResidualNorm(*ksp,*it,*norm,ctx);
}

PETSC_EXTERN void  kspmonitortrueresidualnorm_(KSP *ksp,PetscInt *it,PetscReal *norm,void *ctx,PetscErrorCode *ierr)
{
  *ierr = KSPMonitorTrueResidualNorm(*ksp,*it,*norm,ctx);
}

PETSC_EXTERN void  kspmonitorsolution_(KSP *ksp,PetscInt *it,PetscReal *norm,void *ctx,PetscErrorCode *ierr)
{
  *ierr = KSPMonitorSolution(*ksp,*it,*norm,ctx);
}

static struct {
  PetscFortranCallbackId monitor;
  PetscFortranCallbackId monitordestroy;
  PetscFortranCallbackId test;
  PetscFortranCallbackId testdestroy;
} _cb;

#undef __FUNCT__
#define __FUNCT__ "ourmonitor"
static PetscErrorCode ourmonitor(KSP ksp,PetscInt i,PetscReal d,void *ctx)
{
  PetscObjectUseFortranCallback(ksp,_cb.monitor,(KSP*,PetscInt*,PetscReal*,void*,PetscErrorCode*),(&ksp,&i,&d,_ctx,&ierr));
}

#undef __FUNCT__
#define __FUNCT__ "ourdestroy"
static PetscErrorCode ourdestroy(void **ctx)
{
  KSP ksp = (KSP)*ctx;
  PetscObjectUseFortranCallback(ksp,_cb.monitordestroy,(void*,PetscErrorCode*),(_ctx,&ierr));
}

/* These are not extern C because they are passed into non-extern C user level functions */
#undef __FUNCT__
#define __FUNCT__ "ourtest"
static PetscErrorCode ourtest(KSP ksp,PetscInt i,PetscReal d,KSPConvergedReason *reason,void *ctx)
{
  PetscObjectUseFortranCallback(ksp,_cb.test,(KSP*,PetscInt*,PetscReal*,KSPConvergedReason*,void*,PetscErrorCode*),(&ksp,&i,&d,reason,_ctx,&ierr));
}

#undef __FUNCT__
#define __FUNCT__ "ourtestdestroy"
static PetscErrorCode ourtestdestroy(void *ctx)
{
  KSP ksp = (KSP)ctx;
  PetscObjectUseFortranCallback(ksp,_cb.testdestroy,(void*,PetscErrorCode*),(_ctx,&ierr));
}

PETSC_EXTERN void PETSC_STDCALL kspmonitorset_(KSP *ksp,void (PETSC_STDCALL *monitor)(KSP*,PetscInt*,PetscReal*,void*,PetscErrorCode*),
                                  void *mctx,void (PETSC_STDCALL *monitordestroy)(void*,PetscErrorCode*),PetscErrorCode *ierr)
{
  CHKFORTRANNULLOBJECT(mctx);
  CHKFORTRANNULLFUNCTION(monitordestroy);

  if ((PetscVoidFunction)monitor == (PetscVoidFunction)kspmonitordefault_) {
    *ierr = KSPMonitorSet(*ksp,KSPMonitorDefault,0,0);
  } else if ((PetscVoidFunction)monitor == (PetscVoidFunction)kspmonitorlgresidualnorm_) {
    *ierr = KSPMonitorSet(*ksp,(PetscErrorCode (*)(KSP,PetscInt,PetscReal,void*))KSPMonitorLGResidualNorm,0,0);
  } else if ((PetscVoidFunction)monitor == (PetscVoidFunction)kspmonitorlgtrueresidualnorm_) {
    *ierr = KSPMonitorSet(*ksp,(PetscErrorCode (*)(KSP,PetscInt,PetscReal,void*))KSPMonitorLGTrueResidualNorm,0,0);
  } else if ((PetscVoidFunction)monitor == (PetscVoidFunction)kspmonitorsolution_) {
    *ierr = KSPMonitorSet(*ksp,KSPMonitorSolution,0,0);
  } else if ((PetscVoidFunction)monitor == (PetscVoidFunction)kspmonitortrueresidualnorm_) {
    *ierr = KSPMonitorSet(*ksp,KSPMonitorTrueResidualNorm,0,0);
  } else if ((PetscVoidFunction)monitor == (PetscVoidFunction)kspmonitorsingularvalue_) {
    *ierr = KSPMonitorSet(*ksp,KSPMonitorSingularValue,0,0);
  } else if ((PetscVoidFunction)monitor == (PetscVoidFunction)kspgmresmonitorkrylov_) {
    *ierr = KSPMonitorSet(*ksp,KSPGMRESMonitorKrylov,0,0);
  } else {
    *ierr = PetscObjectSetFortranCallback((PetscObject)*ksp,PETSC_FORTRAN_CALLBACK_CLASS,&_cb.monitor,(PetscVoidFunction)monitor,mctx); if (*ierr) return;
    if (!monitordestroy) {
      *ierr = KSPMonitorSet(*ksp,ourmonitor,*ksp,0);
    } else {
      *ierr = PetscObjectSetFortranCallback((PetscObject)*ksp,PETSC_FORTRAN_CALLBACK_CLASS,&_cb.monitordestroy,(PetscVoidFunction)monitordestroy,mctx); if (*ierr) return;
      *ierr = KSPMonitorSet(*ksp,ourmonitor,*ksp,ourdestroy);
    }
  }
}

PETSC_EXTERN void PETSC_STDCALL kspsetconvergencetest_(KSP *ksp,
      void (PETSC_STDCALL *converge)(KSP*,PetscInt*,PetscReal*,KSPConvergedReason*,void*,PetscErrorCode*),void **cctx,
      void (PETSC_STDCALL *destroy)(void*,PetscErrorCode*),PetscErrorCode *ierr)
{
  CHKFORTRANNULLOBJECT(cctx);
  CHKFORTRANNULLFUNCTION(destroy);

  if ((PetscVoidFunction)converge == (PetscVoidFunction)kspconvergeddefault_) {
    *ierr = KSPSetConvergenceTest(*ksp,KSPConvergedDefault,*cctx,KSPConvergedDefaultDestroy);
  } else if ((PetscVoidFunction)converge == (PetscVoidFunction)kspconvergedskip_) {
    *ierr = KSPSetConvergenceTest(*ksp,KSPConvergedSkip,0,0);
  } else {
    *ierr = PetscObjectSetFortranCallback((PetscObject)*ksp,PETSC_FORTRAN_CALLBACK_CLASS,&_cb.test,(PetscVoidFunction)converge,cctx); if (*ierr) return;
    if (!destroy) {
      *ierr = KSPSetConvergenceTest(*ksp,ourtest,*ksp,0);
    } else {
      *ierr = PetscObjectSetFortranCallback((PetscObject)*ksp,PETSC_FORTRAN_CALLBACK_CLASS,&_cb.testdestroy,(PetscVoidFunction)destroy,cctx); if (*ierr) return;
      *ierr = KSPSetConvergenceTest(*ksp,ourtest,*ksp,ourtestdestroy);
    }
  }
}

PETSC_EXTERN void PETSC_STDCALL kspconvergeddefaultcreate_(PetscFortranAddr *ctx,PetscErrorCode *ierr)
{
  *ierr = KSPConvergedDefaultCreate((void**)ctx);
}

PETSC_EXTERN void PETSC_STDCALL kspconvergeddefaultdestroy_(PetscFortranAddr *ctx,PetscErrorCode *ierr)
{
  *ierr = KSPConvergedDefaultDestroy(*(void**)ctx);
}

PETSC_EXTERN void PETSC_STDCALL kspgetresidualhistory_(KSP *ksp,PetscInt *na,PetscErrorCode *ierr)
{
  *ierr = KSPGetResidualHistory(*ksp,NULL,na);
}

PETSC_EXTERN void PETSC_STDCALL kspsetcomputerhs_(KSP *ksp,void (PETSC_STDCALL *func)(KSP*,Vec*,void*,PetscErrorCode*),void *ctx,PetscErrorCode *ierr)
{
  DM dm;
  *ierr = KSPGetDM(*ksp,&dm);
  if (!*ierr) dmkspsetcomputerhs_(&dm,func,ctx,ierr);
}

PETSC_EXTERN void PETSC_STDCALL kspsetcomputeinitialguess_(KSP *ksp,void (PETSC_STDCALL *func)(KSP*,Vec*,void*,PetscErrorCode*),void *ctx,PetscErrorCode *ierr)
{
  DM dm;
  *ierr = KSPGetDM(*ksp,&dm);
  if (!*ierr) dmkspsetcomputeinitialguess_(&dm,func,ctx,ierr);
}

PETSC_EXTERN void PETSC_STDCALL kspsetcomputeoperators_(KSP *ksp,void (PETSC_STDCALL *func)(KSP*,Vec*,void*,PetscErrorCode*),void *ctx,PetscErrorCode *ierr)
{
  DM dm;
  *ierr = KSPGetDM(*ksp,&dm);
  if (!*ierr) dmkspsetcomputeoperators_(&dm,func,ctx,ierr);
}

