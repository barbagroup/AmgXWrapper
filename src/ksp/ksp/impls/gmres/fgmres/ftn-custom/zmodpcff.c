#include <petsc/private/fortranimpl.h>
#include <petscksp.h>

#if defined(PETSC_HAVE_FORTRAN_CAPS)
#define kspfgmressetmodifypc_      KSPFGMRESSETMODIFYPC
#define kspfgmresmodifypcnochange_ KSPFGMRESMODIFYPCNOCHANGE
#define kspfgmresmodifypcksp_      KSPFGMRESMODIFYPCKSP
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE)
#define kspfgmressetmodifypc_      kspfgmressetmodifypc
#define kspfgmresmodifypcnochange_ kspfgmresmodifypcnochange
#define kspfgmresmodifypcksp_      kspfgmresmodifypcksp
#endif

static struct {
  PetscFortranCallbackId modify;
  PetscFortranCallbackId destroy;
} _cb;

#undef __FUNCT__
#define __FUNCT__ "ourmodify"
static PetscErrorCode ourmodify(KSP ksp,PetscInt i,PetscInt i2,PetscReal d,void *ctx)
{
  PetscObjectUseFortranCallbackSubType(ksp,_cb.modify,(KSP*,PetscInt*,PetscInt*,PetscReal*,void*,PetscErrorCode*),(&ksp,&i,&i2,&d,_ctx,&ierr));
}

#undef __FUNCT__
#define __FUNCT__ "ourmoddestroy"
static PetscErrorCode ourmoddestroy(void *ctx)
{
  KSP ksp = (KSP)ctx;
  PetscObjectUseFortranCallbackSubType(ksp,_cb.destroy,(void*,PetscErrorCode*),(_ctx,&ierr));
}

PETSC_EXTERN void PETSC_STDCALL kspfgmresmodifypcnochange_(KSP*,PetscInt*,PetscInt*,PetscReal*,void*,PetscErrorCode*);
PETSC_EXTERN void PETSC_STDCALL kspfgmresmodifypcksp_(KSP*,PetscInt*,PetscInt*,PetscReal*,void*,PetscErrorCode*);

PETSC_EXTERN void PETSC_STDCALL kspfgmressetmodifypc_(KSP *ksp,void (PETSC_STDCALL *fcn)(KSP*,PetscInt*,PetscInt*,PetscReal*,void*,PetscErrorCode*),void* ctx,void (PETSC_STDCALL *d)(void*,PetscErrorCode*),PetscErrorCode *ierr)
{
  if ((PetscVoidFunction)fcn == (PetscVoidFunction)kspfgmresmodifypcksp_) {
    *ierr = KSPFGMRESSetModifyPC(*ksp,KSPFGMRESModifyPCKSP,0,0);
  } else if ((PetscVoidFunction)fcn == (PetscVoidFunction)kspfgmresmodifypcnochange_) {
    *ierr = KSPFGMRESSetModifyPC(*ksp,KSPFGMRESModifyPCNoChange,0,0);
  } else {
    *ierr = PetscObjectSetFortranCallback((PetscObject)*ksp,PETSC_FORTRAN_CALLBACK_SUBTYPE,&_cb.modify,(PetscVoidFunction)fcn,ctx); if (*ierr) return;
    if (FORTRANNULLFUNCTION(d)) {
      *ierr = KSPFGMRESSetModifyPC(*ksp,ourmodify,*ksp,0);
    } else {
      *ierr = PetscObjectSetFortranCallback((PetscObject)*ksp,PETSC_FORTRAN_CALLBACK_SUBTYPE,&_cb.destroy,(PetscVoidFunction)d,ctx); if (*ierr) return;
      *ierr = KSPFGMRESSetModifyPC(*ksp,ourmodify,*ksp,ourmoddestroy);
    }
  }
}
