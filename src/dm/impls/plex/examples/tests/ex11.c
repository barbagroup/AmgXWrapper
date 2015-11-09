static char help[] = "Tests for DMLabel\n\n";

#include <petscdmplex.h>

#undef __FUNCT__
#define __FUNCT__ "TestInsertion"
static PetscErrorCode TestInsertion()
{
  DMLabel        label, label2;
  const PetscInt values[5] = {0, 3, 4, -1, 176}, N = 10000;
  PetscInt       i, v;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = DMLabelCreate("Test Label", &label);CHKERRQ(ierr);
  for (i = 0; i < N; ++i) {
    ierr = DMLabelSetValue(label, i, values[i%5]);CHKERRQ(ierr);
  }
  /* Test get in hash mode */
  for (i = 0; i < N; ++i) {
    PetscInt val;

    ierr = DMLabelGetValue(label, i, &val);CHKERRQ(ierr);
    if (val != values[i%5]) SETERRQ3(PETSC_COMM_SELF, PETSC_ERR_ARG_WRONG, "Value %d for point %d should be %d", val, i, values[i%5]);
  }
  /* Test stratum */
  for (v = 0; v < 5; ++v) {
    IS              stratum;
    const PetscInt *points;
    PetscInt        n;

    ierr = DMLabelGetStratumIS(label, values[v], &stratum);CHKERRQ(ierr);
    ierr = ISGetIndices(stratum, &points);CHKERRQ(ierr);
    ierr = ISGetLocalSize(stratum, &n);CHKERRQ(ierr);
    for (i = 0; i < n; ++i) {
      if (points[i] != i*5+v) SETERRQ2(PETSC_COMM_SELF, PETSC_ERR_ARG_WRONG, "Point %d should be %d", points[i], i*5+v);
    }
    ierr = ISRestoreIndices(stratum, &points);CHKERRQ(ierr);
    ierr = ISDestroy(&stratum);CHKERRQ(ierr);
  }
  /* Test get in array mode */
  for (i = 0; i < N; ++i) {
    PetscInt val;

    ierr = DMLabelGetValue(label, i, &val);CHKERRQ(ierr);
    if (val != values[i%5]) SETERRQ2(PETSC_COMM_SELF, PETSC_ERR_ARG_WRONG, "Value %d should be %d", val, values[i%5]);
  }
  /* Test Duplicate */
  ierr = DMLabelDuplicate(label, &label2);CHKERRQ(ierr);
  for (i = 0; i < N; ++i) {
    PetscInt val;

    ierr = DMLabelGetValue(label2, i, &val);CHKERRQ(ierr);
    if (val != values[i%5]) SETERRQ2(PETSC_COMM_SELF, PETSC_ERR_ARG_WRONG, "Value %d should be %d", val, values[i%5]);
  }
  ierr = DMLabelDestroy(&label2);CHKERRQ(ierr);
  ierr = DMLabelDestroy(&label);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TestEmptyStrata"
static PetscErrorCode TestEmptyStrata(MPI_Comm comm)
{
  DM             dm, dmDist;
  PetscInt       c0[6]  = {2,3,6,7,9,11};
  PetscInt       c1[6]  = {4,5,7,8,10,12};
  PetscInt       c2[4]  = {13,15,19,21};
  PetscInt       c3[4]  = {14,16,20,22};
  PetscInt       c4[4]  = {15,17,21,23};
  PetscInt       c5[4]  = {16,18,22,24};
  PetscInt       c6[4]  = {13,14,19,20};
  PetscInt       c7[4]  = {15,16,21,22};
  PetscInt       c8[4]  = {17,18,23,24};
  PetscInt       c9[4]  = {13,14,15,16};
  PetscInt       c10[4] = {15,16,17,18};
  PetscInt       c11[4] = {19,20,21,22};
  PetscInt       c12[4] = {21,22,23,24};
  PetscInt       dim    = 3;
  PetscMPIInt    rank;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MPI_Comm_rank(comm, &rank);CHKERRQ(ierr);
  /* A 3D box with two adjacent cells, sharing one face and four vertices */
  ierr = DMCreate(comm, &dm);CHKERRQ(ierr);
  ierr = DMSetType(dm, DMPLEX);CHKERRQ(ierr);
  ierr = DMSetDimension(dm, dim);CHKERRQ(ierr);
  if (!rank) {
    ierr = DMPlexSetChart(dm, 0, 25);CHKERRQ(ierr);
    ierr = DMPlexSetConeSize(dm, 0, 6);CHKERRQ(ierr);
    ierr = DMPlexSetConeSize(dm, 1, 6);CHKERRQ(ierr);
    ierr = DMPlexSetConeSize(dm, 2, 4);CHKERRQ(ierr);
    ierr = DMPlexSetConeSize(dm, 3, 4);CHKERRQ(ierr);
    ierr = DMPlexSetConeSize(dm, 4, 4);CHKERRQ(ierr);
    ierr = DMPlexSetConeSize(dm, 5, 4);CHKERRQ(ierr);
    ierr = DMPlexSetConeSize(dm, 6, 4);CHKERRQ(ierr);
    ierr = DMPlexSetConeSize(dm, 7, 4);CHKERRQ(ierr);
    ierr = DMPlexSetConeSize(dm, 8, 4);CHKERRQ(ierr);
    ierr = DMPlexSetConeSize(dm, 9, 4);CHKERRQ(ierr);
    ierr = DMPlexSetConeSize(dm, 10, 4);CHKERRQ(ierr);
    ierr = DMPlexSetConeSize(dm, 11, 4);CHKERRQ(ierr);
    ierr = DMPlexSetConeSize(dm, 12, 4);CHKERRQ(ierr);
  }
  ierr = DMSetUp(dm);CHKERRQ(ierr);
  if (!rank) {
    ierr = DMPlexSetCone(dm, 0, c0);CHKERRQ(ierr);
    ierr = DMPlexSetCone(dm, 1, c1);CHKERRQ(ierr);
    ierr = DMPlexSetCone(dm, 2, c2);CHKERRQ(ierr);
    ierr = DMPlexSetCone(dm, 3, c3);CHKERRQ(ierr);
    ierr = DMPlexSetCone(dm, 4, c4);CHKERRQ(ierr);
    ierr = DMPlexSetCone(dm, 5, c5);CHKERRQ(ierr);
    ierr = DMPlexSetCone(dm, 6, c6);CHKERRQ(ierr);
    ierr = DMPlexSetCone(dm, 7, c7);CHKERRQ(ierr);
    ierr = DMPlexSetCone(dm, 8, c8);CHKERRQ(ierr);
    ierr = DMPlexSetCone(dm, 9, c9);CHKERRQ(ierr);
    ierr = DMPlexSetCone(dm, 10, c10);CHKERRQ(ierr);
    ierr = DMPlexSetCone(dm, 11, c11);CHKERRQ(ierr);
    ierr = DMPlexSetCone(dm, 12, c12);CHKERRQ(ierr);
  }
  ierr = DMPlexSymmetrize(dm);CHKERRQ(ierr);
  /* Create a user managed depth label, so that we can leave out edges */
  if (!rank) {
    DMLabel  label;
    PetscInt i;

    ierr = DMPlexCreateLabel(dm, "depth");CHKERRQ(ierr);
    ierr = DMPlexGetDepthLabel(dm, &label);CHKERRQ(ierr);
    for (i = 0; i < 25; ++i) {
      if (i < 2)       {ierr = DMLabelSetValue(label, i, 3);CHKERRQ(ierr);}
      else if (i < 13) {ierr = DMLabelSetValue(label, i, 2);CHKERRQ(ierr);}
      else             {
        if (i==13) {ierr = DMLabelAddStratum(label, 1);CHKERRQ(ierr);}
        ierr = DMLabelSetValue(label, i, 0);CHKERRQ(ierr);
      }
    }
  }
  ierr = DMPlexDistribute(dm, 1, NULL, &dmDist);CHKERRQ(ierr);
  if (dmDist) {
    ierr = DMDestroy(&dm);CHKERRQ(ierr);CHKERRQ(ierr);
    dm   = dmDist;
  }
  /* Create a cell vector */
  {
    Vec          v;
    PetscSection s;
    PetscInt     numComp[] = {1};
    PetscInt     dof[]     = {0,0,0,1};
    PetscInt     N;

    ierr = DMPlexCreateSection(dm, dim, 1, numComp, dof, 0, NULL, NULL, NULL, NULL, &s);CHKERRQ(ierr);
    ierr = DMSetDefaultSection(dm, s);CHKERRQ(ierr);
    ierr = PetscSectionDestroy(&s);CHKERRQ(ierr);
    ierr = DMCreateGlobalVector(dm, &v);CHKERRQ(ierr);
    ierr = VecGetSize(v, &N);CHKERRQ(ierr);
    if (N != 2) {
      ierr = DMView(dm, PETSC_VIEWER_STDOUT_(comm));CHKERRQ(ierr);
      SETERRQ1(PETSC_COMM_SELF, PETSC_ERR_ARG_WRONG, "FAIL: Vector size %d != 2\n", N);
    }
    ierr = VecDestroy(&v);CHKERRQ(ierr);
  }
  ierr = DMDestroy(&dm);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc, char **argv)
{
  /*AppCtx         user;                 /* user-defined work context */
  PetscErrorCode ierr;

  ierr = PetscInitialize(&argc, &argv, NULL, help);CHKERRQ(ierr);
  /*ierr = ProcessOptions(PETSC_COMM_WORLD, &user);CHKERRQ(ierr);*/
  ierr = TestInsertion();CHKERRQ(ierr);
  ierr = TestEmptyStrata(PETSC_COMM_WORLD);CHKERRQ(ierr);
  ierr = PetscFinalize();
  return 0;
}
