#if !defined(__CUSPARSEMATIMPL)
#define __CUSPARSEMATIMPL

#include <../src/vec/vec/impls/seq/seqcusp/cuspvecimpl.h>

#if CUDA_VERSION>=4020
#include <cusparse_v2.h>
#else
#include <cusparse.h>
#endif

#include <algorithm>
#include <vector>

#if defined(PETSC_USE_COMPLEX)
#if defined(PETSC_USE_REAL_SINGLE)  
#define cusparse_solve    cusparseCcsrsv_solve
#define cusparse_analysis cusparseCcsrsv_analysis
#define cusparse_csr_spmv cusparseCcsrmv
#define cusparse_csr2csc  cusparseCcsr2csc
#define cusparse_hyb_spmv cusparseChybmv
#define cusparse_csr2hyb  cusparseCcsr2hyb
#define cusparse_hyb2csr  cusparseChyb2csr
cuFloatComplex ALPHA = {1.0f, 0.0f};
cuFloatComplex BETA  = {0.0f, 0.0f};
#elif defined(PETSC_USE_REAL_DOUBLE)
#define cusparse_solve    cusparseZcsrsv_solve
#define cusparse_analysis cusparseZcsrsv_analysis
#define cusparse_csr_spmv cusparseZcsrmv
#define cusparse_csr2csc  cusparseZcsr2csc
#define cusparse_hyb_spmv cusparseZhybmv
#define cusparse_csr2hyb  cusparseZcsr2hyb
#define cusparse_hyb2csr  cusparseZhyb2csr
cuDoubleComplex ALPHA = {1.0, 0.0};
cuDoubleComplex BETA  = {0.0, 0.0};
#endif
#else
PetscScalar ALPHA = 1.0;
PetscScalar BETA  = 0.0;
#if defined(PETSC_USE_REAL_SINGLE)  
#define cusparse_solve    cusparseScsrsv_solve
#define cusparse_analysis cusparseScsrsv_analysis
#define cusparse_csr_spmv cusparseScsrmv
#define cusparse_csr2csc  cusparseScsr2csc
#define cusparse_hyb_spmv cusparseShybmv
#define cusparse_csr2hyb  cusparseScsr2hyb
#define cusparse_hyb2csr  cusparseShyb2csr
#elif defined(PETSC_USE_REAL_DOUBLE)
#define cusparse_solve    cusparseDcsrsv_solve
#define cusparse_analysis cusparseDcsrsv_analysis
#define cusparse_csr_spmv cusparseDcsrmv
#define cusparse_csr2csc  cusparseDcsr2csc
#define cusparse_hyb_spmv cusparseDhybmv
#define cusparse_csr2hyb  cusparseDcsr2hyb
#define cusparse_hyb2csr  cusparseDhyb2csr
#endif
#endif

#define THRUSTINTARRAY32 thrust::device_vector<int>
#define THRUSTINTARRAY thrust::device_vector<PetscInt>
#define THRUSTARRAY thrust::device_vector<PetscScalar>

/* A CSR matrix structure */
struct CsrMatrix {
  PetscInt         num_rows;
  PetscInt         num_cols;
  PetscInt         num_entries;
  THRUSTINTARRAY32 *row_offsets;
  THRUSTINTARRAY32 *column_indices;
  THRUSTARRAY      *values;
};

//#define CUSPMATRIXCSR32 cusp::csr_matrix<int,PetscScalar,cusp::device_memory>

/* This is struct holding the relevant data needed to a MatSolve */
struct Mat_SeqAIJCUSPARSETriFactorStruct {
  /* Data needed for triangular solve */
  cusparseMatDescr_t          descr;
  cusparseSolveAnalysisInfo_t solveInfo;
  cusparseOperation_t         solveOp;
  CsrMatrix                   *csrMat; 
};

/* This is struct holding the relevant data needed to a MatMult */
struct Mat_SeqAIJCUSPARSEMultStruct {
  void               *mat;  /* opaque pointer to a matrix. This could be either a cusparseHybMat_t or a CsrMatrix */
  cusparseMatDescr_t descr; /* Data needed to describe the matrix for a multiply */
  THRUSTINTARRAY     *cprowIndices;   /* compressed row indices used in the parallel SpMV */
  PetscScalar        *alpha; /* pointer to a device "scalar" storing the alpha parameter in the SpMV */
  PetscScalar        *beta; /* pointer to a device "scalar" storing the beta parameter in the SpMV */
};

/* This is a larger struct holding all the triangular factors for a solve, transpose solve, and
 any indices used in a reordering */
struct Mat_SeqAIJCUSPARSETriFactors {
  Mat_SeqAIJCUSPARSETriFactorStruct *loTriFactorPtr; /* pointer for lower triangular (factored matrix) on GPU */
  Mat_SeqAIJCUSPARSETriFactorStruct *upTriFactorPtr; /* pointer for upper triangular (factored matrix) on GPU */
  Mat_SeqAIJCUSPARSETriFactorStruct *loTriFactorPtrTranspose; /* pointer for lower triangular (factored matrix) on GPU for the transpose (useful for BiCG) */
  Mat_SeqAIJCUSPARSETriFactorStruct *upTriFactorPtrTranspose; /* pointer for upper triangular (factored matrix) on GPU for the transpose (useful for BiCG)*/
  THRUSTINTARRAY                    *rpermIndices;  /* indices used for any reordering */
  THRUSTINTARRAY                    *cpermIndices;  /* indices used for any reordering */
  THRUSTARRAY                       *workVector;
  cusparseHandle_t                  handle;   /* a handle to the cusparse library */
  PetscInt                          nnz;      /* number of nonzeros ... need this for accurate logging between ICC and ILU */
};

/* This is a larger struct holding all the matrices for a SpMV, and SpMV Tranpose */
struct Mat_SeqAIJCUSPARSE {
  Mat_SeqAIJCUSPARSEMultStruct *mat; /* pointer to the matrix on the GPU */
  Mat_SeqAIJCUSPARSEMultStruct *matTranspose; /* pointer to the matrix on the GPU (for the transpose ... useful for BiCG) */
  THRUSTARRAY                  *workVector; /*pointer to a workvector to which we can copy the relevant indices of a vector we want to multiply */
  PetscInt                     nonzerorow; /* number of nonzero rows ... used in the flop calculations */
  MatCUSPARSEStorageFormat     format;   /* the storage format for the matrix on the device */
  cudaStream_t                 stream;   /* a stream for the parallel SpMV ... this is not owned and should not be deleted */
  cusparseHandle_t             handle;   /* a handle to the cusparse library ... this may not be owned (if we're working in parallel i.e. multiGPUs) */
};

PETSC_INTERN PetscErrorCode MatCUSPARSECopyToGPU(Mat);
PETSC_INTERN PetscErrorCode MatCUSPARSESetStream(Mat, const cudaStream_t stream);
PETSC_INTERN PetscErrorCode MatCUSPARSESetHandle(Mat, const cusparseHandle_t handle);
PETSC_INTERN PetscErrorCode MatCUSPARSEClearHandle(Mat);
#endif
