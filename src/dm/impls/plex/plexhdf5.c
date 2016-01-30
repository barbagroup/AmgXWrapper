#include <petsc/private/dmpleximpl.h>   /*I      "petscdmplex.h"   I*/
#include <petsc/private/isimpl.h>
#include <petsc/private/vecimpl.h>
#include <petscviewerhdf5.h>

PETSC_EXTERN PetscErrorCode VecView_Seq(Vec, PetscViewer);
PETSC_EXTERN PetscErrorCode VecView_MPI(Vec, PetscViewer);

#if defined(PETSC_HAVE_HDF5)
#undef __FUNCT__
#define __FUNCT__ "DMSequenceView_HDF5"
static PetscErrorCode DMSequenceView_HDF5(DM dm, const char *seqname, PetscInt seqnum, PetscScalar value, PetscViewer viewer)
{
  Vec            stamp;
  PetscMPIInt    rank;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (seqnum < 0) PetscFunctionReturn(0);
  ierr = MPI_Comm_rank(PetscObjectComm((PetscObject) viewer), &rank);CHKERRQ(ierr);
  ierr = VecCreateMPI(PetscObjectComm((PetscObject) viewer), rank ? 0 : 1, 1, &stamp);CHKERRQ(ierr);
  ierr = VecSetBlockSize(stamp, 1);CHKERRQ(ierr);
  ierr = PetscObjectSetName((PetscObject) stamp, seqname);CHKERRQ(ierr);
  if (!rank) {
    PetscReal timeScale;
    PetscBool istime;

    ierr = PetscStrncmp(seqname, "time", 5, &istime);CHKERRQ(ierr);
    if (istime) {ierr = DMPlexGetScale(dm, PETSC_UNIT_TIME, &timeScale);CHKERRQ(ierr); value *= timeScale;}
    ierr = VecSetValue(stamp, 0, value, INSERT_VALUES);CHKERRQ(ierr);
  }
  ierr = VecAssemblyBegin(stamp);CHKERRQ(ierr);
  ierr = VecAssemblyEnd(stamp);CHKERRQ(ierr);
  ierr = PetscViewerHDF5PushGroup(viewer, "/");CHKERRQ(ierr);
  ierr = PetscViewerHDF5SetTimestep(viewer, seqnum);CHKERRQ(ierr);
  ierr = VecView(stamp, viewer);CHKERRQ(ierr);
  ierr = PetscViewerHDF5PopGroup(viewer);CHKERRQ(ierr);
  ierr = VecDestroy(&stamp);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMSequenceLoad_HDF5"
PetscErrorCode DMSequenceLoad_HDF5(DM dm, const char *seqname, PetscInt seqnum, PetscScalar *value, PetscViewer viewer)
{
  Vec            stamp;
  PetscMPIInt    rank;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (seqnum < 0) PetscFunctionReturn(0);
  ierr = MPI_Comm_rank(PetscObjectComm((PetscObject) viewer), &rank);CHKERRQ(ierr);
  ierr = VecCreateMPI(PetscObjectComm((PetscObject) viewer), rank ? 0 : 1, 1, &stamp);CHKERRQ(ierr);
  ierr = VecSetBlockSize(stamp, 1);CHKERRQ(ierr);
  ierr = PetscObjectSetName((PetscObject) stamp, seqname);CHKERRQ(ierr);
  ierr = PetscViewerHDF5PushGroup(viewer, "/");CHKERRQ(ierr);
  ierr = PetscViewerHDF5SetTimestep(viewer, seqnum);CHKERRQ(ierr);
  ierr = VecLoad(stamp, viewer);CHKERRQ(ierr);
  ierr = PetscViewerHDF5PopGroup(viewer);CHKERRQ(ierr);
  if (!rank) {
    const PetscScalar *a;
    PetscReal timeScale;
    PetscBool istime;

    ierr = VecGetArrayRead(stamp, &a);CHKERRQ(ierr);
    *value = a[0];
    ierr = VecRestoreArrayRead(stamp, &a);CHKERRQ(ierr);
    ierr = PetscStrncmp(seqname, "time", 5, &istime);CHKERRQ(ierr);
    if (istime) {ierr = DMPlexGetScale(dm, PETSC_UNIT_TIME, &timeScale);CHKERRQ(ierr); *value /= timeScale;}
  }
  ierr = VecDestroy(&stamp);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecView_Plex_Local_HDF5"
PetscErrorCode VecView_Plex_Local_HDF5(Vec v, PetscViewer viewer)
{
  DM                      dm;
  DM                      dmBC;
  PetscSection            section, sectionGlobal;
  Vec                     gv;
  const char             *name;
  PetscViewerVTKFieldType ft;
  PetscViewerFormat       format;
  PetscInt                seqnum;
  PetscReal               seqval;
  PetscBool               isseq;
  PetscErrorCode          ierr;

  PetscFunctionBegin;
  ierr = PetscObjectTypeCompare((PetscObject) v, VECSEQ, &isseq);CHKERRQ(ierr);
  ierr = VecGetDM(v, &dm);CHKERRQ(ierr);
  ierr = DMGetDefaultSection(dm, &section);CHKERRQ(ierr);
  ierr = DMGetOutputSequenceNumber(dm, &seqnum, &seqval);CHKERRQ(ierr);
  ierr = PetscViewerHDF5SetTimestep(viewer, seqnum);CHKERRQ(ierr);
  ierr = DMSequenceView_HDF5(dm, "time", seqnum, (PetscScalar) seqval, viewer);CHKERRQ(ierr);
  ierr = PetscViewerGetFormat(viewer, &format);CHKERRQ(ierr);
  ierr = DMGetOutputDM(dm, &dmBC);CHKERRQ(ierr);
  ierr = DMGetDefaultGlobalSection(dmBC, &sectionGlobal);CHKERRQ(ierr);
  ierr = DMGetGlobalVector(dmBC, &gv);CHKERRQ(ierr);
  ierr = PetscObjectGetName((PetscObject) v, &name);CHKERRQ(ierr);
  ierr = PetscObjectSetName((PetscObject) gv, name);CHKERRQ(ierr);
  ierr = DMLocalToGlobalBegin(dmBC, v, INSERT_VALUES, gv);CHKERRQ(ierr);
  ierr = DMLocalToGlobalEnd(dmBC, v, INSERT_VALUES, gv);CHKERRQ(ierr);
  ierr = PetscObjectTypeCompare((PetscObject) gv, VECSEQ, &isseq);CHKERRQ(ierr);
  if (isseq) {ierr = VecView_Seq(gv, viewer);CHKERRQ(ierr);}
  else       {ierr = VecView_MPI(gv, viewer);CHKERRQ(ierr);}
  if (format == PETSC_VIEWER_HDF5_VIZ) {
    /* Output visualization representation */
    PetscInt numFields, f;

    ierr = PetscSectionGetNumFields(section, &numFields);CHKERRQ(ierr);
    for (f = 0; f < numFields; ++f) {
      Vec         subv;
      IS          is;
      const char *fname, *fgroup;
      char        subname[PETSC_MAX_PATH_LEN];
      char        group[PETSC_MAX_PATH_LEN];
      PetscInt    pStart, pEnd;
      PetscBool   flag;

      ierr = DMPlexGetFieldType_Internal(dm, section, f, &pStart, &pEnd, &ft);CHKERRQ(ierr);
      fgroup = (ft == PETSC_VTK_POINT_VECTOR_FIELD) || (ft == PETSC_VTK_POINT_FIELD) ? "/vertex_fields" : "/cell_fields";
      ierr = PetscSectionGetFieldName(section, f, &fname);CHKERRQ(ierr);
      if (!fname) continue;
      ierr = PetscViewerHDF5PushGroup(viewer, fgroup);CHKERRQ(ierr);
      ierr = PetscSectionGetField_Internal(section, sectionGlobal, gv, f, pStart, pEnd, &is, &subv);CHKERRQ(ierr);
      ierr = PetscStrcpy(subname, name);CHKERRQ(ierr);
      ierr = PetscStrcat(subname, "_");CHKERRQ(ierr);
      ierr = PetscStrcat(subname, fname);CHKERRQ(ierr);
      ierr = PetscObjectSetName((PetscObject) subv, subname);CHKERRQ(ierr);
      if (isseq) {ierr = VecView_Seq(subv, viewer);CHKERRQ(ierr);}
      else       {ierr = VecView_MPI(subv, viewer);CHKERRQ(ierr);}
      ierr = PetscSectionRestoreField_Internal(section, sectionGlobal, gv, f, pStart, pEnd, &is, &subv);CHKERRQ(ierr);
      ierr = PetscViewerHDF5PopGroup(viewer);CHKERRQ(ierr);
      ierr = PetscSNPrintf(group, PETSC_MAX_PATH_LEN, "%s/%s", fgroup, subname);CHKERRQ(ierr);
      ierr = PetscViewerHDF5HasAttribute(viewer, group, "vector_field_type", &flag);CHKERRQ(ierr);
      if (!flag) {
        if ((ft == PETSC_VTK_POINT_VECTOR_FIELD) || (ft == PETSC_VTK_CELL_VECTOR_FIELD)) {
          ierr = PetscViewerHDF5WriteAttribute(viewer, group, "vector_field_type", PETSC_STRING, "vector");CHKERRQ(ierr);
        } else {
          ierr = PetscViewerHDF5WriteAttribute(viewer, group, "vector_field_type", PETSC_STRING, "scalar");CHKERRQ(ierr);
        }
      }
    }
  }
  ierr = DMRestoreGlobalVector(dmBC, &gv);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecView_Plex_HDF5"
PetscErrorCode VecView_Plex_HDF5(Vec v, PetscViewer viewer)
{
  DM             dm;
  Vec            locv;
  const char    *name;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = VecGetDM(v, &dm);CHKERRQ(ierr);
  ierr = DMGetLocalVector(dm, &locv);CHKERRQ(ierr);
  ierr = PetscObjectGetName((PetscObject) v, &name);CHKERRQ(ierr);
  ierr = PetscObjectSetName((PetscObject) locv, name);CHKERRQ(ierr);
  ierr = DMGlobalToLocalBegin(dm, v, INSERT_VALUES, locv);CHKERRQ(ierr);
  ierr = DMGlobalToLocalEnd(dm, v, INSERT_VALUES, locv);CHKERRQ(ierr);
  ierr = PetscViewerHDF5PushGroup(viewer, "/fields");CHKERRQ(ierr);
  ierr = PetscViewerPushFormat(viewer, PETSC_VIEWER_HDF5_VIZ);CHKERRQ(ierr);
  ierr = VecView_Plex_Local(locv, viewer);CHKERRQ(ierr);
  ierr = PetscViewerPopFormat(viewer);CHKERRQ(ierr);
  ierr = PetscViewerHDF5PopGroup(viewer);CHKERRQ(ierr);
  ierr = DMRestoreLocalVector(dm, &locv);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecView_Plex_HDF5_Native"
PetscErrorCode VecView_Plex_HDF5_Native(Vec v, PetscViewer viewer)
{
  PetscBool      isseq;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscObjectTypeCompare((PetscObject) v, VECSEQ, &isseq);CHKERRQ(ierr);
  ierr = PetscViewerHDF5PushGroup(viewer, "/fields");CHKERRQ(ierr);
  if (isseq) {ierr = VecView_Seq(v, viewer);CHKERRQ(ierr);}
  else       {ierr = VecView_MPI(v, viewer);CHKERRQ(ierr);}
  ierr = PetscViewerHDF5PopGroup(viewer);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecLoad_Plex_HDF5"
PetscErrorCode VecLoad_Plex_HDF5(Vec v, PetscViewer viewer)
{
  DM             dm;
  Vec            locv;
  const char    *name;
  PetscInt       seqnum;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = VecGetDM(v, &dm);CHKERRQ(ierr);
  ierr = DMGetLocalVector(dm, &locv);CHKERRQ(ierr);
  ierr = PetscObjectGetName((PetscObject) v, &name);CHKERRQ(ierr);
  ierr = PetscObjectSetName((PetscObject) locv, name);CHKERRQ(ierr);
  ierr = DMGetOutputSequenceNumber(dm, &seqnum, NULL);CHKERRQ(ierr);
  ierr = PetscViewerHDF5SetTimestep(viewer, seqnum);CHKERRQ(ierr);
  ierr = PetscViewerHDF5PushGroup(viewer, "/fields");CHKERRQ(ierr);
  ierr = VecLoad_Plex_Local(locv, viewer);CHKERRQ(ierr);
  ierr = PetscViewerHDF5PopGroup(viewer);CHKERRQ(ierr);
  ierr = DMLocalToGlobalBegin(dm, locv, INSERT_VALUES, v);CHKERRQ(ierr);
  ierr = DMLocalToGlobalEnd(dm, locv, INSERT_VALUES, v);CHKERRQ(ierr);
  ierr = DMRestoreLocalVector(dm, &locv);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecLoad_Plex_HDF5_Native"
PetscErrorCode VecLoad_Plex_HDF5_Native(Vec v, PetscViewer viewer)
{
  DM             dm;
  PetscInt       seqnum;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = VecGetDM(v, &dm);CHKERRQ(ierr);
  ierr = DMGetOutputSequenceNumber(dm, &seqnum, NULL);CHKERRQ(ierr);
  ierr = PetscViewerHDF5SetTimestep(viewer, seqnum);CHKERRQ(ierr);
  ierr = PetscViewerHDF5PushGroup(viewer, "/fields");CHKERRQ(ierr);
  ierr = VecLoad_Default(v, viewer);CHKERRQ(ierr);
  ierr = PetscViewerHDF5PopGroup(viewer);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMPlexWriteTopology_HDF5_Static"
static PetscErrorCode DMPlexWriteTopology_HDF5_Static(DM dm, IS globalPointNumbers, PetscViewer viewer)
{
  IS              orderIS, conesIS, cellsIS, orntsIS;
  const PetscInt *gpoint;
  PetscInt       *order, *sizes, *cones, *ornts;
  PetscInt        dim, pStart, pEnd, p, conesSize = 0, cellsSize = 0, c = 0, s = 0;
  PetscErrorCode  ierr;

  PetscFunctionBegin;
  ierr = ISGetIndices(globalPointNumbers, &gpoint);CHKERRQ(ierr);
  ierr = DMGetDimension(dm, &dim);CHKERRQ(ierr);
  ierr = DMPlexGetChart(dm, &pStart, &pEnd);CHKERRQ(ierr);
  for (p = pStart; p < pEnd; ++p) {
    if (gpoint[p] >= 0) {
      PetscInt coneSize;

      ierr = DMPlexGetConeSize(dm, p, &coneSize);CHKERRQ(ierr);
      conesSize += 1;
      cellsSize += coneSize;
    }
  }
  ierr = PetscMalloc1(conesSize, &order);CHKERRQ(ierr);
  ierr = PetscMalloc1(conesSize, &sizes);CHKERRQ(ierr);
  ierr = PetscMalloc1(cellsSize, &cones);CHKERRQ(ierr);
  ierr = PetscMalloc1(cellsSize, &ornts);CHKERRQ(ierr);
  for (p = pStart; p < pEnd; ++p) {
    if (gpoint[p] >= 0) {
      const PetscInt *cone, *ornt;
      PetscInt        coneSize, cp;

      ierr = DMPlexGetConeSize(dm, p, &coneSize);CHKERRQ(ierr);
      ierr = DMPlexGetCone(dm, p, &cone);CHKERRQ(ierr);
      ierr = DMPlexGetConeOrientation(dm, p, &ornt);CHKERRQ(ierr);
      order[s]   = gpoint[p];
      sizes[s++] = coneSize;
      for (cp = 0; cp < coneSize; ++cp, ++c) {cones[c] = gpoint[cone[cp]] < 0 ? -(gpoint[cone[cp]]+1) : gpoint[cone[cp]]; ornts[c] = ornt[cp];}
    }
  }
  if (s != conesSize) SETERRQ2(PETSC_COMM_SELF, PETSC_ERR_LIB, "Total number of points %d != %d", s, conesSize);
  if (c != cellsSize) SETERRQ2(PETSC_COMM_SELF, PETSC_ERR_LIB, "Total number of cone points %d != %d", c, cellsSize);
  ierr = ISCreateGeneral(PetscObjectComm((PetscObject) dm), conesSize, order, PETSC_OWN_POINTER, &orderIS);CHKERRQ(ierr);
  ierr = PetscObjectSetName((PetscObject) orderIS, "order");CHKERRQ(ierr);
  ierr = ISCreateGeneral(PetscObjectComm((PetscObject) dm), conesSize, sizes, PETSC_OWN_POINTER, &conesIS);CHKERRQ(ierr);
  ierr = PetscObjectSetName((PetscObject) conesIS, "cones");CHKERRQ(ierr);
  ierr = ISCreateGeneral(PetscObjectComm((PetscObject) dm), cellsSize, cones, PETSC_OWN_POINTER, &cellsIS);CHKERRQ(ierr);
  ierr = PetscObjectSetName((PetscObject) cellsIS, "cells");CHKERRQ(ierr);
  ierr = ISCreateGeneral(PetscObjectComm((PetscObject) dm), cellsSize, ornts, PETSC_OWN_POINTER, &orntsIS);CHKERRQ(ierr);
  ierr = PetscObjectSetName((PetscObject) orntsIS, "orientation");CHKERRQ(ierr);
  ierr = PetscViewerHDF5PushGroup(viewer, "/topology");CHKERRQ(ierr);
  ierr = ISView(orderIS, viewer);CHKERRQ(ierr);
  ierr = ISView(conesIS, viewer);CHKERRQ(ierr);
  ierr = ISView(cellsIS, viewer);CHKERRQ(ierr);
  ierr = ISView(orntsIS, viewer);CHKERRQ(ierr);
  ierr = PetscViewerHDF5PopGroup(viewer);CHKERRQ(ierr);
  ierr = ISDestroy(&orderIS);CHKERRQ(ierr);
  ierr = ISDestroy(&conesIS);CHKERRQ(ierr);
  ierr = ISDestroy(&cellsIS);CHKERRQ(ierr);
  ierr = ISDestroy(&orntsIS);CHKERRQ(ierr);
  ierr = ISRestoreIndices(globalPointNumbers, &gpoint);CHKERRQ(ierr);

  ierr = PetscViewerHDF5WriteAttribute(viewer, "/topology/cells", "cell_dim", PETSC_INT, (void *) &dim);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMPlexWriteTopology_Vertices_HDF5_Static"
static PetscErrorCode DMPlexWriteTopology_Vertices_HDF5_Static(DM dm, DMLabel label, PetscInt labelId, PetscViewer viewer)
{
  IS              cellIS, globalVertexNumbers;
  const PetscInt *gvertex;
  PetscInt       *vertices;
  PetscInt        dim, depth, vStart, vEnd, v, cellHeight, cStart, cMax, cEnd, cell, conesSize = 0, numCornersLocal = 0, numCorners;
  hid_t           fileId, groupId;
  PetscErrorCode  ierr;

  PetscFunctionBegin;
  ierr = DMGetDimension(dm, &dim);CHKERRQ(ierr);
  ierr = DMPlexGetDepth(dm, &depth);CHKERRQ(ierr);
  ierr = DMPlexGetDepthStratum(dm, 0, &vStart, &vEnd);CHKERRQ(ierr);
  ierr = DMPlexGetVTKCellHeight(dm, &cellHeight);CHKERRQ(ierr);
  ierr = DMPlexGetHeightStratum(dm, cellHeight, &cStart, &cEnd);CHKERRQ(ierr);
  ierr = DMPlexGetHybridBounds(dm, &cMax, PETSC_NULL, PETSC_NULL, PETSC_NULL);CHKERRQ(ierr);
  if (cMax >= 0) cEnd = PetscMin(cEnd, cMax);
  if (depth == 1) PetscFunctionReturn(0);
  for (cell = cStart; cell < cEnd; ++cell) {
    PetscInt *closure = NULL;
    PetscInt  closureSize, v, Nc = 0;

    if (label) {
      PetscInt value;
      ierr = DMLabelGetValue(label, cell, &value);CHKERRQ(ierr);
      if (value == labelId) continue;
    }
    ierr = DMPlexGetTransitiveClosure(dm, cell, PETSC_TRUE, &closureSize, &closure);CHKERRQ(ierr);
    for (v = 0; v < closureSize*2; v += 2) {
      if ((closure[v] >= vStart) && (closure[v] < vEnd)) ++Nc;
    }
    ierr = DMPlexRestoreTransitiveClosure(dm, cell, PETSC_TRUE, &closureSize, &closure);CHKERRQ(ierr);
    conesSize += Nc;
    if (!numCornersLocal)           numCornersLocal = Nc;
    else if (numCornersLocal != Nc) numCornersLocal = 1;
  }
  ierr = MPIU_Allreduce(&numCornersLocal, &numCorners, 1, MPIU_INT, MPI_MAX, PetscObjectComm((PetscObject) dm));CHKERRQ(ierr);
  if (numCornersLocal && (numCornersLocal != numCorners || numCorners == 1)) SETERRQ(PETSC_COMM_SELF, PETSC_ERR_SUP, "Visualization topology currently only supports identical cell shapes");

  ierr = PetscViewerHDF5PushGroup(viewer, "/viz");CHKERRQ(ierr);
  ierr = PetscViewerHDF5OpenGroup(viewer, &fileId, &groupId);CHKERRQ(ierr);
  PetscStackCallHDF5(H5Gclose,(groupId));
  ierr = PetscViewerHDF5PopGroup(viewer);CHKERRQ(ierr);

  ierr = DMPlexGetVertexNumbering(dm, &globalVertexNumbers);CHKERRQ(ierr);
  ierr = ISGetIndices(globalVertexNumbers, &gvertex);CHKERRQ(ierr);
  ierr = PetscMalloc1(conesSize, &vertices);CHKERRQ(ierr);
  for (cell = cStart, v = 0; cell < cEnd; ++cell) {
    PetscInt *closure = NULL;
    PetscInt  closureSize, Nc = 0, p;

    if (label) {
      PetscInt value;
      ierr = DMLabelGetValue(label, cell, &value);CHKERRQ(ierr);
      if (value == labelId) continue;
    }
    ierr = DMPlexGetTransitiveClosure(dm, cell, PETSC_TRUE, &closureSize, &closure);CHKERRQ(ierr);
    for (p = 0; p < closureSize*2; p += 2) {
      if ((closure[p] >= vStart) && (closure[p] < vEnd)) {
        closure[Nc++] = closure[p];
      }
    }
    ierr = DMPlexInvertCell_Internal(dim, Nc, closure);CHKERRQ(ierr);
    for (p = 0; p < Nc; ++p) {
      const PetscInt gv = gvertex[closure[p] - vStart];
      vertices[v++] = gv < 0 ? -(gv+1) : gv;
    }
    ierr = DMPlexRestoreTransitiveClosure(dm, cell, PETSC_TRUE, &closureSize, &closure);CHKERRQ(ierr);
  }
  if (v != conesSize) SETERRQ2(PETSC_COMM_SELF, PETSC_ERR_LIB, "Total number of cell vertices %d != %d", v, conesSize);
  ierr = ISCreateGeneral(PetscObjectComm((PetscObject) dm), conesSize, vertices, PETSC_OWN_POINTER, &cellIS);CHKERRQ(ierr);
  ierr = PetscLayoutSetBlockSize(cellIS->map, numCorners);CHKERRQ(ierr);
  ierr = PetscObjectSetName((PetscObject) cellIS, "cells");CHKERRQ(ierr);
  ierr = PetscViewerHDF5PushGroup(viewer, "/viz/topology");CHKERRQ(ierr);
  ierr = ISView(cellIS, viewer);CHKERRQ(ierr);
#if 0
  if (numCorners == 1) {
    ierr = ISView(coneIS, viewer);CHKERRQ(ierr);
  } else {
    ierr = PetscViewerHDF5WriteAttribute(viewer, "/viz/topology/cells", "cell_corners", PETSC_INT, (void *) &numCorners);CHKERRQ(ierr);
  }
  ierr = ISDestroy(&coneIS);CHKERRQ(ierr);
#else
  ierr = PetscViewerHDF5WriteAttribute(viewer, "/viz/topology/cells", "cell_corners", PETSC_INT, (void *) &numCorners);CHKERRQ(ierr);
#endif
  ierr = ISDestroy(&cellIS);CHKERRQ(ierr);

  ierr = PetscViewerHDF5WriteAttribute(viewer, "/viz/topology/cells", "cell_dim", PETSC_INT, (void *) &dim);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMPlexWriteCoordinates_HDF5_Static"
static PetscErrorCode DMPlexWriteCoordinates_HDF5_Static(DM dm, PetscViewer viewer)
{
  DM             cdm;
  Vec            coordinates, newcoords;
  PetscReal      lengthScale;
  PetscInt       m, M, bs;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = DMPlexGetScale(dm, PETSC_UNIT_LENGTH, &lengthScale);CHKERRQ(ierr);
  ierr = DMGetCoordinateDM(dm, &cdm);CHKERRQ(ierr);
  ierr = DMGetCoordinates(dm, &coordinates);CHKERRQ(ierr);
  ierr = VecCreate(PetscObjectComm((PetscObject) coordinates), &newcoords);CHKERRQ(ierr);
  ierr = PetscObjectSetName((PetscObject) newcoords, "vertices");CHKERRQ(ierr);
  ierr = VecGetSize(coordinates, &M);CHKERRQ(ierr);
  ierr = VecGetLocalSize(coordinates, &m);CHKERRQ(ierr);
  ierr = VecSetSizes(newcoords, m, M);CHKERRQ(ierr);
  ierr = VecGetBlockSize(coordinates, &bs);CHKERRQ(ierr);
  ierr = VecSetBlockSize(newcoords, bs);CHKERRQ(ierr);
  ierr = VecSetType(newcoords,VECSTANDARD);CHKERRQ(ierr);
  ierr = VecCopy(coordinates, newcoords);CHKERRQ(ierr);
  ierr = VecScale(newcoords, lengthScale);CHKERRQ(ierr);
  /* Did not use DMGetGlobalVector() in order to bypass default group assignment */
  ierr = PetscViewerHDF5PushGroup(viewer, "/geometry");CHKERRQ(ierr);
  ierr = PetscViewerPushFormat(viewer, PETSC_VIEWER_NATIVE);CHKERRQ(ierr);
  ierr = VecView(newcoords, viewer);CHKERRQ(ierr);
  ierr = PetscViewerPopFormat(viewer);CHKERRQ(ierr);
  ierr = PetscViewerHDF5PopGroup(viewer);CHKERRQ(ierr);
  ierr = VecDestroy(&newcoords);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMPlexWriteCoordinates_Vertices_HDF5_Static"
static PetscErrorCode DMPlexWriteCoordinates_Vertices_HDF5_Static(DM dm, PetscViewer viewer)
{
  Vec              coordinates, newcoords;
  PetscSection     cSection;
  PetscScalar     *coords, *ncoords;
  const PetscReal *L;
  const DMBoundaryType *bd;
  PetscReal        lengthScale;
  PetscInt         vStart, vEnd, v, bs, coordSize, dof, off, d;
  hid_t            fileId, groupId;
  PetscErrorCode   ierr;

  PetscFunctionBegin;
  ierr = DMPlexGetDepthStratum(dm, 0, &vStart, &vEnd);CHKERRQ(ierr);
  ierr = DMPlexGetScale(dm, PETSC_UNIT_LENGTH, &lengthScale);CHKERRQ(ierr);
  ierr = DMGetCoordinatesLocal(dm, &coordinates);CHKERRQ(ierr);
  ierr = VecGetBlockSize(coordinates, &bs);CHKERRQ(ierr);
  ierr = VecGetLocalSize(coordinates, &coordSize);CHKERRQ(ierr);
  if (coordSize == (vEnd - vStart)*bs) PetscFunctionReturn(0);
  ierr = DMGetPeriodicity(dm, NULL, &L, &bd);CHKERRQ(ierr);
  ierr = DMGetCoordinateSection(dm, &cSection);CHKERRQ(ierr);
  ierr = VecCreate(PetscObjectComm((PetscObject) coordinates), &newcoords);CHKERRQ(ierr);
  coordSize = 0;
  for (v = vStart; v < vEnd; ++v) {
    ierr = PetscSectionGetDof(cSection, v, &dof);CHKERRQ(ierr);
    if (L && dof == 2) coordSize += dof+1;
    else               coordSize += dof;
  }
  if (L && bs == 2) {ierr = VecSetBlockSize(newcoords, bs+1);CHKERRQ(ierr);}
  else              {ierr = VecSetBlockSize(newcoords, bs);CHKERRQ(ierr);}
  ierr = VecSetSizes(newcoords, coordSize, PETSC_DETERMINE);CHKERRQ(ierr);
  ierr = VecSetType(newcoords,VECSTANDARD);CHKERRQ(ierr);
  ierr = VecGetArray(coordinates, &coords);CHKERRQ(ierr);
  ierr = VecGetArray(newcoords,   &ncoords);CHKERRQ(ierr);
  coordSize = 0;
  for (v = vStart; v < vEnd; ++v) {
    ierr = PetscSectionGetDof(cSection, v, &dof);CHKERRQ(ierr);
    ierr = PetscSectionGetOffset(cSection, v, &off);CHKERRQ(ierr);
    if (L && dof == 2) {
      /* Need to do torus */
      if ((bd[0] == DM_BOUNDARY_PERIODIC) && (bd[1] == DM_BOUNDARY_PERIODIC)) {SETERRQ(PetscObjectComm((PetscObject) dm), PETSC_ERR_SUP, "Cannot embed doubly periodic domain yet");}
      else if ((bd[0] == DM_BOUNDARY_PERIODIC)) {
        /* X-periodic */
        ncoords[coordSize++] = -cos(2.0*PETSC_PI*coords[off+0]/L[0])*(L[0]/(2.0*PETSC_PI));
        ncoords[coordSize++] = coords[off+1];
        ncoords[coordSize++] = sin(2.0*PETSC_PI*coords[off+0]/L[0])*(L[0]/(2.0*PETSC_PI));
      } else if ((bd[1] == DM_BOUNDARY_PERIODIC)) {
        /* Y-periodic */
        ncoords[coordSize++] = coords[off+0];
        ncoords[coordSize++] = sin(2.0*PETSC_PI*coords[off+1]/L[1])*(L[1]/(2.0*PETSC_PI));
        ncoords[coordSize++] = -cos(2.0*PETSC_PI*coords[off+1]/L[1])*(L[1]/(2.0*PETSC_PI));
      } else SETERRQ(PetscObjectComm((PetscObject) dm), PETSC_ERR_SUP, "Cannot handle periodicity in this domain");
    } else {
      for (d = 0; d < dof; ++d, ++coordSize) ncoords[coordSize] = coords[off+d];
    }
  }
  ierr = VecRestoreArray(coordinates, &coords);CHKERRQ(ierr);
  ierr = VecRestoreArray(newcoords,   &ncoords);CHKERRQ(ierr);
  ierr = PetscObjectSetName((PetscObject) newcoords, "vertices");CHKERRQ(ierr);
  ierr = VecScale(newcoords, lengthScale);CHKERRQ(ierr);
  ierr = PetscViewerHDF5PushGroup(viewer, "/viz");CHKERRQ(ierr);
  ierr = PetscViewerHDF5OpenGroup(viewer, &fileId, &groupId);CHKERRQ(ierr);
  PetscStackCallHDF5(H5Gclose,(groupId));
  ierr = PetscViewerHDF5PopGroup(viewer);CHKERRQ(ierr);
  ierr = PetscViewerHDF5PushGroup(viewer, "/viz/geometry");CHKERRQ(ierr);
  ierr = VecView(newcoords, viewer);CHKERRQ(ierr);
  ierr = PetscViewerHDF5PopGroup(viewer);CHKERRQ(ierr);
  ierr = VecDestroy(&newcoords);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMPlexView_HDF5"
/* We only write cells and vertices. Does this screw up parallel reading? */
PetscErrorCode DMPlexView_HDF5(DM dm, PetscViewer viewer)
{
  DMLabel           label   = NULL;
  PetscInt          labelId = 0;
  IS                globalPointNumbers;
  const PetscInt   *gpoint;
  PetscInt          numLabels, l;
  hid_t             fileId, groupId;
  PetscViewerFormat format;
  PetscErrorCode    ierr;

  PetscFunctionBegin;
  ierr = DMPlexCreatePointNumbering(dm, &globalPointNumbers);CHKERRQ(ierr);
  ierr = PetscViewerGetFormat(viewer, &format);CHKERRQ(ierr);
  ierr = DMPlexWriteCoordinates_HDF5_Static(dm, viewer);CHKERRQ(ierr);
  if (format == PETSC_VIEWER_HDF5_VIZ) {ierr = DMPlexWriteCoordinates_Vertices_HDF5_Static(dm, viewer);CHKERRQ(ierr);}
  ierr = DMPlexWriteTopology_HDF5_Static(dm, globalPointNumbers, viewer);CHKERRQ(ierr);
  if (format == PETSC_VIEWER_HDF5_VIZ) {ierr = DMPlexWriteTopology_Vertices_HDF5_Static(dm, label, labelId, viewer);CHKERRQ(ierr);}
  /* Write Labels*/
  ierr = ISGetIndices(globalPointNumbers, &gpoint);CHKERRQ(ierr);
  ierr = PetscViewerHDF5PushGroup(viewer, "/labels");CHKERRQ(ierr);
  ierr = PetscViewerHDF5OpenGroup(viewer, &fileId, &groupId);CHKERRQ(ierr);
  if (groupId != fileId) PetscStackCallHDF5(H5Gclose,(groupId));
  ierr = PetscViewerHDF5PopGroup(viewer);CHKERRQ(ierr);
  ierr = DMGetNumLabels(dm, &numLabels);CHKERRQ(ierr);
  for (l = 0; l < numLabels; ++l) {
    DMLabel         label;
    const char     *name;
    IS              valueIS, globalValueIS;
    const PetscInt *values;
    PetscInt        numValues, v;
    PetscBool       isDepth, output;
    char            group[PETSC_MAX_PATH_LEN];

    ierr = DMGetLabelName(dm, l, &name);CHKERRQ(ierr);
    ierr = DMGetLabelOutput(dm, name, &output);CHKERRQ(ierr);
    ierr = PetscStrncmp(name, "depth", 10, &isDepth);CHKERRQ(ierr);
    if (isDepth || !output) continue;
    ierr = DMGetLabel(dm, name, &label);CHKERRQ(ierr);
    ierr = PetscSNPrintf(group, PETSC_MAX_PATH_LEN, "/labels/%s", name);CHKERRQ(ierr);
    ierr = PetscViewerHDF5PushGroup(viewer, group);CHKERRQ(ierr);
    ierr = PetscViewerHDF5OpenGroup(viewer, &fileId, &groupId);CHKERRQ(ierr);
    if (groupId != fileId) PetscStackCallHDF5(H5Gclose,(groupId));
    ierr = PetscViewerHDF5PopGroup(viewer);CHKERRQ(ierr);
    ierr = DMLabelGetValueIS(label, &valueIS);CHKERRQ(ierr);
    ierr = ISAllGather(valueIS, &globalValueIS);CHKERRQ(ierr);
    ierr = ISSortRemoveDups(globalValueIS);CHKERRQ(ierr);
    ierr = ISGetLocalSize(globalValueIS, &numValues);CHKERRQ(ierr);
    ierr = ISGetIndices(globalValueIS, &values);CHKERRQ(ierr);
    for (v = 0; v < numValues; ++v) {
      IS              stratumIS, globalStratumIS;
      const PetscInt *spoints;
      PetscInt       *gspoints, n, gn, p;
      const char     *iname;

      ierr = PetscSNPrintf(group, PETSC_MAX_PATH_LEN, "/labels/%s/%d", name, values[v]);CHKERRQ(ierr);
      ierr = DMLabelGetStratumIS(label, values[v], &stratumIS);CHKERRQ(ierr);

      ierr = ISGetLocalSize(stratumIS, &n);CHKERRQ(ierr);
      ierr = ISGetIndices(stratumIS, &spoints);CHKERRQ(ierr);
      for (gn = 0, p = 0; p < n; ++p) if (gpoint[spoints[p]] >= 0) ++gn;
      ierr = PetscMalloc1(gn,&gspoints);CHKERRQ(ierr);
      for (gn = 0, p = 0; p < n; ++p) if (gpoint[spoints[p]] >= 0) gspoints[gn++] = gpoint[spoints[p]];
      ierr = ISRestoreIndices(stratumIS, &spoints);CHKERRQ(ierr);
      ierr = ISCreateGeneral(PetscObjectComm((PetscObject) dm), gn, gspoints, PETSC_OWN_POINTER, &globalStratumIS);CHKERRQ(ierr);
      ierr = PetscObjectGetName((PetscObject) stratumIS, &iname);CHKERRQ(ierr);
      ierr = PetscObjectSetName((PetscObject) globalStratumIS, iname);CHKERRQ(ierr);

      ierr = PetscViewerHDF5PushGroup(viewer, group);CHKERRQ(ierr);
      ierr = ISView(globalStratumIS, viewer);CHKERRQ(ierr);
      ierr = PetscViewerHDF5PopGroup(viewer);CHKERRQ(ierr);
      ierr = ISDestroy(&globalStratumIS);CHKERRQ(ierr);
      ierr = ISDestroy(&stratumIS);CHKERRQ(ierr);
    }
    ierr = ISRestoreIndices(globalValueIS, &values);CHKERRQ(ierr);
    ierr = ISDestroy(&globalValueIS);CHKERRQ(ierr);
    ierr = ISDestroy(&valueIS);CHKERRQ(ierr);
  }
  ierr = ISRestoreIndices(globalPointNumbers, &gpoint);CHKERRQ(ierr);
  ierr = ISDestroy(&globalPointNumbers);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

typedef struct {
  PetscMPIInt rank;
  DM          dm;
  PetscViewer viewer;
  DMLabel     label;
} LabelCtx;

static herr_t ReadLabelStratumHDF5_Static(hid_t g_id, const char *name, const H5L_info_t *info, void *op_data)
{
  PetscViewer     viewer = ((LabelCtx *) op_data)->viewer;
  DMLabel         label  = ((LabelCtx *) op_data)->label;
  IS              stratumIS;
  const PetscInt *ind;
  PetscInt        value, N, i;
  const char     *lname;
  char            group[PETSC_MAX_PATH_LEN];
  PetscErrorCode  ierr;

  ierr = PetscOptionsStringToInt(name, &value);
  ierr = ISCreate(PetscObjectComm((PetscObject) viewer), &stratumIS);
  ierr = PetscObjectSetName((PetscObject) stratumIS, "indices");
  ierr = DMLabelGetName(label, &lname);
  ierr = PetscSNPrintf(group, PETSC_MAX_PATH_LEN, "/labels/%s/%s", lname, name);CHKERRQ(ierr);
  ierr = PetscViewerHDF5PushGroup(viewer, group);CHKERRQ(ierr);
  {
    /* Force serial load */
    ierr = PetscViewerHDF5ReadSizes(viewer, "indices", NULL, &N);CHKERRQ(ierr);
    ierr = PetscLayoutSetLocalSize(stratumIS->map, !((LabelCtx *) op_data)->rank ? N : 0);CHKERRQ(ierr);
    ierr = PetscLayoutSetSize(stratumIS->map, N);CHKERRQ(ierr);
  }
  ierr = ISLoad(stratumIS, viewer);
  ierr = PetscViewerHDF5PopGroup(viewer);CHKERRQ(ierr);
  ierr = ISGetLocalSize(stratumIS, &N);
  ierr = ISGetIndices(stratumIS, &ind);
  for (i = 0; i < N; ++i) {ierr = DMLabelSetValue(label, ind[i], value);}
  ierr = ISRestoreIndices(stratumIS, &ind);
  ierr = ISDestroy(&stratumIS);
  return 0;
}

static herr_t ReadLabelHDF5_Static(hid_t g_id, const char *name, const H5L_info_t *info, void *op_data)
{
  DM             dm  = ((LabelCtx *) op_data)->dm;
  hsize_t        idx = 0;
  PetscErrorCode ierr;
  herr_t         err;

  ierr = DMCreateLabel(dm, name); if (ierr) return (herr_t) ierr;
  ierr = DMGetLabel(dm, name, &((LabelCtx *) op_data)->label); if (ierr) return (herr_t) ierr;
  PetscStackCall("H5Literate_by_name",err = H5Literate_by_name(g_id, name, H5_INDEX_NAME, H5_ITER_NATIVE, &idx, ReadLabelStratumHDF5_Static, op_data, 0));
  return err;
}

#undef __FUNCT__
#define __FUNCT__ "DMPlexLoad_HDF5"
/* The first version will read everything onto proc 0, letting the user distribute
   The next will create a naive partition, and then rebalance after reading
*/
PetscErrorCode DMPlexLoad_HDF5(DM dm, PetscViewer viewer)
{
  LabelCtx        ctx;
  PetscSection    coordSection;
  Vec             coordinates;
  IS              orderIS, conesIS, cellsIS, orntsIS;
  const PetscInt *order, *cones, *cells, *ornts;
  PetscReal       lengthScale;
  PetscInt       *cone, *ornt;
  PetscInt        dim, spatialDim, N, numVertices, vStart, vEnd, v, pEnd, p, q, maxConeSize = 0, c;
  hid_t           fileId, groupId;
  hsize_t         idx = 0;
  PetscMPIInt     rank;
  PetscErrorCode  ierr;

  PetscFunctionBegin;
  ierr = MPI_Comm_rank(PetscObjectComm((PetscObject) dm), &rank);CHKERRQ(ierr);
  /* Read toplogy */
  ierr = PetscViewerHDF5ReadAttribute(viewer, "/topology/cells", "cell_dim", PETSC_INT, (void *) &dim);CHKERRQ(ierr);
  ierr = DMSetDimension(dm, dim);CHKERRQ(ierr);
  ierr = PetscViewerHDF5PushGroup(viewer, "/topology");CHKERRQ(ierr);

  ierr = ISCreate(PetscObjectComm((PetscObject) dm), &orderIS);CHKERRQ(ierr);
  ierr = PetscObjectSetName((PetscObject) orderIS, "order");CHKERRQ(ierr);
  ierr = ISCreate(PetscObjectComm((PetscObject) dm), &conesIS);CHKERRQ(ierr);
  ierr = PetscObjectSetName((PetscObject) conesIS, "cones");CHKERRQ(ierr);
  ierr = ISCreate(PetscObjectComm((PetscObject) dm), &cellsIS);CHKERRQ(ierr);
  ierr = PetscObjectSetName((PetscObject) cellsIS, "cells");CHKERRQ(ierr);
  ierr = ISCreate(PetscObjectComm((PetscObject) dm), &orntsIS);CHKERRQ(ierr);
  ierr = PetscObjectSetName((PetscObject) orntsIS, "orientation");CHKERRQ(ierr);
  {
    /* Force serial load */
    ierr = PetscViewerHDF5ReadSizes(viewer, "order", NULL, &pEnd);CHKERRQ(ierr);
    ierr = PetscLayoutSetLocalSize(orderIS->map, !rank ? pEnd : 0);CHKERRQ(ierr);
    ierr = PetscLayoutSetSize(orderIS->map, pEnd);CHKERRQ(ierr);
    ierr = PetscViewerHDF5ReadSizes(viewer, "cones", NULL, &pEnd);CHKERRQ(ierr);
    ierr = PetscLayoutSetLocalSize(conesIS->map, !rank ? pEnd : 0);CHKERRQ(ierr);
    ierr = PetscLayoutSetSize(conesIS->map, pEnd);CHKERRQ(ierr);
    pEnd = !rank ? pEnd : 0;
    ierr = PetscViewerHDF5ReadSizes(viewer, "cells", NULL, &N);CHKERRQ(ierr);
    ierr = PetscLayoutSetLocalSize(cellsIS->map, !rank ? N : 0);CHKERRQ(ierr);
    ierr = PetscLayoutSetSize(cellsIS->map, N);CHKERRQ(ierr);
    ierr = PetscViewerHDF5ReadSizes(viewer, "orientation", NULL, &N);CHKERRQ(ierr);
    ierr = PetscLayoutSetLocalSize(orntsIS->map, !rank ? N : 0);CHKERRQ(ierr);
    ierr = PetscLayoutSetSize(orntsIS->map, N);CHKERRQ(ierr);
  }
  ierr = ISLoad(orderIS, viewer);CHKERRQ(ierr);
  ierr = ISLoad(conesIS, viewer);CHKERRQ(ierr);
  ierr = ISLoad(cellsIS, viewer);CHKERRQ(ierr);
  ierr = ISLoad(orntsIS, viewer);CHKERRQ(ierr);
  ierr = PetscViewerHDF5PopGroup(viewer);CHKERRQ(ierr);
  /* Read geometry */
  ierr = PetscViewerHDF5PushGroup(viewer, "/geometry");CHKERRQ(ierr);
  ierr = VecCreate(PetscObjectComm((PetscObject) dm), &coordinates);CHKERRQ(ierr);
  ierr = PetscObjectSetName((PetscObject) coordinates, "vertices");CHKERRQ(ierr);
  {
    /* Force serial load */
    ierr = PetscViewerHDF5ReadSizes(viewer, "vertices", &spatialDim, &N);CHKERRQ(ierr);
    ierr = VecSetSizes(coordinates, !rank ? N : 0, N);CHKERRQ(ierr);
    ierr = VecSetBlockSize(coordinates, spatialDim);CHKERRQ(ierr);
  }
  ierr = VecLoad(coordinates, viewer);CHKERRQ(ierr);
  ierr = PetscViewerHDF5PopGroup(viewer);CHKERRQ(ierr);
  ierr = DMPlexGetScale(dm, PETSC_UNIT_LENGTH, &lengthScale);CHKERRQ(ierr);
  ierr = VecScale(coordinates, 1.0/lengthScale);CHKERRQ(ierr);
  ierr = VecGetLocalSize(coordinates, &numVertices);CHKERRQ(ierr);
  ierr = VecGetBlockSize(coordinates, &spatialDim);CHKERRQ(ierr);
  numVertices /= spatialDim;
  /* Create Plex */
  ierr = DMPlexSetChart(dm, 0, pEnd);CHKERRQ(ierr);
  ierr = ISGetIndices(orderIS, &order);CHKERRQ(ierr);
  ierr = ISGetIndices(conesIS, &cones);CHKERRQ(ierr);
  for (p = 0; p < pEnd; ++p) {
    ierr = DMPlexSetConeSize(dm, order[p], cones[p]);CHKERRQ(ierr);
    maxConeSize = PetscMax(maxConeSize, cones[p]);
  }
  ierr = DMSetUp(dm);CHKERRQ(ierr);
  ierr = ISGetIndices(cellsIS, &cells);CHKERRQ(ierr);
  ierr = ISGetIndices(orntsIS, &ornts);CHKERRQ(ierr);
  ierr = PetscMalloc2(maxConeSize,&cone,maxConeSize,&ornt);CHKERRQ(ierr);
  for (p = 0, q = 0; p < pEnd; ++p) {
    for (c = 0; c < cones[p]; ++c, ++q) {cone[c] = cells[q]; ornt[c] = ornts[q];}
    ierr = DMPlexSetCone(dm, order[p], cone);CHKERRQ(ierr);
    ierr = DMPlexSetConeOrientation(dm, order[p], ornt);CHKERRQ(ierr);
  }
  ierr = PetscFree2(cone,ornt);CHKERRQ(ierr);
  ierr = ISRestoreIndices(orderIS, &order);CHKERRQ(ierr);
  ierr = ISRestoreIndices(conesIS, &cones);CHKERRQ(ierr);
  ierr = ISRestoreIndices(cellsIS, &cells);CHKERRQ(ierr);
  ierr = ISRestoreIndices(orntsIS, &ornts);CHKERRQ(ierr);
  ierr = ISDestroy(&orderIS);CHKERRQ(ierr);
  ierr = ISDestroy(&conesIS);CHKERRQ(ierr);
  ierr = ISDestroy(&cellsIS);CHKERRQ(ierr);
  ierr = ISDestroy(&orntsIS);CHKERRQ(ierr);
  ierr = DMPlexSymmetrize(dm);CHKERRQ(ierr);
  ierr = DMPlexStratify(dm);CHKERRQ(ierr);
  /* Create coordinates */
  ierr = DMPlexGetDepthStratum(dm, 0, &vStart, &vEnd);CHKERRQ(ierr);
  if (numVertices != vEnd - vStart) SETERRQ2(PETSC_COMM_SELF, PETSC_ERR_ARG_WRONG, "Number of coordinates loaded %d does not match number of vertices %d", numVertices, vEnd - vStart);
  ierr = DMGetCoordinateSection(dm, &coordSection);CHKERRQ(ierr);
  ierr = PetscSectionSetNumFields(coordSection, 1);CHKERRQ(ierr);
  ierr = PetscSectionSetFieldComponents(coordSection, 0, spatialDim);CHKERRQ(ierr);
  ierr = PetscSectionSetChart(coordSection, vStart, vEnd);CHKERRQ(ierr);
  for (v = vStart; v < vEnd; ++v) {
    ierr = PetscSectionSetDof(coordSection, v, spatialDim);CHKERRQ(ierr);
    ierr = PetscSectionSetFieldDof(coordSection, v, 0, spatialDim);CHKERRQ(ierr);
  }
  ierr = PetscSectionSetUp(coordSection);CHKERRQ(ierr);
  ierr = DMSetCoordinates(dm, coordinates);CHKERRQ(ierr);
  ierr = VecDestroy(&coordinates);CHKERRQ(ierr);
  /* Read Labels*/
  ctx.rank   = rank;
  ctx.dm     = dm;
  ctx.viewer = viewer;
  ierr = PetscViewerHDF5PushGroup(viewer, "/labels");CHKERRQ(ierr);
  ierr = PetscViewerHDF5OpenGroup(viewer, &fileId, &groupId);CHKERRQ(ierr);
  PetscStackCallHDF5(H5Literate,(groupId, H5_INDEX_NAME, H5_ITER_NATIVE, &idx, ReadLabelHDF5_Static, &ctx));
  PetscStackCallHDF5(H5Gclose,(groupId));
  ierr = PetscViewerHDF5PopGroup(viewer);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
#endif
