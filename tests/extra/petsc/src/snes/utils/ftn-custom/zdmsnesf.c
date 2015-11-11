#include <petsc/private/fortranimpl.h>
#include <petsc/private/snesimpl.h>
#if defined(PETSC_HAVE_FORTRAN_CAPS)
#define dmsnessetjacobian_      DMSNESSETJACOBIAN
#define dmsnessetfunction_      DMSNESSETFUNCTION
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE)
#define dmsnessetjacobian_      dmsnessetjacobian
#define dmsnessetfunction_      dmsnessetfunction
#endif

static struct {
  PetscFortranCallbackId snesfunction;
  PetscFortranCallbackId snesjacobian;
} _cb;

#undef __FUNCT__
#define __FUNCT__ "ourj"
static PetscErrorCode ourj(SNES snes, Vec X, Mat J, Mat P, void *ptr)
{
  PetscErrorCode ierr;
  void (PETSC_STDCALL *func)(SNES*,Vec*,Mat*,Mat*,void*,PetscErrorCode*),*ctx;
  DM dm;
  DMSNES sdm;

  PetscFunctionBegin;
  ierr = SNESGetDM(snes,&dm);CHKERRQ(ierr);
  ierr = DMGetDMSNES(dm, &sdm);CHKERRQ(ierr);
  ierr = PetscObjectGetFortranCallback((PetscObject) sdm, PETSC_FORTRAN_CALLBACK_SUBTYPE, _cb.snesjacobian, (PetscVoidFunction *) &func, &ctx);CHKERRQ(ierr);
  (*func)(&snes, &X, &J, &P, ctx, &ierr);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

PETSC_EXTERN void PETSC_STDCALL dmsnessetjacobian_(DM *dm, void (PETSC_STDCALL *jac)(DM*,Vec*,Mat*,Mat*,void*,PetscErrorCode*), void *ctx, PetscErrorCode *ierr)
{
  DMSNES sdm;

  *ierr = DMGetDMSNESWrite(*dm, &sdm); if (*ierr) return;
  *ierr = PetscObjectSetFortranCallback((PetscObject) sdm, PETSC_FORTRAN_CALLBACK_SUBTYPE, &_cb.snesjacobian, (PetscVoidFunction) jac, ctx); if (*ierr) return;
  *ierr = DMSNESSetJacobian(*dm, ourj, NULL);
}

#undef __FUNCT__
#define __FUNCT__ "ourf"
static PetscErrorCode ourf(SNES snes, Vec X, Vec F, void *ptr)
{
  PetscErrorCode ierr;
  void (PETSC_STDCALL *func)(SNES*,Vec*,Vec*,void*,PetscErrorCode*), *ctx;
  DM dm;
  DMSNES sdm;

  PetscFunctionBegin;
  ierr = SNESGetDM(snes,&dm);CHKERRQ(ierr);
  ierr = DMGetDMSNES(dm, &sdm);CHKERRQ(ierr);
  ierr = PetscObjectGetFortranCallback((PetscObject) sdm, PETSC_FORTRAN_CALLBACK_SUBTYPE, _cb.snesfunction, (PetscVoidFunction *) &func, &ctx);CHKERRQ(ierr);
  (*func)(&snes, &X, &F, ctx, &ierr);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

PETSC_EXTERN void PETSC_STDCALL dmsnessetfunction_(DM *dm, void (PETSC_STDCALL *func)(SNES*,Vec*,Vec*,void*,PetscErrorCode*), void *ctx, PetscErrorCode *ierr)
{
  DMSNES sdm;

  *ierr = DMGetDMSNESWrite(*dm, &sdm); if (*ierr) return;
  *ierr = PetscObjectSetFortranCallback((PetscObject) sdm, PETSC_FORTRAN_CALLBACK_SUBTYPE, &_cb.snesfunction, (PetscVoidFunction) func, ctx); if (*ierr) return;
  *ierr = DMSNESSetFunction(*dm, ourf, NULL);
}
