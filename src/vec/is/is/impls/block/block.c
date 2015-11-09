
/*
     Provides the functions for index sets (IS) defined by a list of integers.
   These are for blocks of data, each block is indicated with a single integer.
*/
#include <petsc/private/isimpl.h>               /*I  "petscis.h"     I*/
#include <petscvec.h>
#include <petscviewer.h>

typedef struct {
  PetscBool sorted;             /* are the blocks sorted? */
  PetscBool borrowed_indices;   /* do not free indices when IS is destroyed */
  PetscInt  *idx;
} IS_Block;

#undef __FUNCT__
#define __FUNCT__ "ISDestroy_Block"
PetscErrorCode ISDestroy_Block(IS is)
{
  IS_Block       *is_block = (IS_Block*)is->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (!is_block->borrowed_indices) {
    ierr = PetscFree(is_block->idx);CHKERRQ(ierr);
  }
  ierr = PetscObjectComposeFunction((PetscObject)is,"ISBlockSetIndices_C",0);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)is,"ISBlockGetIndices_C",0);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)is,"ISBlockRestoreIndices_C",0);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)is,"ISBlockGetSize_C",0);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)is,"ISBlockGetLocalSize_C",0);CHKERRQ(ierr);
  ierr = PetscFree(is->data);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "ISGetIndices_Block"
PetscErrorCode ISGetIndices_Block(IS in,const PetscInt *idx[])
{
  IS_Block       *sub = (IS_Block*)in->data;
  PetscErrorCode ierr;
  PetscInt       i,j,k,bs,n,*ii,*jj;

  PetscFunctionBegin;
  ierr = PetscLayoutGetBlockSize(in->map, &bs);CHKERRQ(ierr);
  ierr = PetscLayoutGetLocalSize(in->map, &n);CHKERRQ(ierr);
  n   /= bs;
  if (bs == 1) *idx = sub->idx;
  else {
    ierr = PetscMalloc1(bs*n,&jj);CHKERRQ(ierr);
    *idx = jj;
    k    = 0;
    ii   = sub->idx;
    for (i=0; i<n; i++)
      for (j=0; j<bs; j++)
        jj[k++] = bs*ii[i] + j;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "ISRestoreIndices_Block"
PetscErrorCode ISRestoreIndices_Block(IS is,const PetscInt *idx[])
{
  IS_Block       *sub = (IS_Block*)is->data;
  PetscInt       bs;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscLayoutGetBlockSize(is->map, &bs);CHKERRQ(ierr);
  if (bs != 1) {
    ierr = PetscFree(*(void**)idx);CHKERRQ(ierr);
  } else {
    if (*idx != sub->idx) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONG,"Must restore with value from ISGetIndices()");
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "ISGetSize_Block"
PetscErrorCode ISGetSize_Block(IS is,PetscInt *size)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscLayoutGetSize(is->map, size);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "ISGetLocalSize_Block"
PetscErrorCode ISGetLocalSize_Block(IS is,PetscInt *size)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscLayoutGetLocalSize(is->map, size);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "ISInvertPermutation_Block"
PetscErrorCode ISInvertPermutation_Block(IS is,PetscInt nlocal,IS *isout)
{
  IS_Block       *sub = (IS_Block*)is->data;
  PetscInt       i,*ii,bs,n,*idx = sub->idx;
  PetscMPIInt    size;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MPI_Comm_size(PetscObjectComm((PetscObject)is),&size);CHKERRQ(ierr);
  ierr = PetscLayoutGetBlockSize(is->map, &bs);CHKERRQ(ierr);
  ierr = PetscLayoutGetLocalSize(is->map, &n);CHKERRQ(ierr);
  n   /= bs;
  if (size == 1) {
    ierr = PetscMalloc1(n,&ii);CHKERRQ(ierr);
    for (i=0; i<n; i++) ii[idx[i]] = i;
    ierr = ISCreateBlock(PETSC_COMM_SELF,bs,n,ii,PETSC_OWN_POINTER,isout);CHKERRQ(ierr);
    ierr = ISSetPermutation(*isout);CHKERRQ(ierr);
  } else SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SUP,"No inversion written yet for block IS");
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "ISView_Block"
PetscErrorCode ISView_Block(IS is, PetscViewer viewer)
{
  IS_Block       *sub = (IS_Block*)is->data;
  PetscErrorCode ierr;
  PetscInt       i,bs,n,*idx = sub->idx;
  PetscBool      iascii;

  PetscFunctionBegin;
  ierr = PetscLayoutGetBlockSize(is->map, &bs);CHKERRQ(ierr);
  ierr = PetscLayoutGetLocalSize(is->map, &n);CHKERRQ(ierr);
  n   /= bs;
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERASCII,&iascii);CHKERRQ(ierr);
  if (iascii) {
    ierr = PetscViewerASCIIPushSynchronized(viewer);CHKERRQ(ierr);
    if (is->isperm) {
      ierr = PetscViewerASCIISynchronizedPrintf(viewer,"Block Index set is permutation\n");CHKERRQ(ierr);
    }
    ierr = PetscViewerASCIISynchronizedPrintf(viewer,"Block size %D\n",bs);CHKERRQ(ierr);
    ierr = PetscViewerASCIISynchronizedPrintf(viewer,"Number of block indices in set %D\n",n);CHKERRQ(ierr);
    ierr = PetscViewerASCIISynchronizedPrintf(viewer,"The first indices of each block are\n");CHKERRQ(ierr);
    for (i=0; i<n; i++) {
      ierr = PetscViewerASCIISynchronizedPrintf(viewer,"Block %D Index %D\n",i,idx[i]);CHKERRQ(ierr);
    }
    ierr = PetscViewerFlush(viewer);CHKERRQ(ierr);
    ierr = PetscViewerASCIIPopSynchronized(viewer);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "ISSort_Block"
PetscErrorCode ISSort_Block(IS is)
{
  IS_Block       *sub = (IS_Block*)is->data;
  PetscInt       bs, n;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (sub->sorted) PetscFunctionReturn(0);
  ierr = PetscLayoutGetBlockSize(is->map, &bs);CHKERRQ(ierr);
  ierr = PetscLayoutGetLocalSize(is->map, &n);CHKERRQ(ierr);
  ierr = PetscSortInt(n/bs,sub->idx);CHKERRQ(ierr);
  sub->sorted = PETSC_TRUE;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "ISSortRemoveDups_Block"
PetscErrorCode ISSortRemoveDups_Block(IS is)
{
  IS_Block       *sub = (IS_Block*)is->data;
  PetscInt       bs, n, nb;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (sub->sorted) PetscFunctionReturn(0);
  ierr = PetscLayoutGetBlockSize(is->map, &bs);CHKERRQ(ierr);
  ierr = PetscLayoutGetLocalSize(is->map, &n);CHKERRQ(ierr);
  nb   = n/bs;
  ierr = PetscSortRemoveDupsInt(&nb,sub->idx);CHKERRQ(ierr);
  ierr = PetscLayoutSetLocalSize(is->map, nb*bs);CHKERRQ(ierr);
  sub->sorted = PETSC_TRUE;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "ISSorted_Block"
PetscErrorCode ISSorted_Block(IS is,PetscBool  *flg)
{
  IS_Block *sub = (IS_Block*)is->data;

  PetscFunctionBegin;
  *flg = sub->sorted;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "ISDuplicate_Block"
PetscErrorCode ISDuplicate_Block(IS is,IS *newIS)
{
  PetscErrorCode ierr;
  IS_Block       *sub = (IS_Block*)is->data;
  PetscInt        bs, n;

  PetscFunctionBegin;
  ierr = PetscLayoutGetBlockSize(is->map, &bs);CHKERRQ(ierr);
  ierr = PetscLayoutGetLocalSize(is->map, &n);CHKERRQ(ierr);
  n   /= bs;
  ierr = ISCreateBlock(PetscObjectComm((PetscObject)is),bs,n,sub->idx,PETSC_COPY_VALUES,newIS);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "ISIdentity_Block"
PetscErrorCode ISIdentity_Block(IS is,PetscBool  *ident)
{
  IS_Block      *is_block = (IS_Block*)is->data;
  PetscInt       i,bs,n,*idx = is_block->idx;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscLayoutGetBlockSize(is->map, &bs);CHKERRQ(ierr);
  ierr = PetscLayoutGetLocalSize(is->map, &n);CHKERRQ(ierr);
  n   /= bs;
  is->isidentity = PETSC_TRUE;
  *ident         = PETSC_TRUE;
  for (i=0; i<n; i++) {
    if (idx[i] != i) {
      is->isidentity = PETSC_FALSE;
      *ident         = PETSC_FALSE;
      PetscFunctionReturn(0);
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "ISCopy_Block"
static PetscErrorCode ISCopy_Block(IS is,IS isy)
{
  IS_Block       *is_block = (IS_Block*)is->data,*isy_block = (IS_Block*)isy->data;
  PetscInt       bs, n, N, bsy, ny, Ny;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscLayoutGetBlockSize(is->map, &bs);CHKERRQ(ierr);
  ierr = PetscLayoutGetLocalSize(is->map, &n);CHKERRQ(ierr);
  ierr = PetscLayoutGetSize(is->map, &N);CHKERRQ(ierr);
  ierr = PetscLayoutGetBlockSize(isy->map, &bsy);CHKERRQ(ierr);
  ierr = PetscLayoutGetLocalSize(isy->map, &ny);CHKERRQ(ierr);
  ierr = PetscLayoutGetSize(isy->map, &Ny);CHKERRQ(ierr);
  if (n != ny || N != Ny || bs != bsy) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_INCOMP,"Index sets incompatible");
  isy_block->sorted = is_block->sorted;
  ierr = PetscMemcpy(isy_block->idx,is_block->idx,(n/bs)*sizeof(PetscInt));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "ISOnComm_Block"
static PetscErrorCode ISOnComm_Block(IS is,MPI_Comm comm,PetscCopyMode mode,IS *newis)
{
  PetscErrorCode ierr;
  IS_Block       *sub = (IS_Block*)is->data;
  PetscInt       bs, n;

  PetscFunctionBegin;
  if (mode == PETSC_OWN_POINTER) SETERRQ(comm,PETSC_ERR_ARG_WRONG,"Cannot use PETSC_OWN_POINTER");
  ierr = PetscLayoutGetBlockSize(is->map, &bs);CHKERRQ(ierr);
  ierr = PetscLayoutGetLocalSize(is->map, &n);CHKERRQ(ierr);
  ierr = ISCreateBlock(comm,bs,n/bs,sub->idx,mode,newis);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "ISSetBlockSize_Block"
static PetscErrorCode ISSetBlockSize_Block(IS is,PetscInt bs)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscLayoutSetBlockSize(is->map, bs);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "ISToGeneral_Block"
static PetscErrorCode ISToGeneral_Block(IS inis)
{
  IS_Block       *sub   = (IS_Block*)inis->data;
  PetscInt       bs,n;
  const PetscInt *idx;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = ISGetBlockSize(inis,&bs);CHKERRQ(ierr);
  ierr = ISGetLocalSize(inis,&n);CHKERRQ(ierr);
  ierr = ISGetIndices(inis,&idx);CHKERRQ(ierr);
  if (bs == 1) {
    PetscCopyMode mode = sub->borrowed_indices ? PETSC_USE_POINTER : PETSC_OWN_POINTER;
    sub->borrowed_indices = PETSC_TRUE; /* prevent deallocation when changing the subtype*/
    ierr = ISSetType(inis,ISGENERAL);CHKERRQ(ierr);
    ierr = ISGeneralSetIndices(inis,n,idx,mode);CHKERRQ(ierr);
  } else {
    ierr = ISSetType(inis,ISGENERAL);CHKERRQ(ierr);
    ierr = ISGeneralSetIndices(inis,n,idx,PETSC_OWN_POINTER);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}


static struct _ISOps myops = { ISGetSize_Block,
                               ISGetLocalSize_Block,
                               ISGetIndices_Block,
                               ISRestoreIndices_Block,
                               ISInvertPermutation_Block,
                               ISSort_Block,
                               ISSortRemoveDups_Block,
                               ISSorted_Block,
                               ISDuplicate_Block,
                               ISDestroy_Block,
                               ISView_Block,
                               ISLoad_Default,
                               ISIdentity_Block,
                               ISCopy_Block,
                               ISToGeneral_Block,
                               ISOnComm_Block,
                               ISSetBlockSize_Block,
                               0};

#undef __FUNCT__
#define __FUNCT__ "ISBlockSetIndices"
/*@
   ISBlockSetIndices - The indices are relative to entries, not blocks.

   Collective on IS

   Input Parameters:
+  is - the index set
.  bs - number of elements in each block, one for each block and count of block not indices
.   n - the length of the index set (the number of blocks)
.  idx - the list of integers, these are by block, not by location
+  mode - see PetscCopyMode, only PETSC_COPY_VALUES and PETSC_OWN_POINTER are supported


   Notes:
   When the communicator is not MPI_COMM_SELF, the operations on the
   index sets, IS, are NOT conceptually the same as MPI_Group operations.
   The index sets are then distributed sets of indices and thus certain operations
   on them are collective.

   Example:
   If you wish to index the values {0,1,4,5}, then use
   a block size of 2 and idx of {0,2}.

   Level: beginner

  Concepts: IS^block
  Concepts: index sets^block
  Concepts: block^index set

.seealso: ISCreateStride(), ISCreateGeneral(), ISAllGather()
@*/
PetscErrorCode  ISBlockSetIndices(IS is,PetscInt bs,PetscInt n,const PetscInt idx[],PetscCopyMode mode)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscUseMethod(is,"ISBlockSetIndices_C",(IS,PetscInt,PetscInt,const PetscInt[],PetscCopyMode),(is,bs,n,idx,mode));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "ISBlockSetIndices_Block"
PetscErrorCode  ISBlockSetIndices_Block(IS is,PetscInt bs,PetscInt n,const PetscInt idx[],PetscCopyMode mode)
{
  PetscErrorCode ierr;
  PetscInt       i,min,max;
  IS_Block       *sub   = (IS_Block*)is->data;
  PetscBool      sorted = PETSC_TRUE;

  PetscFunctionBegin;
  if (!sub->borrowed_indices) {
    ierr = PetscFree(sub->idx);CHKERRQ(ierr);
  } else {
    sub->borrowed_indices = PETSC_FALSE;
  }
  ierr = PetscLayoutSetLocalSize(is->map, n*bs);CHKERRQ(ierr);
  ierr = PetscLayoutSetBlockSize(is->map, bs);CHKERRQ(ierr);
  ierr = PetscLayoutSetUp(is->map);CHKERRQ(ierr);
  for (i=1; i<n; i++) {
    if (idx[i] < idx[i-1]) {sorted = PETSC_FALSE; break;}
  }
  if (n) min = max = idx[0];
  else   min = max = 0;
  for (i=1; i<n; i++) {
    if (idx[i] < min) min = idx[i];
    if (idx[i] > max) max = idx[i];
  }
  if (mode == PETSC_COPY_VALUES) {
    ierr = PetscMalloc1(n,&sub->idx);CHKERRQ(ierr);
    ierr = PetscLogObjectMemory((PetscObject)is,n*sizeof(PetscInt));CHKERRQ(ierr);
    ierr = PetscMemcpy(sub->idx,idx,n*sizeof(PetscInt));CHKERRQ(ierr);
  } else if (mode == PETSC_OWN_POINTER) {
    sub->idx = (PetscInt*) idx;
    ierr = PetscLogObjectMemory((PetscObject)is,n*sizeof(PetscInt));CHKERRQ(ierr);
  } else if (mode == PETSC_USE_POINTER) {
    sub->idx = (PetscInt*) idx;
    sub->borrowed_indices = PETSC_TRUE;
  }

  sub->sorted = sorted;
  is->min     = bs*min;
  is->max     = bs*max+bs-1;
  is->data    = (void*)sub;
  ierr = PetscMemcpy(is->ops,&myops,sizeof(myops));CHKERRQ(ierr);
  is->isperm  = PETSC_FALSE;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "ISCreateBlock"
/*@
   ISCreateBlock - Creates a data structure for an index set containing
   a list of integers. The indices are relative to entries, not blocks.

   Collective on MPI_Comm

   Input Parameters:
+  comm - the MPI communicator
.  bs - number of elements in each block
.  n - the length of the index set (the number of blocks)
.  idx - the list of integers, one for each block and count of block not indices
-  mode - see PetscCopyMode, only PETSC_COPY_VALUES and PETSC_OWN_POINTER are supported in this routine

   Output Parameter:
.  is - the new index set

   Notes:
   When the communicator is not MPI_COMM_SELF, the operations on the
   index sets, IS, are NOT conceptually the same as MPI_Group operations.
   The index sets are then distributed sets of indices and thus certain operations
   on them are collective.

   Example:
   If you wish to index the values {0,1,6,7}, then use
   a block size of 2 and idx of {0,3}.

   Level: beginner

  Concepts: IS^block
  Concepts: index sets^block
  Concepts: block^index set

.seealso: ISCreateStride(), ISCreateGeneral(), ISAllGather()
@*/
PetscErrorCode  ISCreateBlock(MPI_Comm comm,PetscInt bs,PetscInt n,const PetscInt idx[],PetscCopyMode mode,IS *is)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidPointer(is,5);
  if (n < 0) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"length < 0");
  if (n) PetscValidIntPointer(idx,4);

  ierr = ISCreate(comm,is);CHKERRQ(ierr);
  ierr = ISSetType(*is,ISBLOCK);CHKERRQ(ierr);
  ierr = ISBlockSetIndices(*is,bs,n,idx,mode);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "ISBlockGetIndices_Block"
PetscErrorCode  ISBlockGetIndices_Block(IS is,const PetscInt *idx[])
{
  IS_Block *sub = (IS_Block*)is->data;

  PetscFunctionBegin;
  *idx = sub->idx;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "ISBlockRestoreIndices_Block"
PetscErrorCode  ISBlockRestoreIndices_Block(IS is,const PetscInt *idx[])
{
  PetscFunctionBegin;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "ISBlockGetIndices"
/*@C
   ISBlockGetIndices - Gets the indices associated with each block.

   Not Collective

   Input Parameter:
.  is - the index set

   Output Parameter:
.  idx - the integer indices, one for each block and count of block not indices

   Level: intermediate

   Concepts: IS^block
   Concepts: index sets^getting indices
   Concepts: index sets^block

.seealso: ISGetIndices(), ISBlockRestoreIndices()
@*/
PetscErrorCode  ISBlockGetIndices(IS is,const PetscInt *idx[])
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscUseMethod(is,"ISBlockGetIndices_C",(IS,const PetscInt*[]),(is,idx));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "ISBlockRestoreIndices"
/*@C
   ISBlockRestoreIndices - Restores the indices associated with each block.

   Not Collective

   Input Parameter:
.  is - the index set

   Output Parameter:
.  idx - the integer indices

   Level: intermediate

   Concepts: IS^block
   Concepts: index sets^getting indices
   Concepts: index sets^block

.seealso: ISRestoreIndices(), ISBlockGetIndices()
@*/
PetscErrorCode  ISBlockRestoreIndices(IS is,const PetscInt *idx[])
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscUseMethod(is,"ISBlockRestoreIndices_C",(IS,const PetscInt*[]),(is,idx));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "ISBlockGetLocalSize"
/*@
   ISBlockGetLocalSize - Returns the local number of blocks in the index set.

   Not Collective

   Input Parameter:
.  is - the index set

   Output Parameter:
.  size - the local number of blocks

   Level: intermediate

   Concepts: IS^block sizes
   Concepts: index sets^block sizes

.seealso: ISGetBlockSize(), ISBlockGetSize(), ISGetSize(), ISCreateBlock()
@*/
PetscErrorCode  ISBlockGetLocalSize(IS is,PetscInt *size)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscUseMethod(is,"ISBlockGetLocalSize_C",(IS,PetscInt*),(is,size));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "ISBlockGetLocalSize_Block"
PetscErrorCode  ISBlockGetLocalSize_Block(IS is,PetscInt *size)
{
  PetscInt       bs, n;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscLayoutGetBlockSize(is->map, &bs);CHKERRQ(ierr);
  ierr = PetscLayoutGetLocalSize(is->map, &n);CHKERRQ(ierr);
  *size = n/bs;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "ISBlockGetSize"
/*@
   ISBlockGetSize - Returns the global number of blocks in the index set.

   Not Collective

   Input Parameter:
.  is - the index set

   Output Parameter:
.  size - the global number of blocks

   Level: intermediate

   Concepts: IS^block sizes
   Concepts: index sets^block sizes

.seealso: ISGetBlockSize(), ISBlockGetLocalSize(), ISGetSize(), ISCreateBlock()
@*/
PetscErrorCode  ISBlockGetSize(IS is,PetscInt *size)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscUseMethod(is,"ISBlockGetSize_C",(IS,PetscInt*),(is,size));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "ISBlockGetSize_Block"
PetscErrorCode  ISBlockGetSize_Block(IS is,PetscInt *size)
{
  PetscInt       bs, N;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscLayoutGetBlockSize(is->map, &bs);CHKERRQ(ierr);
  ierr = PetscLayoutGetSize(is->map, &N);CHKERRQ(ierr);
  *size = N/bs;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "ISCreate_Block"
PETSC_EXTERN PetscErrorCode ISCreate_Block(IS is)
{
  PetscErrorCode ierr;
  IS_Block       *sub;

  PetscFunctionBegin;
  ierr = PetscNewLog(is,&sub);CHKERRQ(ierr);
  is->data = sub;
  ierr = PetscObjectComposeFunction((PetscObject)is,"ISBlockSetIndices_C",ISBlockSetIndices_Block);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)is,"ISBlockGetIndices_C",ISBlockGetIndices_Block);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)is,"ISBlockRestoreIndices_C",ISBlockRestoreIndices_Block);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)is,"ISBlockGetSize_C",ISBlockGetSize_Block);CHKERRQ(ierr);
  ierr = PetscObjectComposeFunction((PetscObject)is,"ISBlockGetLocalSize_C",ISBlockGetLocalSize_Block);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
