#include <petsc/private/fortranimpl.h>
#include <petscmat.h>

#if defined(PETSC_HAVE_FORTRAN_CAPS)
#define matcreateaijviennacl_                 MATCREATEAIJVIENNACL
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE)
#define matcreateaijviennacl_                 matcreateaijviennacl
#endif

PETSC_EXTERN void PETSC_STDCALL matcreateaijviennacl_(MPI_Comm *comm,PetscInt *m,PetscInt *n,PetscInt *M,PetscInt *N,PetscInt *d_nz,PetscInt *d_nnz,PetscInt *o_nz,PetscInt *o_nnz,Mat *newmat,PetscErrorCode *ierr)
{
  CHKFORTRANNULLINTEGER(d_nnz);
  CHKFORTRANNULLINTEGER(o_nnz);

  *ierr = MatCreateAIJViennaCL(MPI_Comm_f2c(*(MPI_Fint*)&*comm),*m,*n,*M,*N,*d_nz,d_nnz,*o_nz,o_nnz,newmat);
}

