
#if !defined(__fcopy_h)

#include <petscsys.h>
#if defined(PETSC_HAVE_FORTRAN_CAPS)
#define fortrancopy_ FORTRANCOPY
#define fortranzero_ FORTRANZERO
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE)
#define fortrancopy_ fortrancopy
#define fortranzero_ fortranzero
#endif
PETSC_EXTERN void fortrancopy_(PetscInt*,PetscScalar*,PetscScalar*);
PETSC_EXTERN void fortranzero_(PetscInt*,PetscScalar*);
#endif
