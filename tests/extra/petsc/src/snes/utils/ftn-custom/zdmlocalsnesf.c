#include <petsc/private/fortranimpl.h>
#include <petsc/private/snesimpl.h>
#if defined(PETSC_HAVE_FORTRAN_CAPS)
#define dmsnessetjacobianlocal_      DMSNESSETJACOBIANLOCAL
#define dmsnessetfunctionlocal_      DMSNESSETFUNCTIONLOCAL
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE)
#define dmsnessetjacobianlocal_      dmsnessetjacobianlocal
#define dmsnessetfunctionlocal_      dmsnessetfunctionlocal
#endif

static struct {
  PetscFortranCallbackId lf;
  PetscFortranCallbackId lj;
} _cb;

#undef __FUNCT__
#define __FUNCT__ "sourlj"
static PetscErrorCode sourlj(DM dm, Vec X, Mat J, Mat P, void *ptr)
{
  PetscErrorCode ierr;
  void (PETSC_STDCALL *func)(DM*,Vec*,Mat*,Mat*,void*,PetscErrorCode*),*ctx;
  DMSNES sdm;

  PetscFunctionBegin;
  ierr = DMGetDMSNES(dm, &sdm);CHKERRQ(ierr);
  ierr = PetscObjectGetFortranCallback((PetscObject) sdm, PETSC_FORTRAN_CALLBACK_SUBTYPE, _cb.lj, (PetscVoidFunction *) &func, &ctx);CHKERRQ(ierr);
  (*func)(&dm, &X, &J, &P, ctx, &ierr);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

PETSC_EXTERN void PETSC_STDCALL dmsnessetjacobianlocal_(DM *dm, void (PETSC_STDCALL *jac)(DM*,Vec*,Mat*,Mat*,void*,PetscErrorCode*), void *ctx, PetscErrorCode *ierr)
{
  DMSNES sdm;

  *ierr = DMGetDMSNESWrite(*dm, &sdm); if (*ierr) return;
  *ierr = PetscObjectSetFortranCallback((PetscObject) sdm, PETSC_FORTRAN_CALLBACK_SUBTYPE, &_cb.lj, (PetscVoidFunction) jac, ctx); if (*ierr) return;
  *ierr = DMSNESSetJacobianLocal(*dm, sourlj, NULL);
}

#undef __FUNCT__
#define __FUNCT__ "sourlf"
static PetscErrorCode sourlf(DM dm, Vec X, Vec F, void *ptr)
{
  PetscErrorCode ierr;
  void (PETSC_STDCALL *func)(DM*,Vec*,Vec*,void*,PetscErrorCode*), *ctx;
  DMSNES sdm;

  PetscFunctionBegin;
  ierr = DMGetDMSNES(dm, &sdm);CHKERRQ(ierr);
  ierr = PetscObjectGetFortranCallback((PetscObject) sdm, PETSC_FORTRAN_CALLBACK_SUBTYPE, _cb.lf, (PetscVoidFunction *) &func, &ctx);CHKERRQ(ierr);
  (*func)(&dm, &X, &F, ctx, &ierr);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

PETSC_EXTERN void PETSC_STDCALL dmsnessetfunctionlocal_(DM *dm, void (PETSC_STDCALL *func)(DM*,Vec*,Vec*,void*,PetscErrorCode*), void *ctx, PetscErrorCode *ierr)
{
  DMSNES sdm;

  *ierr = DMGetDMSNESWrite(*dm, &sdm); if (*ierr) return;
  *ierr = PetscObjectSetFortranCallback((PetscObject) sdm, PETSC_FORTRAN_CALLBACK_SUBTYPE, &_cb.lf, (PetscVoidFunction) func, ctx); if (*ierr) return;
  *ierr = DMSNESSetFunctionLocal(*dm, sourlf, NULL);
}
