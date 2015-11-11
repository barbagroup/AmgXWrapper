/*
   Implements the sequential cusp vectors.
*/

#define PETSC_SKIP_COMPLEX
#define PETSC_SKIP_SPINLOCK

#include <petscconf.h>
#include <petsc/private/vecimpl.h>          /*I "petscvec.h" I*/
#include <../src/vec/vec/impls/dvecimpl.h>
#include <../src/vec/vec/impls/seq/seqcusp/cuspvecimpl.h>

#include <cuda_runtime.h>

#undef __FUNCT__
#define __FUNCT__ "VecCUSPAllocateCheckHost"
/*
    Allocates space for the vector array on the Host if it does not exist.
    Does NOT change the PetscCUSPFlag for the vector
    Does NOT zero the CUSP array
 */
PetscErrorCode VecCUSPAllocateCheckHost(Vec v)
{
  PetscErrorCode ierr;
  PetscScalar    *array;
  Vec_Seq        *s = (Vec_Seq*)v->data;
  PetscInt       n = v->map->n;

  PetscFunctionBegin;
  if (!s) {
    ierr = PetscNewLog((PetscObject)v,&s);CHKERRQ(ierr);
    v->data = s;
  }
  if (!s->array) {
    ierr               = PetscMalloc1(n,&array);CHKERRQ(ierr);
    ierr               = PetscLogObjectMemory((PetscObject)v,n*sizeof(PetscScalar));CHKERRQ(ierr);
    s->array           = array;
    s->array_allocated = array;
    if (v->valid_GPU_array == PETSC_CUSP_UNALLOCATED) {
      v->valid_GPU_array = PETSC_CUSP_CPU;
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecCUSPAllocateCheck"
/*
    Allocates space for the vector array on the GPU if it does not exist.
    Does NOT change the PetscCUSPFlag for the vector
    Does NOT zero the CUSP array

 */
PetscErrorCode VecCUSPAllocateCheck(Vec v)
{
  cudaError_t    err;
  cudaStream_t   stream;
  Vec_CUSP       *veccusp;

  PetscFunctionBegin;
  if (!v->spptr) {
    try {
      v->spptr = new Vec_CUSP;
      veccusp = (Vec_CUSP*)v->spptr;
      veccusp->GPUarray = new CUSPARRAY;
      veccusp->GPUarray->resize((PetscBLASInt)v->map->n);
      err = cudaStreamCreate(&stream);CHKERRCUSP(err);
      veccusp->stream = stream;
      veccusp->hostDataRegisteredAsPageLocked = PETSC_FALSE;
      v->ops->destroy = VecDestroy_SeqCUSP;
      if (v->valid_GPU_array == PETSC_CUSP_UNALLOCATED) {
        if (v->data && ((Vec_Seq*)v->data)->array) {
          v->valid_GPU_array = PETSC_CUSP_CPU;
        } else {
          v->valid_GPU_array = PETSC_CUSP_GPU;
        }
      }
    } catch(char *ex) {
      SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"CUSP error: %s", ex);
    }
  }
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "VecCUSPCopyToGPU"
/* Copies a vector from the CPU to the GPU unless we already have an up-to-date copy on the GPU */
PetscErrorCode VecCUSPCopyToGPU(Vec v)
{
  PetscErrorCode ierr;
  cudaError_t    err;
  Vec_CUSP       *veccusp;
  CUSPARRAY      *varray;

  PetscFunctionBegin;
  ierr = VecCUSPAllocateCheck(v);CHKERRQ(ierr);
  if (v->valid_GPU_array == PETSC_CUSP_CPU) {
    ierr = PetscLogEventBegin(VEC_CUSPCopyToGPU,v,0,0,0);CHKERRQ(ierr);
    try {
      veccusp=(Vec_CUSP*)v->spptr;
      varray=veccusp->GPUarray;
      err = cudaMemcpy(varray->data().get(),((Vec_Seq*)v->data)->array,v->map->n*sizeof(PetscScalar),cudaMemcpyHostToDevice);CHKERRCUSP(err);
    } catch(char *ex) {
      SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"CUSP error: %s", ex);
    }
    ierr = PetscLogEventEnd(VEC_CUSPCopyToGPU,v,0,0,0);CHKERRQ(ierr);
    v->valid_GPU_array = PETSC_CUSP_BOTH;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecCUSPCopyToGPUSome"
static PetscErrorCode VecCUSPCopyToGPUSome(Vec v, PetscCUSPIndices ci)
{
  CUSPARRAY      *varray;
  PetscErrorCode ierr;
  cudaError_t    err;
  PetscScalar    *cpuPtr, *gpuPtr;
  Vec_Seq        *s;
  VecScatterCUSPIndices_PtoP ptop_scatter = (VecScatterCUSPIndices_PtoP)ci->scatter;

  PetscFunctionBegin;
  ierr = VecCUSPAllocateCheck(v);CHKERRQ(ierr);
  if (v->valid_GPU_array == PETSC_CUSP_CPU) {
    s = (Vec_Seq*)v->data;

    ierr   = PetscLogEventBegin(VEC_CUSPCopyToGPUSome,v,0,0,0);CHKERRQ(ierr);
    varray = ((Vec_CUSP*)v->spptr)->GPUarray;
    gpuPtr = varray->data().get() + ptop_scatter->recvLowestIndex;
    cpuPtr = s->array + ptop_scatter->recvLowestIndex;

    /* Note : this code copies the smallest contiguous chunk of data
       containing ALL of the indices */
    err = cudaMemcpy(gpuPtr,cpuPtr,ptop_scatter->nr*sizeof(PetscScalar),cudaMemcpyHostToDevice);CHKERRCUSP(err);

    // Set the buffer states
    v->valid_GPU_array = PETSC_CUSP_BOTH;
    ierr = PetscLogEventEnd(VEC_CUSPCopyToGPUSome,v,0,0,0);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "VecCUSPCopyFromGPU"
/*
     VecCUSPCopyFromGPU - Copies a vector from the GPU to the CPU unless we already have an up-to-date copy on the CPU
*/
PetscErrorCode VecCUSPCopyFromGPU(Vec v)
{
  PetscErrorCode ierr;
  cudaError_t    err;
  Vec_CUSP       *veccusp;
  CUSPARRAY      *varray;

  PetscFunctionBegin;
  ierr = VecCUSPAllocateCheckHost(v);CHKERRQ(ierr);
  if (v->valid_GPU_array == PETSC_CUSP_GPU) {
    ierr = PetscLogEventBegin(VEC_CUSPCopyFromGPU,v,0,0,0);CHKERRQ(ierr);
    try {
      veccusp=(Vec_CUSP*)v->spptr;
      varray=veccusp->GPUarray;
      err = cudaMemcpy(((Vec_Seq*)v->data)->array,varray->data().get(),v->map->n*sizeof(PetscScalar),cudaMemcpyDeviceToHost);CHKERRCUSP(err);
    } catch(char *ex) {
      SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"CUSP error: %s", ex);
    }
    ierr = PetscLogEventEnd(VEC_CUSPCopyFromGPU,v,0,0,0);CHKERRQ(ierr);
    v->valid_GPU_array = PETSC_CUSP_BOTH;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecCUSPCopyFromGPUSome"
/* Note that this function only copies *some* of the values up from the GPU to CPU,
   which means that we need recombine the data at some point before using any of the standard functions.
   We could add another few flag-types to keep track of this, or treat things like VecGetArray VecRestoreArray
   where you have to always call in pairs
*/
PetscErrorCode VecCUSPCopyFromGPUSome(Vec v, PetscCUSPIndices ci)
{
  CUSPARRAY      *varray;
  PetscErrorCode ierr;
  cudaError_t    err;
  PetscScalar    *cpuPtr, *gpuPtr;
  Vec_Seq        *s;
  VecScatterCUSPIndices_PtoP ptop_scatter = (VecScatterCUSPIndices_PtoP)ci->scatter;

  PetscFunctionBegin;
  ierr = VecCUSPAllocateCheckHost(v);CHKERRQ(ierr);
  if (v->valid_GPU_array == PETSC_CUSP_GPU) {
    ierr   = PetscLogEventBegin(VEC_CUSPCopyFromGPUSome,v,0,0,0);CHKERRQ(ierr);

    varray=((Vec_CUSP*)v->spptr)->GPUarray;
    s = (Vec_Seq*)v->data;
    gpuPtr = varray->data().get() + ptop_scatter->sendLowestIndex;
    cpuPtr = s->array + ptop_scatter->sendLowestIndex;

    /* Note : this code copies the smallest contiguous chunk of data
       containing ALL of the indices */
    err = cudaMemcpy(cpuPtr,gpuPtr,ptop_scatter->ns*sizeof(PetscScalar),cudaMemcpyDeviceToHost);CHKERRCUSP(err);

    ierr = VecCUSPRestoreArrayRead(v,&varray);CHKERRQ(ierr);
    ierr = PetscLogEventEnd(VEC_CUSPCopyFromGPUSome,v,0,0,0);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecCopy_SeqCUSP_Private"
static PetscErrorCode VecCopy_SeqCUSP_Private(Vec xin,Vec yin)
{
  PetscScalar       *ya;
  const PetscScalar *xa;
  PetscErrorCode    ierr;

  PetscFunctionBegin;
  ierr = VecCUSPAllocateCheckHost(xin);
  ierr = VecCUSPAllocateCheckHost(yin);
  if (xin != yin) {
    ierr = VecGetArrayRead(xin,&xa);CHKERRQ(ierr);
    ierr = VecGetArray(yin,&ya);CHKERRQ(ierr);
    ierr = PetscMemcpy(ya,xa,xin->map->n*sizeof(PetscScalar));CHKERRQ(ierr);
    ierr = VecRestoreArrayRead(xin,&xa);CHKERRQ(ierr);
    ierr = VecRestoreArray(yin,&ya);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecSetRandom_SeqCUSP_Private"
static PetscErrorCode VecSetRandom_SeqCUSP_Private(Vec xin,PetscRandom r)
{
  PetscErrorCode ierr;
  PetscInt       n = xin->map->n,i;
  PetscScalar    *xx;

  PetscFunctionBegin;
  ierr = VecGetArray(xin,&xx);CHKERRQ(ierr);
  for (i=0; i<n; i++) {ierr = PetscRandomGetValue(r,&xx[i]);CHKERRQ(ierr);}
  ierr = VecRestoreArray(xin,&xx);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecDestroy_SeqCUSP_Private"
static PetscErrorCode VecDestroy_SeqCUSP_Private(Vec v)
{
  Vec_Seq        *vs = (Vec_Seq*)v->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscObjectSAWsViewOff(v);CHKERRQ(ierr);
#if defined(PETSC_USE_LOG)
  PetscLogObjectState((PetscObject)v,"Length=%D",v->map->n);
#endif
  if (vs) {
    if (vs->array_allocated) ierr = PetscFree(vs->array_allocated);CHKERRQ(ierr);
    ierr = PetscFree(vs);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecResetArray_SeqCUSP_Private"
static PetscErrorCode VecResetArray_SeqCUSP_Private(Vec vin)
{
  Vec_Seq *v = (Vec_Seq*)vin->data;

  PetscFunctionBegin;
  v->array         = v->unplacedarray;
  v->unplacedarray = 0;
  PetscFunctionReturn(0);
}

/* these following 3 public versions are necessary because we use CUSP in the regular PETSc code and these need to be called from plain C code. */
#undef __FUNCT__
#define __FUNCT__ "VecCUSPAllocateCheck_Public"
PetscErrorCode VecCUSPAllocateCheck_Public(Vec v)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = VecCUSPAllocateCheck(v);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecCUSPCopyToGPU_Public"
PetscErrorCode VecCUSPCopyToGPU_Public(Vec v)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = VecCUSPCopyToGPU(v);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}



#undef __FUNCT__
#define __FUNCT__ "VecCUSPCopyToGPUSome_Public"
/*
    VecCUSPCopyToGPUSome_Public - Copies certain entries down to the GPU from the CPU of a vector

   Input Parameters:
+    v - the vector
-    indices - the requested indices, this should be created with CUSPIndicesCreate()

*/
PetscErrorCode VecCUSPCopyToGPUSome_Public(Vec v, PetscCUSPIndices ci)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = VecCUSPCopyToGPUSome(v,ci);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecCUSPCopyFromGPUSome_Public"
/*
  VecCUSPCopyFromGPUSome_Public - Copies certain entries up to the CPU from the GPU of a vector

  Input Parameters:
 +    v - the vector
 -    indices - the requested indices, this should be created with CUSPIndicesCreate()
*/
PetscErrorCode VecCUSPCopyFromGPUSome_Public(Vec v, PetscCUSPIndices ci)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = VecCUSPCopyFromGPUSome(v,ci);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/*MC
   VECSEQCUSP - VECSEQCUSP = "seqcusp" - The basic sequential vector, modified to use CUSP

   Options Database Keys:
. -vec_type seqcusp - sets the vector type to VECSEQCUSP during a call to VecSetFromOptions()

  Level: beginner

.seealso: VecCreate(), VecSetType(), VecSetFromOptions(), VecCreateSeqWithArray(), VECMPI, VecType, VecCreateMPI(), VecCreateSeq()
M*/

/* for VecAYPX_SeqCUSP*/
namespace cusp
{
namespace blas
{
namespace detail
{
  template <typename T>
    struct AYPX : public thrust::binary_function<T,T,T>
    {
      T alpha;

      AYPX(T _alpha) : alpha(_alpha) {}

      __host__ __device__
      T operator()(T x, T y)
      {
        return alpha * y + x;
      }
    };
}

 template <typename ForwardIterator1,
           typename ForwardIterator2,
           typename ScalarType>
void aypx(ForwardIterator1 first1,ForwardIterator1 last1,ForwardIterator2 first2,ScalarType alpha)
           {
             thrust::transform(first1,last1,first2,first2,detail::AYPX<ScalarType>(alpha));
           }
 template <typename Array1, typename Array2, typename ScalarType>
   void aypx(const Array1& x, Array2& y, ScalarType alpha)
 {
#if defined(CUSP_VERSION) && CUSP_VERSION >= 500
   cusp::assert_same_dimensions(x,y);
#else
   detail::assert_same_dimensions(x,y);
#endif
   aypx(x.begin(),x.end(),y.begin(),alpha);
 }
}
}

#undef __FUNCT__
#define __FUNCT__ "VecAYPX_SeqCUSP"
PetscErrorCode VecAYPX_SeqCUSP(Vec yin, PetscScalar alpha, Vec xin)
{
  CUSPARRAY      *xarray,*yarray;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = VecCUSPGetArrayRead(xin,&xarray);CHKERRQ(ierr);
  ierr = VecCUSPGetArrayReadWrite(yin,&yarray);CHKERRQ(ierr);
  try {
    if (alpha != 0.0) {
      cusp::blas::aypx(*xarray,*yarray,alpha);
      ierr = PetscLogFlops(2.0*yin->map->n);CHKERRQ(ierr);
    } else {
      cusp::blas::copy(*xarray,*yarray);
    }
    ierr = WaitForGPU();CHKERRCUSP(ierr);
  } catch(char *ex) {
    SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"CUSP error: %s", ex);
  }
  ierr = VecCUSPRestoreArrayRead(xin,&xarray);CHKERRQ(ierr);
  ierr = VecCUSPRestoreArrayReadWrite(yin,&yarray);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "VecAXPY_SeqCUSP"
PetscErrorCode VecAXPY_SeqCUSP(Vec yin,PetscScalar alpha,Vec xin)
{
  CUSPARRAY      *xarray,*yarray;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (alpha != 0.0) {
    ierr = VecCUSPGetArrayRead(xin,&xarray);CHKERRQ(ierr);
    ierr = VecCUSPGetArrayReadWrite(yin,&yarray);CHKERRQ(ierr);
    try {
      cusp::blas::axpy(*xarray,*yarray,alpha);
      ierr = WaitForGPU();CHKERRCUSP(ierr);
    } catch(char *ex) {
      SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"CUSP error: %s", ex);
    }
    ierr = VecCUSPRestoreArrayRead(xin,&xarray);CHKERRQ(ierr);
    ierr = VecCUSPRestoreArrayReadWrite(yin,&yarray);CHKERRQ(ierr);
    ierr = PetscLogFlops(2.0*yin->map->n);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

struct VecCUSPPointwiseDivide
{
  template <typename Tuple>
  __host__ __device__
  void operator()(Tuple t)
  {
    thrust::get<0>(t) = thrust::get<1>(t) / thrust::get<2>(t);
  }
};

#undef __FUNCT__
#define __FUNCT__ "VecPointwiseDivide_SeqCUSP"
PetscErrorCode VecPointwiseDivide_SeqCUSP(Vec win, Vec xin, Vec yin)
{
  CUSPARRAY      *warray=NULL,*xarray=NULL,*yarray=NULL;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = VecCUSPGetArrayRead(xin,&xarray);CHKERRQ(ierr);
  ierr = VecCUSPGetArrayRead(yin,&yarray);CHKERRQ(ierr);
  ierr = VecCUSPGetArrayWrite(win,&warray);CHKERRQ(ierr);
  try {
    thrust::for_each(
      thrust::make_zip_iterator(
        thrust::make_tuple(
          warray->begin(),
          xarray->begin(),
          yarray->begin())),
      thrust::make_zip_iterator(
        thrust::make_tuple(
          warray->end(),
          xarray->end(),
          yarray->end())),
      VecCUSPPointwiseDivide());
    ierr = WaitForGPU();CHKERRCUSP(ierr);
  } catch(char *ex) {
    SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"CUSP error: %s", ex);
  }
  ierr = PetscLogFlops(win->map->n);CHKERRQ(ierr);
  ierr = VecCUSPRestoreArrayRead(xin,&xarray);CHKERRQ(ierr);
  ierr = VecCUSPRestoreArrayRead(yin,&yarray);CHKERRQ(ierr);
  ierr = VecCUSPRestoreArrayWrite(win,&warray);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}


struct VecCUSPWAXPY
{
  template <typename Tuple>
  __host__ __device__
  void operator()(Tuple t)
  {
    thrust::get<0>(t) = thrust::get<1>(t) + thrust::get<2>(t)*thrust::get<3>(t);
  }
};

struct VecCUSPSum
{
  template <typename Tuple>
  __host__ __device__
  void operator()(Tuple t)
  {
    thrust::get<0>(t) = thrust::get<1>(t) + thrust::get<2>(t);
  }
};

struct VecCUSPDiff
{
  template <typename Tuple>
  __host__ __device__
  void operator()(Tuple t)
  {
    thrust::get<0>(t) = thrust::get<1>(t) - thrust::get<2>(t);
  }
};

#undef __FUNCT__
#define __FUNCT__ "VecWAXPY_SeqCUSP"
PetscErrorCode VecWAXPY_SeqCUSP(Vec win,PetscScalar alpha,Vec xin, Vec yin)
{
  CUSPARRAY      *xarray=NULL,*yarray=NULL,*warray=NULL;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (alpha == 0.0) {
    ierr = VecCopy_SeqCUSP(yin,win);CHKERRQ(ierr);
  } else {
    ierr = VecCUSPGetArrayRead(xin,&xarray);CHKERRQ(ierr);
    ierr = VecCUSPGetArrayRead(yin,&yarray);CHKERRQ(ierr);
    ierr = VecCUSPGetArrayWrite(win,&warray);CHKERRQ(ierr);
    if (alpha == 1.0) {
      try {
        thrust::for_each(
          thrust::make_zip_iterator(
            thrust::make_tuple(
              warray->begin(),
              yarray->begin(),
              xarray->begin())),
          thrust::make_zip_iterator(
            thrust::make_tuple(
              warray->end(),
              yarray->end(),
              xarray->end())),
          VecCUSPSum());
      } catch(char *ex) {
        SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"CUSP error: %s", ex);
      }
      ierr = PetscLogFlops(win->map->n);CHKERRQ(ierr);
    } else if (alpha == -1.0) {
      try {
        thrust::for_each(
          thrust::make_zip_iterator(
            thrust::make_tuple(
              warray->begin(),
              yarray->begin(),
              xarray->begin())),
          thrust::make_zip_iterator(
            thrust::make_tuple(
              warray->end(),
              yarray->end(),
              xarray->end())),
          VecCUSPDiff());
      } catch(char *ex) {
        SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"CUSP error: %s", ex);
      }
      ierr = PetscLogFlops(win->map->n);CHKERRQ(ierr);
    } else {
      try {
        thrust::for_each(
          thrust::make_zip_iterator(
            thrust::make_tuple(
              warray->begin(),
              yarray->begin(),
              thrust::make_constant_iterator(alpha),
              xarray->begin())),
          thrust::make_zip_iterator(
            thrust::make_tuple(
              warray->end(),
              yarray->end(),
              thrust::make_constant_iterator(alpha),
              xarray->end())),
          VecCUSPWAXPY());
      } catch(char *ex) {
        SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"CUSP error: %s", ex);
      }
      ierr = PetscLogFlops(2*win->map->n);CHKERRQ(ierr);
    }
    ierr = WaitForGPU();CHKERRCUSP(ierr);
    ierr = VecCUSPRestoreArrayRead(xin,&xarray);CHKERRQ(ierr);
    ierr = VecCUSPRestoreArrayRead(yin,&yarray);CHKERRQ(ierr);
    ierr = VecCUSPRestoreArrayWrite(win,&warray);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

/* These functions are for the CUSP implementation of MAXPY with the loop unrolled on the CPU */
struct VecCUSPMAXPY4
{
  template <typename Tuple>
  __host__ __device__
  void operator()(Tuple t)
  {
    /*y += a1*x1 +a2*x2 + 13*x3 +a4*x4 */
    thrust::get<0>(t) += thrust::get<1>(t)*thrust::get<2>(t)+thrust::get<3>(t)*thrust::get<4>(t)+thrust::get<5>(t)*thrust::get<6>(t)+thrust::get<7>(t)*thrust::get<8>(t);
  }
};


struct VecCUSPMAXPY3
{
  template <typename Tuple>
  __host__ __device__
  void operator()(Tuple t)
  {
    /*y += a1*x1 +a2*x2 + a3*x3 */
    thrust::get<0>(t) += thrust::get<1>(t)*thrust::get<2>(t)+thrust::get<3>(t)*thrust::get<4>(t)+thrust::get<5>(t)*thrust::get<6>(t);
  }
};

struct VecCUSPMAXPY2
{
  template <typename Tuple>
  __host__ __device__
  void operator()(Tuple t)
  {
    /*y += a1*x1 +a2*x2*/
    thrust::get<0>(t) += thrust::get<1>(t)*thrust::get<2>(t)+thrust::get<3>(t)*thrust::get<4>(t);
  }
};
#undef __FUNCT__
#define __FUNCT__ "VecMAXPY_SeqCUSP"
PetscErrorCode VecMAXPY_SeqCUSP(Vec xin, PetscInt nv,const PetscScalar *alpha,Vec *y)
{
  PetscErrorCode ierr;
  CUSPARRAY      *xarray,*yy0,*yy1,*yy2,*yy3;
  PetscInt       n = xin->map->n,j,j_rem;
  PetscScalar    alpha0,alpha1,alpha2,alpha3;

  PetscFunctionBegin;
  ierr = PetscLogFlops(nv*2.0*n);CHKERRQ(ierr);
  ierr = VecCUSPGetArrayReadWrite(xin,&xarray);CHKERRQ(ierr);
  switch (j_rem=nv&0x3) {
  case 3:
    alpha0 = alpha[0];
    alpha1 = alpha[1];
    alpha2 = alpha[2];
    alpha += 3;
    ierr   = VecCUSPGetArrayRead(y[0],&yy0);CHKERRQ(ierr);
    ierr   = VecCUSPGetArrayRead(y[1],&yy1);CHKERRQ(ierr);
    ierr   = VecCUSPGetArrayRead(y[2],&yy2);CHKERRQ(ierr);
    try {
      thrust::for_each(
        thrust::make_zip_iterator(
          thrust::make_tuple(
            xarray->begin(),
            thrust::make_constant_iterator(alpha0),
            yy0->begin(),
            thrust::make_constant_iterator(alpha1),
            yy1->begin(),
            thrust::make_constant_iterator(alpha2),
            yy2->begin())),
        thrust::make_zip_iterator(
          thrust::make_tuple(
            xarray->end(),
            thrust::make_constant_iterator(alpha0),
            yy0->end(),
            thrust::make_constant_iterator(alpha1),
            yy1->end(),
            thrust::make_constant_iterator(alpha2),
            yy2->end())),
        VecCUSPMAXPY3());
    } catch(char *ex) {
      SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"CUSP error: %s", ex);
    }
    ierr = VecCUSPRestoreArrayRead(y[0],&yy0);CHKERRQ(ierr);
    ierr = VecCUSPRestoreArrayRead(y[1],&yy1);CHKERRQ(ierr);
    ierr = VecCUSPRestoreArrayRead(y[2],&yy2);CHKERRQ(ierr);
    y   += 3;
    break;
  case 2:
    alpha0 = alpha[0];
    alpha1 = alpha[1];
    alpha +=2;
    ierr   = VecCUSPGetArrayRead(y[0],&yy0);CHKERRQ(ierr);
    ierr   = VecCUSPGetArrayRead(y[1],&yy1);CHKERRQ(ierr);
    try {
      thrust::for_each(
        thrust::make_zip_iterator(
          thrust::make_tuple(
            xarray->begin(),
            thrust::make_constant_iterator(alpha0),
            yy0->begin(),
            thrust::make_constant_iterator(alpha1),
            yy1->begin())),
        thrust::make_zip_iterator(
          thrust::make_tuple(
            xarray->end(),
            thrust::make_constant_iterator(alpha0),
            yy0->end(),
            thrust::make_constant_iterator(alpha1),
            yy1->end())),
        VecCUSPMAXPY2());
    } catch(char *ex) {
      SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"CUSP error: %s", ex);
    }
    y +=2;
    break;
  case 1:
    alpha0 = *alpha++;
    ierr   = VecAXPY_SeqCUSP(xin,alpha0,y[0]);
    y     +=1;
    break;
  }
  for (j=j_rem; j<nv; j+=4) {
    alpha0 = alpha[0];
    alpha1 = alpha[1];
    alpha2 = alpha[2];
    alpha3 = alpha[3];
    alpha += 4;
    ierr   = VecCUSPGetArrayRead(y[0],&yy0);CHKERRQ(ierr);
    ierr   = VecCUSPGetArrayRead(y[1],&yy1);CHKERRQ(ierr);
    ierr   = VecCUSPGetArrayRead(y[2],&yy2);CHKERRQ(ierr);
    ierr   = VecCUSPGetArrayRead(y[3],&yy3);CHKERRQ(ierr);
    try {
      thrust::for_each(
        thrust::make_zip_iterator(
          thrust::make_tuple(
            xarray->begin(),
            thrust::make_constant_iterator(alpha0),
            yy0->begin(),
            thrust::make_constant_iterator(alpha1),
            yy1->begin(),
            thrust::make_constant_iterator(alpha2),
            yy2->begin(),
            thrust::make_constant_iterator(alpha3),
            yy3->begin())),
        thrust::make_zip_iterator(
          thrust::make_tuple(
            xarray->end(),
            thrust::make_constant_iterator(alpha0),
            yy0->end(),
            thrust::make_constant_iterator(alpha1),
            yy1->end(),
            thrust::make_constant_iterator(alpha2),
            yy2->end(),
            thrust::make_constant_iterator(alpha3),
            yy3->end())),
        VecCUSPMAXPY4());
    } catch(char *ex) {
      SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"CUSP error: %s", ex);
    }
    ierr = VecCUSPRestoreArrayRead(y[0],&yy0);CHKERRQ(ierr);
    ierr = VecCUSPRestoreArrayRead(y[1],&yy1);CHKERRQ(ierr);
    ierr = VecCUSPRestoreArrayRead(y[2],&yy2);CHKERRQ(ierr);
    ierr = VecCUSPRestoreArrayRead(y[3],&yy3);CHKERRQ(ierr);
    y   += 4;
  }
  ierr = VecCUSPRestoreArrayReadWrite(xin,&xarray);CHKERRQ(ierr);
  ierr = WaitForGPU();CHKERRCUSP(ierr);
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "VecDot_SeqCUSP"
PetscErrorCode VecDot_SeqCUSP(Vec xin,Vec yin,PetscScalar *z)
{
  CUSPARRAY      *xarray,*yarray;
  PetscErrorCode ierr;
  //  PetscScalar    *xptr,*yptr,*zgpu;
  //PetscReal tmp;

  PetscFunctionBegin;
  //VecNorm_SeqCUSP(xin, NORM_2, &tmp);
  //VecNorm_SeqCUSP(yin, NORM_2, &tmp);
  ierr = VecCUSPGetArrayRead(xin,&xarray);CHKERRQ(ierr);
  ierr = VecCUSPGetArrayRead(yin,&yarray);CHKERRQ(ierr);
  try {
#if defined(PETSC_USE_COMPLEX)
    *z = cusp::blas::dotc(*yarray,*xarray);
#else
    *z = cusp::blas::dot(*yarray,*xarray);
#endif
  } catch(char *ex) {
    SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"CUSP error: %s", ex);
  }
  ierr = WaitForGPU();CHKERRCUSP(ierr);
  if (xin->map->n >0) {
    ierr = PetscLogFlops(2.0*xin->map->n-1);CHKERRQ(ierr);
  }
  ierr = VecCUSPRestoreArrayRead(xin,&xarray);CHKERRQ(ierr);
  ierr = VecCUSPRestoreArrayRead(yin,&yarray);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

//
// CUDA kernels for MDot to follow
//

// set work group size to be a power of 2 (128 is usually a good compromise between portability and speed)
#define MDOT_WORKGROUP_SIZE 128
#define MDOT_WORKGROUP_NUM  128

// M = 2:
__global__ void VecMDot_SeqCUSP_kernel2(const PetscScalar *x,const PetscScalar *y0,const PetscScalar *y1,
                                        PetscInt size, PetscScalar *group_results)
{
  __shared__ PetscScalar tmp_buffer[2*MDOT_WORKGROUP_SIZE];
  PetscInt entries_per_group = (size - 1) / gridDim.x + 1;
  entries_per_group = (entries_per_group == 0) ? 1 : entries_per_group;  // for very small vectors, a group should still do some work
  PetscInt vec_start_index = blockIdx.x * entries_per_group;
  PetscInt vec_stop_index  = min((blockIdx.x + 1) * entries_per_group, size); // don't go beyond vec size

  PetscScalar entry_x    = 0;
  PetscScalar group_sum0 = 0;
  PetscScalar group_sum1 = 0;
  for (PetscInt i = vec_start_index + threadIdx.x; i < vec_stop_index; i += blockDim.x) {
    entry_x     = x[i];   // load only once from global memory!
    group_sum0 += entry_x * y0[i];
    group_sum1 += entry_x * y1[i];
  }
  tmp_buffer[threadIdx.x]                       = group_sum0;
  tmp_buffer[threadIdx.x + MDOT_WORKGROUP_SIZE] = group_sum1;

  // parallel reduction
  for (PetscInt stride = blockDim.x/2; stride > 0; stride /= 2) {
    __syncthreads();
    if (threadIdx.x < stride) {
      tmp_buffer[threadIdx.x                      ] += tmp_buffer[threadIdx.x+stride                      ];
      tmp_buffer[threadIdx.x + MDOT_WORKGROUP_SIZE] += tmp_buffer[threadIdx.x+stride + MDOT_WORKGROUP_SIZE];
    }
  }

  // write result of group to group_results
  if (threadIdx.x == 0) {
    group_results[blockIdx.x]             = tmp_buffer[0];
    group_results[blockIdx.x + gridDim.x] = tmp_buffer[MDOT_WORKGROUP_SIZE];
  }
}

// M = 3:
__global__ void VecMDot_SeqCUSP_kernel3(const PetscScalar *x,const PetscScalar *y0,const PetscScalar *y1,const PetscScalar *y2,
                                        PetscInt size, PetscScalar *group_results)
{
  __shared__ PetscScalar tmp_buffer[3*MDOT_WORKGROUP_SIZE];
  PetscInt entries_per_group = (size - 1) / gridDim.x + 1;
  entries_per_group = (entries_per_group == 0) ? 1 : entries_per_group;  // for very small vectors, a group should still do some work
  PetscInt vec_start_index = blockIdx.x * entries_per_group;
  PetscInt vec_stop_index  = min((blockIdx.x + 1) * entries_per_group, size); // don't go beyond vec size

  PetscScalar entry_x    = 0;
  PetscScalar group_sum0 = 0;
  PetscScalar group_sum1 = 0;
  PetscScalar group_sum2 = 0;
  for (PetscInt i = vec_start_index + threadIdx.x; i < vec_stop_index; i += blockDim.x) {
    entry_x     = x[i];   // load only once from global memory!
    group_sum0 += entry_x * y0[i];
    group_sum1 += entry_x * y1[i];
    group_sum2 += entry_x * y2[i];
  }
  tmp_buffer[threadIdx.x]                           = group_sum0;
  tmp_buffer[threadIdx.x +     MDOT_WORKGROUP_SIZE] = group_sum1;
  tmp_buffer[threadIdx.x + 2 * MDOT_WORKGROUP_SIZE] = group_sum2;

  // parallel reduction
  for (PetscInt stride = blockDim.x/2; stride > 0; stride /= 2) {
    __syncthreads();
    if (threadIdx.x < stride) {
      tmp_buffer[threadIdx.x                          ] += tmp_buffer[threadIdx.x+stride                          ];
      tmp_buffer[threadIdx.x +     MDOT_WORKGROUP_SIZE] += tmp_buffer[threadIdx.x+stride +     MDOT_WORKGROUP_SIZE];
      tmp_buffer[threadIdx.x + 2 * MDOT_WORKGROUP_SIZE] += tmp_buffer[threadIdx.x+stride + 2 * MDOT_WORKGROUP_SIZE];
    }
  }

  // write result of group to group_results
  if (threadIdx.x == 0) {
    group_results[blockIdx.x                ] = tmp_buffer[0];
    group_results[blockIdx.x +     gridDim.x] = tmp_buffer[    MDOT_WORKGROUP_SIZE];
    group_results[blockIdx.x + 2 * gridDim.x] = tmp_buffer[2 * MDOT_WORKGROUP_SIZE];
  }
}

// M = 4:
__global__ void VecMDot_SeqCUSP_kernel4(const PetscScalar *x,const PetscScalar *y0,const PetscScalar *y1,const PetscScalar *y2,const PetscScalar *y3,
                                        PetscInt size, PetscScalar *group_results)
{
  __shared__ PetscScalar tmp_buffer[4*MDOT_WORKGROUP_SIZE];
  PetscInt entries_per_group = (size - 1) / gridDim.x + 1;
  entries_per_group = (entries_per_group == 0) ? 1 : entries_per_group;  // for very small vectors, a group should still do some work
  PetscInt vec_start_index = blockIdx.x * entries_per_group;
  PetscInt vec_stop_index  = min((blockIdx.x + 1) * entries_per_group, size); // don't go beyond vec size

  PetscScalar entry_x    = 0;
  PetscScalar group_sum0 = 0;
  PetscScalar group_sum1 = 0;
  PetscScalar group_sum2 = 0;
  PetscScalar group_sum3 = 0;
  for (PetscInt i = vec_start_index + threadIdx.x; i < vec_stop_index; i += blockDim.x) {
    entry_x     = x[i];   // load only once from global memory!
    group_sum0 += entry_x * y0[i];
    group_sum1 += entry_x * y1[i];
    group_sum2 += entry_x * y2[i];
    group_sum3 += entry_x * y3[i];
  }
  tmp_buffer[threadIdx.x]                           = group_sum0;
  tmp_buffer[threadIdx.x +     MDOT_WORKGROUP_SIZE] = group_sum1;
  tmp_buffer[threadIdx.x + 2 * MDOT_WORKGROUP_SIZE] = group_sum2;
  tmp_buffer[threadIdx.x + 3 * MDOT_WORKGROUP_SIZE] = group_sum3;

  // parallel reduction
  for (PetscInt stride = blockDim.x/2; stride > 0; stride /= 2) {
    __syncthreads();
    if (threadIdx.x < stride) {
      tmp_buffer[threadIdx.x                          ] += tmp_buffer[threadIdx.x+stride                          ];
      tmp_buffer[threadIdx.x +     MDOT_WORKGROUP_SIZE] += tmp_buffer[threadIdx.x+stride +     MDOT_WORKGROUP_SIZE];
      tmp_buffer[threadIdx.x + 2 * MDOT_WORKGROUP_SIZE] += tmp_buffer[threadIdx.x+stride + 2 * MDOT_WORKGROUP_SIZE];
      tmp_buffer[threadIdx.x + 3 * MDOT_WORKGROUP_SIZE] += tmp_buffer[threadIdx.x+stride + 3 * MDOT_WORKGROUP_SIZE];
    }
  }

  // write result of group to group_results
  if (threadIdx.x == 0) {
    group_results[blockIdx.x                ] = tmp_buffer[0];
    group_results[blockIdx.x +     gridDim.x] = tmp_buffer[    MDOT_WORKGROUP_SIZE];
    group_results[blockIdx.x + 2 * gridDim.x] = tmp_buffer[2 * MDOT_WORKGROUP_SIZE];
    group_results[blockIdx.x + 3 * gridDim.x] = tmp_buffer[3 * MDOT_WORKGROUP_SIZE];
  }
}

// M = 8:
__global__ void VecMDot_SeqCUSP_kernel8(const PetscScalar *x,const PetscScalar *y0,const PetscScalar *y1,const PetscScalar *y2,const PetscScalar *y3,
                                          const PetscScalar *y4,const PetscScalar *y5,const PetscScalar *y6,const PetscScalar *y7,
                                          PetscInt size, PetscScalar *group_results)
{
  __shared__ PetscScalar tmp_buffer[8*MDOT_WORKGROUP_SIZE];
  PetscInt entries_per_group = (size - 1) / gridDim.x + 1;
  entries_per_group = (entries_per_group == 0) ? 1 : entries_per_group;  // for very small vectors, a group should still do some work
  PetscInt vec_start_index = blockIdx.x * entries_per_group;
  PetscInt vec_stop_index  = min((blockIdx.x + 1) * entries_per_group, size); // don't go beyond vec size

  PetscScalar entry_x    = 0;
  PetscScalar group_sum0 = 0;
  PetscScalar group_sum1 = 0;
  PetscScalar group_sum2 = 0;
  PetscScalar group_sum3 = 0;
  PetscScalar group_sum4 = 0;
  PetscScalar group_sum5 = 0;
  PetscScalar group_sum6 = 0;
  PetscScalar group_sum7 = 0;
  for (PetscInt i = vec_start_index + threadIdx.x; i < vec_stop_index; i += blockDim.x) {
    entry_x     = x[i];   // load only once from global memory!
    group_sum0 += entry_x * y0[i];
    group_sum1 += entry_x * y1[i];
    group_sum2 += entry_x * y2[i];
    group_sum3 += entry_x * y3[i];
    group_sum4 += entry_x * y4[i];
    group_sum5 += entry_x * y5[i];
    group_sum6 += entry_x * y6[i];
    group_sum7 += entry_x * y7[i];
  }
  tmp_buffer[threadIdx.x]                           = group_sum0;
  tmp_buffer[threadIdx.x +     MDOT_WORKGROUP_SIZE] = group_sum1;
  tmp_buffer[threadIdx.x + 2 * MDOT_WORKGROUP_SIZE] = group_sum2;
  tmp_buffer[threadIdx.x + 3 * MDOT_WORKGROUP_SIZE] = group_sum3;
  tmp_buffer[threadIdx.x + 4 * MDOT_WORKGROUP_SIZE] = group_sum4;
  tmp_buffer[threadIdx.x + 5 * MDOT_WORKGROUP_SIZE] = group_sum5;
  tmp_buffer[threadIdx.x + 6 * MDOT_WORKGROUP_SIZE] = group_sum6;
  tmp_buffer[threadIdx.x + 7 * MDOT_WORKGROUP_SIZE] = group_sum7;

  // parallel reduction
  for (PetscInt stride = blockDim.x/2; stride > 0; stride /= 2) {
    __syncthreads();
    if (threadIdx.x < stride) {
      tmp_buffer[threadIdx.x                          ] += tmp_buffer[threadIdx.x+stride                          ];
      tmp_buffer[threadIdx.x +     MDOT_WORKGROUP_SIZE] += tmp_buffer[threadIdx.x+stride +     MDOT_WORKGROUP_SIZE];
      tmp_buffer[threadIdx.x + 2 * MDOT_WORKGROUP_SIZE] += tmp_buffer[threadIdx.x+stride + 2 * MDOT_WORKGROUP_SIZE];
      tmp_buffer[threadIdx.x + 3 * MDOT_WORKGROUP_SIZE] += tmp_buffer[threadIdx.x+stride + 3 * MDOT_WORKGROUP_SIZE];
      tmp_buffer[threadIdx.x + 4 * MDOT_WORKGROUP_SIZE] += tmp_buffer[threadIdx.x+stride + 4 * MDOT_WORKGROUP_SIZE];
      tmp_buffer[threadIdx.x + 5 * MDOT_WORKGROUP_SIZE] += tmp_buffer[threadIdx.x+stride + 5 * MDOT_WORKGROUP_SIZE];
      tmp_buffer[threadIdx.x + 6 * MDOT_WORKGROUP_SIZE] += tmp_buffer[threadIdx.x+stride + 6 * MDOT_WORKGROUP_SIZE];
      tmp_buffer[threadIdx.x + 7 * MDOT_WORKGROUP_SIZE] += tmp_buffer[threadIdx.x+stride + 7 * MDOT_WORKGROUP_SIZE];
    }
  }

  // write result of group to group_results
  if (threadIdx.x == 0) {
    group_results[blockIdx.x                ] = tmp_buffer[0];
    group_results[blockIdx.x +     gridDim.x] = tmp_buffer[    MDOT_WORKGROUP_SIZE];
    group_results[blockIdx.x + 2 * gridDim.x] = tmp_buffer[2 * MDOT_WORKGROUP_SIZE];
    group_results[blockIdx.x + 3 * gridDim.x] = tmp_buffer[3 * MDOT_WORKGROUP_SIZE];
    group_results[blockIdx.x + 4 * gridDim.x] = tmp_buffer[4 * MDOT_WORKGROUP_SIZE];
    group_results[blockIdx.x + 5 * gridDim.x] = tmp_buffer[5 * MDOT_WORKGROUP_SIZE];
    group_results[blockIdx.x + 6 * gridDim.x] = tmp_buffer[6 * MDOT_WORKGROUP_SIZE];
    group_results[blockIdx.x + 7 * gridDim.x] = tmp_buffer[7 * MDOT_WORKGROUP_SIZE];
  }
}


#undef __FUNCT__
#define __FUNCT__ "VecMDot_SeqCUSP"
PetscErrorCode VecMDot_SeqCUSP(Vec xin,PetscInt nv,const Vec yin[],PetscScalar *z)
{
  PetscErrorCode ierr;
  PetscInt       i,j,n = xin->map->n,current_y_index = 0;
  CUSPARRAY      *xarray,*y0array,*y1array,*y2array,*y3array,*y4array,*y5array,*y6array,*y7array;
  PetscScalar    *group_results_gpu,*xptr,*y0ptr,*y1ptr,*y2ptr,*y3ptr,*y4ptr,*y5ptr,*y6ptr,*y7ptr;
  PetscScalar    group_results_cpu[MDOT_WORKGROUP_NUM * 8]; // we process at most eight vectors in one kernel
  cudaError_t    cuda_ierr;

  PetscFunctionBegin;
  if (nv <= 0) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_LIB,"Number of vectors provided to VecMDot_SeqCUSP not positive.");
  /* Handle the case of local size zero first */
  if (!xin->map->n) {
    for (i=0; i<nv; ++i) z[i] = 0;
    PetscFunctionReturn(0);
  }

  // allocate scratchpad memory for the results of individual work groups:
  cuda_ierr = cudaMalloc((void**)&group_results_gpu, sizeof(PetscScalar) * MDOT_WORKGROUP_NUM * 8);
  if (cuda_ierr != cudaSuccess) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"Could not allocate CUDA work memory. Error code: %d", (int)cuda_ierr);

  ierr = VecCUSPGetArrayRead(xin,&xarray);CHKERRQ(ierr);
  xptr = thrust::raw_pointer_cast(xarray->data());

  while (current_y_index < nv)
  {
    switch (nv - current_y_index) {

    case 7:
    case 6:
    case 5:
    case 4:
      ierr = VecCUSPGetArrayRead(yin[current_y_index  ],&y0array);CHKERRQ(ierr);
      ierr = VecCUSPGetArrayRead(yin[current_y_index+1],&y1array);CHKERRQ(ierr);
      ierr = VecCUSPGetArrayRead(yin[current_y_index+2],&y2array);CHKERRQ(ierr);
      ierr = VecCUSPGetArrayRead(yin[current_y_index+3],&y3array);CHKERRQ(ierr);

#if defined(PETSC_USE_COMPLEX)
      z[current_y_index]   = cusp::blas::dot(*y0array,*xarray);
      z[current_y_index+1] = cusp::blas::dot(*y1array,*xarray);
      z[current_y_index+2] = cusp::blas::dot(*y2array,*xarray);
      z[current_y_index+3] = cusp::blas::dot(*y3array,*xarray);
#else
      // extract raw device pointers:
      y0ptr = thrust::raw_pointer_cast(y0array->data());
      y1ptr = thrust::raw_pointer_cast(y1array->data());
      y2ptr = thrust::raw_pointer_cast(y2array->data());
      y3ptr = thrust::raw_pointer_cast(y3array->data());

      // run kernel:
      VecMDot_SeqCUSP_kernel4<<<MDOT_WORKGROUP_NUM,MDOT_WORKGROUP_SIZE>>>(xptr,y0ptr,y1ptr,y2ptr,y3ptr,n,group_results_gpu);

      // copy results back to
      cuda_ierr = cudaMemcpy(group_results_cpu,group_results_gpu,sizeof(PetscScalar) * MDOT_WORKGROUP_NUM * 4,cudaMemcpyDeviceToHost);
      if (cuda_ierr != cudaSuccess) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"Could not copy CUDA buffer to host. Error code: %d", (int)cuda_ierr);

      // sum group results into z:
      for (j=0; j<4; ++j) {
        z[current_y_index + j] = 0;
        for (i=j*MDOT_WORKGROUP_NUM; i<(j+1)*MDOT_WORKGROUP_NUM; ++i) z[current_y_index + j] += group_results_cpu[i];
      }
#endif
      ierr = VecCUSPRestoreArrayRead(yin[current_y_index  ],&y0array);CHKERRQ(ierr);
      ierr = VecCUSPRestoreArrayRead(yin[current_y_index+1],&y1array);CHKERRQ(ierr);
      ierr = VecCUSPRestoreArrayRead(yin[current_y_index+2],&y2array);CHKERRQ(ierr);
      ierr = VecCUSPRestoreArrayRead(yin[current_y_index+3],&y3array);CHKERRQ(ierr);
      current_y_index += 4;
      break;

    case 3:
      ierr = VecCUSPGetArrayRead(yin[current_y_index  ],&y0array);CHKERRQ(ierr);
      ierr = VecCUSPGetArrayRead(yin[current_y_index+1],&y1array);CHKERRQ(ierr);
      ierr = VecCUSPGetArrayRead(yin[current_y_index+2],&y2array);CHKERRQ(ierr);

#if defined(PETSC_USE_COMPLEX)
      z[current_y_index]   = cusp::blas::dot(*y0array,*xarray);
      z[current_y_index+1] = cusp::blas::dot(*y1array,*xarray);
      z[current_y_index+2] = cusp::blas::dot(*y2array,*xarray);
#else
      // extract raw device pointers:
      y0ptr = thrust::raw_pointer_cast(y0array->data());
      y1ptr = thrust::raw_pointer_cast(y1array->data());
      y2ptr = thrust::raw_pointer_cast(y2array->data());

      // run kernel:
      VecMDot_SeqCUSP_kernel3<<<MDOT_WORKGROUP_NUM,MDOT_WORKGROUP_SIZE>>>(xptr,y0ptr,y1ptr,y2ptr,n,group_results_gpu);

      // copy results back to
      cuda_ierr = cudaMemcpy(group_results_cpu,group_results_gpu,sizeof(PetscScalar) * MDOT_WORKGROUP_NUM * 3,cudaMemcpyDeviceToHost);
      if (cuda_ierr != cudaSuccess) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"Could not copy CUDA buffer to host. Error code: %d", (int)cuda_ierr);

      // sum group results into z:
      for (j=0; j<3; ++j) {
        z[current_y_index + j] = 0;
        for (i=j*MDOT_WORKGROUP_NUM; i<(j+1)*MDOT_WORKGROUP_NUM; ++i) z[current_y_index + j] += group_results_cpu[i];
      }
#endif

      ierr = VecCUSPRestoreArrayRead(yin[current_y_index  ],&y0array);CHKERRQ(ierr);
      ierr = VecCUSPRestoreArrayRead(yin[current_y_index+1],&y1array);CHKERRQ(ierr);
      ierr = VecCUSPRestoreArrayRead(yin[current_y_index+2],&y2array);CHKERRQ(ierr);
      current_y_index += 3;
      break;

    case 2:
      ierr = VecCUSPGetArrayRead(yin[current_y_index],&y0array);CHKERRQ(ierr);
      ierr = VecCUSPGetArrayRead(yin[current_y_index+1],&y1array);CHKERRQ(ierr);

#if defined(PETSC_USE_COMPLEX)
      z[current_y_index]   = cusp::blas::dot(*y0array,*xarray);
      z[current_y_index+1] = cusp::blas::dot(*y1array,*xarray);
#else
      // extract raw device pointers:
      y0ptr = thrust::raw_pointer_cast(y0array->data());
      y1ptr = thrust::raw_pointer_cast(y1array->data());

      // run kernel:
      VecMDot_SeqCUSP_kernel2<<<MDOT_WORKGROUP_NUM,MDOT_WORKGROUP_SIZE>>>(xptr,y0ptr,y1ptr,n,group_results_gpu);

      // copy results back to 
      cuda_ierr = cudaMemcpy(group_results_cpu,group_results_gpu,sizeof(PetscScalar) * MDOT_WORKGROUP_NUM * 2,cudaMemcpyDeviceToHost);
      if (cuda_ierr != cudaSuccess) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"Could not copy CUDA buffer to host. Error code: %d", (int)cuda_ierr);

      // sum group results into z:
      for (j=0; j<2; ++j) {
        z[current_y_index + j] = 0;
        for (i=j*MDOT_WORKGROUP_NUM; i<(j+1)*MDOT_WORKGROUP_NUM; ++i) z[current_y_index + j] += group_results_cpu[i];
      }
#endif
      ierr = VecCUSPRestoreArrayRead(yin[current_y_index],&y0array);CHKERRQ(ierr);
      ierr = VecCUSPRestoreArrayRead(yin[current_y_index+1],&y1array);CHKERRQ(ierr);
      current_y_index += 2;
      break;

    case 1:
      ierr = VecCUSPGetArrayRead(yin[current_y_index],&y0array);CHKERRQ(ierr);
#if defined(PETSC_USE_COMPLEX)
      z[current_y_index] = cusp::blas::dotc(*y0array, *xarray);
#else
      z[current_y_index] = cusp::blas::dot(*xarray, *y0array);
#endif
      ierr = VecCUSPRestoreArrayRead(yin[current_y_index],&y0array);CHKERRQ(ierr);
      current_y_index += 1;
      break;

    default: // 8 or more vectors left
      ierr = VecCUSPGetArrayRead(yin[current_y_index  ],&y0array);CHKERRQ(ierr);
      ierr = VecCUSPGetArrayRead(yin[current_y_index+1],&y1array);CHKERRQ(ierr);
      ierr = VecCUSPGetArrayRead(yin[current_y_index+2],&y2array);CHKERRQ(ierr);
      ierr = VecCUSPGetArrayRead(yin[current_y_index+3],&y3array);CHKERRQ(ierr);
      ierr = VecCUSPGetArrayRead(yin[current_y_index+4],&y4array);CHKERRQ(ierr);
      ierr = VecCUSPGetArrayRead(yin[current_y_index+5],&y5array);CHKERRQ(ierr);
      ierr = VecCUSPGetArrayRead(yin[current_y_index+6],&y6array);CHKERRQ(ierr);
      ierr = VecCUSPGetArrayRead(yin[current_y_index+7],&y7array);CHKERRQ(ierr);

#if defined(PETSC_USE_COMPLEX)
      z[current_y_index]   = cusp::blas::dot(*y0array,*xarray);
      z[current_y_index+1] = cusp::blas::dot(*y1array,*xarray);
      z[current_y_index+2] = cusp::blas::dot(*y2array,*xarray);
      z[current_y_index+3] = cusp::blas::dot(*y3array,*xarray);
      z[current_y_index+4] = cusp::blas::dot(*y4array,*xarray);
      z[current_y_index+5] = cusp::blas::dot(*y5array,*xarray);
      z[current_y_index+6] = cusp::blas::dot(*y6array,*xarray);
      z[current_y_index+7] = cusp::blas::dot(*y7array,*xarray);
#else
      // extract raw device pointers:
      y0ptr = thrust::raw_pointer_cast(y0array->data());
      y1ptr = thrust::raw_pointer_cast(y1array->data());
      y2ptr = thrust::raw_pointer_cast(y2array->data());
      y3ptr = thrust::raw_pointer_cast(y3array->data());
      y4ptr = thrust::raw_pointer_cast(y4array->data());
      y5ptr = thrust::raw_pointer_cast(y5array->data());
      y6ptr = thrust::raw_pointer_cast(y6array->data());
      y7ptr = thrust::raw_pointer_cast(y7array->data());

      // run kernel:
      VecMDot_SeqCUSP_kernel8<<<MDOT_WORKGROUP_NUM,MDOT_WORKGROUP_SIZE>>>(xptr,y0ptr,y1ptr,y2ptr,y3ptr,y4ptr,y5ptr,y6ptr,y7ptr,n,group_results_gpu);

      // copy results back to
      cuda_ierr = cudaMemcpy(group_results_cpu,group_results_gpu,sizeof(PetscScalar) * MDOT_WORKGROUP_NUM * 8,cudaMemcpyDeviceToHost);
      if (cuda_ierr != cudaSuccess) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"Could not copy CUDA buffer to host. Error code: %d", (int)cuda_ierr);

      // sum group results into z:
      for (j=0; j<8; ++j) {
        z[current_y_index + j] = 0;
        for (i=j*MDOT_WORKGROUP_NUM; i<(j+1)*MDOT_WORKGROUP_NUM; ++i) z[current_y_index + j] += group_results_cpu[i];
      }
#endif
      ierr = VecCUSPRestoreArrayRead(yin[current_y_index  ],&y0array);CHKERRQ(ierr);
      ierr = VecCUSPRestoreArrayRead(yin[current_y_index+1],&y1array);CHKERRQ(ierr);
      ierr = VecCUSPRestoreArrayRead(yin[current_y_index+2],&y2array);CHKERRQ(ierr);
      ierr = VecCUSPRestoreArrayRead(yin[current_y_index+3],&y3array);CHKERRQ(ierr);
      ierr = VecCUSPRestoreArrayRead(yin[current_y_index+4],&y4array);CHKERRQ(ierr);
      ierr = VecCUSPRestoreArrayRead(yin[current_y_index+5],&y5array);CHKERRQ(ierr);
      ierr = VecCUSPRestoreArrayRead(yin[current_y_index+6],&y6array);CHKERRQ(ierr);
      ierr = VecCUSPRestoreArrayRead(yin[current_y_index+7],&y7array);CHKERRQ(ierr);
      current_y_index += 8;
      break;
    }
  }
  ierr = VecCUSPRestoreArrayRead(xin,&xarray);CHKERRQ(ierr);

  cuda_ierr = cudaFree(group_results_gpu);
  if (cuda_ierr != cudaSuccess) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"Could not copy CUDA buffer to host: %d", (int)cuda_ierr);
  ierr = PetscLogFlops(PetscMax(nv*(2.0*n-1),0.0));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef MDOT_WORKGROUP_SIZE
#undef MDOT_WORKGROUP_NUM



#undef __FUNCT__
#define __FUNCT__ "VecSet_SeqCUSP"
PetscErrorCode VecSet_SeqCUSP(Vec xin,PetscScalar alpha)
{
  CUSPARRAY      *xarray=NULL;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  /* if there's a faster way to do the case alpha=0.0 on the GPU we should do that*/
  ierr = VecCUSPGetArrayWrite(xin,&xarray);CHKERRQ(ierr);
  try {
    cusp::blas::fill(*xarray,alpha);
  } catch(char *ex) {
    SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"CUSP error: %s", ex);
  }
  ierr = WaitForGPU();CHKERRCUSP(ierr);
  ierr = VecCUSPRestoreArrayWrite(xin,&xarray);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecScale_SeqCUSP"
PetscErrorCode VecScale_SeqCUSP(Vec xin, PetscScalar alpha)
{
  CUSPARRAY      *xarray;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (alpha == 0.0) {
    ierr = VecSet_SeqCUSP(xin,alpha);CHKERRQ(ierr);
  } else if (alpha != 1.0) {
    ierr = VecCUSPGetArrayReadWrite(xin,&xarray);CHKERRQ(ierr);
    try {
      cusp::blas::scal(*xarray,alpha);
    } catch(char *ex) {
      SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"CUSP error: %s", ex);
    }
    ierr = VecCUSPRestoreArrayReadWrite(xin,&xarray);CHKERRQ(ierr);
  }
  ierr = WaitForGPU();CHKERRCUSP(ierr);
  ierr = PetscLogFlops(xin->map->n);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "VecTDot_SeqCUSP"
PetscErrorCode VecTDot_SeqCUSP(Vec xin,Vec yin,PetscScalar *z)
{
  CUSPARRAY      *xarray,*yarray;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  //#if defined(PETSC_USE_COMPLEX)
  /*Not working for complex*/
  //#else
  ierr = VecCUSPGetArrayRead(xin,&xarray);CHKERRQ(ierr);
  ierr = VecCUSPGetArrayRead(yin,&yarray);CHKERRQ(ierr);
  try {
    *z = cusp::blas::dot(*xarray,*yarray);
  } catch(char *ex) {
    SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"CUSP error: %s", ex);
  }
  //#endif
  ierr = WaitForGPU();CHKERRCUSP(ierr);
  if (xin->map->n > 0) {
    ierr = PetscLogFlops(2.0*xin->map->n-1);CHKERRQ(ierr);
  }
  ierr = VecCUSPRestoreArrayRead(yin,&yarray);CHKERRQ(ierr);
  ierr = VecCUSPRestoreArrayRead(xin,&xarray);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
#undef __FUNCT__
#define __FUNCT__ "VecCopy_SeqCUSP"
PetscErrorCode VecCopy_SeqCUSP(Vec xin,Vec yin)
{
  CUSPARRAY      *xarray,*yarray;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (xin != yin) {
    if (xin->valid_GPU_array == PETSC_CUSP_GPU) {
      ierr = VecCUSPGetArrayRead(xin,&xarray);CHKERRQ(ierr);
      ierr = VecCUSPGetArrayWrite(yin,&yarray);CHKERRQ(ierr);
      try {
        cusp::blas::copy(*xarray,*yarray);
      } catch(char *ex) {
        SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"CUSP error: %s", ex);
      }
      ierr = WaitForGPU();CHKERRCUSP(ierr);
      ierr = VecCUSPRestoreArrayRead(xin,&xarray);CHKERRQ(ierr);
      ierr = VecCUSPRestoreArrayWrite(yin,&yarray);CHKERRQ(ierr);

    } else if (xin->valid_GPU_array == PETSC_CUSP_CPU) {
      /* copy in CPU if we are on the CPU*/
      ierr = VecCopy_SeqCUSP_Private(xin,yin);CHKERRQ(ierr);
    } else if (xin->valid_GPU_array == PETSC_CUSP_BOTH) {
      /* if xin is valid in both places, see where yin is and copy there (because it's probably where we'll want to next use it) */
      if (yin->valid_GPU_array == PETSC_CUSP_CPU) {
        /* copy in CPU */
        ierr = VecCopy_SeqCUSP_Private(xin,yin);CHKERRQ(ierr);

      } else if (yin->valid_GPU_array == PETSC_CUSP_GPU) {
        /* copy in GPU */
        ierr = VecCUSPGetArrayRead(xin,&xarray);CHKERRQ(ierr);
        ierr = VecCUSPGetArrayWrite(yin,&yarray);CHKERRQ(ierr);
        try {
          cusp::blas::copy(*xarray,*yarray);
          ierr = WaitForGPU();CHKERRCUSP(ierr);
        } catch(char *ex) {
          SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"CUSP error: %s", ex);
        }
        ierr = VecCUSPRestoreArrayRead(xin,&xarray);CHKERRQ(ierr);
        ierr = VecCUSPRestoreArrayWrite(yin,&yarray);CHKERRQ(ierr);
      } else if (yin->valid_GPU_array == PETSC_CUSP_BOTH) {
        /* xin and yin are both valid in both places (or yin was unallocated before the earlier call to allocatecheck
           default to copy in GPU (this is an arbitrary choice) */
        ierr = VecCUSPGetArrayRead(xin,&xarray);CHKERRQ(ierr);
        ierr = VecCUSPGetArrayWrite(yin,&yarray);CHKERRQ(ierr);
        try {
          cusp::blas::copy(*xarray,*yarray);
          ierr = WaitForGPU();CHKERRCUSP(ierr);
        } catch(char *ex) {
          SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"CUSP error: %s", ex);
        }
        ierr = VecCUSPRestoreArrayRead(xin,&xarray);CHKERRQ(ierr);
        ierr = VecCUSPRestoreArrayWrite(yin,&yarray);CHKERRQ(ierr);
      } else {
        ierr = VecCopy_SeqCUSP_Private(xin,yin);CHKERRQ(ierr);
      }
    }
  }
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "VecSwap_SeqCUSP"
PetscErrorCode VecSwap_SeqCUSP(Vec xin,Vec yin)
{
  PetscErrorCode ierr;
  PetscBLASInt   one = 1,bn;
  CUSPARRAY      *xarray,*yarray;

  PetscFunctionBegin;
  ierr = PetscBLASIntCast(xin->map->n,&bn);CHKERRQ(ierr);
  if (xin != yin) {
    ierr = VecCUSPGetArrayReadWrite(xin,&xarray);CHKERRQ(ierr);
    ierr = VecCUSPGetArrayReadWrite(yin,&yarray);CHKERRQ(ierr);

#if defined(PETSC_USE_COMPLEX)
#if defined(PETSC_USE_REAL_SINGLE)
    cublasCswap(bn,(cuFloatComplex*)VecCUSPCastToRawPtr(*xarray),one,(cuFloatComplex*)VecCUSPCastToRawPtr(*yarray),one);
#else
    cublasZswap(bn,(cuDoubleComplex*)VecCUSPCastToRawPtr(*xarray),one,(cuDoubleComplex*)VecCUSPCastToRawPtr(*yarray),one);
#endif
#else
#if defined(PETSC_USE_REAL_SINGLE)
    cublasSswap(bn,VecCUSPCastToRawPtr(*xarray),one,VecCUSPCastToRawPtr(*yarray),one);
#else
    cublasDswap(bn,VecCUSPCastToRawPtr(*xarray),one,VecCUSPCastToRawPtr(*yarray),one);
#endif
#endif
    ierr = cublasGetError();CHKERRCUSP(ierr);
    ierr = WaitForGPU();CHKERRCUSP(ierr);
    ierr = VecCUSPRestoreArrayReadWrite(xin,&xarray);CHKERRQ(ierr);
    ierr = VecCUSPRestoreArrayReadWrite(yin,&yarray);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

struct VecCUSPAX
{
  template <typename Tuple>
  __host__ __device__
  void operator()(Tuple t)
  {
    thrust::get<0>(t) = thrust::get<1>(t)*thrust::get<2>(t);
  }
};
#undef __FUNCT__
#define __FUNCT__ "VecAXPBY_SeqCUSP"
PetscErrorCode VecAXPBY_SeqCUSP(Vec yin,PetscScalar alpha,PetscScalar beta,Vec xin)
{
  PetscErrorCode ierr;
  PetscScalar    a = alpha,b = beta;
  CUSPARRAY      *xarray,*yarray;

  PetscFunctionBegin;
  if (a == 0.0) {
    ierr = VecScale_SeqCUSP(yin,beta);CHKERRQ(ierr);
  } else if (b == 1.0) {
    ierr = VecAXPY_SeqCUSP(yin,alpha,xin);CHKERRQ(ierr);
  } else if (a == 1.0) {
    ierr = VecAYPX_SeqCUSP(yin,beta,xin);CHKERRQ(ierr);
  } else if (b == 0.0) {
    ierr = VecCUSPGetArrayRead(xin,&xarray);CHKERRQ(ierr);
    ierr = VecCUSPGetArrayReadWrite(yin,&yarray);CHKERRQ(ierr);
    try {
      thrust::for_each(
        thrust::make_zip_iterator(
          thrust::make_tuple(
            yarray->begin(),
            thrust::make_constant_iterator(a),
            xarray->begin())),
        thrust::make_zip_iterator(
          thrust::make_tuple(
            yarray->end(),
            thrust::make_constant_iterator(a),
            xarray->end())),
        VecCUSPAX());
    } catch(char *ex) {
      SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"CUSP error: %s", ex);
    }
    ierr = PetscLogFlops(xin->map->n);CHKERRQ(ierr);
    ierr = WaitForGPU();CHKERRCUSP(ierr);
    ierr = VecCUSPRestoreArrayRead(xin,&xarray);CHKERRQ(ierr);
    ierr = VecCUSPRestoreArrayReadWrite(yin,&yarray);CHKERRQ(ierr);
  } else {
    ierr = VecCUSPGetArrayRead(xin,&xarray);CHKERRQ(ierr);
    ierr = VecCUSPGetArrayReadWrite(yin,&yarray);CHKERRQ(ierr);
    try {
      cusp::blas::axpby(*xarray,*yarray,*yarray,a,b);
    } catch(char *ex) {
      SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"CUSP error: %s", ex);
    }
    ierr = VecCUSPRestoreArrayRead(xin,&xarray);CHKERRQ(ierr);
    ierr = VecCUSPRestoreArrayReadWrite(yin,&yarray);CHKERRQ(ierr);
    ierr = WaitForGPU();CHKERRCUSP(ierr);
    ierr = PetscLogFlops(3.0*xin->map->n);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

/* structs below are for special cases of VecAXPBYPCZ_SeqCUSP */
struct VecCUSPXPBYPCZ
{
  /* z = x + b*y + c*z */
  template <typename Tuple>
  __host__ __device__
  void operator()(Tuple t)
  {
    thrust::get<0>(t) = thrust::get<1>(t)*thrust::get<0>(t)+thrust::get<2>(t)+thrust::get<4>(t)*thrust::get<3>(t);
  }
};
struct VecCUSPAXPBYPZ
{
  /* z = ax + b*y + z */
  template <typename Tuple>
  __host__ __device__
  void operator()(Tuple t)
  {
    thrust::get<0>(t) += thrust::get<2>(t)*thrust::get<1>(t)+thrust::get<4>(t)*thrust::get<3>(t);
  }
};

#undef __FUNCT__
#define __FUNCT__ "VecAXPBYPCZ_SeqCUSP"
PetscErrorCode VecAXPBYPCZ_SeqCUSP(Vec zin,PetscScalar alpha,PetscScalar beta,PetscScalar gamma,Vec xin,Vec yin)
{
  PetscErrorCode ierr;
  PetscInt       n = zin->map->n;
  CUSPARRAY      *xarray,*yarray,*zarray;

  PetscFunctionBegin;
  ierr = VecCUSPGetArrayRead(xin,&xarray);CHKERRQ(ierr);
  ierr = VecCUSPGetArrayRead(yin,&yarray);CHKERRQ(ierr);
  ierr = VecCUSPGetArrayReadWrite(zin,&zarray);CHKERRQ(ierr);
  if (alpha == 1.0) {
    try {
      thrust::for_each(
        thrust::make_zip_iterator(
          thrust::make_tuple(
            zarray->begin(),
            thrust::make_constant_iterator(gamma),
            xarray->begin(),
            yarray->begin(),
            thrust::make_constant_iterator(beta))),
        thrust::make_zip_iterator(
          thrust::make_tuple(
            zarray->end(),
            thrust::make_constant_iterator(gamma),
            xarray->end(),
            yarray->end(),
            thrust::make_constant_iterator(beta))),
        VecCUSPXPBYPCZ());
    } catch(char *ex) {
      SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"CUSP error: %s", ex);
    }
    ierr = PetscLogFlops(4.0*n);CHKERRQ(ierr);
  } else if (gamma == 1.0) {
    try {
      thrust::for_each(
        thrust::make_zip_iterator(
          thrust::make_tuple(
            zarray->begin(),
            xarray->begin(),
            thrust::make_constant_iterator(alpha),
            yarray->begin(),
            thrust::make_constant_iterator(beta))),
        thrust::make_zip_iterator(
          thrust::make_tuple(
            zarray->end(),
            xarray->end(),
            thrust::make_constant_iterator(alpha),
            yarray->end(),
            thrust::make_constant_iterator(beta))),
        VecCUSPAXPBYPZ());
    } catch(char *ex) {
      SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"CUSP error: %s", ex);
    }
    ierr = PetscLogFlops(4.0*n);CHKERRQ(ierr);
  } else {
    try {
      cusp::blas::axpbypcz(*xarray,*yarray,*zarray,*zarray,alpha,beta,gamma);
    } catch(char *ex) {
      SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"CUSP error: %s", ex);
    }
    ierr = VecCUSPRestoreArrayReadWrite(zin,&zarray);CHKERRQ(ierr);
    ierr = VecCUSPRestoreArrayRead(xin,&xarray);CHKERRQ(ierr);
    ierr = VecCUSPRestoreArrayRead(yin,&yarray);CHKERRQ(ierr);
    ierr = PetscLogFlops(5.0*n);CHKERRQ(ierr);
  }
  ierr = WaitForGPU();CHKERRCUSP(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecPointwiseMult_SeqCUSP"
PetscErrorCode VecPointwiseMult_SeqCUSP(Vec win,Vec xin,Vec yin)
{
  PetscErrorCode ierr;
  PetscInt       n = win->map->n;
  CUSPARRAY      *xarray,*yarray,*warray;

  PetscFunctionBegin;
  ierr = VecCUSPGetArrayRead(xin,&xarray);CHKERRQ(ierr);
  ierr = VecCUSPGetArrayRead(yin,&yarray);CHKERRQ(ierr);
  ierr = VecCUSPGetArrayReadWrite(win,&warray);CHKERRQ(ierr);
  try {
    cusp::blas::xmy(*xarray,*yarray,*warray);
  } catch(char *ex) {
    SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"CUSP error: %s", ex);
  }
  ierr = VecCUSPRestoreArrayRead(xin,&xarray);CHKERRQ(ierr);
  ierr = VecCUSPRestoreArrayRead(yin,&yarray);CHKERRQ(ierr);
  ierr = VecCUSPRestoreArrayReadWrite(win,&warray);CHKERRQ(ierr);
  ierr = PetscLogFlops(n);CHKERRQ(ierr);
  ierr = WaitForGPU();CHKERRCUSP(ierr);
  PetscFunctionReturn(0);
}


/* should do infinity norm in cusp */

#undef __FUNCT__
#define __FUNCT__ "VecNorm_SeqCUSP"
PetscErrorCode VecNorm_SeqCUSP(Vec xin,NormType type,PetscReal *z)
{
  const PetscScalar *xx;
  PetscErrorCode    ierr;
  PetscInt          n = xin->map->n;
  PetscBLASInt      one = 1, bn;
  CUSPARRAY         *xarray;

  PetscFunctionBegin;
  ierr = PetscBLASIntCast(n,&bn);CHKERRQ(ierr);
  if (type == NORM_2 || type == NORM_FROBENIUS) {
    ierr = VecCUSPGetArrayRead(xin,&xarray);CHKERRQ(ierr);
    try {
      *z = cusp::blas::nrm2(*xarray);
    } catch(char *ex) {
      SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"CUSP error: %s", ex);
    }
    ierr = WaitForGPU();CHKERRCUSP(ierr);
    ierr = VecCUSPRestoreArrayRead(xin,&xarray);CHKERRQ(ierr);
    ierr = PetscLogFlops(PetscMax(2.0*n-1,0.0));CHKERRQ(ierr);
  } else if (type == NORM_INFINITY) {
    PetscInt  i;
    PetscReal max = 0.0,tmp;

    ierr = VecGetArrayRead(xin,&xx);CHKERRQ(ierr);
    for (i=0; i<n; i++) {
      if ((tmp = PetscAbsScalar(*xx)) > max) max = tmp;
      /* check special case of tmp == NaN */
      if (tmp != tmp) {max = tmp; break;}
      xx++;
    }
    ierr = VecRestoreArrayRead(xin,&xx);CHKERRQ(ierr);
    *z   = max;
  } else if (type == NORM_1) {
    ierr = VecCUSPGetArrayRead(xin,&xarray);CHKERRQ(ierr);
#if defined(PETSC_USE_COMPLEX)
#if defined(PETSC_USE_REAL_SINGLE)
    *z = cublasScasum(bn,(cuFloatComplex*)VecCUSPCastToRawPtr(*xarray),one);
#else
    *z = cublasDzasum(bn,(cuDoubleComplex*)VecCUSPCastToRawPtr(*xarray),one);
#endif
#else
#if defined(PETSC_USE_REAL_SINGLE)
    *z = cublasSasum(bn,VecCUSPCastToRawPtr(*xarray),one);
#else
    *z = cublasDasum(bn,VecCUSPCastToRawPtr(*xarray),one);
#endif
#endif
    ierr = cublasGetError();CHKERRCUSP(ierr);
    ierr = VecCUSPRestoreArrayRead(xin,&xarray);CHKERRQ(ierr);
    ierr = WaitForGPU();CHKERRCUSP(ierr);
    ierr = PetscLogFlops(PetscMax(n-1.0,0.0));CHKERRQ(ierr);
  } else if (type == NORM_1_AND_2) {
    ierr = VecNorm_SeqCUSP(xin,NORM_1,z);CHKERRQ(ierr);
    ierr = VecNorm_SeqCUSP(xin,NORM_2,z+1);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}


/*the following few functions should be modified to actually work with the GPU so they don't force unneccesary allocation of CPU memory */

#undef __FUNCT__
#define __FUNCT__ "VecSetRandom_SeqCUSP"
PetscErrorCode VecSetRandom_SeqCUSP(Vec xin,PetscRandom r)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = VecSetRandom_SeqCUSP_Private(xin,r);CHKERRQ(ierr);
  xin->valid_GPU_array = PETSC_CUSP_CPU;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecResetArray_SeqCUSP"
PetscErrorCode VecResetArray_SeqCUSP(Vec vin)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = VecCUSPCopyFromGPU(vin);CHKERRQ(ierr);
  ierr = VecResetArray_SeqCUSP_Private(vin);CHKERRQ(ierr);
  vin->valid_GPU_array = PETSC_CUSP_CPU;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecPlaceArray_SeqCUSP"
PetscErrorCode VecPlaceArray_SeqCUSP(Vec vin,const PetscScalar *a)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = VecCUSPCopyFromGPU(vin);CHKERRQ(ierr);
  ierr = VecPlaceArray_Seq(vin,a);CHKERRQ(ierr);
  vin->valid_GPU_array = PETSC_CUSP_CPU;
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "VecReplaceArray_SeqCUSP"
PetscErrorCode VecReplaceArray_SeqCUSP(Vec vin,const PetscScalar *a)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = VecCUSPCopyFromGPU(vin);CHKERRQ(ierr);
  ierr = VecReplaceArray_Seq(vin,a);CHKERRQ(ierr);
  vin->valid_GPU_array = PETSC_CUSP_CPU;
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "VecCreateSeqCUSP"
/*@
   VecCreateSeqCUSP - Creates a standard, sequential array-style vector.

   Collective on MPI_Comm

   Input Parameter:
+  comm - the communicator, should be PETSC_COMM_SELF
-  n - the vector length

   Output Parameter:
.  V - the vector

   Notes:
   Use VecDuplicate() or VecDuplicateVecs() to form additional vectors of the
   same type as an existing vector.

   Level: intermediate

   Concepts: vectors^creating sequential

.seealso: VecCreateMPI(), VecCreate(), VecDuplicate(), VecDuplicateVecs(), VecCreateGhost()
@*/
PetscErrorCode  VecCreateSeqCUSP(MPI_Comm comm,PetscInt n,Vec *v)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = VecCreate(comm,v);CHKERRQ(ierr);
  ierr = VecSetSizes(*v,n,n);CHKERRQ(ierr);
  ierr = VecSetType(*v,VECSEQCUSP);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

/*The following template functions are for VecDotNorm2_SeqCUSP.  Note that there is no complex support as currently written*/
template <typename T>
struct cuspdotnormcalculate : thrust::unary_function<T,T>
{
  __host__ __device__
  T operator()(T x)
  {
#if defined(PETSC_USE_COMPLEX)
    //return thrust::make_tuple(thrust::get<0>(x)*thrust::get<1>(x), thrust::get<1>(x)*thrust::get<1>(x));
#else
    return thrust::make_tuple(thrust::get<0>(x)*thrust::get<1>(x), thrust::get<1>(x)*thrust::get<1>(x));
#endif
  }
};

template <typename T>
struct cuspdotnormreduce : thrust::binary_function<T,T,T>
{
  __host__ __device__
  T operator()(T x,T y)
  {
    return thrust::make_tuple(thrust::get<0>(x)+thrust::get<0>(y), thrust::get<1>(x)+thrust::get<1>(y));
  }
};

#undef __FUNCT__
#define __FUNCT__ "VecDotNorm2_SeqCUSP"
PetscErrorCode VecDotNorm2_SeqCUSP(Vec s, Vec t, PetscScalar *dp, PetscScalar *nm)
{
  PetscErrorCode                         ierr;
  PetscScalar                            zero = 0.0;
  PetscReal                              n=s->map->n;
  thrust::tuple<PetscScalar,PetscScalar> result;
  CUSPARRAY                              *sarray,*tarray;

  PetscFunctionBegin;
  /*ierr = VecCUSPCopyToGPU(s);CHKERRQ(ierr);
   ierr = VecCUSPCopyToGPU(t);CHKERRQ(ierr);*/
  ierr = VecCUSPGetArrayRead(s,&sarray);CHKERRQ(ierr);
  ierr = VecCUSPGetArrayRead(t,&tarray);CHKERRQ(ierr);
  try {
#if defined(PETSC_USE_COMPLEX)
    ierr = VecDot_SeqCUSP(s,t,dp);CHKERRQ(ierr);
    ierr = VecDot_SeqCUSP(t,t,nm);CHKERRQ(ierr);
    //printf("VecDotNorm2_SeqCUSP=%1.5g,%1.5g\n",PetscRealPart(*dp),PetscImaginaryPart(*dp));
    //printf("VecDotNorm2_SeqCUSP=%1.5g,%1.5g\n",PetscRealPart(*nm),PetscImaginaryPart(*nm));
#else
    result = thrust::transform_reduce(
              thrust::make_zip_iterator(
                thrust::make_tuple(
                  sarray->begin(),
                  tarray->begin())),
              thrust::make_zip_iterator(
                thrust::make_tuple(
                  sarray->end(),
                  tarray->end())),
              cuspdotnormcalculate<thrust::tuple<PetscScalar,PetscScalar> >(),
              thrust::make_tuple(zero,zero),                                   /*init */
              cuspdotnormreduce<thrust::tuple<PetscScalar, PetscScalar> >());  /* binary function */
    *dp = thrust::get<0>(result);
    *nm = thrust::get<1>(result);
#endif
  } catch(char *ex) {
      SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"CUSP error: %s", ex);
  }
  ierr = VecCUSPRestoreArrayRead(s,&sarray);CHKERRQ(ierr);
  ierr = VecCUSPRestoreArrayRead(t,&tarray);CHKERRQ(ierr);
  ierr = WaitForGPU();CHKERRCUSP(ierr);
  ierr = PetscLogFlops(4.0*n);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecDuplicate_SeqCUSP"
PetscErrorCode VecDuplicate_SeqCUSP(Vec win,Vec *V)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = VecCreateSeqCUSP(PetscObjectComm((PetscObject)win),win->map->n,V);CHKERRQ(ierr);
  ierr = PetscLayoutReference(win->map,&(*V)->map);CHKERRQ(ierr);
  ierr = PetscObjectListDuplicate(((PetscObject)win)->olist,&((PetscObject)(*V))->olist);CHKERRQ(ierr);
  ierr = PetscFunctionListDuplicate(((PetscObject)win)->qlist,&((PetscObject)(*V))->qlist);CHKERRQ(ierr);
  (*V)->stash.ignorenegidx = win->stash.ignorenegidx;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecDestroy_SeqCUSP"
PetscErrorCode VecDestroy_SeqCUSP(Vec v)
{
  PetscErrorCode ierr;
  cudaError_t    err;
  PetscFunctionBegin;
  try {
    if (v->spptr) {
      delete ((Vec_CUSP*)v->spptr)->GPUarray;
      err = cudaStreamDestroy(((Vec_CUSP*)v->spptr)->stream);CHKERRCUSP(err);
      delete (Vec_CUSP*)v->spptr;
    }
  } catch(char *ex) {
    SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_LIB,"CUSP error: %s", ex);
  }
  ierr = VecDestroy_SeqCUSP_Private(v);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}


#if defined(PETSC_USE_COMPLEX)
struct conjugate 
{
  __host__ __device__
  PetscScalar operator()(PetscScalar x)
  {
    return cusp::conj(x);
  }
};
#endif


#undef __FUNCT__
#define __FUNCT__ "VecConjugate_SeqCUSP"
PetscErrorCode VecConjugate_SeqCUSP(Vec xin)
{
  PetscErrorCode ierr;
  CUSPARRAY      *xarray;

  PetscFunctionBegin;
  ierr = VecCUSPGetArrayReadWrite(xin,&xarray);CHKERRQ(ierr);
#if defined(PETSC_USE_COMPLEX)
  thrust::transform(xarray->begin(), xarray->end(), xarray->begin(), conjugate());
#endif
  ierr = VecCUSPRestoreArrayReadWrite(xin,&xarray);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecGetLocalVector_SeqCUSP"
PetscErrorCode VecGetLocalVector_SeqCUSP(Vec v,Vec w)
{
  VecType        t;
  PetscErrorCode ierr;
  cudaError_t    err;
  PetscBool      flg;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(v,VEC_CLASSID,1);
  PetscValidHeaderSpecific(w,VEC_CLASSID,2);
  ierr = VecGetType(w,&t);CHKERRQ(ierr);
  ierr = PetscStrcmp(t,VECSEQCUSP,&flg);CHKERRQ(ierr);
  if (!flg) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONG,"Vector of type %s passed to argument #2. Should be %s.\n",t,VECSEQCUSP);

  if (w->data) {
    if (((Vec_Seq*)w->data)->array_allocated) PetscFree(((Vec_Seq*)w->data)->array_allocated);
    ((Vec_Seq*)w->data)->array = 0;
    ((Vec_Seq*)w->data)->array_allocated = 0;
    ((Vec_Seq*)w->data)->unplacedarray = 0;
  }
  if (w->spptr) {
    if (((Vec_CUSP*)w->spptr)->GPUarray) delete ((Vec_CUSP*)w->spptr)->GPUarray;
    err = cudaStreamDestroy(((Vec_CUSP*)w->spptr)->stream);CHKERRCUSP(err);
    delete (Vec_CUSP*)w->spptr;
    w->spptr = 0;
  }

  if (v->petscnative) {
    ierr = PetscFree(w->data);CHKERRQ(ierr);
    w->data = v->data;
    w->valid_GPU_array = v->valid_GPU_array;
    w->spptr = v->spptr;
    ierr = PetscObjectStateIncrease((PetscObject)w);CHKERRQ(ierr);
  } else {
    ierr = VecGetArray(v,&((Vec_Seq*)w->data)->array);CHKERRQ(ierr);
    w->valid_GPU_array = PETSC_CUSP_CPU;
    ierr = VecCUSPAllocateCheck(w);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecRestoreLocalVector_SeqCUSP"
PetscErrorCode VecRestoreLocalVector_SeqCUSP(Vec v,Vec w)
{
  VecType        t;
  PetscErrorCode ierr;
  cudaError_t    err;
  PetscBool      flg;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(v,VEC_CLASSID,1);
  PetscValidHeaderSpecific(w,VEC_CLASSID,2);
  ierr = VecGetType(w,&t);CHKERRQ(ierr);
  ierr = PetscStrcmp(t,VECSEQCUSP,&flg);CHKERRQ(ierr);
  if (!flg) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONG,"Vector of type %s passed to argument #2. Should be %s.\n",t,VECSEQCUSP);

  if (v->petscnative) {
    v->data = w->data;
    v->valid_GPU_array = w->valid_GPU_array;
    v->spptr = w->spptr;
    ierr = VecCUSPCopyFromGPU(v);CHKERRQ(ierr);
    ierr = PetscObjectStateIncrease((PetscObject)v);CHKERRQ(ierr);
    w->data = 0;
    w->valid_GPU_array = PETSC_CUSP_UNALLOCATED;
    w->spptr = 0;
  } else {
    ierr = VecRestoreArray(v,&((Vec_Seq*)w->data)->array);CHKERRQ(ierr);
    if ((Vec_CUSP*)w->spptr) {
      delete ((Vec_CUSP*)w->spptr)->GPUarray;
      err = cudaStreamDestroy(((Vec_CUSP*)w->spptr)->stream);CHKERRCUSP(err);
      delete (Vec_CUSP*)w->spptr;
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecCreate_SeqCUSP"
PETSC_EXTERN PetscErrorCode VecCreate_SeqCUSP(Vec V)
{
  PetscErrorCode ierr;
  PetscMPIInt    size;

  PetscFunctionBegin;
  ierr = MPI_Comm_size(PetscObjectComm((PetscObject)V),&size);CHKERRQ(ierr);
  if (size > 1) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONG,"Cannot create VECSEQCUSP on more than one process");
  ierr = VecCreate_Seq_Private(V,0);CHKERRQ(ierr);
  ierr = PetscObjectChangeTypeName((PetscObject)V,VECSEQCUSP);CHKERRQ(ierr);

  V->ops->dot                    = VecDot_SeqCUSP;
  V->ops->norm                   = VecNorm_SeqCUSP;
  V->ops->tdot                   = VecTDot_SeqCUSP;
  V->ops->scale                  = VecScale_SeqCUSP;
  V->ops->copy                   = VecCopy_SeqCUSP;
  V->ops->set                    = VecSet_SeqCUSP;
  V->ops->swap                   = VecSwap_SeqCUSP;
  V->ops->axpy                   = VecAXPY_SeqCUSP;
  V->ops->axpby                  = VecAXPBY_SeqCUSP;
  V->ops->axpbypcz               = VecAXPBYPCZ_SeqCUSP;
  V->ops->pointwisemult          = VecPointwiseMult_SeqCUSP;
  V->ops->pointwisedivide        = VecPointwiseDivide_SeqCUSP;
  V->ops->setrandom              = VecSetRandom_SeqCUSP;
  V->ops->dot_local              = VecDot_SeqCUSP;
  V->ops->tdot_local             = VecTDot_SeqCUSP;
  V->ops->norm_local             = VecNorm_SeqCUSP;
  V->ops->mdot_local             = VecMDot_SeqCUSP;
  V->ops->maxpy                  = VecMAXPY_SeqCUSP;
  V->ops->mdot                   = VecMDot_SeqCUSP;
  V->ops->aypx                   = VecAYPX_SeqCUSP;
  V->ops->waxpy                  = VecWAXPY_SeqCUSP;
  V->ops->dotnorm2               = VecDotNorm2_SeqCUSP;
  V->ops->placearray             = VecPlaceArray_SeqCUSP;
  V->ops->replacearray           = VecReplaceArray_SeqCUSP;
  V->ops->resetarray             = VecResetArray_SeqCUSP;
  V->ops->destroy                = VecDestroy_SeqCUSP;
  V->ops->duplicate              = VecDuplicate_SeqCUSP;
  V->ops->conjugate              = VecConjugate_SeqCUSP;
  V->ops->getlocalvector         = VecGetLocalVector_SeqCUSP;
  V->ops->restorelocalvector     = VecRestoreLocalVector_SeqCUSP;
  V->ops->getlocalvectorread     = VecGetLocalVector_SeqCUSP;
  V->ops->restorelocalvectorread = VecRestoreLocalVector_SeqCUSP;

  ierr = VecCUSPAllocateCheck(V);CHKERRQ(ierr);
  V->valid_GPU_array      = PETSC_CUSP_GPU;
  ierr = VecSet(V,0.0);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecCUSPGetArrayReadWrite"
PETSC_EXTERN PetscErrorCode VecCUSPGetArrayReadWrite(Vec v, CUSPARRAY **a)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  *a   = 0;
  ierr = VecCUSPCopyToGPU(v);CHKERRQ(ierr);
  *a   = ((Vec_CUSP*)v->spptr)->GPUarray;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecCUSPRestoreArrayReadWrite"
PETSC_EXTERN PetscErrorCode VecCUSPRestoreArrayReadWrite(Vec v, CUSPARRAY **a)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  v->valid_GPU_array = PETSC_CUSP_GPU;

  ierr = PetscObjectStateIncrease((PetscObject)v);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecCUSPGetArrayRead"
PETSC_EXTERN PetscErrorCode VecCUSPGetArrayRead(Vec v, CUSPARRAY **a)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  *a   = 0;
  ierr = VecCUSPCopyToGPU(v);CHKERRQ(ierr);
  *a   = ((Vec_CUSP*)v->spptr)->GPUarray;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecCUSPRestoreArrayRead"
PETSC_EXTERN PetscErrorCode VecCUSPRestoreArrayRead(Vec v, CUSPARRAY **a)
{
  PetscFunctionBegin;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecCUSPGetArrayWrite"
PETSC_EXTERN PetscErrorCode VecCUSPGetArrayWrite(Vec v, CUSPARRAY **a)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  *a   = 0;
  ierr = VecCUSPAllocateCheck(v);CHKERRQ(ierr);
  *a   = ((Vec_CUSP*)v->spptr)->GPUarray;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecCUSPRestoreArrayWrite"
PETSC_EXTERN PetscErrorCode VecCUSPRestoreArrayWrite(Vec v, CUSPARRAY **a)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  v->valid_GPU_array = PETSC_CUSP_GPU;

  ierr = PetscObjectStateIncrease((PetscObject)v);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "VecCUSPGetCUDAArray"
/*MC
   VecCUSPGetCUDAArray - Provides write access to the CUDA buffer inside a vector.

   Input Parameter:
-  v - the vector

   Output Parameter:
.  a - the CUDA pointer

   Level: intermediate

.seealso: VecCUSPGetArrayRead(), VecCUSPGetArrayWrite()
M*/
PETSC_EXTERN PetscErrorCode VecCUSPGetCUDAArray(Vec v, PetscScalar **a)
{
  PetscErrorCode ierr;
  CUSPARRAY      *cusparray;

  PetscFunctionBegin;
  PetscValidPointer(a,1);
  ierr = VecCUSPAllocateCheck(v);CHKERRQ(ierr);
  ierr = VecCUSPGetArrayWrite(v, &cusparray);CHKERRQ(ierr);
  *a   = thrust::raw_pointer_cast(cusparray->data());
  PetscFunctionReturn(0);
}



#undef __FUNCT__
#define __FUNCT__ "VecCUSPRestoreCUDAArray"
PETSC_EXTERN PetscErrorCode VecCUSPRestoreCUDAArray(Vec v, PetscScalar **a)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  /* Note: cannot call VecCUSPRestoreArrayWrite() here because the CUSP vector is not available. */
  v->valid_GPU_array = PETSC_CUSP_GPU;
  ierr = PetscObjectStateIncrease((PetscObject)v);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
