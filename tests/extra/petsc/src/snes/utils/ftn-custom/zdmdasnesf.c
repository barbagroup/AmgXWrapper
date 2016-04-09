#include <petsc/private/fortranimpl.h>
#include <petsc/private/dmdaimpl.h>
#include <petsc/private/snesimpl.h>
#if defined(PETSC_HAVE_FORTRAN_CAPS)
#define dmdasnessetjacobianlocal_      DMDASNESSETJACOBIANLOCAL
#define dmdasnessetfunctionlocal_      DMDASNESSETFUNCTIONLOCAL
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE)
#define dmdasnessetjacobianlocal_      dmdasnessetjacobianlocal
#define dmdasnessetfunctionlocal_      dmdasnessetfunctionlocal
#endif

static struct {
  PetscFortranCallbackId lf1d;
  PetscFortranCallbackId lf2d;
  PetscFortranCallbackId lf3d;
  PetscFortranCallbackId lj1d;
  PetscFortranCallbackId lj2d;
  PetscFortranCallbackId lj3d;
} _cb;

#undef __FUNCT__
#define __FUNCT__ "sourlj1d"
/************************************************/
static PetscErrorCode sourlj1d(DMDALocalInfo *info,PetscScalar *in,Mat A,Mat m,void *ptr)
{
  PetscErrorCode ierr;
  void (PETSC_STDCALL *func)(DMDALocalInfo*,PetscScalar*,Mat*,Mat*,void*,PetscErrorCode*),*ctx;
  DMSNES sdm;

  PetscFunctionBegin;
  ierr = DMGetDMSNES(info->da,&sdm);CHKERRQ(ierr);
  ierr = PetscObjectGetFortranCallback((PetscObject)sdm,PETSC_FORTRAN_CALLBACK_SUBTYPE,_cb.lj1d,(PetscVoidFunction*)&func,&ctx);CHKERRQ(ierr);
  (*func)(info,&in[info->dof*info->gxs],&A,&m,ctx,&ierr);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "sourlj2d"
static PetscErrorCode sourlj2d(DMDALocalInfo *info,PetscScalar **in,Mat A,Mat m,void *ptr)
{
  PetscErrorCode ierr;
  void (PETSC_STDCALL *func)(DMDALocalInfo*,PetscScalar*,Mat*,Mat*,void*,PetscErrorCode*),*ctx;
  DMSNES sdm;

  PetscFunctionBegin;
  ierr = DMGetDMSNES(info->da,&sdm);CHKERRQ(ierr);
  ierr = PetscObjectGetFortranCallback((PetscObject)sdm,PETSC_FORTRAN_CALLBACK_SUBTYPE,_cb.lj2d,(PetscVoidFunction*)&func,&ctx);CHKERRQ(ierr);
  (*func)(info,&in[info->gys][info->dof*info->gxs],&A,&m,ctx,&ierr);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "sourlj3d"
static PetscErrorCode sourlj3d(DMDALocalInfo *info,PetscScalar ***in,Mat A,Mat m,void *ptr)
{
  PetscErrorCode ierr;
  void (PETSC_STDCALL *func)(DMDALocalInfo*,PetscScalar*,Mat*,Mat*,void*,PetscErrorCode*),*ctx;
  DMSNES sdm;

  PetscFunctionBegin;
  ierr = DMGetDMSNES(info->da,&sdm);CHKERRQ(ierr);
  ierr = PetscObjectGetFortranCallback((PetscObject)sdm,PETSC_FORTRAN_CALLBACK_SUBTYPE,_cb.lj2d,(PetscVoidFunction*)&func,&ctx);CHKERRQ(ierr);
  (*func)(info,&in[info->gzs][info->gys][info->dof*info->gxs],&A,&m,ctx,&ierr);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

PETSC_EXTERN void PETSC_STDCALL dmdasnessetjacobianlocal_(DM *da,void (PETSC_STDCALL *jac)(DMDALocalInfo*,void*,void*,void*,void*,void*,PetscErrorCode*),void *ctx,PetscErrorCode *ierr)
{
  DMSNES   sdm;
  PetscInt dim;

  *ierr = DMGetDMSNESWrite(*da,&sdm); if (*ierr) return;
  *ierr = DMDAGetInfo(*da,&dim,0,0,0,0,0,0,0,0,0,0,0,0); if (*ierr) return;
  if (dim == 2) {
    *ierr = PetscObjectSetFortranCallback((PetscObject)sdm,PETSC_FORTRAN_CALLBACK_SUBTYPE,&_cb.lj2d,(PetscVoidFunction)jac,ctx); if (*ierr) return;
    *ierr = DMDASNESSetJacobianLocal(*da,(PetscErrorCode (*)(DMDALocalInfo*,void*,Mat,Mat,void*))sourlj2d,NULL);
  } else if (dim == 3) {
    *ierr = PetscObjectSetFortranCallback((PetscObject)sdm,PETSC_FORTRAN_CALLBACK_SUBTYPE,&_cb.lj3d,(PetscVoidFunction)jac,ctx); if (*ierr) return;
    *ierr = DMDASNESSetJacobianLocal(*da,(PetscErrorCode (*)(DMDALocalInfo*,void*,Mat,Mat,void*))sourlj3d,NULL);
  } else if (dim == 1) {
    *ierr = PetscObjectSetFortranCallback((PetscObject)sdm,PETSC_FORTRAN_CALLBACK_SUBTYPE,&_cb.lj1d,(PetscVoidFunction)jac,ctx); if (*ierr) return;
    *ierr = DMDASNESSetJacobianLocal(*da,(PetscErrorCode (*)(DMDALocalInfo*,void*,Mat,Mat,void*))sourlj1d,NULL);
  } else *ierr = 1;
}

/************************************************/

#undef __FUNCT__
#define __FUNCT__ "sourlf1d"
static PetscErrorCode sourlf1d(DMDALocalInfo *info,PetscScalar *in,PetscScalar *out,void *ptr)
{
  PetscErrorCode ierr;
  void (PETSC_STDCALL *func)(DMDALocalInfo*,PetscScalar*,PetscScalar*,void*,PetscErrorCode*),*ctx;
  DMSNES sdm;

  PetscFunctionBegin;
  ierr = DMGetDMSNES(info->da,&sdm);CHKERRQ(ierr);
  ierr = PetscObjectGetFortranCallback((PetscObject)sdm,PETSC_FORTRAN_CALLBACK_SUBTYPE,_cb.lf1d,(PetscVoidFunction*)&func,&ctx);CHKERRQ(ierr);
  (*func)(info,&in[info->dof*info->gxs],&out[info->dof*info->xs],ctx,&ierr);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "sourlf2d"
static PetscErrorCode sourlf2d(DMDALocalInfo *info,PetscScalar **in,PetscScalar **out,void *ptr)
{
  PetscErrorCode ierr;
  void (PETSC_STDCALL *func)(DMDALocalInfo*,PetscScalar*,PetscScalar*,void*,PetscErrorCode*),*ctx;
  DMSNES sdm;

  PetscFunctionBegin;
  ierr = DMGetDMSNES(info->da,&sdm);CHKERRQ(ierr);
  ierr = PetscObjectGetFortranCallback((PetscObject)sdm,PETSC_FORTRAN_CALLBACK_SUBTYPE,_cb.lf2d,(PetscVoidFunction*)&func,&ctx);CHKERRQ(ierr);
  (*func)(info,&in[info->gys][info->dof*info->gxs],&out[info->ys][info->dof*info->xs],ctx,&ierr);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "sourlf3d"
static PetscErrorCode sourlf3d(DMDALocalInfo *info,PetscScalar ***in,PetscScalar ***out,void *ptr)
{
  PetscErrorCode ierr;
  void (PETSC_STDCALL *func)(DMDALocalInfo*,PetscScalar*,PetscScalar*,void*,PetscErrorCode*),*ctx;
  DMSNES sdm;

  PetscFunctionBegin;
  ierr = DMGetDMSNES(info->da,&sdm);CHKERRQ(ierr);
  ierr = PetscObjectGetFortranCallback((PetscObject)sdm,PETSC_FORTRAN_CALLBACK_SUBTYPE,_cb.lf3d,(PetscVoidFunction*)&func,&ctx);CHKERRQ(ierr);
  (*func)(info,&in[info->gzs][info->gys][info->dof*info->gxs],&out[info->zs][info->ys][info->dof*info->xs],ctx,&ierr);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

PETSC_EXTERN void PETSC_STDCALL dmdasnessetfunctionlocal_(DM *da,InsertMode *mode,void (PETSC_STDCALL *func)(DMDALocalInfo*,void*,void*,void*,PetscErrorCode*),void *ctx,PetscErrorCode *ierr)
{
  DMSNES   sdm;
  PetscInt dim;

  *ierr = DMGetDMSNESWrite(*da,&sdm); if (*ierr) return;
  *ierr = DMDAGetInfo(*da,&dim,0,0,0,0,0,0,0,0,0,0,0,0); if (*ierr) return;
  if (dim == 2) {
    *ierr = PetscObjectSetFortranCallback((PetscObject)sdm,PETSC_FORTRAN_CALLBACK_SUBTYPE,&_cb.lf2d,(PetscVoidFunction)func,ctx); if (*ierr) return;
    *ierr = DMDASNESSetFunctionLocal(*da,*mode,(PetscErrorCode (*)(DMDALocalInfo*,void*,void*,void*))sourlf2d,NULL);
  } else if (dim == 3) {
    *ierr = PetscObjectSetFortranCallback((PetscObject)sdm,PETSC_FORTRAN_CALLBACK_SUBTYPE,&_cb.lf3d,(PetscVoidFunction)func,ctx); if (*ierr) return;
    *ierr = DMDASNESSetFunctionLocal(*da,*mode,(PetscErrorCode (*)(DMDALocalInfo*,void*,void*,void*))sourlf3d,NULL);
  } else if (dim == 1) {
    *ierr = PetscObjectSetFortranCallback((PetscObject)sdm,PETSC_FORTRAN_CALLBACK_SUBTYPE,&_cb.lf1d,(PetscVoidFunction)func,ctx); if (*ierr) return;
    *ierr = DMDASNESSetFunctionLocal(*da,*mode,(PetscErrorCode (*)(DMDALocalInfo*,void*,void*,void*))sourlf1d,NULL);
  } else *ierr = 1;
}



