#include <petsc/private/fortranimpl.h>
#include <petscviewer.h>

#if defined(PETSC_HAVE_FORTRAN_CAPS)
#define petscviewersetformat_      PETSCVIEWERSETFORMAT
#define petscviewersettype_        PETSCVIEWERSETTYPE
#define petscviewerpushformat_     PETSCVIEWERPUSHFORMAT
#define petscviewerpopformat_      PETSCVIEWERPOPFORMAT
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE)
#define petscviewersetformat_      petscviewersetformat
#define petscviewersettype_        petscviewersettype
#define petscviewerpushformat_     petscviewerpushformat
#define petscviewerpopformat_      petscviewerpopformat
#endif

PETSC_EXTERN void PETSC_STDCALL petscviewersettype_(PetscViewer *x,CHAR type_name PETSC_MIXED_LEN(len),PetscErrorCode *ierr PETSC_END_LEN(len))
{
  char *t;

  FIXCHAR(type_name,len,t);
  *ierr = PetscViewerSetType(*x,t);
  FREECHAR(type_name,t);
}

PETSC_EXTERN void PETSC_STDCALL petscviewersetformat_(PetscViewer *vin,PetscViewerFormat *format,PetscErrorCode *ierr)
{
  PetscViewer v;
  PetscPatchDefaultViewers_Fortran(vin,v);
  *ierr = PetscViewerSetFormat(v,*format);
}

PETSC_EXTERN void PETSC_STDCALL petscviewerpushformat_(PetscViewer *vin,PetscViewerFormat *format,PetscErrorCode *ierr)
{
  PetscViewer v;
  PetscPatchDefaultViewers_Fortran(vin,v);
  *ierr = PetscViewerPushFormat(v,*format);
}

PETSC_EXTERN void PETSC_STDCALL petscviewerpopformat_(PetscViewer *vin,PetscErrorCode *ierr)
{
  PetscViewer v;
  PetscPatchDefaultViewers_Fortran(vin,v);
  *ierr = PetscViewerPopFormat(v);
}
