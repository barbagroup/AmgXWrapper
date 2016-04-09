static char help[] = "An example of writing a global Vec from a DMPlex with HDF5 format.\n";

#include <petscdmplex.h>
#include <petscviewerhdf5.h>

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc, char **argv)
{
  MPI_Comm       comm;
  DM             dm;
  Vec            v, nv, rv, coord;
  PetscBool      test_read = PETSC_FALSE, verbose = PETSC_FALSE, flg;
  PetscViewer    hdf5Viewer;
  PetscInt       dim         = 2;
  PetscInt       numFields   = 1;
  PetscInt       numBC       = 0;
  PetscInt       numComp[1]  = {2};
  PetscInt       numDof[3]   = {2, 0, 0};
  PetscInt       bcFields[1] = {0};
  IS             bcPoints[1] = {NULL};
  PetscSection   section;
  PetscReal      norm;
  PetscErrorCode ierr;

  ierr = PetscInitialize(&argc, &argv, (char *) 0, help);CHKERRQ(ierr);
  comm = PETSC_COMM_WORLD;

  ierr = PetscOptionsBegin(PETSC_COMM_WORLD,"","Test Options","none");CHKERRQ(ierr);
  ierr = PetscOptionsBool("-test_read","Test reading from the HDF5 file","",PETSC_FALSE,&test_read,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsBool("-verbose","print the Vecs","",PETSC_FALSE,&verbose,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsInt("-dim","the dimension of the problem","",2,&dim,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsEnd();

  ierr = DMPlexCreateBoxMesh(comm, dim, PETSC_TRUE, &dm);CHKERRQ(ierr);
  ierr = DMGetDimension(dm, &dim);CHKERRQ(ierr);
  numDof[0] = dim;
  ierr = DMPlexCreateSection(dm, dim, numFields, numComp, numDof, numBC, bcFields, bcPoints, NULL, NULL, &section);CHKERRQ(ierr);
  ierr = DMSetDefaultSection(dm, section);CHKERRQ(ierr);
  ierr = PetscSectionDestroy(&section);CHKERRQ(ierr);
  ierr = DMSetUseNatural(dm, PETSC_TRUE);CHKERRQ(ierr);
  {
    DM dmDist;

    ierr = DMPlexDistribute(dm, 0, NULL, &dmDist);CHKERRQ(ierr);
    if (dmDist) {
      ierr = DMDestroy(&dm);CHKERRQ(ierr);
      dm   = dmDist;
    }
  }

  ierr = DMCreateGlobalVector(dm, &v);CHKERRQ(ierr);
  ierr = PetscObjectSetName((PetscObject) v, "V");CHKERRQ(ierr);
  ierr = DMGetCoordinates(dm, &coord);CHKERRQ(ierr);
  ierr = VecCopy(coord, v);CHKERRQ(ierr);

  if (verbose) {
    PetscInt size, bs;

    ierr = VecGetSize(v, &size);CHKERRQ(ierr);
    ierr = VecGetBlockSize(v, &bs);CHKERRQ(ierr);
    ierr = PetscPrintf(PETSC_COMM_WORLD, "==== original V in global ordering. size==%d\tblock size=%d\n", size, bs);CHKERRQ(ierr);
    ierr = VecView(v, PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);
  }

  ierr = DMCreateGlobalVector(dm, &nv);CHKERRQ(ierr);
  ierr = PetscObjectSetName((PetscObject) nv, "NV");CHKERRQ(ierr);
  ierr = DMPlexGlobalToNaturalBegin(dm, v, nv);CHKERRQ(ierr);
  ierr = DMPlexGlobalToNaturalEnd(dm, v, nv);CHKERRQ(ierr);

  if (verbose) {
    PetscInt size, bs;

    ierr = VecGetSize(nv, &size);CHKERRQ(ierr);
    ierr = VecGetBlockSize(nv, &bs);CHKERRQ(ierr);
    ierr = PetscPrintf(PETSC_COMM_WORLD, "====  V in natural ordering. size==%d\tblock size=%d\n", size, bs);CHKERRQ(ierr);
    ierr = VecView(nv, PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);
  }

  ierr = VecViewFromOptions(v, NULL, "-global_vec_view");CHKERRQ(ierr);

  if (test_read) {
    ierr = DMCreateGlobalVector(dm, &rv);CHKERRQ(ierr);
    ierr = PetscObjectSetName((PetscObject) rv, "V");CHKERRQ(ierr);
    /* Test native read */
    ierr = PetscViewerHDF5Open(comm, "V.h5", FILE_MODE_READ, &hdf5Viewer);CHKERRQ(ierr);
    ierr = PetscViewerPushFormat(hdf5Viewer, PETSC_VIEWER_NATIVE);CHKERRQ(ierr);
    ierr = VecLoad(rv, hdf5Viewer);CHKERRQ(ierr);
    ierr = PetscViewerPopFormat(hdf5Viewer);CHKERRQ(ierr);
    ierr = PetscViewerDestroy(&hdf5Viewer);CHKERRQ(ierr);
    if (verbose) {
      PetscInt size, bs;

      ierr = VecGetSize(rv, &size);CHKERRQ(ierr);
      ierr = VecGetBlockSize(rv, &bs);CHKERRQ(ierr);
      ierr = PetscPrintf(PETSC_COMM_WORLD, "==== Vector from file. size==%d\tblock size=%d\n", size, bs);CHKERRQ(ierr);
      ierr = VecView(rv, PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);
    }
    ierr = VecEqual(rv, v, &flg);CHKERRQ(ierr);
    if (flg) {
      ierr = PetscPrintf(PETSC_COMM_WORLD, "V and RV are equal\n");CHKERRQ(ierr);
    } else {
      ierr = PetscPrintf(PETSC_COMM_WORLD, "V and RV are not equal\n\n");CHKERRQ(ierr);
      ierr = VecAXPY(rv, -1.0, v);CHKERRQ(ierr);
      ierr = VecNorm(rv, NORM_INFINITY, &norm);CHKERRQ(ierr);
      ierr = PetscPrintf(PETSC_COMM_WORLD, "diff norm is = %g\n", (double) norm);CHKERRQ(ierr);
      ierr = VecView(rv, PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);
    }
    /* Test raw read */
    ierr = PetscViewerHDF5Open(comm, "V.h5", FILE_MODE_READ, &hdf5Viewer);CHKERRQ(ierr);
    ierr = VecLoad(rv, hdf5Viewer);CHKERRQ(ierr);
    ierr = PetscViewerDestroy(&hdf5Viewer);CHKERRQ(ierr);
    if (verbose) {
      PetscInt size, bs;

      ierr = VecGetSize(rv, &size);CHKERRQ(ierr);
      ierr = VecGetBlockSize(rv, &bs);CHKERRQ(ierr);
      ierr = PetscPrintf(PETSC_COMM_WORLD, "==== Vector from file. size==%d\tblock size=%d\n", size, bs);CHKERRQ(ierr);
      ierr = VecView(rv, PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);
    }
    ierr = VecEqual(rv, nv, &flg);CHKERRQ(ierr);
    if (flg) {
      ierr = PetscPrintf(PETSC_COMM_WORLD, "NV and RV are equal\n");CHKERRQ(ierr);
    } else {
      ierr = PetscPrintf(PETSC_COMM_WORLD, "NV and RV are not equal\n\n");CHKERRQ(ierr);
      ierr = VecAXPY(rv, -1.0, v);CHKERRQ(ierr);
      ierr = VecNorm(rv, NORM_INFINITY, &norm);CHKERRQ(ierr);
      ierr = PetscPrintf(PETSC_COMM_WORLD, "diff norm is = %g\n", (double) norm);CHKERRQ(ierr);
      ierr = VecView(rv, PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);
    }
    ierr = VecDestroy(&rv);CHKERRQ(ierr);
  }
  ierr = VecDestroy(&nv);CHKERRQ(ierr);
  ierr = VecDestroy(&v);CHKERRQ(ierr);
  ierr = DMDestroy(&dm);CHKERRQ(ierr);
  ierr = PetscFinalize();
  return 0;
}
