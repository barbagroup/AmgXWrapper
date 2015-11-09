
/*
     Provides the functions for index sets (IS) defined by a list of integers.
*/
#include <../src/vec/is/is/impls/general/general.h> /*I  "petscis.h"  I*/
#include <petscvec.h>
#include <petscviewer.h>
#include <petscviewerhdf5.h>

#undef __FUNCT__
#define __FUNCT__ "ISDuplicate_General"
PetscErrorCode ISDuplicate_General(IS is,IS *newIS)
{
  PetscErrorCode ierr;
  IS_General     *sub = (IS_General*)is->data;
  PetscInt       n;

  PetscFunctionBegin;
  ierr = PetscLayoutGetLocalSize(is->map, &n);CHKERRQ(ierr);
  ierr = ISCreateGeneral(PetscObjectComm((PetscObject) is), n, sub->idx, PETSC_COPY_VALUES, newIS);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "ISDestroy_General"
PetscErrorCode ISDestroy_General(IS is)
{
  IS_General     *is_general = (IS_General*)is->data;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (is_general->allocated) {ierr = PetscFree(is_general->idx);CHKERRQ(ierr);}
  ierr = PetscObjectComposeFunction((PetscObject)is,"ISGeneralSetIndices_C",0);CHKERRQ(ierr);
  ierr = PetscFree(is->data);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "ISIdentity_General"
PetscErrorCode ISIdentity_General(IS is, PetscBool *ident)
{
  IS_General *is_general = (IS_General*)is->data;
  PetscInt   i,n,*idx = is_general->idx;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscLayoutGetLocalSize(is->map, &n);CHKERRQ(ierr);
  is->isidentity = PETSC_TRUE;
  *ident         = PETSC_TRUE;
  for (i=0; i<n; i++) {
    if (idx[i] != i) {
      is->isidentity = PETSC_FALSE;
      *ident         = PETSC_FALSE;
      break;
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "ISCopy_General"
static PetscErrorCode ISCopy_General(IS is,IS isy)
{
  IS_General     *is_general = (IS_General*)is->data,*isy_general = (IS_General*)isy->data;
  PetscInt       n, N, ny, Ny;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscLayoutGetLocalSize(is->map, &n);CHKERRQ(ierr);
  ierr = PetscLayoutGetSize(is->map, &N);CHKERRQ(ierr);
  ierr = PetscLayoutGetLocalSize(isy->map, &ny);CHKERRQ(ierr);
  ierr = PetscLayoutGetSize(isy->map, &Ny);CHKERRQ(ierr);
  if (n != ny || N != Ny) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_INCOMP,"Index sets incompatible");
  isy_general->sorted = is_general->sorted;
  ierr = PetscMemcpy(isy_general->idx,is_general->idx,n*sizeof(PetscInt));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "ISOnComm_General"
PetscErrorCode ISOnComm_General(IS is,MPI_Comm comm,PetscCopyMode mode,IS *newis)
{
  PetscErrorCode ierr;
  IS_General     *sub = (IS_General*)is->data;
  PetscInt       n;

  PetscFunctionBegin;
  if (mode == PETSC_OWN_POINTER) SETERRQ(comm,PETSC_ERR_ARG_WRONG,"Cannot use PETSC_OWN_POINTER");
  ierr = PetscLayoutGetLocalSize(is->map, &n);CHKERRQ(ierr);
  ierr = ISCreateGeneral(comm,n,sub->idx,mode,newis);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "ISSetBlockSize_General"
static PetscErrorCode ISSetBlockSize_General(IS is,PetscInt bs)
{
#if defined(PETSC_USE_DEBUG)
  IS_General    *sub = (IS_General*)is->data;
  PetscInt       n;
#endif
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscLayoutSetBlockSize(is->map, bs);CHKERRQ(ierr);
#if defined(PETSC_USE_DEBUG)
  ierr = PetscLayoutGetLocalSize(is->map, &n);CHKERRQ(ierr);
  {
    PetscInt i,j;
    for (i=0; i<n; i+=bs) {
      for (j=0; j<bs; j++) {
        if (sub->idx[i+j] != sub->idx[i]+j) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_ARG_SIZ,"Index set does not have block structure, cannot set block size to %D",bs);
      }
    }
  }
#endif
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "ISContiguousLocal_General"
static PetscErrorCode ISContiguousLocal_General(IS is,PetscInt gstart,PetscInt gend,PetscInt *start,PetscBool *contig)
{
  IS_General *sub = (IS_General*)is->data;
  PetscInt   n,i,p;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  *start  = 0;
  *contig = PETSC_TRUE;
  ierr = PetscLayoutGetLocalSize(is->map, &n);CHKERRQ(ierr);
  if (!n) PetscFunctionReturn(0);
  p = sub->idx[0];
  if (p < gstart) goto nomatch;
  *start = p - gstart;
  if (n > gend-p) goto nomatch;
  for (i=1; i<n; i++,p++) {
    if (sub->idx[i] != p+1) goto nomatch;
  }
  PetscFunctionReturn(0);
nomatch:
  *start  = -1;
  *contig = PETSC_FALSE;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "ISGetIndices_General"
PetscErrorCode ISGetIndices_General(IS in,const PetscInt *idx[])
{
  IS_General *sub = (IS_General*)in->data;

  PetscFunctionBegin;
  *idx = sub->idx;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "ISRestoreIndices_General"
PetscErrorCode ISRestoreIndices_General(IS in,const PetscInt *idx[])
{
  IS_General *sub = (IS_General*)in->data;

  PetscFunctionBegin;
  if (*idx != sub->idx) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_WRONG,"Must restore with value from ISGetIndices()");
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "ISGetSize_General"
PetscErrorCode ISGetSize_General(IS is,PetscInt *size)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscLayoutGetSize(is->map, size);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "ISGetLocalSize_General"
PetscErrorCode ISGetLocalSize_General(IS is,PetscInt *size)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscLayoutGetLocalSize(is->map, size);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "ISInvertPermutation_General"
PetscErrorCode ISInvertPermutation_General(IS is,PetscInt nlocal,IS *isout)
{
  IS_General     *sub = (IS_General*)is->data;
  PetscInt       i,*ii,n,nstart;
  const PetscInt *idx = sub->idx;
  PetscMPIInt    size;
  IS             istmp,nistmp;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscLayoutGetLocalSize(is->map, &n);CHKERRQ(ierr);
  ierr = MPI_Comm_size(PetscObjectComm((PetscObject)is),&size);CHKERRQ(ierr);
  if (size == 1) {
    ierr = PetscMalloc1(n,&ii);CHKERRQ(ierr);
    for (i=0; i<n; i++) ii[idx[i]] = i;
    ierr = ISCreateGeneral(PETSC_COMM_SELF,n,ii,PETSC_OWN_POINTER,isout);CHKERRQ(ierr);
    ierr = ISSetPermutation(*isout);CHKERRQ(ierr);
  } else {
    /* crude, nonscalable get entire IS on each processor */
    if (nlocal == PETSC_DECIDE) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SUP,"Do not yet support nlocal of PETSC_DECIDE");
    ierr = ISAllGather(is,&istmp);CHKERRQ(ierr);
    ierr = ISSetPermutation(istmp);CHKERRQ(ierr);
    ierr = ISInvertPermutation(istmp,PETSC_DECIDE,&nistmp);CHKERRQ(ierr);
    ierr = ISDestroy(&istmp);CHKERRQ(ierr);
    /* get the part we need */
    ierr = MPI_Scan(&nlocal,&nstart,1,MPIU_INT,MPI_SUM,PetscObjectComm((PetscObject)is));CHKERRQ(ierr);
#if defined(PETSC_USE_DEBUG)
    {
      PetscInt    N;
      PetscMPIInt rank;
      ierr = MPI_Comm_rank(PetscObjectComm((PetscObject)is),&rank);CHKERRQ(ierr);
      ierr = PetscLayoutGetSize(is->map, &N);CHKERRQ(ierr);
      if (rank == size-1) {
        if (nstart != N) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_INCOMP,"Sum of nlocal lengths %d != total IS length %d",nstart,N);
      }
    }
#endif
    nstart -= nlocal;
    ierr    = ISGetIndices(nistmp,&idx);CHKERRQ(ierr);
    ierr    = ISCreateGeneral(PetscObjectComm((PetscObject)is),nlocal,idx+nstart,PETSC_COPY_VALUES,isout);CHKERRQ(ierr);
    ierr    = ISRestoreIndices(nistmp,&idx);CHKERRQ(ierr);
    ierr    = ISDestroy(&nistmp);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#if defined(PETSC_HAVE_HDF5)
#undef __FUNCT__
#define __FUNCT__ "ISView_General_HDF5"
PetscErrorCode ISView_General_HDF5(IS is, PetscViewer viewer)
{
  hid_t           filespace;  /* file dataspace identifier */
  hid_t           chunkspace; /* chunk dataset property identifier */
  hid_t           plist_id;   /* property list identifier */
  hid_t           dset_id;    /* dataset identifier */
  hid_t           memspace;   /* memory dataspace identifier */
  hid_t           inttype;    /* int type (H5T_NATIVE_INT or H5T_NATIVE_LLONG) */
  hid_t           file_id, group;
  hsize_t         dim, maxDims[3], dims[3], chunkDims[3], count[3],offset[3];
  PetscInt        bs, N, n, timestep, low;
  const PetscInt *ind;
  const char     *isname;
  PetscErrorCode  ierr;

  PetscFunctionBegin;
  ierr = ISGetBlockSize(is,&bs);CHKERRQ(ierr);
  ierr = PetscViewerHDF5OpenGroup(viewer, &file_id, &group);CHKERRQ(ierr);
  ierr = PetscViewerHDF5GetTimestep(viewer, &timestep);CHKERRQ(ierr);

  /* Create the dataspace for the dataset.
   *
   * dims - holds the current dimensions of the dataset
   *
   * maxDims - holds the maximum dimensions of the dataset (unlimited
   * for the number of time steps with the current dimensions for the
   * other dimensions; so only additional time steps can be added).
   *
   * chunkDims - holds the size of a single time step (required to
   * permit extending dataset).
   */
  dim = 0;
  if (timestep >= 0) {
    dims[dim]      = timestep+1;
    maxDims[dim]   = H5S_UNLIMITED;
    chunkDims[dim] = 1;
    ++dim;
  }
  ierr = ISGetSize(is, &N);CHKERRQ(ierr);
  ierr = ISGetLocalSize(is, &n);CHKERRQ(ierr);
  ierr = PetscHDF5IntCast(N/bs,dims + dim);CHKERRQ(ierr);

  maxDims[dim]   = dims[dim];
  chunkDims[dim] = dims[dim];
  ++dim;
  if (bs >= 1) {
    dims[dim]      = bs;
    maxDims[dim]   = dims[dim];
    chunkDims[dim] = dims[dim];
    ++dim;
  }
  PetscStackCallHDF5Return(filespace,H5Screate_simple,(dim, dims, maxDims));

#if defined(PETSC_USE_64BIT_INDICES)
  inttype = H5T_NATIVE_LLONG;
#else
  inttype = H5T_NATIVE_INT;
#endif

  /* Create the dataset with default properties and close filespace */
  ierr = PetscObjectGetName((PetscObject) is, &isname);CHKERRQ(ierr);
  if (!H5Lexists(group, isname, H5P_DEFAULT)) {
    /* Create chunk */
    PetscStackCallHDF5Return(chunkspace,H5Pcreate,(H5P_DATASET_CREATE));
    PetscStackCallHDF5(H5Pset_chunk,(chunkspace, dim, chunkDims));

#if (H5_VERS_MAJOR * 10000 + H5_VERS_MINOR * 100 + H5_VERS_RELEASE >= 10800)
    PetscStackCallHDF5Return(dset_id,H5Dcreate2,(group, isname, inttype, filespace, H5P_DEFAULT, chunkspace, H5P_DEFAULT));
#else
    PetscStackCallHDF5Return(dset_id,H5Dcreate,(group, isname, inttype, filespace, H5P_DEFAULT));
#endif
    PetscStackCallHDF5(H5Pclose,(chunkspace));
  } else {
    PetscStackCallHDF5Return(dset_id,H5Dopen2,(group, isname, H5P_DEFAULT));
    PetscStackCallHDF5(H5Dset_extent,(dset_id, dims));
  }
  PetscStackCallHDF5(H5Sclose,(filespace));

  /* Each process defines a dataset and writes it to the hyperslab in the file */
  dim = 0;
  if (timestep >= 0) {
    count[dim] = 1;
    ++dim;
  }
  ierr = PetscHDF5IntCast(n/bs,count + dim);CHKERRQ(ierr);
  ++dim;
  if (bs >= 1) {
    count[dim] = bs;
    ++dim;
  }
  if (n > 0) {
    PetscStackCallHDF5Return(memspace,H5Screate_simple,(dim, count, NULL));
  } else {
    /* Can't create dataspace with zero for any dimension, so create null dataspace. */
    PetscStackCallHDF5Return(memspace,H5Screate,(H5S_NULL));
  }

  /* Select hyperslab in the file */
  ierr = PetscLayoutGetRange(is->map, &low, NULL);CHKERRQ(ierr);
  dim  = 0;
  if (timestep >= 0) {
    offset[dim] = timestep;
    ++dim;
  }
  ierr = PetscHDF5IntCast(low/bs,offset + dim);CHKERRQ(ierr);
  ++dim;
  if (bs >= 1) {
    offset[dim] = 0;
    ++dim;
  }
  if (n > 0) {
    PetscStackCallHDF5Return(filespace,H5Dget_space,(dset_id));
    PetscStackCallHDF5(H5Sselect_hyperslab,(filespace, H5S_SELECT_SET, offset, NULL, count, NULL));
  } else {
    /* Create null filespace to match null memspace. */
    PetscStackCallHDF5Return(filespace,H5Screate,(H5S_NULL));
  }

  /* Create property list for collective dataset write */
  PetscStackCallHDF5Return(plist_id,H5Pcreate,(H5P_DATASET_XFER));
#if defined(PETSC_HAVE_H5PSET_FAPL_MPIO)
  PetscStackCallHDF5(H5Pset_dxpl_mpio,(plist_id, H5FD_MPIO_COLLECTIVE));
#endif
  /* To write dataset independently use H5Pset_dxpl_mpio(plist_id, H5FD_MPIO_INDEPENDENT) */

  ierr   = ISGetIndices(is, &ind);CHKERRQ(ierr);
  PetscStackCallHDF5(H5Dwrite,(dset_id, inttype, memspace, filespace, plist_id, ind));
  PetscStackCallHDF5(H5Fflush,(file_id, H5F_SCOPE_GLOBAL));
  ierr   = ISGetIndices(is, &ind);CHKERRQ(ierr);

  /* Close/release resources */
  if (group != file_id) PetscStackCallHDF5(H5Gclose,(group));
  PetscStackCallHDF5(H5Pclose,(plist_id));
  PetscStackCallHDF5(H5Sclose,(filespace));
  PetscStackCallHDF5(H5Sclose,(memspace));
  PetscStackCallHDF5(H5Dclose,(dset_id));
  ierr = PetscInfo1(is, "Wrote IS object with name %s\n", isname);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
#endif

#undef __FUNCT__
#define __FUNCT__ "ISView_General_Binary"
PetscErrorCode ISView_General_Binary(IS is,PetscViewer viewer)
{
  PetscErrorCode ierr;
  IS_General     *isa = (IS_General*) is->data;
  PetscMPIInt    rank,size,mesgsize,tag = ((PetscObject)viewer)->tag, mesglen;
  PetscInt       n,N,len,j,tr[2];
  int            fdes;
  MPI_Status     status;
  PetscInt       message_count,flowcontrolcount,*values;

  PetscFunctionBegin;
  ierr = PetscLayoutGetLocalSize(is->map, &n);CHKERRQ(ierr);
  ierr = PetscLayoutGetSize(is->map, &N);CHKERRQ(ierr);
  ierr = PetscViewerBinaryGetDescriptor(viewer,&fdes);CHKERRQ(ierr);

  /* determine maximum message to arrive */
  ierr = MPI_Comm_rank(PetscObjectComm((PetscObject)is),&rank);CHKERRQ(ierr);
  ierr = MPI_Comm_size(PetscObjectComm((PetscObject)is),&size);CHKERRQ(ierr);

  tr[0] = IS_FILE_CLASSID;
  tr[1] = N;
  ierr  = PetscViewerBinaryWrite(viewer,tr,2,PETSC_INT,PETSC_FALSE);CHKERRQ(ierr);
  ierr  = MPI_Reduce(&n,&len,1,MPIU_INT,MPI_SUM,0,PetscObjectComm((PetscObject)is));CHKERRQ(ierr);

  ierr = PetscViewerFlowControlStart(viewer,&message_count,&flowcontrolcount);CHKERRQ(ierr);
  if (!rank) {
    ierr = PetscBinaryWrite(fdes,isa->idx,n,PETSC_INT,PETSC_FALSE);CHKERRQ(ierr);

    ierr = PetscMalloc1(len,&values);CHKERRQ(ierr);
    ierr = PetscMPIIntCast(len,&mesgsize);CHKERRQ(ierr);
    /* receive and save messages */
    for (j=1; j<size; j++) {
      ierr = PetscViewerFlowControlStepMaster(viewer,j,&message_count,flowcontrolcount);CHKERRQ(ierr);
      ierr = MPI_Recv(values,mesgsize,MPIU_INT,j,tag,PetscObjectComm((PetscObject)is),&status);CHKERRQ(ierr);
      ierr = MPI_Get_count(&status,MPIU_INT,&mesglen);CHKERRQ(ierr);
      ierr = PetscBinaryWrite(fdes,values,(PetscInt)mesglen,PETSC_INT,PETSC_TRUE);CHKERRQ(ierr);
    }
    ierr = PetscViewerFlowControlEndMaster(viewer,&message_count);CHKERRQ(ierr);
    ierr = PetscFree(values);CHKERRQ(ierr);
  } else {
    ierr = PetscViewerFlowControlStepWorker(viewer,rank,&message_count);CHKERRQ(ierr);
    ierr = PetscMPIIntCast(n,&mesgsize);CHKERRQ(ierr);
    ierr = MPI_Send(isa->idx,mesgsize,MPIU_INT,0,tag,PetscObjectComm((PetscObject)is));CHKERRQ(ierr);
    ierr = PetscViewerFlowControlEndWorker(viewer,&message_count);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "ISView_General"
PetscErrorCode ISView_General(IS is,PetscViewer viewer)
{
  IS_General     *sub = (IS_General*)is->data;
  PetscErrorCode ierr;
  PetscInt       i,n,*idx = sub->idx;
  PetscBool      iascii,isbinary,ishdf5;

  PetscFunctionBegin;
  ierr = PetscLayoutGetLocalSize(is->map, &n);CHKERRQ(ierr);
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERASCII,&iascii);CHKERRQ(ierr);
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERBINARY,&isbinary);CHKERRQ(ierr);
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERHDF5,&ishdf5);CHKERRQ(ierr);
  if (iascii) {
    MPI_Comm    comm;
    PetscMPIInt rank,size;

    ierr = PetscObjectGetComm((PetscObject)viewer,&comm);CHKERRQ(ierr);
    ierr = MPI_Comm_rank(comm,&rank);CHKERRQ(ierr);
    ierr = MPI_Comm_size(comm,&size);CHKERRQ(ierr);

    ierr = PetscViewerASCIIPushSynchronized(viewer);CHKERRQ(ierr);
    if (size > 1) {
      if (is->isperm) {
        ierr = PetscViewerASCIISynchronizedPrintf(viewer,"[%d] Index set is permutation\n",rank);CHKERRQ(ierr);
      }
      ierr = PetscViewerASCIISynchronizedPrintf(viewer,"[%d] Number of indices in set %D\n",rank,n);CHKERRQ(ierr);
      for (i=0; i<n; i++) {
        ierr = PetscViewerASCIISynchronizedPrintf(viewer,"[%d] %D %D\n",rank,i,idx[i]);CHKERRQ(ierr);
      }
    } else {
      if (is->isperm) {
        ierr = PetscViewerASCIISynchronizedPrintf(viewer,"Index set is permutation\n");CHKERRQ(ierr);
      }
      ierr = PetscViewerASCIISynchronizedPrintf(viewer,"Number of indices in set %D\n",n);CHKERRQ(ierr);
      for (i=0; i<n; i++) {
        ierr = PetscViewerASCIISynchronizedPrintf(viewer,"%D %D\n",i,idx[i]);CHKERRQ(ierr);
      }
    }
    ierr = PetscViewerFlush(viewer);CHKERRQ(ierr);
    ierr = PetscViewerASCIIPopSynchronized(viewer);CHKERRQ(ierr);
  } else if (isbinary) {
    ierr = ISView_General_Binary(is,viewer);CHKERRQ(ierr);
  } else if (ishdf5) {
#if defined(PETSC_HAVE_HDF5)
    ierr = ISView_General_HDF5(is,viewer);CHKERRQ(ierr);
#endif
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "ISSort_General"
PetscErrorCode ISSort_General(IS is)
{
  IS_General     *sub = (IS_General*)is->data;
  PetscInt       n;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (sub->sorted) PetscFunctionReturn(0);
  ierr = PetscLayoutGetLocalSize(is->map, &n);CHKERRQ(ierr);
  ierr = PetscSortInt(n,sub->idx);CHKERRQ(ierr);
  sub->sorted = PETSC_TRUE;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "ISSortRemoveDups_General"
PetscErrorCode ISSortRemoveDups_General(IS is)
{
  IS_General     *sub = (IS_General*)is->data;
  PetscInt       n;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (sub->sorted) PetscFunctionReturn(0);
  ierr = PetscLayoutGetLocalSize(is->map, &n);CHKERRQ(ierr);
  ierr = PetscSortRemoveDupsInt(&n,sub->idx);CHKERRQ(ierr);
  ierr = PetscLayoutSetLocalSize(is->map, n);CHKERRQ(ierr);
  sub->sorted = PETSC_TRUE;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "ISSorted_General"
PetscErrorCode ISSorted_General(IS is,PetscBool  *flg)
{
  IS_General *sub = (IS_General*)is->data;

  PetscFunctionBegin;
  *flg = sub->sorted;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "ISToGeneral_General"
PetscErrorCode  ISToGeneral_General(IS is)
{
  PetscFunctionBegin;
  PetscFunctionReturn(0);
}

static struct _ISOps myops = { ISGetSize_General,
                               ISGetLocalSize_General,
                               ISGetIndices_General,
                               ISRestoreIndices_General,
                               ISInvertPermutation_General,
                               ISSort_General,
                               ISSortRemoveDups_General,
                               ISSorted_General,
                               ISDuplicate_General,
                               ISDestroy_General,
                               ISView_General,
                               ISLoad_Default,
                               ISIdentity_General,
                               ISCopy_General,
                               ISToGeneral_General,
                               ISOnComm_General,
                               ISSetBlockSize_General,
                               ISContiguousLocal_General};

#undef __FUNCT__
#define __FUNCT__ "ISCreateGeneral_Private"
PetscErrorCode ISCreateGeneral_Private(IS is)
{
  PetscErrorCode ierr;
  IS_General     *sub   = (IS_General*)is->data;
  const PetscInt *idx   = sub->idx;
  PetscBool      sorted = PETSC_TRUE;
  PetscInt       n,i,min,max;

  PetscFunctionBegin;
  ierr = PetscLayoutGetLocalSize(is->map, &n);CHKERRQ(ierr);
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
  sub->sorted    = sorted;
  is->min        = min;
  is->max        = max;
  is->isperm     = PETSC_FALSE;
  is->isidentity = PETSC_FALSE;
  ierr = ISViewFromOptions(is,NULL,"-is_view");CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "ISCreateGeneral"
/*@
   ISCreateGeneral - Creates a data structure for an index set
   containing a list of integers.

   Collective on MPI_Comm

   Input Parameters:
+  comm - the MPI communicator
.  n - the length of the index set
.  idx - the list of integers
-  mode - see PetscCopyMode for meaning of this flag.

   Output Parameter:
.  is - the new index set

   Notes:
   When the communicator is not MPI_COMM_SELF, the operations on IS are NOT
   conceptually the same as MPI_Group operations. The IS are then
   distributed sets of indices and thus certain operations on them are
   collective.


   Level: beginner

  Concepts: index sets^creating
  Concepts: IS^creating

.seealso: ISCreateStride(), ISCreateBlock(), ISAllGather()
@*/
PetscErrorCode  ISCreateGeneral(MPI_Comm comm,PetscInt n,const PetscInt idx[],PetscCopyMode mode,IS *is)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = ISCreate(comm,is);CHKERRQ(ierr);
  ierr = ISSetType(*is,ISGENERAL);CHKERRQ(ierr);
  ierr = ISGeneralSetIndices(*is,n,idx,mode);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "ISGeneralSetIndices"
/*@
   ISGeneralSetIndices - Sets the indices for an ISGENERAL index set

   Collective on IS

   Input Parameters:
+  is - the index set
.  n - the length of the index set
.  idx - the list of integers
-  mode - see PetscCopyMode for meaning of this flag.

   Level: beginner

  Concepts: index sets^creating
  Concepts: IS^creating

.seealso: ISCreateGeneral(), ISCreateStride(), ISCreateBlock(), ISAllGather()
@*/
PetscErrorCode  ISGeneralSetIndices(IS is,PetscInt n,const PetscInt idx[],PetscCopyMode mode)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscUseMethod(is,"ISGeneralSetIndices_C",(IS,PetscInt,const PetscInt[],PetscCopyMode),(is,n,idx,mode));CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "ISGeneralSetIndices_General"
PetscErrorCode  ISGeneralSetIndices_General(IS is,PetscInt n,const PetscInt idx[],PetscCopyMode mode)
{
  PetscErrorCode ierr;
  IS_General     *sub = (IS_General*)is->data;

  PetscFunctionBegin;
  if (n < 0) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"length < 0");
  if (n) PetscValidIntPointer(idx,3);

  if (sub->allocated) {ierr = PetscFree(sub->idx);CHKERRQ(ierr);}
  ierr = PetscLayoutSetLocalSize(is->map, n);CHKERRQ(ierr);
  if (mode == PETSC_COPY_VALUES) {
    ierr           = PetscMalloc1(n,&sub->idx);CHKERRQ(ierr);
    ierr           = PetscLogObjectMemory((PetscObject)is,n*sizeof(PetscInt));CHKERRQ(ierr);
    ierr           = PetscMemcpy(sub->idx,idx,n*sizeof(PetscInt));CHKERRQ(ierr);
    sub->allocated = PETSC_TRUE;
  } else if (mode == PETSC_OWN_POINTER) {
    sub->idx       = (PetscInt*)idx;
    sub->allocated = PETSC_TRUE;
  } else {
    sub->idx       = (PetscInt*)idx;
    sub->allocated = PETSC_FALSE;
  }
  ierr = ISCreateGeneral_Private(is);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "ISCreate_General"
PETSC_EXTERN PetscErrorCode ISCreate_General(IS is)
{
  PetscErrorCode ierr;
  IS_General     *sub;

  PetscFunctionBegin;
  ierr = PetscMemcpy(is->ops,&myops,sizeof(myops));CHKERRQ(ierr);
  ierr = PetscNewLog(is,&sub);CHKERRQ(ierr);
  is->data = (void *) sub;
  ierr = PetscObjectComposeFunction((PetscObject)is,"ISGeneralSetIndices_C",ISGeneralSetIndices_General);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}






