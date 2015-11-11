#if !defined(__MPICUSPARSEMATIMPL)
#define __MPICUSPARSEMATIMPL

#include <cusparse_v2.h>
#include <../src/vec/vec/impls/seq/seqcusp/cuspvecimpl.h>

typedef struct {
  /* The following are used by GPU capabilities to store matrix storage formats on the device */
  MatCUSPARSEStorageFormat diagGPUMatFormat;
  MatCUSPARSEStorageFormat offdiagGPUMatFormat;
  cudaStream_t             stream;
  cusparseHandle_t         handle;
} Mat_MPIAIJCUSPARSE;

PETSC_INTERN PetscErrorCode MatCUSPARSESetStream(Mat, const cudaStream_t stream);
PETSC_INTERN PetscErrorCode MatCUSPARSESetHandle(Mat, const cusparseHandle_t handle);
PETSC_INTERN PetscErrorCode MatCUSPARSEClearHandle(Mat);

#endif
