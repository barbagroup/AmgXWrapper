
#if !defined(__DENSE_H)
#define __DENSE_H
#include <petsc/private/matimpl.h>
#include <../src/mat/impls/aij/seq/aij.h> /* Mat_MatTransMatMult is defined here */

/*
  MATSEQDENSE format - conventional dense Fortran storage (by columns)
*/

typedef struct {
  PetscScalar  *v;                /* matrix elements */
  PetscBool    roworiented;       /* if true, row oriented input (default) */
  PetscInt     pad;               /* padding */
  PetscBLASInt *pivots;           /* pivots in LU factorization */
  PetscBLASInt lda;               /* Lapack leading dimension of data */
  PetscBool    changelda;         /* change lda on resize? Default unless user set lda */
  PetscBLASInt Mmax,Nmax;         /* indicates the largest dimensions of data possible */
  PetscBool    user_alloc;        /* true if the user provided the dense data */

  Mat_MatTransMatMult *atb;       /* used by MatTransposeMatMult_SeqAIJ_SeqDense */
} Mat_SeqDense;

extern PetscErrorCode MatMult_SeqDense(Mat A,Vec,Vec);
extern PetscErrorCode MatMultAdd_SeqDense(Mat A,Vec,Vec,Vec);
extern PetscErrorCode MatMultTranspose_SeqDense(Mat A,Vec,Vec);
extern PetscErrorCode MatMultTransposeAdd_SeqDense(Mat A,Vec,Vec,Vec);
extern PetscErrorCode MatMatMultSymbolic_SeqDense_SeqDense(Mat,Mat,PetscReal,Mat*);
extern PetscErrorCode MatMatMultNumeric_SeqDense_SeqDense(Mat,Mat,Mat);
extern PetscErrorCode MatTransposeMatMult_SeqDense_SeqDense(Mat,Mat,MatReuse,PetscReal,Mat*);
extern PetscErrorCode MatTransposeMatMultSymbolic_SeqDense_SeqDense(Mat,Mat,PetscReal,Mat*);
extern PetscErrorCode MatTransposeMatMultNumeric_SeqDense_SeqDense(Mat,Mat,Mat);
extern PetscErrorCode MatMatMultSymbolic_SeqAIJ_SeqDense(Mat,Mat,PetscReal,Mat*);
extern PetscErrorCode MatMatMultNumeric_SeqAIJ_SeqDense(Mat,Mat,Mat);

PETSC_INTERN PetscErrorCode MatDestroy_SeqDense_MatTransMatMult(Mat);

PETSC_EXTERN PetscErrorCode MatMatMult_SeqAIJ_SeqDense(Mat,Mat,MatReuse,PetscReal,Mat*);
PETSC_EXTERN PetscErrorCode MatMatMult_SeqDense_SeqDense(Mat,Mat,MatReuse,PetscReal,Mat*);

#endif
