#include <petscvec.h>
#include <../src/sys/f90-src/f90impl.h>

#if defined(PETSC_HAVE_FORTRAN_CAPS)
#define vecgetarraymystruct_            VECGETARRAYMYSTRUCT
#define vecrestorearraymystruct_        VECRESTOREARRAYMYSTRUCT
#define f90array1dcreatemystruct_       F90ARRAY1DCREATEMYSTRUCT
#define f90array1daccessmystruct_       F90ARRAY1DACCESSMYSTRUCT
#define f90array1ddestroymystruct_      F90ARRAY1DDESTROYMYSTRUCT
#define f90array1dgetaddrmystruct_      F90ARRAY1DGETADDRMYSTRUCT
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE)
#define vecgetarraymystruct_            vecgetarraymystruct
#define vecrestorearraymystruct_        vecrestorearraymystruct
#define f90array1dcreatemystruct_       f90array1dcreatemystruct
#define f90array1daccessmystruct_       f90array1daccessmystruct
#define f90array1ddestroymystruct_      f90array1ddestroymystruct
#define f90array1dgetaddrmystruc_       f90array1dgetaddrmystruct
#endif

PETSC_EXTERN void PETSC_STDCALL f90array1dcreatemystruct_(void *,PetscInt *,PetscInt *,F90Array1d * PETSC_F90_2PTR_PROTO_NOVAR);
PETSC_EXTERN void PETSC_STDCALL f90array1daccessmystruct_(F90Array1d*,void** PETSC_F90_2PTR_PROTO_NOVAR);
PETSC_EXTERN void PETSC_STDCALL f90array1ddestroymystruct_(F90Array1d *ptr PETSC_F90_2PTR_PROTO_NOVAR);

PETSC_EXTERN void PETSC_STDCALL f90array1dgetaddrmystruct_(void *array, PetscFortranAddr *address)
{
  *address = (PetscFortranAddr)array;
}

PETSC_EXTERN void PETSC_STDCALL vecgetarraymystruct_(Vec *x,F90Array1d *ptr,int *__ierr PETSC_F90_2PTR_PROTO(ptrd))
{
  PetscScalar *fa;
  PetscInt    len,one = 1;
  if (!ptr) {
    *__ierr = PetscError(((PetscObject)*x)->comm,__LINE__,PETSC_FUNCTION_NAME,__FILE__,PETSC_ERR_ARG_BADPTR,PETSC_ERROR_INITIAL,"ptr==NULL");
    return;
  }
  *__ierr = VecGetArray(*x,&fa);      if (*__ierr) return;
  *__ierr = VecGetLocalSize(*x,&len); if (*__ierr) return;
  f90array1dcreatemystruct_(fa,&one,&len,ptr PETSC_F90_2PTR_PARAM(ptrd));
}

PETSC_EXTERN void PETSC_STDCALL vecrestorearraymystruct_(Vec *x,F90Array1d *ptr,int *__ierr PETSC_F90_2PTR_PROTO(ptrd))
{
  PetscScalar *fa;
  f90array1daccessmystruct_(ptr,(void**)&fa PETSC_F90_2PTR_PARAM(ptrd));
  f90array1ddestroymystruct_(ptr PETSC_F90_2PTR_PARAM(ptrd));
  *__ierr = VecRestoreArray(*x,&fa);
}

