#include <petsc/private/fortranimpl.h>
#include <petscis.h>

#if defined(PETSC_HAVE_FORTRAN_CAPS)
#define isblockgetindices_     ISBLOCKGETINDICES
#define isblockrestoreindices_ ISBLOCKRESTOREINDICES
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE)
#define isblockgetindices_     isblockgetindices
#define isblockrestoreindices_ isblockrestoreindices
#endif

PETSC_EXTERN void PETSC_STDCALL isblockgetindices_(IS *x,PetscInt *fa,size_t *ia,PetscErrorCode *ierr)
{
  const PetscInt *lx;

  *ierr = ISBlockGetIndices(*x,&lx); if (*ierr) return;
  *ia   = PetscIntAddressToFortran(fa,(PetscInt*)lx);
}

PETSC_EXTERN void PETSC_STDCALL isblockrestoreindices_(IS *x,PetscInt *fa,size_t *ia,PetscErrorCode *ierr)
{
  const PetscInt *lx = PetscIntAddressFromFortran(fa,*ia);
  *ierr = ISBlockRestoreIndices(*x,&lx);
}
