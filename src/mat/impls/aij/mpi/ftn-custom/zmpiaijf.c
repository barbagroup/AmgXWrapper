#include <petsc/private/fortranimpl.h>
#include <petscmat.h>

#if defined(PETSC_HAVE_FORTRAN_CAPS)
#define matmpiaijgetseqaij_              MATMPIAIJGETSEQAIJ
#define matcreateaij_                 MATCREATEAIJ
#define matmpiaijsetpreallocation_       MATMPIAIJSETPREALLOCATION
#elif !defined(PETSC_HAVE_FORTRAN_UNDERSCORE)
#define matmpiaijgetseqaij_              matmpiaijgetseqaij
#define matcreateaij_                 matcreateaij
#define matmpiaijsetpreallocation_       matmpiaijsetpreallocation
#endif

PETSC_EXTERN void PETSC_STDCALL matmpiaijgetseqaij_(Mat *A,Mat *Ad,Mat *Ao,PetscInt *ic,size_t *iic,PetscErrorCode *ierr)
{
  const PetscInt *i;
  *ierr = MatMPIAIJGetSeqAIJ(*A,Ad,Ao,&i);if (*ierr) return;
  *iic  = PetscIntAddressToFortran(ic,(PetscInt*)i);
}

PETSC_EXTERN void PETSC_STDCALL matcreateaij_(MPI_Comm *comm,PetscInt *m,PetscInt *n,PetscInt *M,PetscInt *N,PetscInt *d_nz,PetscInt *d_nnz,PetscInt *o_nz,PetscInt *o_nnz,Mat *newmat,PetscErrorCode *ierr)
{
  CHKFORTRANNULLINTEGER(d_nnz);
  CHKFORTRANNULLINTEGER(o_nnz);

  *ierr = MatCreateAIJ(MPI_Comm_f2c(*(MPI_Fint*)&*comm),*m,*n,*M,*N,*d_nz,d_nnz,*o_nz,o_nnz,newmat);
}

PETSC_EXTERN void PETSC_STDCALL matmpiaijsetpreallocation_(Mat *mat,PetscInt *d_nz,PetscInt *d_nnz,PetscInt *o_nz,PetscInt *o_nnz,PetscErrorCode *ierr)
{
  CHKFORTRANNULLINTEGER(d_nnz);
  CHKFORTRANNULLINTEGER(o_nnz);
  *ierr = MatMPIAIJSetPreallocation(*mat,*d_nz,d_nnz,*o_nz,o_nnz);
}

