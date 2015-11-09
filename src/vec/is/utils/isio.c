#include <petscis.h>         /*I  "petscis.h"  I*/
#include <petsc/private/isimpl.h>
#include <petscviewerhdf5.h>

#if defined(PETSC_HAVE_HDF5)
#undef __FUNCT__
#define __FUNCT__ "ISLoad_HDF5"
/*
     This should handle properly the cases where PetscInt is 32 or 64 and hsize_t is 32 or 64. These means properly casting with
   checks back and forth between the two types of variables.
*/
PetscErrorCode ISLoad_HDF5(IS is, PetscViewer viewer)
{
  hid_t           inttype;    /* int type (H5T_NATIVE_INT or H5T_NATIVE_LLONG) */
  hid_t           file_id, group, dset_id, filespace, memspace, plist_id;
  int             rdim, dim;
  hsize_t         dims[3], count[3], offset[3];
  PetscInt        n, N, bs, bsInd, lenInd, low, timestep;
  const PetscInt *ind;
  const char     *isname;
  PetscErrorCode  ierr;

  PetscFunctionBegin;
  ierr = PetscViewerHDF5OpenGroup(viewer, &file_id, &group);CHKERRQ(ierr);
  ierr = PetscViewerHDF5GetTimestep(viewer, &timestep);CHKERRQ(ierr);
  ierr = ISGetBlockSize(is, &bs);CHKERRQ(ierr);
  /* Create the dataset with default properties and close filespace */
  ierr = PetscObjectGetName((PetscObject) is, &isname);CHKERRQ(ierr);
#if (H5_VERS_MAJOR * 10000 + H5_VERS_MINOR * 100 + H5_VERS_RELEASE >= 10800)
  PetscStackCallHDF5Return(dset_id,H5Dopen2,(group, isname, H5P_DEFAULT));
#else
  PetscStackCallHDF5Return(dset_id,H5Dopen,(group, isname));
#endif
  /* Retrieve the dataspace for the dataset */
  PetscStackCallHDF5Return(filespace,H5Dget_space,(dset_id));
  dim = 0;
  if (timestep >= 0) ++dim;
  ++dim;
  ++dim;
  PetscStackCallHDF5Return(rdim,H5Sget_simple_extent_dims,(filespace, dims, NULL));
  bsInd = rdim-1;
  lenInd = timestep >= 0 ? 1 : 0;
  if (rdim != dim) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_FILE_UNEXPECTED, "Dimension of array in file %d not %d as expected",rdim,dim);
  else if (bs != (PetscInt) dims[bsInd]) {
    ierr = ISSetBlockSize(is, dims[bsInd]);
    if (ierr) SETERRQ2(PETSC_COMM_SELF, PETSC_ERR_FILE_UNEXPECTED, "Block size %d specified for IS does not match blocksize in file %d",bs,dims[bsInd]);
    bs = dims[bsInd];
  }

  /* Set Vec sizes,blocksize,and type if not already set */
  ierr = ISGetLocalSize(is, &n);CHKERRQ(ierr);
  ierr = ISGetSize(is, &N);CHKERRQ(ierr);
  if (n < 0 && N < 0) {ierr = PetscLayoutSetSize(is->map, dims[lenInd]*bs);CHKERRQ(ierr);}
  ierr = PetscLayoutSetUp(is->map);CHKERRQ(ierr);
  /* If sizes and type already set,check if the vector global size is correct */
  ierr = ISGetSize(is, &N);CHKERRQ(ierr);
  if (N/bs != (PetscInt) dims[lenInd]) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_FILE_UNEXPECTED, "IS in file different length (%d) then input vector (%d)", (PetscInt) dims[lenInd], N/bs);

  /* Each process defines a dataset and reads it from the hyperslab in the file */
  ierr = ISGetLocalSize(is, &n);CHKERRQ(ierr);
  dim  = 0;
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
  PetscStackCallHDF5Return(memspace,H5Screate_simple,(dim, count, NULL));

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
  PetscStackCallHDF5(H5Sselect_hyperslab,(filespace, H5S_SELECT_SET, offset, NULL, count, NULL));

  /* Create property list for collective dataset read */
  PetscStackCallHDF5Return(plist_id,H5Pcreate,(H5P_DATASET_XFER));
#if defined(PETSC_HAVE_H5PSET_FAPL_MPIO)
  PetscStackCallHDF5(H5Pset_dxpl_mpio,(plist_id, H5FD_MPIO_COLLECTIVE));
#endif
  /* To write dataset independently use H5Pset_dxpl_mpio(plist_id, H5FD_MPIO_INDEPENDENT) */

#if defined(PETSC_USE_64BIT_INDICES)
  inttype = H5T_NATIVE_LLONG;
#else
  inttype = H5T_NATIVE_INT;
#endif
  ierr   = PetscMalloc1(n,&ind);CHKERRQ(ierr);
  PetscStackCallHDF5(H5Dread,(dset_id, inttype, memspace, filespace, plist_id, (void *) ind));
  ierr   = ISGeneralSetIndices(is, n, ind, PETSC_OWN_POINTER);CHKERRQ(ierr);

  /* Close/release resources */
  if (group != file_id) PetscStackCallHDF5(H5Gclose,(group));
  PetscStackCallHDF5(H5Pclose,(plist_id));
  PetscStackCallHDF5(H5Sclose,(filespace));
  PetscStackCallHDF5(H5Sclose,(memspace));
  PetscStackCallHDF5(H5Dclose,(dset_id));
  PetscFunctionReturn(0);
}
#endif

#undef __FUNCT__
#define __FUNCT__ "ISLoad_Default"
PetscErrorCode ISLoad_Default(IS is, PetscViewer viewer)
{
  PetscBool      isbinary;
#if defined(PETSC_HAVE_HDF5)
  PetscBool      ishdf5;
#endif
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERBINARY,&isbinary);CHKERRQ(ierr);
#if defined(PETSC_HAVE_HDF5)
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERHDF5,&ishdf5);CHKERRQ(ierr);
#endif
  if (isbinary) {
    SETERRQ(PetscObjectComm((PetscObject) is), PETSC_ERR_SUP, "This should be implemented");
#if defined(PETSC_HAVE_HDF5)
  } else if (ishdf5) {
    if (!((PetscObject) is)->name) SETERRQ(PETSC_COMM_SELF, PETSC_ERR_SUP, "Since HDF5 format gives ASCII name for each object in file; must use ISLoad() after setting name of Vec with PetscObjectSetName()");
    ierr = ISLoad_HDF5(is, viewer);CHKERRQ(ierr);
#endif
  }
  PetscFunctionReturn(0);
}
