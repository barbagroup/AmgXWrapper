#include <petsc/private/fortranimpl.h>
#include <petscds.h>
#include <../src/sys/f90-src/f90impl.h>

#if defined(PETSC_HAVE_FORTRAN_CAPS)
#define petscdsgettabulation_        PETSCDSGETTABULATION
#define petscdsrestoretabulation_    PETSCDSRESTORETABULATION
#define petscdsgetbdtabulation_      PETSCDSGETBDTABULATION
#define petscdsrestorebdtabulation_  PETSCDSRESTOREBDTABULATION
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE)
#define petscdsgettabulation_        petscdsgettabulation
#define petscdsrestoretabulation_    petscdsrestoretabulation
#define petscdsgetbdtabulation_      petscdsgetbdtabulation
#define petscdsrestorebdtabulation_  petscdsrestorebdtabulation
#endif

PETSC_EXTERN void PETSC_STDCALL petscdsgettabulation_(PetscDS *prob, PetscInt *f, F90Array1d *ptrB, F90Array1d *ptrD, PetscErrorCode *ierr PETSC_F90_2PTR_PROTO(ptrb) PETSC_F90_2PTR_PROTO(ptrd))
{
  PetscFE         fe;
  PetscQuadrature q;
  PetscInt        dim, Nb, Nc, Nq;
  PetscReal     **basis, **basisDer;

  *ierr = PetscDSGetSpatialDimension(*prob, &dim);if (*ierr) return;
  *ierr = PetscDSGetDiscretization(*prob, *f, (PetscObject *) &fe);if (*ierr) return;
  *ierr = PetscFEGetDimension(fe, &Nb);if (*ierr) return;
  *ierr = PetscFEGetNumComponents(fe, &Nc);if (*ierr) return;
  *ierr = PetscFEGetQuadrature(fe, &q);if (*ierr) return;
  *ierr = PetscQuadratureGetData(q, NULL, &Nq, NULL, NULL);if (*ierr) return;
  *ierr = PetscDSGetTabulation(*prob, &basis, &basisDer);if (*ierr) return;
  *ierr = F90Array1dCreate((void *) basis[*f],    PETSC_REAL, 1, Nq*Nb*Nc, ptrB PETSC_F90_2PTR_PARAM(ptrb));if (*ierr) return;
  *ierr = F90Array1dCreate((void *) basisDer[*f], PETSC_REAL, 1, Nq*Nb*Nc*dim, ptrD PETSC_F90_2PTR_PARAM(ptrd));
}

PETSC_EXTERN void PETSC_STDCALL petscdsrestoretabulation_(PetscDS *prob, PetscInt *f, F90Array1d *ptrB, F90Array1d *ptrD, PetscErrorCode *ierr PETSC_F90_2PTR_PROTO(ptrb) PETSC_F90_2PTR_PROTO(ptrd))
{
  *ierr = F90Array1dDestroy(ptrB, PETSC_REAL PETSC_F90_2PTR_PARAM(ptrb));if (*ierr) return;
  *ierr = F90Array1dDestroy(ptrD, PETSC_REAL PETSC_F90_2PTR_PARAM(ptrd));
}

PETSC_EXTERN void PETSC_STDCALL petscdsgetbdtabulation_(PetscDS *prob, PetscInt *f, F90Array1d *ptrB, F90Array1d *ptrD, PetscErrorCode *ierr PETSC_F90_2PTR_PROTO(ptrb) PETSC_F90_2PTR_PROTO(ptrd))
{
  PetscFE         fe;
  PetscQuadrature q;
  PetscInt        dim, Nb, Nc, Nq;
  PetscReal     **basis, **basisDer;

  *ierr = PetscDSGetSpatialDimension(*prob, &dim);if (*ierr) return;
  dim  -= 1;
  *ierr = PetscDSGetBdDiscretization(*prob, *f, (PetscObject *) &fe);if (*ierr) return;
  *ierr = PetscFEGetDimension(fe, &Nb);if (*ierr) return;
  *ierr = PetscFEGetNumComponents(fe, &Nc);if (*ierr) return;
  *ierr = PetscFEGetQuadrature(fe, &q);if (*ierr) return;
  *ierr = PetscQuadratureGetData(q, NULL, &Nq, NULL, NULL);if (*ierr) return;
  *ierr = PetscDSGetBdTabulation(*prob, &basis, &basisDer);if (*ierr) return;
  *ierr = F90Array1dCreate((void *) basis[*f],    PETSC_REAL, 1, Nq*Nb*Nc, ptrB PETSC_F90_2PTR_PARAM(ptrb));if (*ierr) return;
  *ierr = F90Array1dCreate((void *) basisDer[*f], PETSC_REAL, 1, Nq*Nb*Nc*dim, ptrD PETSC_F90_2PTR_PARAM(ptrd));
}

PETSC_EXTERN void PETSC_STDCALL petscdsrestorebdtabulation_(PetscDS *prob, PetscInt *f, F90Array1d *ptrB, F90Array1d *ptrD, PetscErrorCode *ierr PETSC_F90_2PTR_PROTO(ptrb) PETSC_F90_2PTR_PROTO(ptrd))
{
  *ierr = F90Array1dDestroy(ptrB, PETSC_REAL PETSC_F90_2PTR_PARAM(ptrb));if (*ierr) return;
  *ierr = F90Array1dDestroy(ptrD, PETSC_REAL PETSC_F90_2PTR_PARAM(ptrd));
}
