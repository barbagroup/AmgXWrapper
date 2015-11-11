
#include <petscvec.h>
#include <../src/sys/f90-src/f90impl.h>

#if defined(PETSC_HAVE_FORTRAN_CAPS)
#define vecgetarrayf90_            VECGETARRAYF90
#define vecrestorearrayf90_        VECRESTOREARRAYF90
#define vecgetarrayreadf90_        VECGETARRAYREADF90
#define vecrestorearrayreadf90_    VECRESTOREARRAYREADF90
#define vecduplicatevecsf90_       VECDUPLICATEVECSF90
#define vecdestroyvecsf90_         VECDESTROYVECSF90
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE)
#define vecgetarrayf90_            vecgetarrayf90
#define vecrestorearrayf90_        vecrestorearrayf90
#define vecgetarrayreadf90_        vecgetarrayreadf90
#define vecrestorearrayreadf90_    vecrestorearrayreadf90
#define vecduplicatevecsf90_       vecduplicatevecsf90
#define vecdestroyvecsf90_         vecdestroyvecsf90
#endif

PETSC_EXTERN void PETSC_STDCALL vecgetarrayf90_(Vec *x,F90Array1d *ptr,int *__ierr PETSC_F90_2PTR_PROTO(ptrd))
{
  PetscScalar *fa;
  PetscInt    len;
  if (!ptr) {
    *__ierr = PetscError(((PetscObject)*x)->comm,__LINE__,PETSC_FUNCTION_NAME,__FILE__,PETSC_ERR_ARG_BADPTR,PETSC_ERROR_INITIAL,"ptr==NULL, maybe #include <petsc/finclude/petscvec.h90> is missing?");
    return;
  }
  *__ierr = VecGetArray(*x,&fa);      if (*__ierr) return;
  *__ierr = VecGetLocalSize(*x,&len); if (*__ierr) return;
  *__ierr = F90Array1dCreate(fa,PETSC_SCALAR,1,len,ptr PETSC_F90_2PTR_PARAM(ptrd));
}
PETSC_EXTERN void PETSC_STDCALL vecrestorearrayf90_(Vec *x,F90Array1d *ptr,int *__ierr PETSC_F90_2PTR_PROTO(ptrd))
{
  PetscScalar *fa;
  *__ierr = F90Array1dAccess(ptr,PETSC_SCALAR,(void**)&fa PETSC_F90_2PTR_PARAM(ptrd));if (*__ierr) return;
  *__ierr = F90Array1dDestroy(ptr,PETSC_SCALAR PETSC_F90_2PTR_PARAM(ptrd));if (*__ierr) return;
  *__ierr = VecRestoreArray(*x,&fa);
}

PETSC_EXTERN void PETSC_STDCALL vecgetarrayreadf90_(Vec *x,F90Array1d *ptr,int *__ierr PETSC_F90_2PTR_PROTO(ptrd))
{
  const PetscScalar *fa;
  PetscInt          len;
  if (!ptr) {
    *__ierr = PetscError(((PetscObject)*x)->comm,__LINE__,PETSC_FUNCTION_NAME,__FILE__,PETSC_ERR_ARG_BADPTR,PETSC_ERROR_INITIAL,"ptr==NULL, maybe #include <petsc/finclude/petscvec.h90> is missing?");
    return;
  }
  *__ierr = VecGetArrayRead(*x,&fa);      if (*__ierr) return;
  *__ierr = VecGetLocalSize(*x,&len); if (*__ierr) return;
  *__ierr = F90Array1dCreate((PetscScalar*)fa,PETSC_SCALAR,1,len,ptr PETSC_F90_2PTR_PARAM(ptrd));
}
PETSC_EXTERN void PETSC_STDCALL vecrestorearrayreadf90_(Vec *x,F90Array1d *ptr,int *__ierr PETSC_F90_2PTR_PROTO(ptrd))
{
  const PetscScalar *fa;
  *__ierr = F90Array1dAccess(ptr,PETSC_SCALAR,(void**)&fa PETSC_F90_2PTR_PARAM(ptrd));if (*__ierr) return;
  *__ierr = F90Array1dDestroy(ptr,PETSC_SCALAR PETSC_F90_2PTR_PARAM(ptrd));if (*__ierr) return;
  *__ierr = VecRestoreArrayRead(*x,&fa);
}

PETSC_EXTERN void PETSC_STDCALL vecduplicatevecsf90_(Vec *v,int *m,F90Array1d *ptr,int *__ierr PETSC_F90_2PTR_PROTO(ptrd))
{
  Vec              *lV;
  PetscFortranAddr *newvint;
  int              i;
  *__ierr = VecDuplicateVecs(*v,*m,&lV); if (*__ierr) return;
  *__ierr = PetscMalloc1(*m,&newvint);  if (*__ierr) return;

  for (i=0; i<*m; i++) newvint[i] = (PetscFortranAddr)lV[i];
  *__ierr = PetscFree(lV); if (*__ierr) return;
  *__ierr = F90Array1dCreate(newvint,PETSC_FORTRANADDR,1,*m,ptr PETSC_F90_2PTR_PARAM(ptrd));
}

PETSC_EXTERN void PETSC_STDCALL vecdestroyvecsf90_(int *m,F90Array1d *ptr,int *__ierr PETSC_F90_2PTR_PROTO(ptrd))
{
  Vec *vecs;
  int i;

  *__ierr = F90Array1dAccess(ptr,PETSC_FORTRANADDR,(void**)&vecs PETSC_F90_2PTR_PARAM(ptrd));if (*__ierr) return;
  for (i=0; i<*m; i++) {
    *__ierr = VecDestroy(&vecs[i]);
    if (*__ierr) return;
  }
  *__ierr = F90Array1dDestroy(ptr,PETSC_FORTRANADDR PETSC_F90_2PTR_PARAM(ptrd));if (*__ierr) return;
  *__ierr = PetscFree(vecs);
}
