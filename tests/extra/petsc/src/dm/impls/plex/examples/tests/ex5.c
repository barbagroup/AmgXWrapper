static char help[] = "Tests for creation of hybrid meshes\n\n";

/* TODO
 - Propagate hybridSize with distribution
 - Test with multiple fault segments
 - Test with embedded fault
 - Test with multiple faults
 - Move over all PyLith tests?
*/

#include <petscdmplex.h>
/* List of test meshes

Triangle
--------
Test 0:
Two triangles sharing a face

        4
      / | \
     8  |  9
    /   |   \
   2  0 7 1  5
    \   |   /
     6  |  10
      \ | /
        3

should become two triangles separated by a zero-volume cell with 4 vertices

        5--16--8              4--12--6 3
      / |      | \          / |      | | \
    11  |      |  12       9  |      | |  4
    /   |      |   \      /   |      | |   \
   3  0 10  2 14 1  6    2  0 8  1  10 6 0  1
    \   |      |   /      \   |      | |   /
     9  |      |  13       7  |      | |  5
      \ |      | /          \ |      | | /
        4--15--7              3--11--5 2

Test 1:
Four triangles sharing two faces which are oriented against each other

          9
         / \
        /   \
      17  2  16
      /       \
     /         \
    8-----15----5
     \         /|\
      \       / | \
      18  3  12 |  14
        \   /   |   \
         \ /    |    \
          4  0 11  1  7
           \    |    /
            \   |   /
            10  |  13
              \ | /
               \|/
                6

Fault mesh

0 --> 0
1 --> 1
2 --> 2
3 --> 3
4 --> 5
5 --> 6
6 --> 8
7 --> 11
8 --> 15

       2
       |
  6----8----4
       |    |
       3    |
          0-7-1
            |
            |
            5

should become four triangles separated by two zero-volume cells with 4 vertices

          11
          / \
         /   \
        /     \
      22   2   21
      /         \
     /           \
   10-----20------7
28  |     5    26/ \
   14----25----12   \
     \         /|   |\
      \       / |   | \
      23  3  17 |   |  19
        \   /   |   |   \
         \ /    |   |    \
          6  0 24 4 16 1  9
           \    |   |    /
            \   |   |   /
            15  |   |  18
              \ |   | /
               \|   |/
               13---8
                 27

Tetrahedron
-----------
Test 0:
Two tets sharing a face

 cell   5 _______    cell
 0    / | \      \       1
    16  |  18     22
    /8 19 10\      \
   2-15-|----4--21--6
    \  9| 7 /      /
    14  |  17     20
      \ | /      /
        3-------

should become two tetrahedrons separated by a zero-volume cell with 3 faces/3 edges/6 vertices

 cell   6 ___36___10______    cell
 0    / | \        |\      \     1
    24  |  26      | 32     30
    /12 27 14\    33  \      \
   3-23-|----5--35-|---9--29--7
    \ 13| 11/      |18 /      /
    22  |  25      | 31     28
      \ | /        |/      /
        4----34----8------
         cell 2

In parallel,

 cell   5 ___28____8      4______    cell
 0    / | \        |\     |\      \     0
    19  |   21     | 24   | 13  6  11
    /10 22 12\    25  \   |8 \      \
   2-18-|----4--27-|---7  14  3--10--1
    \ 11| 9 /      |13 /  |  /      /
    17  |  20      | 23   | 12  5  9
      \ | /        |/     |/      /
        3----26----6      2------
         cell 1

Test 1:
Four tets sharing two faces

Cells:    0-3,4-5
Vertices: 6-15
Faces:    16-29,30-34
Edges:    35-52,53-56

Quadrilateral
-------------
Test 0:
Two quads sharing a face

   5--10---4--14---7
   |       |       |
  11   0   9   1  13
   |       |       |
   2---8---3--12---6

should become two quads separated by a zero-volume cell with 4 vertices

   6--13---5-20-10--17---8    5--10---4-14--7  4---7---2
   |       |     |       |    |       |     |  |       |
  14   0  12  2 18   1  16   11   0   9  1 12  8   0   6
   |       |     |       |    |       |     |  |       |
   3--11---4-19--9--15---7    2---8---3-13--6  3---5---1

Test 1:

Original mesh with 9 cells,

  9 ----10 ----11 ----12
  |      |      |      |
  |      |      |      |
  |      |      |      |
  |      |      |      |
 13 ----14 ----15 ----16
  |      |      |      |
  |      |      |      |
  |      |      |      |
  |      |      |      |
 17 ----18 ----19 ----20
  |      |      |      |
  |      |      |      |
  |      |      |      |
  |      |      |      |
 21 ----22 ----23 ----24

After first fault,

 12 ----13 ----14-28 ----15
  |      |      |  |      |
  |  0   |  1   | 9|  2   |
  |      |      |  |      |
  |      |      |  |      |
 16 ----17 ----18-29 ----19
  |      |      |  |      |
  |  3   |  4   |10|  5   |
  |      |      |  |      |
  |      |      |  |      |
 20 ----21-----22-30 ----23
  |      |      |  \--11- |
  |  6   |  7   |     8   |
  |      |      |         |
  |      |      |         |
 24 ----25 ----26--------27

After second fault,

 14 ----15 ----16-30 ----17
  |      |      |  |      |
  |  0   |  1   | 9|  2   |
  |      |      |  |      |
  |      |      |  |      |
 18 ----19 ----20-31 ----21
  |      |      |  |      |
  |  3   |  4   |10|  5   |
  |      |      |  |      |
  |      |      |  |      |
 33 ----34-----24-32 ----25
  |  12  | 13 / |  \-11-- |
 22 ----23---/  |         |
  |      |   7  |     8   |
  |  6   |      |         |
  |      |      |         |
  |      |      |         |
 26 ----27 ----28--------29

Hexahedron
----------
Test 0:
Two hexes sharing a face

cell   9-----31------8-----42------13 cell
0     /|            /|            /|     1
    32 |   15      30|   21      41|
    /  |          /  |          /  |
   6-----29------7-----40------12  |
   |   |     18  |   |     24  |   |
   |  36         |  35         |   44
   |19 |         |17 |         |23 |
  33   |  16    34   |   22   43   |
   |   5-----27--|---4-----39--|---11
   |  /          |  /          |  /
   | 28   14     | 26    20    | 38
   |/            |/            |/
   2-----25------3-----37------10

should become two hexes separated by a zero-volume cell with 8 vertices

                         cell 2
cell  10-----41------9-----62------18----52------14 cell
0     /|            /|            /|            /|     1
    42 |   20      40|  32       56|   26      51|
    /  |          /  |          /  |          /  |
   7-----39------8-----61------17--|-50------13  |
   |   |     23  |   |         |   |     29  |   |
   |  46         |  45         |   58        |   54
   |24 |         |22 |         |30 |         |28 |
  43   |  21    44   |        57   |   27   53   |
   |   6-----37--|---5-----60--|---16----49--|---12
   |  /          |  /          |  /          |  /
   | 38   19     | 36   31     | 55    25    | 48
   |/            |/            |/            |/
   3-----35------4-----59------15----47------11

In parallel,

                         cell 2
cell   9-----31------8-----44------13     8----20------4  cell
0     /|            /|            /|     /|           /|     1
    32 |   15      30|  22       38|   24 |  10      19|
    /  |          /  |          /  |   /  |         /  |
   6-----29------7-----43------12  |  7----18------3   |
   |   |     18  |   |         |   |  |   |    13  |   |
   |  36         |  35         |   40 |  26        |   22
   |19 |         |17 |         |20 |  |14 |        |12 |
  33   |  16    34   |        39   |  25  |  11   21   |
   |   5-----27--|---4-----42--|---11 |   6----17--|---2
   |  /          |  /          |  /   |  /         |  /
   | 28   14     | 26   21     | 37   |23     9    | 16
   |/            |/            |/     |/           |/
   2-----25------3-----41------10     5----15------1

Test 1:

*/

typedef struct {
  PetscInt  debug;       /* The debugging level */
  PetscInt  dim;         /* The topological mesh dimension */
  PetscBool cellSimplex; /* Use simplices or hexes */
  PetscInt  testNum;     /* The particular mesh to test */
} AppCtx;

#undef __FUNCT__
#define __FUNCT__ "ProcessOptions"
PetscErrorCode ProcessOptions(MPI_Comm comm, AppCtx *options)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  options->debug       = 0;
  options->dim         = 2;
  options->cellSimplex = PETSC_TRUE;
  options->testNum     = 0;

  ierr = PetscOptionsBegin(comm, "", "Meshing Problem Options", "DMPLEX");CHKERRQ(ierr);
  ierr = PetscOptionsInt("-debug", "The debugging level", "ex5.c", options->debug, &options->debug, NULL);CHKERRQ(ierr);
  ierr = PetscOptionsInt("-dim", "The topological mesh dimension", "ex5.c", options->dim, &options->dim, NULL);CHKERRQ(ierr);
  ierr = PetscOptionsBool("-cell_simplex", "Use simplices if true, otherwise hexes", "ex5.c", options->cellSimplex, &options->cellSimplex, NULL);CHKERRQ(ierr);
  ierr = PetscOptionsInt("-test_num", "The particular mesh to test", "ex5.c", options->testNum, &options->testNum, NULL);CHKERRQ(ierr);
  ierr = PetscOptionsEnd();
  PetscFunctionReturn(0);
};

#undef __FUNCT__
#define __FUNCT__ "CreateSimplex_2D"
PetscErrorCode CreateSimplex_2D(MPI_Comm comm, PetscInt testNum, DM *dm)
{
  DM             idm = NULL;
  PetscInt       p;
  PetscMPIInt    rank;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MPI_Comm_rank(comm, &rank);CHKERRQ(ierr);
  if (!rank) {
    switch (testNum) {
    case 0:
    {
      PetscInt    numPoints[2]        = {4, 2};
      PetscInt    coneSize[6]         = {3, 3, 0, 0, 0, 0};
      PetscInt    cones[6]            = {2, 3, 4,  5, 4, 3};
      PetscInt    coneOrientations[6] = {0, 0, 0,  0, 0, 0};
      PetscScalar vertexCoords[8]     = {-0.5, 0.5, 0.0, 0.0, 0.0, 1.0, 0.5, 0.5};
      PetscInt    markerPoints[8]     = {2, 1, 3, 1, 4, 1, 5, 1};
      PetscInt    faultPoints[2]      = {3, 4};

      ierr = DMPlexCreateFromDAG(*dm, 1, numPoints, coneSize, cones, coneOrientations, vertexCoords);CHKERRQ(ierr);
      for (p = 0; p < 4; ++p) {ierr = DMSetLabelValue(*dm, "marker", markerPoints[p*2], markerPoints[p*2+1]);CHKERRQ(ierr);}
      for (p = 0; p < 2; ++p) {ierr = DMSetLabelValue(*dm, "fault", faultPoints[p], 1);CHKERRQ(ierr);}
    }
    break;
    case 1:
    {
      PetscInt    numPoints[2]         = {6, 4};
      PetscInt    coneSize[10]         = {3, 3, 3, 3, 0, 0, 0, 0, 0, 0};
      PetscInt    cones[12]            = {4, 6, 5,  5, 6, 7,  8, 5, 9,  8, 4, 5};
      PetscInt    coneOrientations[12] = {0, 0, 0,  0, 0, 0,  0, 0, 0,  0, 0, 0};
      PetscScalar vertexCoords[12]     = {-1.0, 0.0, 0.0, 1.0, 0.0, -1.0, 1.0, 0.0, -2.0, 1.0, -1.0, 2.0};
      PetscInt    markerPoints[6]      = {4, 1, 6, 1, 8, 1};
      PetscInt    faultPoints[3]       = {5, 6, 8};

      ierr = DMPlexCreateFromDAG(*dm, 1, numPoints, coneSize, cones, coneOrientations, vertexCoords);CHKERRQ(ierr);
      for (p = 0; p < 3; ++p) {ierr = DMSetLabelValue(*dm, "marker", markerPoints[p*2], markerPoints[p*2+1]);CHKERRQ(ierr);}
      for (p = 0; p < 3; ++p) {ierr = DMSetLabelValue(*dm, "fault", faultPoints[p], 1);CHKERRQ(ierr);}
    }
    break;
    default:
      SETERRQ1(comm, PETSC_ERR_ARG_OUTOFRANGE, "No test mesh %d", testNum);
    }
  } else {
    PetscInt numPoints[3] = {0, 0, 0};

    ierr = DMPlexCreateFromDAG(*dm, 1, numPoints, NULL, NULL, NULL, NULL);CHKERRQ(ierr);
    ierr = DMCreateLabel(*dm, "fault");CHKERRQ(ierr);
  }
  ierr = DMPlexInterpolate(*dm, &idm);CHKERRQ(ierr);
  ierr = DMPlexCopyCoordinates(*dm, idm);CHKERRQ(ierr);
  ierr = DMCopyLabels(*dm, idm);CHKERRQ(ierr);
  ierr = DMViewFromOptions(idm, NULL, "-in_dm_view");CHKERRQ(ierr);
  ierr = DMDestroy(dm);CHKERRQ(ierr);
  *dm  = idm;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "CreateSimplex_3D"
PetscErrorCode CreateSimplex_3D(MPI_Comm comm, AppCtx *user, DM dm)
{
  PetscInt       depth = 3, testNum  = user->testNum, p;
  PetscMPIInt    rank;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MPI_Comm_rank(comm, &rank);CHKERRQ(ierr);
  if (!rank) {
    switch (testNum) {
    case 0:
    {
      PetscInt    numPoints[4]         = {5, 7, 9, 2};
      PetscInt    coneSize[23]         = {4, 4, 0, 0, 0, 0, 0, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2};
      PetscInt    cones[47]            = {7, 8, 9, 10,  11, 10, 13, 12,  15, 17, 14,  16, 18, 15,  14, 19, 16,  17, 18, 19,  17, 21, 20,  18, 22, 21,  22, 19, 20,   2, 3,  2, 4,  2, 5,  3, 4,  4, 5,  5, 3,  3, 6,  4, 6,  5, 6};
      PetscInt    coneOrientations[47] = {0, 0, 0,  0,   0, -3,  2,  2,   0, -2, -2,   0, -2, -2,   0, -2, -2,   0,  0,  0,   0,  0, -2,   0,  0, -2,  -2,  0,  0,   0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0};
      PetscScalar vertexCoords[15]     = {0.0, 0.0, -0.5,  0.0, -0.5, 0.0,  1.0, 0.0, 0.0,  0.0, 0.5, 0.0,  0.0, 0.0, 0.5};
      PetscInt    markerPoints[20]     = {2, 1, 3, 1, 4, 1, 5, 1, 14, 1, 15, 1, 16, 1, 17, 1, 18, 1, 19, 1};
      PetscInt    faultPoints[3]      = {3, 4, 5};

      ierr = DMPlexCreateFromDAG(dm, depth, numPoints, coneSize, cones, coneOrientations, vertexCoords);CHKERRQ(ierr);
      for (p = 0; p < 10; ++p) {
        ierr = DMSetLabelValue(dm, "marker", markerPoints[p*2], markerPoints[p*2+1]);CHKERRQ(ierr);
      }
      for(p = 0; p < 3; ++p) {
        ierr = DMSetLabelValue(dm, "fault", faultPoints[p], 1);CHKERRQ(ierr);
      }
    }
    break;
    case 1:
    {
      PetscInt    numPoints[4]         = {6, 13, 12, 4};
      PetscInt    coneSize[35]         = {4, 4, 4, 4, 0, 0, 0, 0, 0, 0, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2};
      PetscInt    cones[78]            = {10, 11, 12, 13,  10, 15, 16, 14,  17, 18, 14, 19,  20, 13, 19, 21,  22, 23, 24,  25, 26, 22,  24, 27, 25,  23, 26, 27,  28, 29, 23,  24, 30, 28,  22, 29, 30,   31, 32, 28,  29, 33, 31,  32, 33, 23,  26, 34, 33,  34, 27, 32,  6, 5,  5, 7,  7, 6,  6, 4,  4, 5,  7, 4,  7, 9,  9, 5,  6, 9,  9, 8,  8, 7,  5, 8,  4, 8};
      PetscInt    coneOrientations[78] = { 0,  0,  0,  0,  -3,  1,  0,  2,   0,  0, -1,  0,   0, -1, -2,  0,   0,  0,  0,   0,  0, -2,  -2,  0, -2,  -2, -2, -2,   0,  0,  0,   0,  0, -2,   0, -2, -2,    0,  0,  0,   0,  0, -2,  -2, -2,  0,  -2,  0, -2,  -2, -2, -2,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0,  0, 0};
      PetscScalar vertexCoords[18]     = {-1.0, 0.0, 0.0,  0.0, -1.0, 0.0,  0.0, 0.0, 1.0,  0.0, 1.0, 0.0,  0.0, 0.0, -1.0,  1.0, 0.0, 0.0};
      PetscInt    markerPoints[14]     = {5, 1, 6, 1, 7, 1, 10, 1, 22, 1, 23, 1, 24, 1};
      PetscInt    faultPoints[4]       = {5, 6, 7, 8};

      ierr = DMPlexCreateFromDAG(dm, depth, numPoints, coneSize, cones, coneOrientations, vertexCoords);CHKERRQ(ierr);
      for (p = 0; p < 7; ++p) {
        ierr = DMSetLabelValue(dm, "marker", markerPoints[p*2], markerPoints[p*2+1]);CHKERRQ(ierr);
      }
      for(p = 0; p < 4; ++p) {
        ierr = DMSetLabelValue(dm, "fault", faultPoints[p], 1);CHKERRQ(ierr);
      }
    }
    break;
    default:
      SETERRQ1(comm, PETSC_ERR_ARG_OUTOFRANGE, "No test mesh %d", testNum);
    }
  } else {
    PetscInt numPoints[4] = {0, 0, 0, 0};

    ierr = DMPlexCreateFromDAG(dm, depth, numPoints, NULL, NULL, NULL, NULL);CHKERRQ(ierr);
    ierr = DMCreateLabel(dm, "fault");CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "CreateQuad_2D"
PetscErrorCode CreateQuad_2D(MPI_Comm comm, PetscInt testNum, DM *dm)
{
  DM             idm = NULL;
  PetscInt       p;
  PetscMPIInt    rank;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MPI_Comm_rank(comm, &rank);CHKERRQ(ierr);
  if (!rank) {
    switch (testNum) {
    case 0:
    {
      PetscInt    numPoints[2]        = {6, 2};
      PetscInt    coneSize[8]         = {4, 4, 0, 0, 0, 0, 0, 0};
      PetscInt    cones[8]            = {2, 3, 4, 5,  3, 6, 7, 4};
      PetscInt    coneOrientations[8] = {0, 0, 0, 0,  0, 0, 0, 0};
      PetscScalar vertexCoords[12]    = {-0.5, 0.0, 0.0, 0.0, 0.0, 1.0, -0.5, 1.0, 0.5, 0.0, 0.5, 1.0};
      PetscInt    markerPoints[12]    = {2, 1, 3, 1, 4, 1, 5, 1, 6, 1, 7, 1};
      PetscInt    faultPoints[2]      = {3, 4};

      ierr = DMPlexCreateFromDAG(*dm, 1, numPoints, coneSize, cones, coneOrientations, vertexCoords);CHKERRQ(ierr);
      for (p = 0; p < 6; ++p) {ierr = DMSetLabelValue(*dm, "marker", markerPoints[p*2], markerPoints[p*2+1]);CHKERRQ(ierr);}
      for (p = 0; p < 2; ++p) {ierr = DMSetLabelValue(*dm, "fault", faultPoints[p], 1);CHKERRQ(ierr);}
    }
    break;
    case 1:
    {
      PetscInt    numPoints[2]         = {16, 9};
      PetscInt    coneSize[25]         = {4, 4, 4, 4, 4, 4, 4, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
      PetscInt    cones[36]            = {9,  13, 14, 10,
                                          10, 14, 15, 11,
                                          11, 15, 16, 12,
                                          13, 17, 18, 14,
                                          14, 18, 19, 15,
                                          15, 19, 20, 16,
                                          17, 21, 22, 18,
                                          18, 22, 23, 19,
                                          19, 23, 24, 20};
      PetscInt    coneOrientations[36] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
      PetscScalar vertexCoords[32]     = {-3.0,  3.0,  -1.0,  3.0,  1.0,  3.0,  3.0,  3.0,  -3.0,  1.0,  -1.0,  1.0,  1.0,  1.0,  3.0,  1.0,
                                          -3.0, -1.0,  -1.0, -1.0,  1.0, -1.0,  3.0, -1.0,  -3.0, -3.0,  -1.0, -3.0,  1.0, -3.0,  3.0, -3.0};
      PetscInt    faultPoints[3]       = {11, 15, 19};
      PetscInt    faultBPoints[2]      = {17, 18};

      ierr = DMPlexCreateFromDAG(*dm, 1, numPoints, coneSize, cones, coneOrientations, vertexCoords);CHKERRQ(ierr);
      for (p = 0; p < 3; ++p) {ierr = DMSetLabelValue(*dm, "fault",  faultPoints[p], 1);CHKERRQ(ierr);}
      for (p = 0; p < 2; ++p) {ierr = DMSetLabelValue(*dm, "faultB", faultBPoints[p], 1);CHKERRQ(ierr);}
    }
    break;
    default:
      SETERRQ1(comm, PETSC_ERR_ARG_OUTOFRANGE, "No test mesh %d", testNum);
    }
  } else {
    PetscInt numPoints[3] = {0, 0, 0};

    ierr = DMPlexCreateFromDAG(*dm, 1, numPoints, NULL, NULL, NULL, NULL);CHKERRQ(ierr);
    ierr = DMCreateLabel(*dm, "fault");CHKERRQ(ierr);
  }
  ierr = DMPlexInterpolate(*dm, &idm);CHKERRQ(ierr);
  ierr = DMPlexCopyCoordinates(*dm, idm);CHKERRQ(ierr);
  ierr = DMCopyLabels(*dm, idm);CHKERRQ(ierr);
  ierr = DMViewFromOptions(idm, NULL, "-in_dm_view");CHKERRQ(ierr);
  ierr = DMDestroy(dm);CHKERRQ(ierr);
  *dm  = idm;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "CreateHex_3D"
PetscErrorCode CreateHex_3D(MPI_Comm comm, PetscInt testNum, DM *dm)
{
  DM             idm   = NULL;
  PetscInt       depth = 3, p;
  PetscMPIInt    rank;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MPI_Comm_rank(comm, &rank);CHKERRQ(ierr);
  if (!rank) {
    switch (testNum) {
    case 0:
    {
      PetscInt    numPoints[2]         = {12, 2};
      PetscInt    coneSize[14]         = {8,8, 0,0,0,0,0,0,0,0,0,0,0,0};
      PetscInt    cones[16]            = {2,5,4,3,6,7,8,9,  3,4,11,10,7,12,13,8};
      PetscInt    coneOrientations[16] = {0,0,0,0,0,0,0,0,  0,0, 0,0 ,0, 0, 0,0};
      PetscScalar vertexCoords[36]     = {-0.5,0.0,0.0, 0.0,0.0,0.0, 0.0,1.0,0.0, -0.5,1.0,0.0,
                                          -0.5,0.0,1.0, 0.0,0.0,1.0, 0.0,1.0,1.0, -0.5,1.0,1.0,
                                           0.5,0.0,0.0, 0.5,1.0,0.0, 0.5,0.0,1.0,  0.5,1.0,1.0};
      PetscInt    markerPoints[52]     = {2,1,3,1,4,1,5,1,6,1,7,1,8,1,9,1};
      PetscInt    faultPoints[4]       = {3, 4, 7, 8};

      ierr = DMPlexCreateFromDAG(*dm, 1, numPoints, coneSize, cones, coneOrientations, vertexCoords);CHKERRQ(ierr);
      ierr = DMPlexCheckSymmetry(*dm);CHKERRQ(ierr);
      ierr = DMPlexInterpolate(*dm, &idm);CHKERRQ(ierr);
      ierr = DMPlexCopyCoordinates(*dm, idm);CHKERRQ(ierr);
      for(p = 0; p < 8; ++p) {ierr = DMSetLabelValue(idm, "marker", markerPoints[p*2], markerPoints[p*2+1]);CHKERRQ(ierr);}
      for(p = 0; p < 4; ++p) {ierr = DMSetLabelValue(idm, "fault", faultPoints[p], 1);CHKERRQ(ierr);}
    }
    break;
    case 1:
    {
      /* Cell Adjacency Graph:
        0 -- { 8, 13, 21, 24} --> 1
        0 -- {20, 21, 23, 24} --> 5 F
        1 -- {10, 15, 21, 24} --> 2
        1 -- {13, 14, 15, 24} --> 6
        2 -- {21, 22, 24, 25} --> 4 F
        3 -- {21, 24, 30, 35} --> 4
        3 -- {21, 24, 28, 33} --> 5
       */
      PetscInt    numPoints[2]         = {30, 7};
      PetscInt    coneSize[37]         = {8,8,8,8,8,8,8, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
      PetscInt    cones[56]            = { 8, 21, 20,  7, 13, 12, 23, 24,
                                          14, 15, 10,  9, 13,  8, 21, 24,
                                          15, 16, 11, 10, 24, 21, 22, 25,
                                          30, 29, 28, 21, 35, 24, 33, 34,
                                          24, 21, 30, 35, 25, 36, 31, 22,
                                          27, 20, 21, 28, 32, 33, 24, 23,
                                          15, 24, 13, 14, 19, 18, 17, 26};
      PetscInt    coneOrientations[56] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
      PetscScalar vertexCoords[90]     = {-2.0, -2.0, -2.0,  -2.0, -1.0, -2.0,  -3.0,  0.0, -2.0,  -2.0,  1.0, -2.0,  -2.0,  2.0, -2.0,  -2.0, -2.0,  0.0,
                                          -2.0, -1.0,  0.0,  -3.0,  0.0,  0.0,  -2.0,  1.0,  0.0,  -2.0,  2.0,  0.0,  -2.0, -1.0,  2.0,  -3.0,  0.0,  2.0,
                                          -2.0,  1.0,  2.0,   0.0, -2.0, -2.0,   0.0,  0.0, -2.0,   0.0,  2.0, -2.0,   0.0, -2.0,  0.0,   0.0,  0.0,  0.0,
                                           0.0,  2.0,  0.0,   0.0,  0.0,  2.0,   2.0, -2.0, -2.0,   2.0, -1.0, -2.0,   3.0,  0.0, -2.0,   2.0,  1.0, -2.0,
                                           2.0,  2.0, -2.0,   2.0, -2.0,  0.0,   2.0, -1.0,  0.0,   3.0,  0.0,  0.0,   2.0,  1.0,  0.0,   2.0,  2.0,  0.0};
      PetscInt    faultPoints[6]       = {20, 21, 22, 23, 24, 25};

      ierr = DMPlexCreateFromDAG(*dm, 1, numPoints, coneSize, cones, coneOrientations, vertexCoords);CHKERRQ(ierr);
      ierr = DMPlexCheckSymmetry(*dm);CHKERRQ(ierr);
      ierr = DMPlexInterpolate(*dm, &idm);CHKERRQ(ierr);
      ierr = DMPlexCopyCoordinates(*dm, idm);CHKERRQ(ierr);
      for(p = 0; p < 6; ++p) {ierr = DMSetLabelValue(idm, "fault", faultPoints[p], 1);CHKERRQ(ierr);}
    }
    break;
    case 2:
    {
      /* Buried fault edge */
      PetscInt    numPoints[2]         = {18, 4};
      PetscInt    coneSize[22]         = {8,8,8,8, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
      PetscInt    cones[32]            = { 4,  5,  8,  7, 13, 16, 17, 14,
                                           5,  6,  9,  8, 14, 17, 18, 15,
                                           7,  8, 11, 10, 16, 19, 20, 17,
                                           8,  9, 12, 11, 17, 20, 21, 18};
      PetscInt    coneOrientations[32] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
      PetscScalar vertexCoords[54]     = {-2.0, -2.0,  0.0,  -2.0,  0.0,  0.0,  -2.0,  2.0,  0.0,   0.0, -2.0,  0.0,   0.0,  0.0,  0.0,   0.0,  2.0,  0.0,
                                           2.0, -2.0,  0.0,   2.0,  0.0,  0.0,   2.0,  2.0,  0.0,  -2.0, -2.0,  2.0,  -2.0,  0.0,  2.0,  -2.0,  2.0,  2.0,
                                           0.0, -2.0,  2.0,   0.0,  0.0,  2.0,   0.0,  2.0,  2.0,   2.0, -2.0,  2.0,   2.0,  0.0,  2.0,   2.0,  2.0,  2.0};
      PetscInt    faultPoints[4]       = {7, 8, 16, 17};

      ierr = DMPlexCreateFromDAG(*dm, 1, numPoints, coneSize, cones, coneOrientations, vertexCoords);CHKERRQ(ierr);
      ierr = DMPlexCheckSymmetry(*dm);CHKERRQ(ierr);
      ierr = DMPlexInterpolate(*dm, &idm);CHKERRQ(ierr);
      ierr = DMPlexCopyCoordinates(*dm, idm);CHKERRQ(ierr);
      for(p = 0; p < 4; ++p) {ierr = DMSetLabelValue(idm, "fault", faultPoints[p], 1);CHKERRQ(ierr);}
    }
    break;
    default: SETERRQ1(comm, PETSC_ERR_ARG_OUTOFRANGE, "No test mesh %d", testNum);
    }
  } else {
    PetscInt numPoints[4] = {0, 0, 0, 0};

    ierr = DMPlexCreateFromDAG(*dm, depth, numPoints, NULL, NULL, NULL, NULL);CHKERRQ(ierr);
    ierr = DMPlexInterpolate(*dm, &idm);CHKERRQ(ierr);
    ierr = DMPlexCopyCoordinates(*dm, idm);CHKERRQ(ierr);
    ierr = DMCreateLabel(idm, "fault");CHKERRQ(ierr);
  }
  ierr = DMViewFromOptions(idm, NULL, "-in_dm_view");CHKERRQ(ierr);
  ierr = DMDestroy(dm);CHKERRQ(ierr);
  *dm  = idm;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "CreateMesh"
PetscErrorCode CreateMesh(MPI_Comm comm, AppCtx *user, DM *dm)
{
  PetscInt       dim          = user->dim;
  PetscBool      cellSimplex  = user->cellSimplex, hasFaultB;
  PetscMPIInt    rank;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = MPI_Comm_rank(comm, &rank);CHKERRQ(ierr);
  ierr = DMCreate(comm, dm);CHKERRQ(ierr);
  ierr = DMSetType(*dm, DMPLEX);CHKERRQ(ierr);
  ierr = DMSetDimension(*dm, dim);CHKERRQ(ierr);
  switch (dim) {
  case 2:
    if (cellSimplex) {
      ierr = CreateSimplex_2D(comm, user->testNum, dm);CHKERRQ(ierr);
    } else {
      ierr = CreateQuad_2D(comm, user->testNum, dm);CHKERRQ(ierr);
    }
    break;
  case 3:
    if (cellSimplex) {
      ierr = CreateSimplex_3D(comm, user, *dm);CHKERRQ(ierr);
    } else {
      ierr = CreateHex_3D(comm, user->testNum, dm);CHKERRQ(ierr);
    }
    break;
  default:
    SETERRQ1(comm, PETSC_ERR_ARG_OUTOFRANGE, "Cannot make hybrid meshes for dimension %d", dim);
  }
  ierr = DMPlexCheckSymmetry(*dm);CHKERRQ(ierr);
  ierr = DMPlexCheckSkeleton(*dm, user->cellSimplex, 0);CHKERRQ(ierr);
  ierr = DMPlexCheckFaces(*dm, user->cellSimplex, 0);CHKERRQ(ierr);
  {
    DM      dmHybrid = NULL;
    DMLabel faultLabel, hybridLabel;

    ierr = DMGetLabel(*dm, "fault", &faultLabel);CHKERRQ(ierr);
    ierr = DMPlexCreateHybridMesh(*dm, faultLabel, &hybridLabel, &dmHybrid);CHKERRQ(ierr);
    ierr = DMLabelView(hybridLabel, PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);
    ierr = DMLabelDestroy(&hybridLabel);CHKERRQ(ierr);
    ierr = DMDestroy(dm);CHKERRQ(ierr);
    *dm  = dmHybrid;
  }
  ierr = DMHasLabel(*dm, "faultB", &hasFaultB);CHKERRQ(ierr);
  if (hasFaultB) {
    DM      dmHybrid = NULL;
    DMLabel faultLabel, hybridLabel;

    ierr = DMViewFromOptions(*dm, NULL, "-orig_dm_view");CHKERRQ(ierr);
    ierr = DMPlexCheckSymmetry(*dm);CHKERRQ(ierr);
    ierr = DMPlexCheckSkeleton(*dm, user->cellSimplex, 0);CHKERRQ(ierr);
    ierr = DMPlexCheckFaces(*dm, user->cellSimplex, 0);CHKERRQ(ierr);
    ierr = DMGetLabel(*dm, "faultB", &faultLabel);CHKERRQ(ierr);
    ierr = DMPlexCreateHybridMesh(*dm, faultLabel, &hybridLabel, &dmHybrid);CHKERRQ(ierr);
    ierr = DMLabelView(hybridLabel, PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);
    ierr = DMLabelDestroy(&hybridLabel);CHKERRQ(ierr);
    ierr = DMDestroy(dm);CHKERRQ(ierr);
    *dm  = dmHybrid;
  }
  {
    DM distributedMesh = NULL;

    /* Distribute mesh over processes */
    ierr = DMPlexDistribute(*dm, 0, NULL, &distributedMesh);CHKERRQ(ierr);
    if (distributedMesh) {
      ierr = DMViewFromOptions(distributedMesh, NULL, "-dm_view");CHKERRQ(ierr);
      ierr = DMPlexCheckSymmetry(*dm);CHKERRQ(ierr);
      ierr = DMPlexCheckSkeleton(*dm, user->cellSimplex, 0);CHKERRQ(ierr);
      ierr = DMDestroy(dm);CHKERRQ(ierr);
      *dm  = distributedMesh;
    }
  }
  ierr = PetscObjectSetName((PetscObject) *dm, "Hybrid Mesh");CHKERRQ(ierr);
  ierr = DMViewFromOptions(*dm, NULL, "-dm_view");CHKERRQ(ierr);
  ierr = DMPlexCheckSymmetry(*dm);CHKERRQ(ierr);
  ierr = DMPlexCheckSkeleton(*dm, user->cellSimplex, 0);CHKERRQ(ierr);
  ierr = DMPlexCheckFaces(*dm, user->cellSimplex, 0);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc, char **argv)
{
  DM             dm;
  AppCtx         user;                 /* user-defined work context */
  PetscErrorCode ierr;

  ierr = PetscInitialize(&argc, &argv, NULL, help);CHKERRQ(ierr);
  ierr = ProcessOptions(PETSC_COMM_WORLD, &user);CHKERRQ(ierr);
  ierr = CreateMesh(PETSC_COMM_WORLD, &user, &dm);CHKERRQ(ierr);
  ierr = DMDestroy(&dm);CHKERRQ(ierr);
  ierr = PetscFinalize();
  return 0;
}
