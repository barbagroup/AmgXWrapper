
#if !defined(__ADJ_H)
#define __ADJ_H
#include <petsc/private/matimpl.h>


/*
  MATMPIAdj format - Compressed row storage for storing adjacency lists, and possibly weights
                     This is for grid reorderings (to reduce bandwidth)
                     grid partitionings, etc. This is NOT currently a dynamic data-structure.

*/

typedef struct {
  PetscInt  nz;
  PetscInt  *diag;                   /* pointers to diagonal elements, if they exist */
  PetscInt  *i;                      /* pointer to beginning of each row */
  PetscInt  *j;                      /* column values: j + i[k] is start of row k */
  PetscInt  *values;                 /* numerical values */
  PetscBool symmetric;               /* user indicates the nonzero structure is symmetric */
  PetscBool freeaij;                 /* free a, i,j at destroy */
  PetscBool freeaijwithfree;         /* use free() to free i,j instead of PetscFree() */
  PetscScalar *rowvalues;            /* scalar work space for MatGetRow() */
  PetscInt    rowvalues_alloc;
} Mat_MPIAdj;

/*where should I put this declaration???*/
PetscErrorCode MatGetSubMatrices_MPIAdj(Mat,PetscInt,const IS*,const IS*,MatReuse,Mat **);
PetscErrorCode MatGetSubMatricesMPI_MPIAdj(Mat,PetscInt, const IS[],const IS[],MatReuse,Mat **);


#endif
