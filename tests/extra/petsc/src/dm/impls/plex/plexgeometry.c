#include <petsc/private/dmpleximpl.h>   /*I      "petscdmplex.h"   I*/

#undef __FUNCT__
#define __FUNCT__ "DMPlexGetLineIntersection_2D_Internal"
static PetscErrorCode DMPlexGetLineIntersection_2D_Internal(const PetscReal segmentA[], const PetscReal segmentB[], PetscReal intersection[], PetscBool *hasIntersection)
{
  const PetscReal p0_x  = segmentA[0*2+0];
  const PetscReal p0_y  = segmentA[0*2+1];
  const PetscReal p1_x  = segmentA[1*2+0];
  const PetscReal p1_y  = segmentA[1*2+1];
  const PetscReal p2_x  = segmentB[0*2+0];
  const PetscReal p2_y  = segmentB[0*2+1];
  const PetscReal p3_x  = segmentB[1*2+0];
  const PetscReal p3_y  = segmentB[1*2+1];
  const PetscReal s1_x  = p1_x - p0_x;
  const PetscReal s1_y  = p1_y - p0_y;
  const PetscReal s2_x  = p3_x - p2_x;
  const PetscReal s2_y  = p3_y - p2_y;
  const PetscReal denom = (-s2_x * s1_y + s1_x * s2_y);

  PetscFunctionBegin;
  *hasIntersection = PETSC_FALSE;
  /* Non-parallel lines */
  if (denom != 0.0) {
    const PetscReal s = (-s1_y * (p0_x - p2_x) + s1_x * (p0_y - p2_y)) / denom;
    const PetscReal t = ( s2_x * (p0_y - p2_y) - s2_y * (p0_x - p2_x)) / denom;

    if (s >= 0 && s <= 1 && t >= 0 && t <= 1) {
      *hasIntersection = PETSC_TRUE;
      if (intersection) {
        intersection[0] = p0_x + (t * s1_x);
        intersection[1] = p0_y + (t * s1_y);
      }
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMPlexLocatePoint_Simplex_2D_Internal"
static PetscErrorCode DMPlexLocatePoint_Simplex_2D_Internal(DM dm, const PetscScalar point[], PetscInt c, PetscInt *cell)
{
  const PetscInt  embedDim = 2;
  const PetscReal eps      = PETSC_SQRT_MACHINE_EPSILON;
  PetscReal       x        = PetscRealPart(point[0]);
  PetscReal       y        = PetscRealPart(point[1]);
  PetscReal       v0[2], J[4], invJ[4], detJ;
  PetscReal       xi, eta;
  PetscErrorCode  ierr;

  PetscFunctionBegin;
  ierr = DMPlexComputeCellGeometryFEM(dm, c, NULL, v0, J, invJ, &detJ);CHKERRQ(ierr);
  xi  = invJ[0*embedDim+0]*(x - v0[0]) + invJ[0*embedDim+1]*(y - v0[1]);
  eta = invJ[1*embedDim+0]*(x - v0[0]) + invJ[1*embedDim+1]*(y - v0[1]);

  if ((xi >= -eps) && (eta >= -eps) && (xi + eta <= 2.0+eps)) *cell = c;
  else *cell = -1;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMPlexLocatePoint_General_2D_Internal"
static PetscErrorCode DMPlexLocatePoint_General_2D_Internal(DM dm, const PetscScalar point[], PetscInt c, PetscInt *cell)
{
  PetscSection       coordSection;
  Vec             coordsLocal;
  PetscScalar    *coords = NULL;
  const PetscInt  faces[8]  = {0, 1, 1, 2, 2, 3, 3, 0};
  PetscReal       x         = PetscRealPart(point[0]);
  PetscReal       y         = PetscRealPart(point[1]);
  PetscInt        crossings = 0, f;
  PetscErrorCode  ierr;

  PetscFunctionBegin;
  ierr = DMGetCoordinatesLocal(dm, &coordsLocal);CHKERRQ(ierr);
  ierr = DMGetCoordinateSection(dm, &coordSection);CHKERRQ(ierr);
  ierr = DMPlexVecGetClosure(dm, coordSection, coordsLocal, c, NULL, &coords);CHKERRQ(ierr);
  for (f = 0; f < 4; ++f) {
    PetscReal x_i   = PetscRealPart(coords[faces[2*f+0]*2+0]);
    PetscReal y_i   = PetscRealPart(coords[faces[2*f+0]*2+1]);
    PetscReal x_j   = PetscRealPart(coords[faces[2*f+1]*2+0]);
    PetscReal y_j   = PetscRealPart(coords[faces[2*f+1]*2+1]);
    PetscReal slope = (y_j - y_i) / (x_j - x_i);
    PetscBool cond1 = (x_i <= x) && (x < x_j) ? PETSC_TRUE : PETSC_FALSE;
    PetscBool cond2 = (x_j <= x) && (x < x_i) ? PETSC_TRUE : PETSC_FALSE;
    PetscBool above = (y < slope * (x - x_i) + y_i) ? PETSC_TRUE : PETSC_FALSE;
    if ((cond1 || cond2)  && above) ++crossings;
  }
  if (crossings % 2) *cell = c;
  else *cell = -1;
  ierr = DMPlexVecRestoreClosure(dm, coordSection, coordsLocal, c, NULL, &coords);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMPlexLocatePoint_Simplex_3D_Internal"
static PetscErrorCode DMPlexLocatePoint_Simplex_3D_Internal(DM dm, const PetscScalar point[], PetscInt c, PetscInt *cell)
{
  const PetscInt embedDim = 3;
  PetscReal      v0[3], J[9], invJ[9], detJ;
  PetscReal      x = PetscRealPart(point[0]);
  PetscReal      y = PetscRealPart(point[1]);
  PetscReal      z = PetscRealPart(point[2]);
  PetscReal      xi, eta, zeta;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = DMPlexComputeCellGeometryFEM(dm, c, NULL, v0, J, invJ, &detJ);CHKERRQ(ierr);
  xi   = invJ[0*embedDim+0]*(x - v0[0]) + invJ[0*embedDim+1]*(y - v0[1]) + invJ[0*embedDim+2]*(z - v0[2]);
  eta  = invJ[1*embedDim+0]*(x - v0[0]) + invJ[1*embedDim+1]*(y - v0[1]) + invJ[1*embedDim+2]*(z - v0[2]);
  zeta = invJ[2*embedDim+0]*(x - v0[0]) + invJ[2*embedDim+1]*(y - v0[1]) + invJ[2*embedDim+2]*(z - v0[2]);

  if ((xi >= 0.0) && (eta >= 0.0) && (zeta >= 0.0) && (xi + eta + zeta <= 2.0)) *cell = c;
  else *cell = -1;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMPlexLocatePoint_General_3D_Internal"
static PetscErrorCode DMPlexLocatePoint_General_3D_Internal(DM dm, const PetscScalar point[], PetscInt c, PetscInt *cell)
{
  PetscSection   coordSection;
  Vec            coordsLocal;
  PetscScalar   *coords;
  const PetscInt faces[24] = {0, 3, 2, 1,  5, 4, 7, 6,  3, 0, 4, 5,
                              1, 2, 6, 7,  3, 5, 6, 2,  0, 1, 7, 4};
  PetscBool      found = PETSC_TRUE;
  PetscInt       f;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = DMGetCoordinatesLocal(dm, &coordsLocal);CHKERRQ(ierr);
  ierr = DMGetCoordinateSection(dm, &coordSection);CHKERRQ(ierr);
  ierr = DMPlexVecGetClosure(dm, coordSection, coordsLocal, c, NULL, &coords);CHKERRQ(ierr);
  for (f = 0; f < 6; ++f) {
    /* Check the point is under plane */
    /*   Get face normal */
    PetscReal v_i[3];
    PetscReal v_j[3];
    PetscReal normal[3];
    PetscReal pp[3];
    PetscReal dot;

    v_i[0]    = PetscRealPart(coords[faces[f*4+3]*3+0]-coords[faces[f*4+0]*3+0]);
    v_i[1]    = PetscRealPart(coords[faces[f*4+3]*3+1]-coords[faces[f*4+0]*3+1]);
    v_i[2]    = PetscRealPart(coords[faces[f*4+3]*3+2]-coords[faces[f*4+0]*3+2]);
    v_j[0]    = PetscRealPart(coords[faces[f*4+1]*3+0]-coords[faces[f*4+0]*3+0]);
    v_j[1]    = PetscRealPart(coords[faces[f*4+1]*3+1]-coords[faces[f*4+0]*3+1]);
    v_j[2]    = PetscRealPart(coords[faces[f*4+1]*3+2]-coords[faces[f*4+0]*3+2]);
    normal[0] = v_i[1]*v_j[2] - v_i[2]*v_j[1];
    normal[1] = v_i[2]*v_j[0] - v_i[0]*v_j[2];
    normal[2] = v_i[0]*v_j[1] - v_i[1]*v_j[0];
    pp[0]     = PetscRealPart(coords[faces[f*4+0]*3+0] - point[0]);
    pp[1]     = PetscRealPart(coords[faces[f*4+0]*3+1] - point[1]);
    pp[2]     = PetscRealPart(coords[faces[f*4+0]*3+2] - point[2]);
    dot       = normal[0]*pp[0] + normal[1]*pp[1] + normal[2]*pp[2];

    /* Check that projected point is in face (2D location problem) */
    if (dot < 0.0) {
      found = PETSC_FALSE;
      break;
    }
  }
  if (found) *cell = c;
  else *cell = -1;
  ierr = DMPlexVecRestoreClosure(dm, coordSection, coordsLocal, c, NULL, &coords);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscGridHashInitialize_Internal"
static PetscErrorCode PetscGridHashInitialize_Internal(PetscGridHash box, PetscInt dim, const PetscScalar point[])
{
  PetscInt d;

  PetscFunctionBegin;
  box->dim = dim;
  for (d = 0; d < dim; ++d) box->lower[d] = box->upper[d] = PetscRealPart(point[d]);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscGridHashCreate"
PetscErrorCode PetscGridHashCreate(MPI_Comm comm, PetscInt dim, const PetscScalar point[], PetscGridHash *box)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscMalloc1(1, box);CHKERRQ(ierr);
  ierr = PetscGridHashInitialize_Internal(*box, dim, point);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscGridHashEnlarge"
PetscErrorCode PetscGridHashEnlarge(PetscGridHash box, const PetscScalar point[])
{
  PetscInt d;

  PetscFunctionBegin;
  for (d = 0; d < box->dim; ++d) {
    box->lower[d] = PetscMin(box->lower[d], PetscRealPart(point[d]));
    box->upper[d] = PetscMax(box->upper[d], PetscRealPart(point[d]));
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscGridHashSetGrid"
PetscErrorCode PetscGridHashSetGrid(PetscGridHash box, const PetscInt n[], const PetscReal h[])
{
  PetscInt d;

  PetscFunctionBegin;
  for (d = 0; d < box->dim; ++d) {
    box->extent[d] = box->upper[d] - box->lower[d];
    if (n[d] == PETSC_DETERMINE) {
      box->h[d] = h[d];
      box->n[d] = PetscCeilReal(box->extent[d]/h[d]);
    } else {
      box->n[d] = n[d];
      box->h[d] = box->extent[d]/n[d];
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscGridHashGetEnclosingBox"
PetscErrorCode PetscGridHashGetEnclosingBox(PetscGridHash box, PetscInt numPoints, const PetscScalar points[], PetscInt dboxes[], PetscInt boxes[])
{
  const PetscReal *lower = box->lower;
  const PetscReal *upper = box->upper;
  const PetscReal *h     = box->h;
  const PetscInt  *n     = box->n;
  const PetscInt   dim   = box->dim;
  PetscInt         d, p;

  PetscFunctionBegin;
  for (p = 0; p < numPoints; ++p) {
    for (d = 0; d < dim; ++d) {
      PetscInt dbox = PetscFloorReal((PetscRealPart(points[p*dim+d]) - lower[d])/h[d]);

      if (dbox == n[d] && PetscAbsReal(PetscRealPart(points[p*dim+d]) - upper[d]) < 1.0e-9) dbox = n[d]-1;
      if (dbox < 0 || dbox >= n[d]) SETERRQ4(PETSC_COMM_SELF, PETSC_ERR_ARG_WRONG, "Input point %d (%g, %g, %g) is outside of our bounding box",
                                             p, PetscRealPart(points[p*dim+0]), dim > 1 ? PetscRealPart(points[p*dim+1]) : 0.0, dim > 2 ? PetscRealPart(points[p*dim+2]) : 0.0);
      dboxes[p*dim+d] = dbox;
    }
    if (boxes) for (d = 1, boxes[p] = dboxes[p*dim]; d < dim; ++d) boxes[p] += dboxes[p*dim+d]*n[d-1];
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscGridHashDestroy"
PetscErrorCode PetscGridHashDestroy(PetscGridHash *box)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (*box) {
    ierr = PetscSectionDestroy(&(*box)->cellSection);CHKERRQ(ierr);
    ierr = ISDestroy(&(*box)->cells);CHKERRQ(ierr);
    ierr = DMLabelDestroy(&(*box)->cellsSparse);CHKERRQ(ierr);
  }
  ierr = PetscFree(*box);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMPlexLocatePoint_Internal"
PetscErrorCode DMPlexLocatePoint_Internal(DM dm, PetscInt dim, const PetscScalar point[], PetscInt cellStart, PetscInt *cell)
{
  PetscInt       coneSize;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  switch (dim) {
  case 2:
    ierr = DMPlexGetConeSize(dm, cellStart, &coneSize);CHKERRQ(ierr);
    switch (coneSize) {
    case 3:
      ierr = DMPlexLocatePoint_Simplex_2D_Internal(dm, point, cellStart, cell);CHKERRQ(ierr);
      break;
    case 4:
      ierr = DMPlexLocatePoint_General_2D_Internal(dm, point, cellStart, cell);CHKERRQ(ierr);
      break;
    default:
      SETERRQ1(PetscObjectComm((PetscObject)dm), PETSC_ERR_ARG_OUTOFRANGE, "No point location for cell with cone size %D", coneSize);
    }
    break;
  case 3:
    ierr = DMPlexGetConeSize(dm, cellStart, &coneSize);CHKERRQ(ierr);
    switch (coneSize) {
    case 4:
      ierr = DMPlexLocatePoint_Simplex_3D_Internal(dm, point, cellStart, cell);CHKERRQ(ierr);
      break;
    case 6:
      ierr = DMPlexLocatePoint_General_3D_Internal(dm, point, cellStart, cell);CHKERRQ(ierr);
      break;
    default:
      SETERRQ1(PetscObjectComm((PetscObject)dm), PETSC_ERR_ARG_OUTOFRANGE, "No point location for cell with cone size %D", coneSize);
    }
    break;
  default:
    SETERRQ1(PetscObjectComm((PetscObject)dm), PETSC_ERR_ARG_OUTOFRANGE, "No point location for mesh dimension %D", dim);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMPlexComputeGridHash_Internal"
PetscErrorCode DMPlexComputeGridHash_Internal(DM dm, PetscGridHash *localBox)
{
  MPI_Comm           comm;
  PetscGridHash      lbox;
  Vec                coordinates;
  PetscSection       coordSection;
  Vec                coordsLocal;
  const PetscScalar *coords;
  PetscInt          *dboxes, *boxes;
  PetscInt           n[3] = {10, 10, 10};
  PetscInt           dim, N, cStart, cEnd, cMax, c, i;
  PetscErrorCode     ierr;

  PetscFunctionBegin;
  ierr = PetscObjectGetComm((PetscObject) dm, &comm);CHKERRQ(ierr);
  ierr = DMGetCoordinatesLocal(dm, &coordinates);CHKERRQ(ierr);
  ierr = DMGetCoordinateDim(dm, &dim);CHKERRQ(ierr);
  ierr = VecGetLocalSize(coordinates, &N);CHKERRQ(ierr);
  ierr = VecGetArrayRead(coordinates, &coords);CHKERRQ(ierr);
  ierr = PetscGridHashCreate(comm, dim, coords, &lbox);CHKERRQ(ierr);
  for (i = 0; i < N; i += dim) {ierr = PetscGridHashEnlarge(lbox, &coords[i]);CHKERRQ(ierr);}
  ierr = VecRestoreArrayRead(coordinates, &coords);CHKERRQ(ierr);
  ierr = PetscGridHashSetGrid(lbox, n, NULL);CHKERRQ(ierr);
#if 0
  /* Could define a custom reduction to merge these */
  ierr = MPIU_Allreduce(lbox->lower, gbox->lower, 3, MPIU_REAL, MPI_MIN, comm);CHKERRQ(ierr);
  ierr = MPIU_Allreduce(lbox->upper, gbox->upper, 3, MPIU_REAL, MPI_MAX, comm);CHKERRQ(ierr);
#endif
  /* Is there a reason to snap the local bounding box to a division of the global box? */
  /* Should we compute all overlaps of local boxes? We could do this with a rendevouz scheme partitioning the global box */
  /* Create label */
  ierr = DMPlexGetHeightStratum(dm, 0, &cStart, &cEnd);CHKERRQ(ierr);
  ierr = DMPlexGetHybridBounds(dm, &cMax, NULL, NULL, NULL);CHKERRQ(ierr);
  if (cMax >= 0) cEnd = PetscMin(cEnd, cMax);
  ierr = DMLabelCreate("cells", &lbox->cellsSparse);CHKERRQ(ierr);
  ierr = DMLabelCreateIndex(lbox->cellsSparse, cStart, cEnd);CHKERRQ(ierr);
  /* Compute boxes which overlap each cell: http://stackoverflow.com/questions/13790208/triangle-square-intersection-test-in-2d */
  ierr = DMGetCoordinatesLocal(dm, &coordsLocal);CHKERRQ(ierr);
  ierr = DMGetCoordinateSection(dm, &coordSection);CHKERRQ(ierr);
  ierr = PetscCalloc2(16 * dim, &dboxes, 16, &boxes);CHKERRQ(ierr);
  for (c = cStart; c < cEnd; ++c) {
    const PetscReal *h       = lbox->h;
    PetscScalar     *ccoords = NULL;
    PetscInt         csize   = 0;
    PetscScalar      point[3];
    PetscInt         dlim[6], d, e, i, j, k;

    /* Find boxes enclosing each vertex */
    ierr = DMPlexVecGetClosure(dm, coordSection, coordsLocal, c, &csize, &ccoords);CHKERRQ(ierr);
    ierr = PetscGridHashGetEnclosingBox(lbox, csize/dim, ccoords, dboxes, boxes);CHKERRQ(ierr);
    /* Mark cells containing the vertices */
    for (e = 0; e < csize/dim; ++e) {ierr = DMLabelSetValue(lbox->cellsSparse, c, boxes[e]);CHKERRQ(ierr);}
    /* Get grid of boxes containing these */
    for (d = 0;   d < dim; ++d) {dlim[d*2+0] = dlim[d*2+1] = dboxes[d];}
    for (d = dim; d < 3;   ++d) {dlim[d*2+0] = dlim[d*2+1] = 0;}
    for (e = 1; e < dim+1; ++e) {
      for (d = 0; d < dim; ++d) {
        dlim[d*2+0] = PetscMin(dlim[d*2+0], dboxes[e*dim+d]);
        dlim[d*2+1] = PetscMax(dlim[d*2+1], dboxes[e*dim+d]);
      }
    }
    /* Check for intersection of box with cell */
    for (k = dlim[2*2+0], point[2] = lbox->lower[2] + k*h[2]; k <= dlim[2*2+1]; ++k, point[2] += h[2]) {
      for (j = dlim[1*2+0], point[1] = lbox->lower[1] + j*h[1]; j <= dlim[1*2+1]; ++j, point[1] += h[1]) {
        for (i = dlim[0*2+0], point[0] = lbox->lower[0] + i*h[0]; i <= dlim[0*2+1]; ++i, point[0] += h[0]) {
          const PetscInt box = (k*lbox->n[1] + j)*lbox->n[0] + i;
          PetscScalar    cpoint[3];
          PetscInt       cell, edge, ii, jj, kk;

          /* Check whether cell contains any vertex of these subboxes TODO vectorize this */
          for (kk = 0, cpoint[2] = point[2]; kk < (dim > 2 ? 2 : 1); ++kk, cpoint[2] += h[2]) {
            for (jj = 0, cpoint[1] = point[1]; jj < (dim > 1 ? 2 : 1); ++jj, cpoint[1] += h[1]) {
              for (ii = 0, cpoint[0] = point[0]; ii < 2; ++ii, cpoint[0] += h[0]) {

                ierr = DMPlexLocatePoint_Internal(dm, dim, cpoint, c, &cell);CHKERRQ(ierr);
                if (cell >= 0) {DMLabelSetValue(lbox->cellsSparse, c, box);CHKERRQ(ierr); ii = jj = kk = 2;}
              }
            }
          }
          /* Check whether cell edge intersects any edge of these subboxes TODO vectorize this */
          for (edge = 0; edge < dim+1; ++edge) {
            PetscReal segA[6], segB[6];

            for (d = 0; d < dim; ++d) {segA[d] = PetscRealPart(ccoords[edge*dim+d]); segA[dim+d] = PetscRealPart(ccoords[((edge+1)%(dim+1))*dim+d]);}
            for (kk = 0; kk < (dim > 2 ? 2 : 1); ++kk) {
              if (dim > 2) {segB[2]     = PetscRealPart(point[2]);
                            segB[dim+2] = PetscRealPart(point[2]) + kk*h[2];}
              for (jj = 0; jj < (dim > 1 ? 2 : 1); ++jj) {
                if (dim > 1) {segB[1]     = PetscRealPart(point[1]);
                              segB[dim+1] = PetscRealPart(point[1]) + jj*h[1];}
                for (ii = 0; ii < 2; ++ii) {
                  PetscBool intersects;

                  segB[0]     = PetscRealPart(point[0]);
                  segB[dim+0] = PetscRealPart(point[0]) + ii*h[0];
                  ierr = DMPlexGetLineIntersection_2D_Internal(segA, segB, NULL, &intersects);CHKERRQ(ierr);
                  if (intersects) {DMLabelSetValue(lbox->cellsSparse, c, box);CHKERRQ(ierr); edge = ii = jj = kk = dim+1;}
                }
              }
            }
          }
        }
      }
    }
    ierr = DMPlexVecRestoreClosure(dm, coordSection, coordsLocal, c, NULL, &ccoords);CHKERRQ(ierr);
  }
  ierr = PetscFree2(dboxes, boxes);CHKERRQ(ierr);
  ierr = DMLabelConvertToSection(lbox->cellsSparse, &lbox->cellSection, &lbox->cells);CHKERRQ(ierr);
  ierr = DMLabelDestroy(&lbox->cellsSparse);CHKERRQ(ierr);
  *localBox = lbox;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMLocatePoints_Plex"
PetscErrorCode DMLocatePoints_Plex(DM dm, Vec v, IS *cellIS)
{
  DM_Plex        *mesh = (DM_Plex *) dm->data;
  PetscBool       hash = mesh->useHashLocation;
  PetscInt        bs, numPoints, p;
  PetscInt        dim, cStart, cEnd, cMax, numCells, c;
  const PetscInt *boxCells;
  PetscInt       *cells;
  PetscScalar    *a;
  PetscErrorCode  ierr;

  PetscFunctionBegin;
  ierr = DMGetCoordinateDim(dm, &dim);CHKERRQ(ierr);
  ierr = VecGetBlockSize(v, &bs);CHKERRQ(ierr);
  if (bs != dim) SETERRQ2(PetscObjectComm((PetscObject)dm), PETSC_ERR_ARG_WRONG, "Block size for point vector %D must be the mesh coordinate dimension %D", bs, dim);
  ierr = DMPlexGetHeightStratum(dm, 0, &cStart, &cEnd);CHKERRQ(ierr);
  ierr = DMPlexGetHybridBounds(dm, &cMax, NULL, NULL, NULL);CHKERRQ(ierr);
  if (cMax >= 0) cEnd = PetscMin(cEnd, cMax);
  ierr = VecGetLocalSize(v, &numPoints);CHKERRQ(ierr);
  ierr = VecGetArray(v, &a);CHKERRQ(ierr);
  numPoints /= bs;
  ierr = PetscMalloc1(numPoints, &cells);CHKERRQ(ierr);
  if (hash) {
    if (!mesh->lbox) {ierr = DMPlexComputeGridHash_Internal(dm, &mesh->lbox);CHKERRQ(ierr);}
    /* Designate the local box for each point */
    /* Send points to correct process */
    /* Search cells that lie in each subbox */
    /*   Should we bin points before doing search? */
    ierr = ISGetIndices(mesh->lbox->cells, &boxCells);CHKERRQ(ierr);
  }
  for (p = 0; p < numPoints; ++p) {
    const PetscScalar *point = &a[p*bs];
    PetscInt           dbin[3], bin, cell = -1, cellOffset;

    if (hash) {
      ierr = PetscGridHashGetEnclosingBox(mesh->lbox, 1, point, dbin, &bin);CHKERRQ(ierr);
      /* TODO Lay an interface over this so we can switch between Section (dense) and Label (sparse) */
      ierr = PetscSectionGetDof(mesh->lbox->cellSection, bin, &numCells);CHKERRQ(ierr);
      ierr = PetscSectionGetOffset(mesh->lbox->cellSection, bin, &cellOffset);CHKERRQ(ierr);
      for (c = cellOffset; c < cellOffset + numCells; ++c) {
        ierr = DMPlexLocatePoint_Internal(dm, dim, point, boxCells[c], &cell);CHKERRQ(ierr);
        if (cell >= 0) break;
      }
      if (cell < 0) SETERRQ1(PETSC_COMM_SELF, PETSC_ERR_ARG_WRONG, "Point %D not found in mesh", p);
    } else {
      for (c = cStart; c < cEnd; ++c) {
        ierr = DMPlexLocatePoint_Internal(dm, dim, point, c, &cell);CHKERRQ(ierr);
        if (cell >= 0) break;
      }
    }
    cells[p] = cell;
  }
  if (hash) {ierr = ISRestoreIndices(mesh->lbox->cells, &boxCells);CHKERRQ(ierr);}
  /* Check for highest numbered proc that claims a point (do we care?) */
  ierr = VecRestoreArray(v, &a);CHKERRQ(ierr);
  ierr = ISCreateGeneral(PETSC_COMM_SELF, numPoints, cells, PETSC_OWN_POINTER, cellIS);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMPlexComputeProjection2Dto1D_Internal"
/*
  DMPlexComputeProjection2Dto1D_Internal - Rewrite coordinates to be the 1D projection of the 2D
*/
PetscErrorCode DMPlexComputeProjection2Dto1D_Internal(PetscScalar coords[], PetscReal R[])
{
  const PetscReal x = PetscRealPart(coords[2] - coords[0]);
  const PetscReal y = PetscRealPart(coords[3] - coords[1]);
  const PetscReal r = PetscSqrtReal(x*x + y*y), c = x/r, s = y/r;

  PetscFunctionBegin;
  R[0] = c; R[1] = -s;
  R[2] = s; R[3] =  c;
  coords[0] = 0.0;
  coords[1] = r;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMPlexComputeProjection3Dto1D_Internal"
/*
  DMPlexComputeProjection3Dto1D_Internal - Rewrite coordinates to be the 1D projection of the 3D

  This uses the basis completion described by Frisvad,

  http://www.imm.dtu.dk/~jerf/papers/abstracts/onb.html
  DOI:10.1080/2165347X.2012.689606
*/
PetscErrorCode DMPlexComputeProjection3Dto1D_Internal(PetscScalar coords[], PetscReal R[])
{
  PetscReal      x    = PetscRealPart(coords[3] - coords[0]);
  PetscReal      y    = PetscRealPart(coords[4] - coords[1]);
  PetscReal      z    = PetscRealPart(coords[5] - coords[2]);
  PetscReal      r    = PetscSqrtReal(x*x + y*y + z*z);
  PetscReal      rinv = 1. / r;
  PetscFunctionBegin;

  x *= rinv; y *= rinv; z *= rinv;
  if (x > 0.) {
    PetscReal inv1pX   = 1./ (1. + x);

    R[0] = x; R[1] = -y;              R[2] = -z;
    R[3] = y; R[4] = 1. - y*y*inv1pX; R[5] =     -y*z*inv1pX;
    R[6] = z; R[7] =     -y*z*inv1pX; R[8] = 1. - z*z*inv1pX;
  }
  else {
    PetscReal inv1mX   = 1./ (1. - x);

    R[0] = x; R[1] = z;               R[2] = y;
    R[3] = y; R[4] =     -y*z*inv1mX; R[5] = 1. - y*y*inv1mX;
    R[6] = z; R[7] = 1. - z*z*inv1mX; R[8] =     -y*z*inv1mX;
  }
  coords[0] = 0.0;
  coords[1] = r;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMPlexComputeProjection3Dto2D_Internal"
/*
  DMPlexComputeProjection3Dto2D_Internal - Rewrite coordinates to be the 2D projection of the 3D
*/
PetscErrorCode DMPlexComputeProjection3Dto2D_Internal(PetscInt coordSize, PetscScalar coords[], PetscReal R[])
{
  PetscReal      x1[3],  x2[3], n[3], norm;
  PetscReal      x1p[3], x2p[3], xnp[3];
  PetscReal      sqrtz, alpha;
  const PetscInt dim = 3;
  PetscInt       d, e, p;

  PetscFunctionBegin;
  /* 0) Calculate normal vector */
  for (d = 0; d < dim; ++d) {
    x1[d] = PetscRealPart(coords[1*dim+d] - coords[0*dim+d]);
    x2[d] = PetscRealPart(coords[2*dim+d] - coords[0*dim+d]);
  }
  n[0] = x1[1]*x2[2] - x1[2]*x2[1];
  n[1] = x1[2]*x2[0] - x1[0]*x2[2];
  n[2] = x1[0]*x2[1] - x1[1]*x2[0];
  norm = PetscSqrtReal(n[0]*n[0] + n[1]*n[1] + n[2]*n[2]);
  n[0] /= norm;
  n[1] /= norm;
  n[2] /= norm;
  /* 1) Take the normal vector and rotate until it is \hat z

    Let the normal vector be <nx, ny, nz> and alpha = 1/sqrt(1 - nz^2), then

    R = /  alpha nx nz  alpha ny nz -1/alpha \
        | -alpha ny     alpha nx        0    |
        \     nx            ny         nz    /

    will rotate the normal vector to \hat z
  */
  sqrtz = PetscSqrtReal(1.0 - n[2]*n[2]);
  /* Check for n = z */
  if (sqrtz < 1.0e-10) {
    if (n[2] < 0.0) {
      if (coordSize > 9) {
        coords[2] = PetscRealPart(coords[3*dim+0] - coords[0*dim+0]);
        coords[3] = PetscRealPart(coords[3*dim+1] - coords[0*dim+1]);
        coords[4] = x2[0];
        coords[5] = x2[1];
        coords[6] = x1[0];
        coords[7] = x1[1];
      } else {
        coords[2] = x2[0];
        coords[3] = x2[1];
        coords[4] = x1[0];
        coords[5] = x1[1];
      }
      R[0] = 1.0; R[1] = 0.0; R[2] = 0.0;
      R[3] = 0.0; R[4] = 1.0; R[5] = 0.0;
      R[6] = 0.0; R[7] = 0.0; R[8] = -1.0;
    } else {
      for (p = 3; p < coordSize/3; ++p) {
        coords[p*2+0] = PetscRealPart(coords[p*dim+0] - coords[0*dim+0]);
        coords[p*2+1] = PetscRealPart(coords[p*dim+1] - coords[0*dim+1]);
      }
      coords[2] = x1[0];
      coords[3] = x1[1];
      coords[4] = x2[0];
      coords[5] = x2[1];
      R[0] = 1.0; R[1] = 0.0; R[2] = 0.0;
      R[3] = 0.0; R[4] = 1.0; R[5] = 0.0;
      R[6] = 0.0; R[7] = 0.0; R[8] = 1.0;
    }
    coords[0] = 0.0;
    coords[1] = 0.0;
    PetscFunctionReturn(0);
  }
  alpha = 1.0/sqrtz;
  R[0] =  alpha*n[0]*n[2]; R[1] = alpha*n[1]*n[2]; R[2] = -sqrtz;
  R[3] = -alpha*n[1];      R[4] = alpha*n[0];      R[5] = 0.0;
  R[6] =  n[0];            R[7] = n[1];            R[8] = n[2];
  for (d = 0; d < dim; ++d) {
    x1p[d] = 0.0;
    x2p[d] = 0.0;
    for (e = 0; e < dim; ++e) {
      x1p[d] += R[d*dim+e]*x1[e];
      x2p[d] += R[d*dim+e]*x2[e];
    }
  }
  if (PetscAbsReal(x1p[2]) > 1.0e-9) SETERRQ(PETSC_COMM_SELF, PETSC_ERR_PLIB, "Invalid rotation calculated");
  if (PetscAbsReal(x2p[2]) > 1.0e-9) SETERRQ(PETSC_COMM_SELF, PETSC_ERR_PLIB, "Invalid rotation calculated");
  /* 2) Project to (x, y) */
  for (p = 3; p < coordSize/3; ++p) {
    for (d = 0; d < dim; ++d) {
      xnp[d] = 0.0;
      for (e = 0; e < dim; ++e) {
        xnp[d] += R[d*dim+e]*PetscRealPart(coords[p*dim+e] - coords[0*dim+e]);
      }
      if (d < dim-1) coords[p*2+d] = xnp[d];
    }
  }
  coords[0] = 0.0;
  coords[1] = 0.0;
  coords[2] = x1p[0];
  coords[3] = x1p[1];
  coords[4] = x2p[0];
  coords[5] = x2p[1];
  /* Output R^T which rotates \hat z to the input normal */
  for (d = 0; d < dim; ++d) {
    for (e = d+1; e < dim; ++e) {
      PetscReal tmp;

      tmp        = R[d*dim+e];
      R[d*dim+e] = R[e*dim+d];
      R[e*dim+d] = tmp;
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "Volume_Triangle_Internal"
PETSC_UNUSED
PETSC_STATIC_INLINE void Volume_Triangle_Internal(PetscReal *vol, PetscReal coords[])
{
  /* Signed volume is 1/2 the determinant

   |  1  1  1 |
   | x0 x1 x2 |
   | y0 y1 y2 |

     but if x0,y0 is the origin, we have

   | x1 x2 |
   | y1 y2 |
  */
  const PetscReal x1 = coords[2] - coords[0], y1 = coords[3] - coords[1];
  const PetscReal x2 = coords[4] - coords[0], y2 = coords[5] - coords[1];
  PetscReal       M[4], detM;
  M[0] = x1; M[1] = x2;
  M[2] = y1; M[3] = y2;
  DMPlex_Det2D_Internal(&detM, M);
  *vol = 0.5*detM;
  (void)PetscLogFlops(5.0);
}

#undef __FUNCT__
#define __FUNCT__ "Volume_Triangle_Origin_Internal"
PETSC_STATIC_INLINE void Volume_Triangle_Origin_Internal(PetscReal *vol, PetscReal coords[])
{
  DMPlex_Det2D_Internal(vol, coords);
  *vol *= 0.5;
}

#undef __FUNCT__
#define __FUNCT__ "Volume_Tetrahedron_Internal"
PETSC_UNUSED
PETSC_STATIC_INLINE void Volume_Tetrahedron_Internal(PetscReal *vol, PetscReal coords[])
{
  /* Signed volume is 1/6th of the determinant

   |  1  1  1  1 |
   | x0 x1 x2 x3 |
   | y0 y1 y2 y3 |
   | z0 z1 z2 z3 |

     but if x0,y0,z0 is the origin, we have

   | x1 x2 x3 |
   | y1 y2 y3 |
   | z1 z2 z3 |
  */
  const PetscReal x1 = coords[3] - coords[0], y1 = coords[4]  - coords[1], z1 = coords[5]  - coords[2];
  const PetscReal x2 = coords[6] - coords[0], y2 = coords[7]  - coords[1], z2 = coords[8]  - coords[2];
  const PetscReal x3 = coords[9] - coords[0], y3 = coords[10] - coords[1], z3 = coords[11] - coords[2];
  PetscReal       M[9], detM;
  M[0] = x1; M[1] = x2; M[2] = x3;
  M[3] = y1; M[4] = y2; M[5] = y3;
  M[6] = z1; M[7] = z2; M[8] = z3;
  DMPlex_Det3D_Internal(&detM, M);
  *vol = -0.16666666666666666666666*detM;
  (void)PetscLogFlops(10.0);
}

#undef __FUNCT__
#define __FUNCT__ "Volume_Tetrahedron_Origin_Internal"
PETSC_STATIC_INLINE void Volume_Tetrahedron_Origin_Internal(PetscReal *vol, PetscReal coords[])
{
  DMPlex_Det3D_Internal(vol, coords);
  *vol *= -0.16666666666666666666666;
}

#undef __FUNCT__
#define __FUNCT__ "DMPlexComputeLineGeometry_Internal"
static PetscErrorCode DMPlexComputeLineGeometry_Internal(DM dm, PetscInt e, PetscReal v0[], PetscReal J[], PetscReal invJ[], PetscReal *detJ)
{
  PetscSection   coordSection;
  Vec            coordinates;
  PetscScalar   *coords = NULL;
  PetscInt       numCoords, d;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = DMGetCoordinatesLocal(dm, &coordinates);CHKERRQ(ierr);
  ierr = DMGetCoordinateSection(dm, &coordSection);CHKERRQ(ierr);
  ierr = DMPlexVecGetClosure(dm, coordSection, coordinates, e, &numCoords, &coords);CHKERRQ(ierr);
  *detJ = 0.0;
  if (numCoords == 6) {
    const PetscInt dim = 3;
    PetscReal      R[9], J0;

    if (v0)   {for (d = 0; d < dim; d++) v0[d] = PetscRealPart(coords[d]);}
    ierr = DMPlexComputeProjection3Dto1D_Internal(coords, R);CHKERRQ(ierr);
    if (J)    {
      J0   = 0.5*PetscRealPart(coords[1]);
      J[0] = R[0]*J0; J[1] = R[1]; J[2] = R[2];
      J[3] = R[3]*J0; J[4] = R[4]; J[5] = R[5];
      J[6] = R[6]*J0; J[7] = R[7]; J[8] = R[8];
      DMPlex_Det3D_Internal(detJ, J);
    }
    if (invJ) {DMPlex_Invert2D_Internal(invJ, J, *detJ);}
  } else if (numCoords == 4) {
    const PetscInt dim = 2;
    PetscReal      R[4], J0;

    if (v0)   {for (d = 0; d < dim; d++) v0[d] = PetscRealPart(coords[d]);}
    ierr = DMPlexComputeProjection2Dto1D_Internal(coords, R);CHKERRQ(ierr);
    if (J)    {
      J0   = 0.5*PetscRealPart(coords[1]);
      J[0] = R[0]*J0; J[1] = R[1];
      J[2] = R[2]*J0; J[3] = R[3];
      DMPlex_Det2D_Internal(detJ, J);
    }
    if (invJ) {DMPlex_Invert2D_Internal(invJ, J, *detJ);}
  } else if (numCoords == 2) {
    const PetscInt dim = 1;

    if (v0)   {for (d = 0; d < dim; d++) v0[d] = PetscRealPart(coords[d]);}
    if (J)    {
      J[0]  = 0.5*(PetscRealPart(coords[1]) - PetscRealPart(coords[0]));
      *detJ = J[0];
      ierr = PetscLogFlops(2.0);CHKERRQ(ierr);
    }
    if (invJ) {invJ[0] = 1.0/J[0]; ierr = PetscLogFlops(1.0);CHKERRQ(ierr);}
  } else SETERRQ1(PETSC_COMM_SELF, PETSC_ERR_ARG_WRONG, "The number of coordinates for this segment is %D != 2", numCoords);
  ierr = DMPlexVecRestoreClosure(dm, coordSection, coordinates, e, &numCoords, &coords);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMPlexComputeTriangleGeometry_Internal"
static PetscErrorCode DMPlexComputeTriangleGeometry_Internal(DM dm, PetscInt e, PetscReal v0[], PetscReal J[], PetscReal invJ[], PetscReal *detJ)
{
  PetscSection   coordSection;
  Vec            coordinates;
  PetscScalar   *coords = NULL;
  PetscInt       numCoords, d, f, g;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = DMGetCoordinatesLocal(dm, &coordinates);CHKERRQ(ierr);
  ierr = DMGetCoordinateSection(dm, &coordSection);CHKERRQ(ierr);
  ierr = DMPlexVecGetClosure(dm, coordSection, coordinates, e, &numCoords, &coords);CHKERRQ(ierr);
  *detJ = 0.0;
  if (numCoords == 9) {
    const PetscInt dim = 3;
    PetscReal      R[9], J0[9] = {1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0};

    if (v0)   {for (d = 0; d < dim; d++) v0[d] = PetscRealPart(coords[d]);}
    ierr = DMPlexComputeProjection3Dto2D_Internal(numCoords, coords, R);CHKERRQ(ierr);
    if (J)    {
      const PetscInt pdim = 2;

      for (d = 0; d < pdim; d++) {
        for (f = 0; f < pdim; f++) {
          J0[d*dim+f] = 0.5*(PetscRealPart(coords[(f+1)*pdim+d]) - PetscRealPart(coords[0*pdim+d]));
        }
      }
      ierr = PetscLogFlops(8.0);CHKERRQ(ierr);
      DMPlex_Det3D_Internal(detJ, J0);
      for (d = 0; d < dim; d++) {
        for (f = 0; f < dim; f++) {
          J[d*dim+f] = 0.0;
          for (g = 0; g < dim; g++) {
            J[d*dim+f] += R[d*dim+g]*J0[g*dim+f];
          }
        }
      }
      ierr = PetscLogFlops(18.0);CHKERRQ(ierr);
    }
    if (invJ) {DMPlex_Invert3D_Internal(invJ, J, *detJ);}
  } else if (numCoords == 6) {
    const PetscInt dim = 2;

    if (v0)   {for (d = 0; d < dim; d++) v0[d] = PetscRealPart(coords[d]);}
    if (J)    {
      for (d = 0; d < dim; d++) {
        for (f = 0; f < dim; f++) {
          J[d*dim+f] = 0.5*(PetscRealPart(coords[(f+1)*dim+d]) - PetscRealPart(coords[0*dim+d]));
        }
      }
      ierr = PetscLogFlops(8.0);CHKERRQ(ierr);
      DMPlex_Det2D_Internal(detJ, J);
    }
    if (invJ) {DMPlex_Invert2D_Internal(invJ, J, *detJ);}
  } else SETERRQ1(PETSC_COMM_SELF, PETSC_ERR_ARG_WRONG, "The number of coordinates for this triangle is %D != 6 or 9", numCoords);
  ierr = DMPlexVecRestoreClosure(dm, coordSection, coordinates, e, &numCoords, &coords);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMPlexComputeRectangleGeometry_Internal"
static PetscErrorCode DMPlexComputeRectangleGeometry_Internal(DM dm, PetscInt e, PetscReal v0[], PetscReal J[], PetscReal invJ[], PetscReal *detJ)
{
  PetscSection   coordSection;
  Vec            coordinates;
  PetscScalar   *coords = NULL;
  PetscInt       numCoords, d, f, g;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = DMGetCoordinatesLocal(dm, &coordinates);CHKERRQ(ierr);
  ierr = DMGetCoordinateSection(dm, &coordSection);CHKERRQ(ierr);
  ierr = DMPlexVecGetClosure(dm, coordSection, coordinates, e, &numCoords, &coords);CHKERRQ(ierr);
  *detJ = 0.0;
  if (numCoords == 12) {
    const PetscInt dim = 3;
    PetscReal      R[9], J0[9] = {1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0};

    if (v0)   {for (d = 0; d < dim; d++) v0[d] = PetscRealPart(coords[d]);}
    ierr = DMPlexComputeProjection3Dto2D_Internal(numCoords, coords, R);CHKERRQ(ierr);
    if (J)    {
      const PetscInt pdim = 2;

      for (d = 0; d < pdim; d++) {
        J0[d*dim+0] = 0.5*(PetscRealPart(coords[1*pdim+d]) - PetscRealPart(coords[0*pdim+d]));
        J0[d*dim+1] = 0.5*(PetscRealPart(coords[3*pdim+d]) - PetscRealPart(coords[0*pdim+d]));
      }
      ierr = PetscLogFlops(8.0);CHKERRQ(ierr);
      DMPlex_Det3D_Internal(detJ, J0);
      for (d = 0; d < dim; d++) {
        for (f = 0; f < dim; f++) {
          J[d*dim+f] = 0.0;
          for (g = 0; g < dim; g++) {
            J[d*dim+f] += R[d*dim+g]*J0[g*dim+f];
          }
        }
      }
      ierr = PetscLogFlops(18.0);CHKERRQ(ierr);
    }
    if (invJ) {DMPlex_Invert3D_Internal(invJ, J, *detJ);}
  } else if ((numCoords == 8) || (numCoords == 16)) {
    const PetscInt dim = 2;

    if (v0)   {for (d = 0; d < dim; d++) v0[d] = PetscRealPart(coords[d]);}
    if (J)    {
      for (d = 0; d < dim; d++) {
        J[d*dim+0] = 0.5*(PetscRealPart(coords[1*dim+d]) - PetscRealPart(coords[0*dim+d]));
        J[d*dim+1] = 0.5*(PetscRealPart(coords[3*dim+d]) - PetscRealPart(coords[0*dim+d]));
      }
      ierr = PetscLogFlops(8.0);CHKERRQ(ierr);
      DMPlex_Det2D_Internal(detJ, J);
    }
    if (invJ) {DMPlex_Invert2D_Internal(invJ, J, *detJ);}
  } else SETERRQ1(PETSC_COMM_SELF, PETSC_ERR_ARG_WRONG, "The number of coordinates for this quadrilateral is %D != 8 or 12", numCoords);
  ierr = DMPlexVecRestoreClosure(dm, coordSection, coordinates, e, &numCoords, &coords);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMPlexComputeTetrahedronGeometry_Internal"
static PetscErrorCode DMPlexComputeTetrahedronGeometry_Internal(DM dm, PetscInt e, PetscReal v0[], PetscReal J[], PetscReal invJ[], PetscReal *detJ)
{
  PetscSection   coordSection;
  Vec            coordinates;
  PetscScalar   *coords = NULL;
  const PetscInt dim = 3;
  PetscInt       d;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = DMGetCoordinatesLocal(dm, &coordinates);CHKERRQ(ierr);
  ierr = DMGetCoordinateSection(dm, &coordSection);CHKERRQ(ierr);
  ierr = DMPlexVecGetClosure(dm, coordSection, coordinates, e, NULL, &coords);CHKERRQ(ierr);
  *detJ = 0.0;
  if (v0)   {for (d = 0; d < dim; d++) v0[d] = PetscRealPart(coords[d]);}
  if (J)    {
    for (d = 0; d < dim; d++) {
      /* I orient with outward face normals */
      J[d*dim+0] = 0.5*(PetscRealPart(coords[2*dim+d]) - PetscRealPart(coords[0*dim+d]));
      J[d*dim+1] = 0.5*(PetscRealPart(coords[1*dim+d]) - PetscRealPart(coords[0*dim+d]));
      J[d*dim+2] = 0.5*(PetscRealPart(coords[3*dim+d]) - PetscRealPart(coords[0*dim+d]));
    }
    ierr = PetscLogFlops(18.0);CHKERRQ(ierr);
    DMPlex_Det3D_Internal(detJ, J);
  }
  if (invJ) {DMPlex_Invert3D_Internal(invJ, J, *detJ);}
  ierr = DMPlexVecRestoreClosure(dm, coordSection, coordinates, e, NULL, &coords);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMPlexComputeHexahedronGeometry_Internal"
static PetscErrorCode DMPlexComputeHexahedronGeometry_Internal(DM dm, PetscInt e, PetscReal v0[], PetscReal J[], PetscReal invJ[], PetscReal *detJ)
{
  PetscSection   coordSection;
  Vec            coordinates;
  PetscScalar   *coords = NULL;
  const PetscInt dim = 3;
  PetscInt       d;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = DMGetCoordinatesLocal(dm, &coordinates);CHKERRQ(ierr);
  ierr = DMGetCoordinateSection(dm, &coordSection);CHKERRQ(ierr);
  ierr = DMPlexVecGetClosure(dm, coordSection, coordinates, e, NULL, &coords);CHKERRQ(ierr);
  *detJ = 0.0;
  if (v0)   {for (d = 0; d < dim; d++) v0[d] = PetscRealPart(coords[d]);}
  if (J)    {
    for (d = 0; d < dim; d++) {
      J[d*dim+0] = 0.5*(PetscRealPart(coords[3*dim+d]) - PetscRealPart(coords[0*dim+d]));
      J[d*dim+1] = 0.5*(PetscRealPart(coords[1*dim+d]) - PetscRealPart(coords[0*dim+d]));
      J[d*dim+2] = 0.5*(PetscRealPart(coords[4*dim+d]) - PetscRealPart(coords[0*dim+d]));
    }
    ierr = PetscLogFlops(18.0);CHKERRQ(ierr);
    DMPlex_Det3D_Internal(detJ, J);
  }
  if (invJ) {DMPlex_Invert3D_Internal(invJ, J, *detJ);}
  ierr = DMPlexVecRestoreClosure(dm, coordSection, coordinates, e, NULL, &coords);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMPlexComputeCellGeometryAffineFEM"
/*@C
  DMPlexComputeCellGeometryAffineFEM - Assuming an affine map, compute the Jacobian, inverse Jacobian, and Jacobian determinant for a given cell

  Collective on DM

  Input Arguments:
+ dm   - the DM
- cell - the cell

  Output Arguments:
+ v0   - the translation part of this affine transform
. J    - the Jacobian of the transform from the reference element
. invJ - the inverse of the Jacobian
- detJ - the Jacobian determinant

  Level: advanced

  Fortran Notes:
  Since it returns arrays, this routine is only available in Fortran 90, and you must
  include petsc.h90 in your code.

.seealso: DMPlexComputeCellGeometryFEM(), DMGetCoordinateSection(), DMGetCoordinateVec()
@*/
PetscErrorCode DMPlexComputeCellGeometryAffineFEM(DM dm, PetscInt cell, PetscReal *v0, PetscReal *J, PetscReal *invJ, PetscReal *detJ)
{
  PetscInt       depth, dim, coneSize;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = DMPlexGetDepth(dm, &depth);CHKERRQ(ierr);
  ierr = DMPlexGetConeSize(dm, cell, &coneSize);CHKERRQ(ierr);
  if (depth == 1) {
    ierr = DMGetDimension(dm, &dim);CHKERRQ(ierr);
  } else {
    DMLabel depth;

    ierr = DMPlexGetDepthLabel(dm, &depth);CHKERRQ(ierr);
    ierr = DMLabelGetValue(depth, cell, &dim);CHKERRQ(ierr);
  }
  switch (dim) {
  case 1:
    ierr = DMPlexComputeLineGeometry_Internal(dm, cell, v0, J, invJ, detJ);CHKERRQ(ierr);
    break;
  case 2:
    switch (coneSize) {
    case 3:
      ierr = DMPlexComputeTriangleGeometry_Internal(dm, cell, v0, J, invJ, detJ);CHKERRQ(ierr);
      break;
    case 4:
      ierr = DMPlexComputeRectangleGeometry_Internal(dm, cell, v0, J, invJ, detJ);CHKERRQ(ierr);
      break;
    default:
      SETERRQ2(PetscObjectComm((PetscObject)dm), PETSC_ERR_SUP, "Unsupported number of faces %D in cell %D for element geometry computation", coneSize, cell);
    }
    break;
  case 3:
    switch (coneSize) {
    case 4:
      ierr = DMPlexComputeTetrahedronGeometry_Internal(dm, cell, v0, J, invJ, detJ);CHKERRQ(ierr);
      break;
    case 6: /* Faces */
    case 8: /* Vertices */
      ierr = DMPlexComputeHexahedronGeometry_Internal(dm, cell, v0, J, invJ, detJ);CHKERRQ(ierr);
      break;
    default:
        SETERRQ2(PetscObjectComm((PetscObject)dm), PETSC_ERR_SUP, "Unsupported number of faces %D in cell %D for element geometry computation", coneSize, cell);
    }
      break;
  default:
    SETERRQ1(PetscObjectComm((PetscObject)dm), PETSC_ERR_SUP, "Unsupported dimension %D for element geometry computation", dim);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMPlexComputeIsoparametricGeometry_Internal"
static PetscErrorCode DMPlexComputeIsoparametricGeometry_Internal(DM dm, PetscFE fe, PetscInt point, PetscReal v0[], PetscReal J[], PetscReal invJ[], PetscReal *detJ)
{
  PetscQuadrature  quad;
  PetscSection     coordSection;
  Vec              coordinates;
  PetscScalar     *coords = NULL;
  const PetscReal *quadPoints;
  PetscReal       *basisDer;
  PetscInt         dim, cdim, pdim, qdim, Nq, numCoords, d, q;
  PetscErrorCode   ierr;

  PetscFunctionBegin;
  ierr = DMGetCoordinatesLocal(dm, &coordinates);CHKERRQ(ierr);
  ierr = DMGetCoordinateSection(dm, &coordSection);CHKERRQ(ierr);
  ierr = DMPlexVecGetClosure(dm, coordSection, coordinates, point, &numCoords, &coords);CHKERRQ(ierr);
  ierr = DMGetDimension(dm, &dim);CHKERRQ(ierr);
  ierr = DMGetCoordinateDim(dm, &cdim);CHKERRQ(ierr);
  ierr = PetscFEGetQuadrature(fe, &quad);CHKERRQ(ierr);
  ierr = PetscFEGetDimension(fe, &pdim);CHKERRQ(ierr);
  ierr = PetscQuadratureGetData(quad, &qdim, &Nq, &quadPoints, NULL);CHKERRQ(ierr);
  ierr = PetscFEGetDefaultTabulation(fe, NULL, &basisDer, NULL);CHKERRQ(ierr);
  *detJ = 0.0;
  if (qdim != dim) SETERRQ2(PETSC_COMM_SELF, PETSC_ERR_ARG_SIZ, "Point dimension %d != quadrature dimension %d", dim, qdim);
  if (numCoords != pdim*cdim) SETERRQ4(PETSC_COMM_SELF, PETSC_ERR_ARG_SIZ, "There are %d coordinates for point %d != %d*%d", numCoords, point, pdim, cdim);
  if (v0) {for (d = 0; d < cdim; d++) v0[d] = PetscRealPart(coords[d]);}
  if (J) {
    for (q = 0; q < Nq; ++q) {
      PetscInt i, j, k, c, r;

      /* J = dx_i/d\xi_j = sum[k=0,n-1] dN_k/d\xi_j * x_i(k) */
      for (k = 0; k < pdim; ++k)
        for (j = 0; j < dim; ++j)
          for (i = 0; i < cdim; ++i)
            J[(q*cdim + i)*dim + j] += basisDer[(q*pdim + k)*dim + j] * PetscRealPart(coords[k*cdim + i]);
      ierr = PetscLogFlops(2.0*pdim*dim*cdim);CHKERRQ(ierr);
      if (cdim > dim) {
        for (c = dim; c < cdim; ++c)
          for (r = 0; r < cdim; ++r)
            J[r*cdim+c] = r == c ? 1.0 : 0.0;
      }
      switch (cdim) {
      case 3:
        DMPlex_Det3D_Internal(detJ, J);
        if (invJ) {DMPlex_Invert3D_Internal(invJ, J, *detJ);}
        break;
      case 2:
        DMPlex_Det2D_Internal(detJ, J);
        if (invJ) {DMPlex_Invert2D_Internal(invJ, J, *detJ);}
        break;
      case 1:
        *detJ = J[0];
        if (invJ) invJ[0] = 1.0/J[0];
      }
    }
  }
  ierr = DMPlexVecRestoreClosure(dm, coordSection, coordinates, point, &numCoords, &coords);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMPlexComputeCellGeometryFEM"
/*@C
  DMPlexComputeCellGeometryFEM - Compute the Jacobian, inverse Jacobian, and Jacobian determinant at each quadrature point in the given cell

  Collective on DM

  Input Arguments:
+ dm   - the DM
. cell - the cell
- fe   - the finite element containing the quadrature

  Output Arguments:
+ v0   - the translation part of this transform
. J    - the Jacobian of the transform from the reference element at each quadrature point
. invJ - the inverse of the Jacobian at each quadrature point
- detJ - the Jacobian determinant at each quadrature point

  Level: advanced

  Fortran Notes:
  Since it returns arrays, this routine is only available in Fortran 90, and you must
  include petsc.h90 in your code.

.seealso: DMGetCoordinateSection(), DMGetCoordinateVec()
@*/
PetscErrorCode DMPlexComputeCellGeometryFEM(DM dm, PetscInt cell, PetscFE fe, PetscReal *v0, PetscReal *J, PetscReal *invJ, PetscReal *detJ)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (!fe) {ierr = DMPlexComputeCellGeometryAffineFEM(dm, cell, v0, J, invJ, detJ);CHKERRQ(ierr);}
  else     {ierr = DMPlexComputeIsoparametricGeometry_Internal(dm, fe, cell, v0, J, invJ, detJ);CHKERRQ(ierr);}
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMPlexComputeGeometryFVM_1D_Internal"
static PetscErrorCode DMPlexComputeGeometryFVM_1D_Internal(DM dm, PetscInt dim, PetscInt cell, PetscReal *vol, PetscReal centroid[], PetscReal normal[])
{
  PetscSection   coordSection;
  Vec            coordinates;
  PetscScalar   *coords = NULL;
  PetscScalar    tmp[2];
  PetscInt       coordSize;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = DMGetCoordinatesLocal(dm, &coordinates);CHKERRQ(ierr);
  ierr = DMGetCoordinateSection(dm, &coordSection);CHKERRQ(ierr);
  ierr = DMPlexVecGetClosure(dm, coordSection, coordinates, cell, &coordSize, &coords);CHKERRQ(ierr);
  if (dim != 2) SETERRQ(PetscObjectComm((PetscObject)dm), PETSC_ERR_SUP, "We only support 2D edges right now");
  ierr = DMPlexLocalizeCoordinate_Internal(dm, dim, coords, &coords[dim], tmp);CHKERRQ(ierr);
  if (centroid) {
    centroid[0] = 0.5*PetscRealPart(coords[0] + tmp[0]);
    centroid[1] = 0.5*PetscRealPart(coords[1] + tmp[1]);
  }
  if (normal) {
    PetscReal norm;

    normal[0]  = -PetscRealPart(coords[1] - tmp[1]);
    normal[1]  =  PetscRealPart(coords[0] - tmp[0]);
    norm       = PetscSqrtReal(normal[0]*normal[0] + normal[1]*normal[1]);
    normal[0] /= norm;
    normal[1] /= norm;
  }
  if (vol) {
    *vol = PetscSqrtReal(PetscSqr(PetscRealPart(coords[0] - tmp[0])) + PetscSqr(PetscRealPart(coords[1] - tmp[1])));
  }
  ierr = DMPlexVecRestoreClosure(dm, coordSection, coordinates, cell, &coordSize, &coords);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMPlexComputeGeometryFVM_2D_Internal"
/* Centroid_i = (\sum_n A_n Cn_i ) / A */
static PetscErrorCode DMPlexComputeGeometryFVM_2D_Internal(DM dm, PetscInt dim, PetscInt cell, PetscReal *vol, PetscReal centroid[], PetscReal normal[])
{
  PetscSection   coordSection;
  Vec            coordinates;
  PetscScalar   *coords = NULL;
  PetscReal      vsum = 0.0, csum[3] = {0.0, 0.0, 0.0}, vtmp, ctmp[4], v0[3], R[9];
  PetscInt       tdim = 2, coordSize, numCorners, p, d, e;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = DMGetCoordinatesLocal(dm, &coordinates);CHKERRQ(ierr);
  ierr = DMPlexGetConeSize(dm, cell, &numCorners);CHKERRQ(ierr);
  ierr = DMGetCoordinateSection(dm, &coordSection);CHKERRQ(ierr);
  ierr = DMPlexVecGetClosure(dm, coordSection, coordinates, cell, &coordSize, &coords);CHKERRQ(ierr);
  ierr = DMGetCoordinateDim(dm, &dim);CHKERRQ(ierr);
  if (normal) {
    if (dim > 2) {
      const PetscReal x0 = PetscRealPart(coords[dim+0] - coords[0]), x1 = PetscRealPart(coords[dim*2+0] - coords[0]);
      const PetscReal y0 = PetscRealPart(coords[dim+1] - coords[1]), y1 = PetscRealPart(coords[dim*2+1] - coords[1]);
      const PetscReal z0 = PetscRealPart(coords[dim+2] - coords[2]), z1 = PetscRealPart(coords[dim*2+2] - coords[2]);
      PetscReal       norm;

      v0[0]     = PetscRealPart(coords[0]);
      v0[1]     = PetscRealPart(coords[1]);
      v0[2]     = PetscRealPart(coords[2]);
      normal[0] = y0*z1 - z0*y1;
      normal[1] = z0*x1 - x0*z1;
      normal[2] = x0*y1 - y0*x1;
      norm = PetscSqrtReal(normal[0]*normal[0] + normal[1]*normal[1] + normal[2]*normal[2]);
      normal[0] /= norm;
      normal[1] /= norm;
      normal[2] /= norm;
    } else {
      for (d = 0; d < dim; ++d) normal[d] = 0.0;
    }
  }
  if (dim == 3) {ierr = DMPlexComputeProjection3Dto2D_Internal(coordSize, coords, R);CHKERRQ(ierr);}
  for (p = 0; p < numCorners; ++p) {
    /* Need to do this copy to get types right */
    for (d = 0; d < tdim; ++d) {
      ctmp[d]      = PetscRealPart(coords[p*tdim+d]);
      ctmp[tdim+d] = PetscRealPart(coords[((p+1)%numCorners)*tdim+d]);
    }
    Volume_Triangle_Origin_Internal(&vtmp, ctmp);
    vsum += vtmp;
    for (d = 0; d < tdim; ++d) {
      csum[d] += (ctmp[d] + ctmp[tdim+d])*vtmp;
    }
  }
  for (d = 0; d < tdim; ++d) {
    csum[d] /= (tdim+1)*vsum;
  }
  ierr = DMPlexVecRestoreClosure(dm, coordSection, coordinates, cell, &coordSize, &coords);CHKERRQ(ierr);
  if (vol) *vol = PetscAbsReal(vsum);
  if (centroid) {
    if (dim > 2) {
      for (d = 0; d < dim; ++d) {
        centroid[d] = v0[d];
        for (e = 0; e < dim; ++e) {
          centroid[d] += R[d*dim+e]*csum[e];
        }
      }
    } else for (d = 0; d < dim; ++d) centroid[d] = csum[d];
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMPlexComputeGeometryFVM_3D_Internal"
/* Centroid_i = (\sum_n V_n Cn_i ) / V */
static PetscErrorCode DMPlexComputeGeometryFVM_3D_Internal(DM dm, PetscInt dim, PetscInt cell, PetscReal *vol, PetscReal centroid[], PetscReal normal[])
{
  PetscSection    coordSection;
  Vec             coordinates;
  PetscScalar    *coords = NULL;
  PetscReal       vsum = 0.0, vtmp, coordsTmp[3*3];
  const PetscInt *faces, *facesO;
  PetscInt        numFaces, f, coordSize, numCorners, p, d;
  PetscErrorCode  ierr;

  PetscFunctionBegin;
  if (PetscUnlikely(dim > 3)) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_ARG_OUTOFRANGE,"No support for dim %D > 3",dim);
  ierr = DMGetCoordinatesLocal(dm, &coordinates);CHKERRQ(ierr);
  ierr = DMGetCoordinateSection(dm, &coordSection);CHKERRQ(ierr);

  if (centroid) for (d = 0; d < dim; ++d) centroid[d] = 0.0;
  ierr = DMPlexGetConeSize(dm, cell, &numFaces);CHKERRQ(ierr);
  ierr = DMPlexGetCone(dm, cell, &faces);CHKERRQ(ierr);
  ierr = DMPlexGetConeOrientation(dm, cell, &facesO);CHKERRQ(ierr);
  for (f = 0; f < numFaces; ++f) {
    ierr = DMPlexVecGetClosure(dm, coordSection, coordinates, faces[f], &coordSize, &coords);CHKERRQ(ierr);
    numCorners = coordSize/dim;
    switch (numCorners) {
    case 3:
      for (d = 0; d < dim; ++d) {
        coordsTmp[0*dim+d] = PetscRealPart(coords[0*dim+d]);
        coordsTmp[1*dim+d] = PetscRealPart(coords[1*dim+d]);
        coordsTmp[2*dim+d] = PetscRealPart(coords[2*dim+d]);
      }
      Volume_Tetrahedron_Origin_Internal(&vtmp, coordsTmp);
      if (facesO[f] < 0) vtmp = -vtmp;
      vsum += vtmp;
      if (centroid) {           /* Centroid of OABC = (a+b+c)/4 */
        for (d = 0; d < dim; ++d) {
          for (p = 0; p < 3; ++p) centroid[d] += coordsTmp[p*dim+d]*vtmp;
        }
      }
      break;
    case 4:
      /* DO FOR PYRAMID */
      /* First tet */
      for (d = 0; d < dim; ++d) {
        coordsTmp[0*dim+d] = PetscRealPart(coords[0*dim+d]);
        coordsTmp[1*dim+d] = PetscRealPart(coords[1*dim+d]);
        coordsTmp[2*dim+d] = PetscRealPart(coords[3*dim+d]);
      }
      Volume_Tetrahedron_Origin_Internal(&vtmp, coordsTmp);
      if (facesO[f] < 0) vtmp = -vtmp;
      vsum += vtmp;
      if (centroid) {
        for (d = 0; d < dim; ++d) {
          for (p = 0; p < 3; ++p) centroid[d] += coordsTmp[p*dim+d]*vtmp;
        }
      }
      /* Second tet */
      for (d = 0; d < dim; ++d) {
        coordsTmp[0*dim+d] = PetscRealPart(coords[1*dim+d]);
        coordsTmp[1*dim+d] = PetscRealPart(coords[2*dim+d]);
        coordsTmp[2*dim+d] = PetscRealPart(coords[3*dim+d]);
      }
      Volume_Tetrahedron_Origin_Internal(&vtmp, coordsTmp);
      if (facesO[f] < 0) vtmp = -vtmp;
      vsum += vtmp;
      if (centroid) {
        for (d = 0; d < dim; ++d) {
          for (p = 0; p < 3; ++p) centroid[d] += coordsTmp[p*dim+d]*vtmp;
        }
      }
      break;
    default:
      SETERRQ1(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Cannot handle faces with %D vertices", numCorners);
    }
    ierr = DMPlexVecRestoreClosure(dm, coordSection, coordinates, faces[f], &coordSize, &coords);CHKERRQ(ierr);
  }
  if (vol)     *vol = PetscAbsReal(vsum);
  if (normal)   for (d = 0; d < dim; ++d) normal[d]    = 0.0;
  if (centroid) for (d = 0; d < dim; ++d) centroid[d] /= (vsum*4);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMPlexComputeCellGeometryFVM"
/*@C
  DMPlexComputeCellGeometryFVM - Compute the volume for a given cell

  Collective on DM

  Input Arguments:
+ dm   - the DM
- cell - the cell

  Output Arguments:
+ volume   - the cell volume
. centroid - the cell centroid
- normal - the cell normal, if appropriate

  Level: advanced

  Fortran Notes:
  Since it returns arrays, this routine is only available in Fortran 90, and you must
  include petsc.h90 in your code.

.seealso: DMGetCoordinateSection(), DMGetCoordinateVec()
@*/
PetscErrorCode DMPlexComputeCellGeometryFVM(DM dm, PetscInt cell, PetscReal *vol, PetscReal centroid[], PetscReal normal[])
{
  PetscInt       depth, dim;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = DMPlexGetDepth(dm, &depth);CHKERRQ(ierr);
  ierr = DMGetDimension(dm, &dim);CHKERRQ(ierr);
  if (depth != dim) SETERRQ(PETSC_COMM_SELF, PETSC_ERR_ARG_WRONG, "Mesh must be interpolated");
  /* We need to keep a pointer to the depth label */
  ierr = DMPlexGetLabelValue(dm, "depth", cell, &depth);CHKERRQ(ierr);
  /* Cone size is now the number of faces */
  switch (depth) {
  case 1:
    ierr = DMPlexComputeGeometryFVM_1D_Internal(dm, dim, cell, vol, centroid, normal);CHKERRQ(ierr);
    break;
  case 2:
    ierr = DMPlexComputeGeometryFVM_2D_Internal(dm, dim, cell, vol, centroid, normal);CHKERRQ(ierr);
    break;
  case 3:
    ierr = DMPlexComputeGeometryFVM_3D_Internal(dm, dim, cell, vol, centroid, normal);CHKERRQ(ierr);
    break;
  default:
    SETERRQ1(PetscObjectComm((PetscObject)dm), PETSC_ERR_SUP, "Unsupported dimension %D for element geometry computation", dim);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMPlexComputeGeometryFEM"
/* This should also take a PetscFE argument I think */
PetscErrorCode DMPlexComputeGeometryFEM(DM dm, Vec *cellgeom)
{
  DM             dmCell;
  Vec            coordinates;
  PetscSection   coordSection, sectionCell;
  PetscScalar   *cgeom;
  PetscInt       cStart, cEnd, cMax, c;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = DMClone(dm, &dmCell);CHKERRQ(ierr);
  ierr = DMGetCoordinateSection(dm, &coordSection);CHKERRQ(ierr);
  ierr = DMGetCoordinatesLocal(dm, &coordinates);CHKERRQ(ierr);
  ierr = DMSetCoordinateSection(dmCell, PETSC_DETERMINE, coordSection);CHKERRQ(ierr);
  ierr = DMSetCoordinatesLocal(dmCell, coordinates);CHKERRQ(ierr);
  ierr = PetscSectionCreate(PetscObjectComm((PetscObject) dm), &sectionCell);CHKERRQ(ierr);
  ierr = DMPlexGetHeightStratum(dm, 0, &cStart, &cEnd);CHKERRQ(ierr);
  ierr = DMPlexGetHybridBounds(dm, &cMax, NULL, NULL, NULL);CHKERRQ(ierr);
  cEnd = cMax < 0 ? cEnd : cMax;
  ierr = PetscSectionSetChart(sectionCell, cStart, cEnd);CHKERRQ(ierr);
  /* TODO This needs to be multiplied by Nq for non-affine */
  for (c = cStart; c < cEnd; ++c) {ierr = PetscSectionSetDof(sectionCell, c, (PetscInt) PetscCeilReal(((PetscReal) sizeof(PetscFECellGeom))/sizeof(PetscScalar)));CHKERRQ(ierr);}
  ierr = PetscSectionSetUp(sectionCell);CHKERRQ(ierr);
  ierr = DMSetDefaultSection(dmCell, sectionCell);CHKERRQ(ierr);
  ierr = PetscSectionDestroy(&sectionCell);CHKERRQ(ierr);
  ierr = DMCreateLocalVector(dmCell, cellgeom);CHKERRQ(ierr);
  ierr = VecGetArray(*cellgeom, &cgeom);CHKERRQ(ierr);
  for (c = cStart; c < cEnd; ++c) {
    PetscFECellGeom *cg;

    ierr = DMPlexPointLocalRef(dmCell, c, cgeom, &cg);CHKERRQ(ierr);
    ierr = PetscMemzero(cg, sizeof(*cg));CHKERRQ(ierr);
    ierr = DMPlexComputeCellGeometryFEM(dmCell, c, NULL, cg->v0, cg->J, cg->invJ, &cg->detJ);CHKERRQ(ierr);
    if (cg->detJ <= 0.0) SETERRQ2(PETSC_COMM_SELF, PETSC_ERR_ARG_OUTOFRANGE, "Invalid determinant %g for element %d", cg->detJ, c);
  }
  ierr = VecRestoreArray(*cellgeom, &cgeom);CHKERRQ(ierr);
  ierr = DMDestroy(&dmCell);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMPlexComputeGeometryFVM"
/*@
  DMPlexComputeGeometryFVM - Computes the cell and face geometry for a finite volume method

  Input Parameter:
. dm - The DM

  Output Parameters:
+ cellgeom - A Vec of PetscFVCellGeom data
. facegeom - A Vec of PetscFVFaceGeom data

  Level: developer

.seealso: PetscFVFaceGeom, PetscFVCellGeom, DMPlexComputeGeometryFEM()
@*/
PetscErrorCode DMPlexComputeGeometryFVM(DM dm, Vec *cellgeom, Vec *facegeom)
{
  DM             dmFace, dmCell;
  DMLabel        ghostLabel;
  PetscSection   sectionFace, sectionCell;
  PetscSection   coordSection;
  Vec            coordinates;
  PetscScalar   *fgeom, *cgeom;
  PetscReal      minradius, gminradius;
  PetscInt       dim, cStart, cEnd, cEndInterior, c, fStart, fEnd, f;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = DMGetDimension(dm, &dim);CHKERRQ(ierr);
  ierr = DMGetCoordinateSection(dm, &coordSection);CHKERRQ(ierr);
  ierr = DMGetCoordinatesLocal(dm, &coordinates);CHKERRQ(ierr);
  /* Make cell centroids and volumes */
  ierr = DMClone(dm, &dmCell);CHKERRQ(ierr);
  ierr = DMSetCoordinateSection(dmCell, PETSC_DETERMINE, coordSection);CHKERRQ(ierr);
  ierr = DMSetCoordinatesLocal(dmCell, coordinates);CHKERRQ(ierr);
  ierr = PetscSectionCreate(PetscObjectComm((PetscObject) dm), &sectionCell);CHKERRQ(ierr);
  ierr = DMPlexGetHeightStratum(dm, 0, &cStart, &cEnd);CHKERRQ(ierr);
  ierr = DMPlexGetHybridBounds(dm, &cEndInterior, NULL, NULL, NULL);CHKERRQ(ierr);
  ierr = PetscSectionSetChart(sectionCell, cStart, cEnd);CHKERRQ(ierr);
  for (c = cStart; c < cEnd; ++c) {ierr = PetscSectionSetDof(sectionCell, c, (PetscInt) PetscCeilReal(((PetscReal) sizeof(PetscFVCellGeom))/sizeof(PetscScalar)));CHKERRQ(ierr);}
  ierr = PetscSectionSetUp(sectionCell);CHKERRQ(ierr);
  ierr = DMSetDefaultSection(dmCell, sectionCell);CHKERRQ(ierr);
  ierr = PetscSectionDestroy(&sectionCell);CHKERRQ(ierr);
  ierr = DMCreateLocalVector(dmCell, cellgeom);CHKERRQ(ierr);
  if (cEndInterior < 0) {
    cEndInterior = cEnd;
  }
  ierr = VecGetArray(*cellgeom, &cgeom);CHKERRQ(ierr);
  for (c = cStart; c < cEndInterior; ++c) {
    PetscFVCellGeom *cg;

    ierr = DMPlexPointLocalRef(dmCell, c, cgeom, &cg);CHKERRQ(ierr);
    ierr = PetscMemzero(cg, sizeof(*cg));CHKERRQ(ierr);
    ierr = DMPlexComputeCellGeometryFVM(dmCell, c, &cg->volume, cg->centroid, NULL);CHKERRQ(ierr);
  }
  /* Compute face normals and minimum cell radius */
  ierr = DMClone(dm, &dmFace);CHKERRQ(ierr);
  ierr = PetscSectionCreate(PetscObjectComm((PetscObject) dm), &sectionFace);CHKERRQ(ierr);
  ierr = DMPlexGetHeightStratum(dm, 1, &fStart, &fEnd);CHKERRQ(ierr);
  ierr = PetscSectionSetChart(sectionFace, fStart, fEnd);CHKERRQ(ierr);
  for (f = fStart; f < fEnd; ++f) {ierr = PetscSectionSetDof(sectionFace, f, (PetscInt) PetscCeilReal(((PetscReal) sizeof(PetscFVFaceGeom))/sizeof(PetscScalar)));CHKERRQ(ierr);}
  ierr = PetscSectionSetUp(sectionFace);CHKERRQ(ierr);
  ierr = DMSetDefaultSection(dmFace, sectionFace);CHKERRQ(ierr);
  ierr = PetscSectionDestroy(&sectionFace);CHKERRQ(ierr);
  ierr = DMCreateLocalVector(dmFace, facegeom);CHKERRQ(ierr);
  ierr = VecGetArray(*facegeom, &fgeom);CHKERRQ(ierr);
  ierr = DMPlexGetLabel(dm, "ghost", &ghostLabel);CHKERRQ(ierr);
  minradius = PETSC_MAX_REAL;
  for (f = fStart; f < fEnd; ++f) {
    PetscFVFaceGeom *fg;
    PetscReal        area;
    PetscInt         ghost = -1, d, numChildren;

    if (ghostLabel) {ierr = DMLabelGetValue(ghostLabel, f, &ghost);CHKERRQ(ierr);}
    ierr = DMPlexGetTreeChildren(dm,f,&numChildren,NULL);CHKERRQ(ierr);
    if (ghost >= 0 || numChildren) continue;
    ierr = DMPlexPointLocalRef(dmFace, f, fgeom, &fg);CHKERRQ(ierr);
    ierr = DMPlexComputeCellGeometryFVM(dm, f, &area, fg->centroid, fg->normal);CHKERRQ(ierr);
    for (d = 0; d < dim; ++d) fg->normal[d] *= area;
    /* Flip face orientation if necessary to match ordering in support, and Update minimum radius */
    {
      PetscFVCellGeom *cL, *cR;
      PetscInt         ncells;
      const PetscInt  *cells;
      PetscReal       *lcentroid, *rcentroid;
      PetscReal        l[3], r[3], v[3];

      ierr = DMPlexGetSupport(dm, f, &cells);CHKERRQ(ierr);
      ierr = DMPlexGetSupportSize(dm, f, &ncells);CHKERRQ(ierr);
      ierr = DMPlexPointLocalRead(dmCell, cells[0], cgeom, &cL);CHKERRQ(ierr);
      lcentroid = cells[0] >= cEndInterior ? fg->centroid : cL->centroid;
      if (ncells > 1) {
        ierr = DMPlexPointLocalRead(dmCell, cells[1], cgeom, &cR);CHKERRQ(ierr);
        rcentroid = cells[1] >= cEndInterior ? fg->centroid : cR->centroid;
      }
      else {
        rcentroid = fg->centroid;
      }
      ierr = DMPlexLocalizeCoordinateReal_Internal(dm, dim, fg->centroid, lcentroid, l);CHKERRQ(ierr);
      ierr = DMPlexLocalizeCoordinateReal_Internal(dm, dim, fg->centroid, rcentroid, r);CHKERRQ(ierr);
      DMPlex_WaxpyD_Internal(dim, -1, l, r, v);
      if (DMPlex_DotRealD_Internal(dim, fg->normal, v) < 0) {
        for (d = 0; d < dim; ++d) fg->normal[d] = -fg->normal[d];
      }
      if (DMPlex_DotRealD_Internal(dim, fg->normal, v) <= 0) {
        if (dim == 2) SETERRQ5(PETSC_COMM_SELF,PETSC_ERR_PLIB,"Direction for face %d could not be fixed, normal (%g,%g) v (%g,%g)", f, (double) fg->normal[0], (double) fg->normal[1], (double) v[0], (double) v[1]);
        if (dim == 3) SETERRQ7(PETSC_COMM_SELF,PETSC_ERR_PLIB,"Direction for face %d could not be fixed, normal (%g,%g,%g) v (%g,%g,%g)", f, (double) fg->normal[0], (double) fg->normal[1], (double) fg->normal[2], (double) v[0], (double) v[1], (double) v[2]);
        SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_PLIB,"Direction for face %d could not be fixed", f);
      }
      if (cells[0] < cEndInterior) {
        DMPlex_WaxpyD_Internal(dim, -1, fg->centroid, cL->centroid, v);
        minradius = PetscMin(minradius, DMPlex_NormD_Internal(dim, v));
      }
      if (ncells > 1 && cells[1] < cEndInterior) {
        DMPlex_WaxpyD_Internal(dim, -1, fg->centroid, cR->centroid, v);
        minradius = PetscMin(minradius, DMPlex_NormD_Internal(dim, v));
      }
    }
  }
  ierr = MPIU_Allreduce(&minradius, &gminradius, 1, MPIU_REAL, MPIU_MIN, PetscObjectComm((PetscObject)dm));CHKERRQ(ierr);
  ierr = DMPlexSetMinRadius(dm, gminradius);CHKERRQ(ierr);
  /* Compute centroids of ghost cells */
  for (c = cEndInterior; c < cEnd; ++c) {
    PetscFVFaceGeom *fg;
    const PetscInt  *cone,    *support;
    PetscInt         coneSize, supportSize, s;

    ierr = DMPlexGetConeSize(dmCell, c, &coneSize);CHKERRQ(ierr);
    if (coneSize != 1) SETERRQ2(PETSC_COMM_SELF, PETSC_ERR_ARG_WRONG, "Ghost cell %d has cone size %d != 1", c, coneSize);
    ierr = DMPlexGetCone(dmCell, c, &cone);CHKERRQ(ierr);
    ierr = DMPlexGetSupportSize(dmCell, cone[0], &supportSize);CHKERRQ(ierr);
    if (supportSize != 2) SETERRQ2(PETSC_COMM_SELF, PETSC_ERR_ARG_WRONG, "Face %d has support size %d != 2", cone[0], supportSize);
    ierr = DMPlexGetSupport(dmCell, cone[0], &support);CHKERRQ(ierr);
    ierr = DMPlexPointLocalRef(dmFace, cone[0], fgeom, &fg);CHKERRQ(ierr);
    for (s = 0; s < 2; ++s) {
      /* Reflect ghost centroid across plane of face */
      if (support[s] == c) {
        const PetscFVCellGeom *ci;
        PetscFVCellGeom       *cg;
        PetscReal              c2f[3], a;

        ierr = DMPlexPointLocalRead(dmCell, support[(s+1)%2], cgeom, &ci);CHKERRQ(ierr);
        DMPlex_WaxpyD_Internal(dim, -1, ci->centroid, fg->centroid, c2f); /* cell to face centroid */
        a    = DMPlex_DotRealD_Internal(dim, c2f, fg->normal)/DMPlex_DotRealD_Internal(dim, fg->normal, fg->normal);
        ierr = DMPlexPointLocalRef(dmCell, support[s], cgeom, &cg);CHKERRQ(ierr);
        DMPlex_WaxpyD_Internal(dim, 2*a, fg->normal, ci->centroid, cg->centroid);
        cg->volume = ci->volume;
      }
    }
  }
  ierr = VecRestoreArray(*facegeom, &fgeom);CHKERRQ(ierr);
  ierr = VecRestoreArray(*cellgeom, &cgeom);CHKERRQ(ierr);
  ierr = DMDestroy(&dmCell);CHKERRQ(ierr);
  ierr = DMDestroy(&dmFace);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMPlexGetMinRadius"
/*@C
  DMPlexGetMinRadius - Returns the minimum distance from any cell centroid to a face

  Not collective

  Input Argument:
. dm - the DM

  Output Argument:
. minradius - the minium cell radius

  Level: developer

.seealso: DMGetCoordinates()
@*/
PetscErrorCode DMPlexGetMinRadius(DM dm, PetscReal *minradius)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  PetscValidPointer(minradius,2);
  *minradius = ((DM_Plex*) dm->data)->minradius;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMPlexSetMinRadius"
/*@C
  DMPlexSetMinRadius - Sets the minimum distance from the cell centroid to a face

  Logically collective

  Input Arguments:
+ dm - the DM
- minradius - the minium cell radius

  Level: developer

.seealso: DMSetCoordinates()
@*/
PetscErrorCode DMPlexSetMinRadius(DM dm, PetscReal minradius)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  ((DM_Plex*) dm->data)->minradius = minradius;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "BuildGradientReconstruction_Internal"
static PetscErrorCode BuildGradientReconstruction_Internal(DM dm, PetscFV fvm, DM dmFace, PetscScalar *fgeom, DM dmCell, PetscScalar *cgeom)
{
  DMLabel        ghostLabel;
  PetscScalar   *dx, *grad, **gref;
  PetscInt       dim, cStart, cEnd, c, cEndInterior, maxNumFaces;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = DMGetDimension(dm, &dim);CHKERRQ(ierr);
  ierr = DMPlexGetHeightStratum(dm, 0, &cStart, &cEnd);CHKERRQ(ierr);
  ierr = DMPlexGetHybridBounds(dm, &cEndInterior, NULL, NULL, NULL);CHKERRQ(ierr);
  ierr = DMPlexGetMaxSizes(dm, &maxNumFaces, NULL);CHKERRQ(ierr);
  ierr = PetscFVLeastSquaresSetMaxFaces(fvm, maxNumFaces);CHKERRQ(ierr);
  ierr = DMPlexGetLabel(dm, "ghost", &ghostLabel);CHKERRQ(ierr);
  ierr = PetscMalloc3(maxNumFaces*dim, &dx, maxNumFaces*dim, &grad, maxNumFaces, &gref);CHKERRQ(ierr);
  for (c = cStart; c < cEndInterior; c++) {
    const PetscInt        *faces;
    PetscInt               numFaces, usedFaces, f, d;
    const PetscFVCellGeom *cg;
    PetscBool              boundary;
    PetscInt               ghost;

    ierr = DMPlexPointLocalRead(dmCell, c, cgeom, &cg);CHKERRQ(ierr);
    ierr = DMPlexGetConeSize(dm, c, &numFaces);CHKERRQ(ierr);
    ierr = DMPlexGetCone(dm, c, &faces);CHKERRQ(ierr);
    if (numFaces < dim) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_INCOMP,"Cell %D has only %D faces, not enough for gradient reconstruction", c, numFaces);
    for (f = 0, usedFaces = 0; f < numFaces; ++f) {
      const PetscFVCellGeom *cg1;
      PetscFVFaceGeom       *fg;
      const PetscInt        *fcells;
      PetscInt               ncell, side;

      ierr = DMLabelGetValue(ghostLabel, faces[f], &ghost);CHKERRQ(ierr);
      ierr = DMPlexIsBoundaryPoint(dm, faces[f], &boundary);CHKERRQ(ierr);
      if ((ghost >= 0) || boundary) continue;
      ierr  = DMPlexGetSupport(dm, faces[f], &fcells);CHKERRQ(ierr);
      side  = (c != fcells[0]); /* c is on left=0 or right=1 of face */
      ncell = fcells[!side];    /* the neighbor */
      ierr  = DMPlexPointLocalRef(dmFace, faces[f], fgeom, &fg);CHKERRQ(ierr);
      ierr  = DMPlexPointLocalRead(dmCell, ncell, cgeom, &cg1);CHKERRQ(ierr);
      for (d = 0; d < dim; ++d) dx[usedFaces*dim+d] = cg1->centroid[d] - cg->centroid[d];
      gref[usedFaces++] = fg->grad[side];  /* Gradient reconstruction term will go here */
    }
    if (!usedFaces) SETERRQ(PETSC_COMM_SELF, PETSC_ERR_USER, "Mesh contains isolated cell (no neighbors). Is it intentional?");
    ierr = PetscFVComputeGradient(fvm, usedFaces, dx, grad);CHKERRQ(ierr);
    for (f = 0, usedFaces = 0; f < numFaces; ++f) {
      ierr = DMLabelGetValue(ghostLabel, faces[f], &ghost);CHKERRQ(ierr);
      ierr = DMPlexIsBoundaryPoint(dm, faces[f], &boundary);CHKERRQ(ierr);
      if ((ghost >= 0) || boundary) continue;
      for (d = 0; d < dim; ++d) gref[usedFaces][d] = grad[usedFaces*dim+d];
      ++usedFaces;
    }
  }
  ierr = PetscFree3(dx, grad, gref);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "BuildGradientReconstruction_Internal_Tree"
static PetscErrorCode BuildGradientReconstruction_Internal_Tree(DM dm, PetscFV fvm, DM dmFace, PetscScalar *fgeom, DM dmCell, PetscScalar *cgeom)
{
  DMLabel        ghostLabel;
  PetscScalar   *dx, *grad, **gref;
  PetscInt       dim, cStart, cEnd, c, cEndInterior, fStart, fEnd, f, nStart, nEnd, maxNumFaces = 0;
  PetscSection   neighSec;
  PetscInt     (*neighbors)[2];
  PetscInt      *counter;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = DMGetDimension(dm, &dim);CHKERRQ(ierr);
  ierr = DMPlexGetHeightStratum(dm, 0, &cStart, &cEnd);CHKERRQ(ierr);
  ierr = DMPlexGetHybridBounds(dm, &cEndInterior, NULL, NULL, NULL);CHKERRQ(ierr);
  if (cEndInterior < 0) {
    cEndInterior = cEnd;
  }
  ierr = PetscSectionCreate(PetscObjectComm((PetscObject)dm),&neighSec);CHKERRQ(ierr);
  ierr = PetscSectionSetChart(neighSec,cStart,cEndInterior);CHKERRQ(ierr);
  ierr = DMPlexGetHeightStratum(dm, 1, &fStart, &fEnd);CHKERRQ(ierr);
  ierr = DMPlexGetLabel(dm, "ghost", &ghostLabel);CHKERRQ(ierr);
  for (f = fStart; f < fEnd; f++) {
    const PetscInt        *fcells;
    PetscBool              boundary;
    PetscInt               ghost = -1;
    PetscInt               numChildren, numCells, c;

    if (ghostLabel) {ierr = DMLabelGetValue(ghostLabel, f, &ghost);CHKERRQ(ierr);}
    ierr = DMPlexIsBoundaryPoint(dm, f, &boundary);CHKERRQ(ierr);
    ierr = DMPlexGetTreeChildren(dm, f, &numChildren, NULL);CHKERRQ(ierr);
    if ((ghost >= 0) || boundary || numChildren) continue;
    ierr = DMPlexGetSupportSize(dm, f, &numCells);CHKERRQ(ierr);
    if (numCells == 2) {
      ierr = DMPlexGetSupport(dm, f, &fcells);CHKERRQ(ierr);
      for (c = 0; c < 2; c++) {
        PetscInt cell = fcells[c];

        if (cell >= cStart && cell < cEndInterior) {
          ierr = PetscSectionAddDof(neighSec,cell,1);CHKERRQ(ierr);
        }
      }
    }
  }
  ierr = PetscSectionSetUp(neighSec);CHKERRQ(ierr);
  ierr = PetscSectionGetMaxDof(neighSec,&maxNumFaces);CHKERRQ(ierr);
  ierr = PetscFVLeastSquaresSetMaxFaces(fvm, maxNumFaces);CHKERRQ(ierr);
  nStart = 0;
  ierr = PetscSectionGetStorageSize(neighSec,&nEnd);CHKERRQ(ierr);
  ierr = PetscMalloc1((nEnd-nStart),&neighbors);CHKERRQ(ierr);
  ierr = PetscCalloc1((cEndInterior-cStart),&counter);CHKERRQ(ierr);
  for (f = fStart; f < fEnd; f++) {
    const PetscInt        *fcells;
    PetscBool              boundary;
    PetscInt               ghost = -1;
    PetscInt               numChildren, numCells, c;

    if (ghostLabel) {ierr = DMLabelGetValue(ghostLabel, f, &ghost);CHKERRQ(ierr);}
    ierr = DMPlexIsBoundaryPoint(dm, f, &boundary);CHKERRQ(ierr);
    ierr = DMPlexGetTreeChildren(dm, f, &numChildren, NULL);CHKERRQ(ierr);
    if ((ghost >= 0) || boundary || numChildren) continue;
    ierr = DMPlexGetSupportSize(dm, f, &numCells);CHKERRQ(ierr);
    if (numCells == 2) {
      ierr  = DMPlexGetSupport(dm, f, &fcells);CHKERRQ(ierr);
      for (c = 0; c < 2; c++) {
        PetscInt cell = fcells[c], off;

        if (cell >= cStart && cell < cEndInterior) {
          ierr = PetscSectionGetOffset(neighSec,cell,&off);CHKERRQ(ierr);
          off += counter[cell - cStart]++;
          neighbors[off][0] = f;
          neighbors[off][1] = fcells[1 - c];
        }
      }
    }
  }
  ierr = PetscFree(counter);CHKERRQ(ierr);
  ierr = PetscMalloc3(maxNumFaces*dim, &dx, maxNumFaces*dim, &grad, maxNumFaces, &gref);CHKERRQ(ierr);
  for (c = cStart; c < cEndInterior; c++) {
    PetscInt               numFaces, f, d, off, ghost = -1;
    const PetscFVCellGeom *cg;

    ierr = DMPlexPointLocalRead(dmCell, c, cgeom, &cg);CHKERRQ(ierr);
    ierr = PetscSectionGetDof(neighSec, c, &numFaces);CHKERRQ(ierr);
    ierr = PetscSectionGetOffset(neighSec, c, &off);CHKERRQ(ierr);
    if (ghostLabel) {ierr = DMLabelGetValue(ghostLabel, c, &ghost);CHKERRQ(ierr);}
    if (ghost < 0 && numFaces < dim) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_ARG_INCOMP,"Cell %D has only %D faces, not enough for gradient reconstruction", c, numFaces);
    for (f = 0; f < numFaces; ++f) {
      const PetscFVCellGeom *cg1;
      PetscFVFaceGeom       *fg;
      const PetscInt        *fcells;
      PetscInt               ncell, side, nface;

      nface = neighbors[off + f][0];
      ncell = neighbors[off + f][1];
      ierr  = DMPlexGetSupport(dm,nface,&fcells);CHKERRQ(ierr);
      side  = (c != fcells[0]);
      ierr  = DMPlexPointLocalRef(dmFace, nface, fgeom, &fg);CHKERRQ(ierr);
      ierr  = DMPlexPointLocalRead(dmCell, ncell, cgeom, &cg1);CHKERRQ(ierr);
      for (d = 0; d < dim; ++d) dx[f*dim+d] = cg1->centroid[d] - cg->centroid[d];
      gref[f] = fg->grad[side];  /* Gradient reconstruction term will go here */
    }
    ierr = PetscFVComputeGradient(fvm, numFaces, dx, grad);CHKERRQ(ierr);
    for (f = 0; f < numFaces; ++f) {
      for (d = 0; d < dim; ++d) gref[f][d] = grad[f*dim+d];
    }
  }
  ierr = PetscFree3(dx, grad, gref);CHKERRQ(ierr);
  ierr = PetscSectionDestroy(&neighSec);CHKERRQ(ierr);
  ierr = PetscFree(neighbors);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMPlexComputeGradientFVM"
/*@
  DMPlexComputeGradientFVM - Compute geometric factors for gradient reconstruction, which are stored in the geometry data, and compute layout for gradient data

  Collective on DM

  Input Arguments:
+ dm  - The DM
. fvm - The PetscFV
. faceGeometry - The face geometry from DMPlexGetFaceGeometryFVM()
- cellGeometry - The face geometry from DMPlexGetCellGeometryFVM()

  Output Parameters:
+ faceGeometry - The geometric factors for gradient calculation are inserted
- dmGrad - The DM describing the layout of gradient data

  Level: developer

.seealso: DMPlexGetFaceGeometryFVM(), DMPlexGetCellGeometryFVM()
@*/
PetscErrorCode DMPlexComputeGradientFVM(DM dm, PetscFV fvm, Vec faceGeometry, Vec cellGeometry, DM *dmGrad)
{
  DM             dmFace, dmCell;
  PetscScalar   *fgeom, *cgeom;
  PetscSection   sectionGrad, parentSection;
  PetscInt       dim, pdim, cStart, cEnd, cEndInterior, c;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = DMGetDimension(dm, &dim);CHKERRQ(ierr);
  ierr = PetscFVGetNumComponents(fvm, &pdim);CHKERRQ(ierr);
  ierr = DMPlexGetHeightStratum(dm, 0, &cStart, &cEnd);CHKERRQ(ierr);
  ierr = DMPlexGetHybridBounds(dm, &cEndInterior, NULL, NULL, NULL);CHKERRQ(ierr);
  /* Construct the interpolant corresponding to each face from the least-square solution over the cell neighborhood */
  ierr = VecGetDM(faceGeometry, &dmFace);CHKERRQ(ierr);
  ierr = VecGetDM(cellGeometry, &dmCell);CHKERRQ(ierr);
  ierr = VecGetArray(faceGeometry, &fgeom);CHKERRQ(ierr);
  ierr = VecGetArray(cellGeometry, &cgeom);CHKERRQ(ierr);
  ierr = DMPlexGetTree(dm,&parentSection,NULL,NULL,NULL,NULL);CHKERRQ(ierr);
  if (!parentSection) {
    ierr = BuildGradientReconstruction_Internal(dm, fvm, dmFace, fgeom, dmCell, cgeom);CHKERRQ(ierr);
  }
  else {
    ierr = BuildGradientReconstruction_Internal_Tree(dm, fvm, dmFace, fgeom, dmCell, cgeom);CHKERRQ(ierr);
  }
  ierr = VecRestoreArray(faceGeometry, &fgeom);CHKERRQ(ierr);
  ierr = VecRestoreArray(cellGeometry, &cgeom);CHKERRQ(ierr);
  /* Create storage for gradients */
  ierr = DMClone(dm, dmGrad);CHKERRQ(ierr);
  ierr = PetscSectionCreate(PetscObjectComm((PetscObject) dm), &sectionGrad);CHKERRQ(ierr);
  ierr = PetscSectionSetChart(sectionGrad, cStart, cEnd);CHKERRQ(ierr);
  for (c = cStart; c < cEnd; ++c) {ierr = PetscSectionSetDof(sectionGrad, c, pdim*dim);CHKERRQ(ierr);}
  ierr = PetscSectionSetUp(sectionGrad);CHKERRQ(ierr);
  ierr = DMSetDefaultSection(*dmGrad, sectionGrad);CHKERRQ(ierr);
  ierr = PetscSectionDestroy(&sectionGrad);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
