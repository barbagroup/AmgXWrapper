#define PETSCDM_DLL
#include <petsc/private/dmpleximpl.h>    /*I   "petscdmplex.h"   I*/
#include <../src/sys/classes/viewer/impls/vtk/vtkvimpl.h>

#undef __FUNCT__
#define __FUNCT__ "DMPlexVTKGetCellType"
PetscErrorCode DMPlexVTKGetCellType(DM dm, PetscInt dim, PetscInt corners, PetscInt *cellType)
{
  PetscFunctionBegin;
  *cellType = -1;
  switch (dim) {
  case 0:
    switch (corners) {
    case 1:
      *cellType = 1; /* VTK_VERTEX */
      break;
    default:
      break;
    }
    break;
  case 1:
    switch (corners) {
    case 2:
      *cellType = 3; /* VTK_LINE */
      break;
    case 3:
      *cellType = 21; /* VTK_QUADRATIC_EDGE */
      break;
    default:
      break;
    }
    break;
  case 2:
    switch (corners) {
    case 3:
      *cellType = 5; /* VTK_TRIANGLE */
      break;
    case 4:
      *cellType = 9; /* VTK_QUAD */
      break;
    case 6:
      *cellType = 22; /* VTK_QUADRATIC_TRIANGLE */
      break;
    case 9:
      *cellType = 23; /* VTK_QUADRATIC_QUAD */
      break;
    default:
      break;
    }
    break;
  case 3:
    switch (corners) {
    case 4:
      *cellType = 10; /* VTK_TETRA */
      break;
    case 8:
      *cellType = 12; /* VTK_HEXAHEDRON */
      break;
    case 10:
      *cellType = 24; /* VTK_QUADRATIC_TETRA */
      break;
    case 27:
      *cellType = 29; /* VTK_QUADRATIC_HEXAHEDRON */
      break;
    default:
      break;
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMPlexVTKWriteCells_ASCII"
PetscErrorCode DMPlexVTKWriteCells_ASCII(DM dm, FILE *fp, PetscInt *totalCells)
{
  MPI_Comm       comm;
  DMLabel        label;
  IS             globalVertexNumbers = NULL;
  const PetscInt *gvertex;
  PetscInt       dim;
  PetscInt       numCorners = 0, totCorners = 0, maxCorners, *corners;
  PetscInt       numCells   = 0, totCells   = 0, maxCells, cellHeight;
  PetscInt       numLabelCells, maxLabelCells, cMax, cStart, cEnd, c, vStart, vEnd, v;
  PetscMPIInt    numProcs, rank, proc, tag;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscObjectGetComm((PetscObject)dm,&comm);CHKERRQ(ierr);
  ierr = PetscCommGetNewTag(comm, &tag);CHKERRQ(ierr);
  ierr = MPI_Comm_size(comm, &numProcs);CHKERRQ(ierr);
  ierr = MPI_Comm_rank(comm, &rank);CHKERRQ(ierr);
  ierr = DMGetDimension(dm, &dim);CHKERRQ(ierr);
  ierr = DMPlexGetVTKCellHeight(dm, &cellHeight);CHKERRQ(ierr);
  ierr = DMPlexGetHeightStratum(dm, cellHeight, &cStart, &cEnd);CHKERRQ(ierr);
  ierr = DMPlexGetDepthStratum(dm, 0, &vStart, &vEnd);CHKERRQ(ierr);
  ierr = DMPlexGetHybridBounds(dm, &cMax, NULL, NULL, NULL);CHKERRQ(ierr);
  if (cMax >= 0) cEnd = PetscMin(cEnd, cMax);
  ierr = DMGetLabel(dm, "vtk", &label);CHKERRQ(ierr);
  ierr = DMGetStratumSize(dm, "vtk", 1, &numLabelCells);CHKERRQ(ierr);
  ierr = MPIU_Allreduce(&numLabelCells, &maxLabelCells, 1, MPIU_INT, MPI_MAX, comm);CHKERRQ(ierr);
  if (!maxLabelCells) label = NULL;
  for (c = cStart; c < cEnd; ++c) {
    PetscInt *closure = NULL;
    PetscInt closureSize, value;

    if (label) {
      ierr = DMLabelGetValue(label, c, &value);CHKERRQ(ierr);
      if (value != 1) continue;
    }
    ierr = DMPlexGetTransitiveClosure(dm, c, PETSC_TRUE, &closureSize, &closure);CHKERRQ(ierr);
    for (v = 0; v < closureSize*2; v += 2) {
      if ((closure[v] >= vStart) && (closure[v] < vEnd)) ++numCorners;
    }
    ierr = DMPlexRestoreTransitiveClosure(dm, c, PETSC_TRUE, &closureSize, &closure);CHKERRQ(ierr);
    ++numCells;
  }
  maxCells = numCells;
  ierr     = MPI_Reduce(&numCells, &totCells, 1, MPIU_INT, MPI_SUM, 0, comm);CHKERRQ(ierr);
  ierr     = MPI_Reduce(&numCells, &maxCells, 1, MPIU_INT, MPI_MAX, 0, comm);CHKERRQ(ierr);
  ierr     = MPI_Reduce(&numCorners, &totCorners, 1, MPIU_INT, MPI_SUM, 0, comm);CHKERRQ(ierr);
  ierr     = MPI_Reduce(&numCorners, &maxCorners, 1, MPIU_INT, MPI_MAX, 0, comm);CHKERRQ(ierr);
  ierr     = DMPlexGetVertexNumbering(dm, &globalVertexNumbers);CHKERRQ(ierr);
  ierr     = ISGetIndices(globalVertexNumbers, &gvertex);CHKERRQ(ierr);
  ierr     = PetscMalloc1(maxCells, &corners);CHKERRQ(ierr);
  ierr     = PetscFPrintf(comm, fp, "CELLS %d %d\n", totCells, totCorners+totCells);CHKERRQ(ierr);
  if (!rank) {
    PetscInt *remoteVertices;
    int      *vertices;

    ierr = PetscMalloc1(maxCorners, &vertices);CHKERRQ(ierr);
    for (c = cStart, numCells = 0; c < cEnd; ++c) {
      PetscInt *closure = NULL;
      PetscInt closureSize, value, nC = 0;

      if (label) {
        ierr = DMLabelGetValue(label, c, &value);CHKERRQ(ierr);
        if (value != 1) continue;
      }
      ierr = DMPlexGetTransitiveClosure(dm, c, PETSC_TRUE, &closureSize, &closure);CHKERRQ(ierr);
      for (v = 0; v < closureSize*2; v += 2) {
        if ((closure[v] >= vStart) && (closure[v] < vEnd)) {
          const PetscInt gv = gvertex[closure[v] - vStart];
          vertices[nC++] = gv < 0 ? -(gv+1) : gv;
        }
      }
      ierr = DMPlexRestoreTransitiveClosure(dm, c, PETSC_TRUE, &closureSize, &closure);CHKERRQ(ierr);
      corners[numCells++] = nC;
      ierr = PetscFPrintf(comm, fp, "%d ", nC);CHKERRQ(ierr);
      ierr = DMPlexInvertCell(dim, nC, vertices);CHKERRQ(ierr);
      for (v = 0; v < nC; ++v) {
        ierr = PetscFPrintf(comm, fp, " %d", vertices[v]);CHKERRQ(ierr);
      }
      ierr = PetscFPrintf(comm, fp, "\n");CHKERRQ(ierr);
    }
    if (numProcs > 1) {ierr = PetscMalloc1(maxCorners+maxCells, &remoteVertices);CHKERRQ(ierr);}
    for (proc = 1; proc < numProcs; ++proc) {
      MPI_Status status;

      ierr = MPI_Recv(&numCorners, 1, MPIU_INT, proc, tag, comm, &status);CHKERRQ(ierr);
      ierr = MPI_Recv(remoteVertices, numCorners, MPIU_INT, proc, tag, comm, &status);CHKERRQ(ierr);
      for (c = 0; c < numCorners;) {
        PetscInt nC = remoteVertices[c++];

        for (v = 0; v < nC; ++v, ++c) {
          vertices[v] = remoteVertices[c];
        }
        ierr = DMPlexInvertCell(dim, nC, vertices);CHKERRQ(ierr);
        ierr = PetscFPrintf(comm, fp, "%d ", nC);CHKERRQ(ierr);
        for (v = 0; v < nC; ++v) {
          ierr = PetscFPrintf(comm, fp, " %d", vertices[v]);CHKERRQ(ierr);
        }
        ierr = PetscFPrintf(comm, fp, "\n");CHKERRQ(ierr);
      }
    }
    if (numProcs > 1) {ierr = PetscFree(remoteVertices);CHKERRQ(ierr);}
    ierr = PetscFree(vertices);CHKERRQ(ierr);
  } else {
    PetscInt *localVertices, numSend = numCells+numCorners, k = 0;

    ierr = PetscMalloc1(numSend, &localVertices);CHKERRQ(ierr);
    for (c = cStart, numCells = 0; c < cEnd; ++c) {
      PetscInt *closure = NULL;
      PetscInt closureSize, value, nC = 0;

      if (label) {
        ierr = DMLabelGetValue(label, c, &value);CHKERRQ(ierr);
        if (value != 1) continue;
      }
      ierr = DMPlexGetTransitiveClosure(dm, c, PETSC_TRUE, &closureSize, &closure);CHKERRQ(ierr);
      for (v = 0; v < closureSize*2; v += 2) {
        if ((closure[v] >= vStart) && (closure[v] < vEnd)) {
          const PetscInt gv = gvertex[closure[v] - vStart];
          closure[nC++] = gv < 0 ? -(gv+1) : gv;
        }
      }
      corners[numCells++] = nC;
      localVertices[k++]  = nC;
      for (v = 0; v < nC; ++v, ++k) {
        localVertices[k] = closure[v];
      }
      ierr = DMPlexRestoreTransitiveClosure(dm, c, PETSC_TRUE, &closureSize, &closure);CHKERRQ(ierr);
    }
    if (k != numSend) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_PLIB, "Invalid number of vertices to send %d should be %d", k, numSend);
    ierr = MPI_Send(&numSend, 1, MPIU_INT, 0, tag, comm);CHKERRQ(ierr);
    ierr = MPI_Send(localVertices, numSend, MPIU_INT, 0, tag, comm);CHKERRQ(ierr);
    ierr = PetscFree(localVertices);CHKERRQ(ierr);
  }
  ierr = ISRestoreIndices(globalVertexNumbers, &gvertex);CHKERRQ(ierr);
  ierr = PetscFPrintf(comm, fp, "CELL_TYPES %d\n", totCells);CHKERRQ(ierr);
  if (!rank) {
    PetscInt cellType;

    for (c = 0; c < numCells; ++c) {
      ierr = DMPlexVTKGetCellType(dm, dim, corners[c], &cellType);CHKERRQ(ierr);
      ierr = PetscFPrintf(comm, fp, "%d\n", cellType);CHKERRQ(ierr);
    }
    for (proc = 1; proc < numProcs; ++proc) {
      MPI_Status status;

      ierr = MPI_Recv(&numCells, 1, MPIU_INT, proc, tag, comm, &status);CHKERRQ(ierr);
      ierr = MPI_Recv(corners, numCells, MPIU_INT, proc, tag, comm, &status);CHKERRQ(ierr);
      for (c = 0; c < numCells; ++c) {
        ierr = DMPlexVTKGetCellType(dm, dim, corners[c], &cellType);CHKERRQ(ierr);
        ierr = PetscFPrintf(comm, fp, "%d\n", cellType);CHKERRQ(ierr);
      }
    }
  } else {
    ierr = MPI_Send(&numCells, 1, MPIU_INT, 0, tag, comm);CHKERRQ(ierr);
    ierr = MPI_Send(corners, numCells, MPIU_INT, 0, tag, comm);CHKERRQ(ierr);
  }
  ierr        = PetscFree(corners);CHKERRQ(ierr);
  *totalCells = totCells;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMPlexVTKWritePartition_ASCII"
PetscErrorCode DMPlexVTKWritePartition_ASCII(DM dm, FILE *fp)
{
  MPI_Comm       comm;
  PetscInt       numCells = 0, cellHeight;
  PetscInt       numLabelCells, cMax, cStart, cEnd, c;
  PetscMPIInt    numProcs, rank, proc, tag;
  PetscBool      hasLabel;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscObjectGetComm((PetscObject)dm,&comm);CHKERRQ(ierr);
  ierr = PetscCommGetNewTag(comm, &tag);CHKERRQ(ierr);
  ierr = MPI_Comm_size(comm, &numProcs);CHKERRQ(ierr);
  ierr = MPI_Comm_rank(comm, &rank);CHKERRQ(ierr);
  ierr = DMPlexGetVTKCellHeight(dm, &cellHeight);CHKERRQ(ierr);
  ierr = DMPlexGetHeightStratum(dm, cellHeight, &cStart, &cEnd);CHKERRQ(ierr);
  ierr = DMPlexGetHybridBounds(dm, &cMax, NULL, NULL, NULL);CHKERRQ(ierr);
  if (cMax >= 0) cEnd = PetscMin(cEnd, cMax);
  ierr = DMGetStratumSize(dm, "vtk", 1, &numLabelCells);CHKERRQ(ierr);
  hasLabel = numLabelCells > 0 ? PETSC_TRUE : PETSC_FALSE;
  for (c = cStart; c < cEnd; ++c) {
    if (hasLabel) {
      PetscInt value;

      ierr = DMGetLabelValue(dm, "vtk", c, &value);CHKERRQ(ierr);
      if (value != 1) continue;
    }
    ++numCells;
  }
  if (!rank) {
    for (c = 0; c < numCells; ++c) {ierr = PetscFPrintf(comm, fp, "%d\n", rank);CHKERRQ(ierr);}
    for (proc = 1; proc < numProcs; ++proc) {
      MPI_Status status;

      ierr = MPI_Recv(&numCells, 1, MPIU_INT, proc, tag, comm, &status);CHKERRQ(ierr);
      for (c = 0; c < numCells; ++c) {ierr = PetscFPrintf(comm, fp, "%d\n", proc);CHKERRQ(ierr);}
    }
  } else {
    ierr = MPI_Send(&numCells, 1, MPIU_INT, 0, tag, comm);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMPlexVTKWriteSection_ASCII"
PetscErrorCode DMPlexVTKWriteSection_ASCII(DM dm, PetscSection section, PetscSection globalSection, Vec v, FILE *fp, PetscInt enforceDof, PetscInt precision, PetscReal scale)
{
  MPI_Comm           comm;
  const MPI_Datatype mpiType = MPIU_SCALAR;
  PetscScalar        *array;
  PetscInt           numDof = 0, maxDof;
  PetscInt           numLabelCells, cellHeight, cMax, cStart, cEnd, numLabelVertices, vMax, vStart, vEnd, pStart, pEnd, p;
  PetscMPIInt        numProcs, rank, proc, tag;
  PetscBool          hasLabel;
  PetscErrorCode     ierr;

  PetscFunctionBegin;
  ierr = PetscObjectGetComm((PetscObject)dm,&comm);CHKERRQ(ierr);
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  PetscValidHeaderSpecific(v,VEC_CLASSID,4);
  if (precision < 0) precision = 6;
  ierr = PetscCommGetNewTag(comm, &tag);CHKERRQ(ierr);
  ierr = MPI_Comm_size(comm, &numProcs);CHKERRQ(ierr);
  ierr = MPI_Comm_rank(comm, &rank);CHKERRQ(ierr);
  ierr = PetscSectionGetChart(section, &pStart, &pEnd);CHKERRQ(ierr);
  /* VTK only wants the values at cells or vertices */
  ierr = DMPlexGetVTKCellHeight(dm, &cellHeight);CHKERRQ(ierr);
  ierr = DMPlexGetHeightStratum(dm, cellHeight, &cStart, &cEnd);CHKERRQ(ierr);
  ierr = DMPlexGetDepthStratum(dm, 0, &vStart, &vEnd);CHKERRQ(ierr);
  ierr = DMPlexGetHybridBounds(dm, &cMax, NULL, NULL, &vMax);CHKERRQ(ierr);
  if (cMax >= 0) cEnd = PetscMin(cEnd, cMax);
  if (vMax >= 0) vEnd = PetscMin(vEnd, vMax);
  pStart   = PetscMax(PetscMin(cStart, vStart), pStart);
  pEnd     = PetscMin(PetscMax(cEnd,   vEnd),   pEnd);
  ierr     = DMGetStratumSize(dm, "vtk", 1, &numLabelCells);CHKERRQ(ierr);
  ierr     = DMGetStratumSize(dm, "vtk", 2, &numLabelVertices);CHKERRQ(ierr);
  hasLabel = numLabelCells > 0 || numLabelVertices > 0 ? PETSC_TRUE : PETSC_FALSE;
  for (p = pStart; p < pEnd; ++p) {
    /* Reject points not either cells or vertices */
    if (((p < cStart) || (p >= cEnd)) && ((p < vStart) || (p >= vEnd))) continue;
    if (hasLabel) {
      PetscInt value;

      if (((p >= cStart) && (p < cEnd) && numLabelCells) ||
          ((p >= vStart) && (p < vEnd) && numLabelVertices)) {
        ierr = DMGetLabelValue(dm, "vtk", p, &value);CHKERRQ(ierr);
        if (value != 1) continue;
      }
    }
    ierr = PetscSectionGetDof(section, p, &numDof);CHKERRQ(ierr);
    if (numDof) break;
  }
  ierr = MPIU_Allreduce(&numDof, &maxDof, 1, MPIU_INT, MPI_MAX, comm);CHKERRQ(ierr);
  enforceDof = PetscMax(enforceDof, maxDof);
  ierr = VecGetArray(v, &array);CHKERRQ(ierr);
  if (!rank) {
    char formatString[8];

    ierr = PetscSNPrintf(formatString, 8, "%%.%de", precision);CHKERRQ(ierr);
    for (p = pStart; p < pEnd; ++p) {
      /* Here we lose a way to filter points by keeping them out of the Numbering */
      PetscInt dof, off, goff, d;

      /* Reject points not either cells or vertices */
      if (((p < cStart) || (p >= cEnd)) && ((p < vStart) || (p >= vEnd))) continue;
      if (hasLabel) {
        PetscInt value;

        if (((p >= cStart) && (p < cEnd) && numLabelCells) ||
            ((p >= vStart) && (p < vEnd) && numLabelVertices)) {
          ierr = DMGetLabelValue(dm, "vtk", p, &value);CHKERRQ(ierr);
          if (value != 1) continue;
        }
      }
      ierr = PetscSectionGetDof(section, p, &dof);CHKERRQ(ierr);
      ierr = PetscSectionGetOffset(section, p, &off);CHKERRQ(ierr);
      ierr = PetscSectionGetOffset(globalSection, p, &goff);CHKERRQ(ierr);
      if (dof && goff >= 0) {
        for (d = 0; d < dof; d++) {
          if (d > 0) {
            ierr = PetscFPrintf(comm, fp, " ");CHKERRQ(ierr);
          }
          ierr = PetscFPrintf(comm, fp, formatString, PetscRealPart(array[off+d])*scale);CHKERRQ(ierr);
        }
        for (d = dof; d < enforceDof; d++) {
          ierr = PetscFPrintf(comm, fp, " 0.0");CHKERRQ(ierr);
        }
        ierr = PetscFPrintf(comm, fp, "\n");CHKERRQ(ierr);
      }
    }
    for (proc = 1; proc < numProcs; ++proc) {
      PetscScalar *remoteValues;
      PetscInt    size = 0, d;
      MPI_Status  status;

      ierr = MPI_Recv(&size, 1, MPIU_INT, proc, tag, comm, &status);CHKERRQ(ierr);
      ierr = PetscMalloc1(size, &remoteValues);CHKERRQ(ierr);
      ierr = MPI_Recv(remoteValues, size, mpiType, proc, tag, comm, &status);CHKERRQ(ierr);
      for (p = 0; p < size/maxDof; ++p) {
        for (d = 0; d < maxDof; ++d) {
          if (d > 0) {
            ierr = PetscFPrintf(comm, fp, " ");CHKERRQ(ierr);
          }
          ierr = PetscFPrintf(comm, fp, formatString, PetscRealPart(remoteValues[p*maxDof+d])*scale);CHKERRQ(ierr);
        }
        for (d = maxDof; d < enforceDof; ++d) {
          ierr = PetscFPrintf(comm, fp, " 0.0");CHKERRQ(ierr);
        }
        ierr = PetscFPrintf(comm, fp, "\n");CHKERRQ(ierr);
      }
      ierr = PetscFree(remoteValues);CHKERRQ(ierr);
    }
  } else {
    PetscScalar *localValues;
    PetscInt    size, k = 0;

    ierr = PetscSectionGetStorageSize(section, &size);CHKERRQ(ierr);
    ierr = PetscMalloc1(size, &localValues);CHKERRQ(ierr);
    for (p = pStart; p < pEnd; ++p) {
      PetscInt dof, off, goff, d;

      /* Reject points not either cells or vertices */
      if (((p < cStart) || (p >= cEnd)) && ((p < vStart) || (p >= vEnd))) continue;
      if (hasLabel) {
        PetscInt value;

        if (((p >= cStart) && (p < cEnd) && numLabelCells) ||
            ((p >= vStart) && (p < vEnd) && numLabelVertices)) {
          ierr = DMGetLabelValue(dm, "vtk", p, &value);CHKERRQ(ierr);
          if (value != 1) continue;
        }
      }
      ierr = PetscSectionGetDof(section, p, &dof);CHKERRQ(ierr);
      ierr = PetscSectionGetOffset(section, p, &off);CHKERRQ(ierr);
      ierr = PetscSectionGetOffset(globalSection, p, &goff);CHKERRQ(ierr);
      if (goff >= 0) {
        for (d = 0; d < dof; ++d) {
          localValues[k++] = array[off+d];
        }
      }
    }
    ierr = MPI_Send(&k, 1, MPIU_INT, 0, tag, comm);CHKERRQ(ierr);
    ierr = MPI_Send(localValues, k, mpiType, 0, tag, comm);CHKERRQ(ierr);
    ierr = PetscFree(localValues);CHKERRQ(ierr);
  }
  ierr = VecRestoreArray(v, &array);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMPlexVTKWriteField_ASCII"
PetscErrorCode DMPlexVTKWriteField_ASCII(DM dm, PetscSection section, PetscSection globalSection, Vec field, const char name[], FILE *fp, PetscInt enforceDof, PetscInt precision, PetscReal scale)
{
  MPI_Comm       comm;
  PetscInt       numDof = 0, maxDof;
  PetscInt       pStart, pEnd, p;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscObjectGetComm((PetscObject)dm,&comm);CHKERRQ(ierr);
  ierr = PetscSectionGetChart(section, &pStart, &pEnd);CHKERRQ(ierr);
  for (p = pStart; p < pEnd; ++p) {
    ierr = PetscSectionGetDof(section, p, &numDof);CHKERRQ(ierr);
    if (numDof) break;
  }
  numDof = PetscMax(numDof, enforceDof);
  ierr = MPIU_Allreduce(&numDof, &maxDof, 1, MPIU_INT, MPI_MAX, PetscObjectComm((PetscObject)dm));CHKERRQ(ierr);
  if (!name) name = "Unknown";
  if (maxDof == 3) {
    ierr = PetscFPrintf(comm, fp, "VECTORS %s double\n", name);CHKERRQ(ierr);
  } else {
    ierr = PetscFPrintf(comm, fp, "SCALARS %s double %d\n", name, maxDof);CHKERRQ(ierr);
    ierr = PetscFPrintf(comm, fp, "LOOKUP_TABLE default\n");CHKERRQ(ierr);
  }
  ierr = DMPlexVTKWriteSection_ASCII(dm, section, globalSection, field, fp, enforceDof, precision, scale);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMPlexVTKWriteAll_ASCII"
static PetscErrorCode DMPlexVTKWriteAll_ASCII(DM dm, PetscViewer viewer)
{
  MPI_Comm                 comm;
  PetscViewer_VTK          *vtk = (PetscViewer_VTK*) viewer->data;
  FILE                     *fp;
  PetscViewerVTKObjectLink link;
  PetscSection             coordSection, globalCoordSection;
  PetscLayout              vLayout;
  Vec                      coordinates;
  PetscReal                lengthScale;
  PetscInt                 vMax, totVertices, totCells;
  PetscBool                hasPoint = PETSC_FALSE, hasCell = PETSC_FALSE, writePartition = PETSC_FALSE;
  PetscErrorCode           ierr;

  PetscFunctionBegin;
  ierr = PetscObjectGetComm((PetscObject)dm,&comm);CHKERRQ(ierr);
  ierr = PetscFOpen(comm, vtk->filename, "wb", &fp);CHKERRQ(ierr);
  ierr = PetscFPrintf(comm, fp, "# vtk DataFile Version 2.0\n");CHKERRQ(ierr);
  ierr = PetscFPrintf(comm, fp, "Simplicial Mesh Example\n");CHKERRQ(ierr);
  ierr = PetscFPrintf(comm, fp, "ASCII\n");CHKERRQ(ierr);
  ierr = PetscFPrintf(comm, fp, "DATASET UNSTRUCTURED_GRID\n");CHKERRQ(ierr);
  /* Vertices */
  ierr = DMPlexGetScale(dm, PETSC_UNIT_LENGTH, &lengthScale);CHKERRQ(ierr);
  ierr = DMGetCoordinateSection(dm, &coordSection);CHKERRQ(ierr);
  ierr = PetscSectionCreateGlobalSection(coordSection, dm->sf, PETSC_FALSE, PETSC_FALSE, &globalCoordSection);CHKERRQ(ierr);
  ierr = DMGetCoordinatesLocal(dm, &coordinates);CHKERRQ(ierr);
  ierr = DMPlexGetHybridBounds(dm, NULL, NULL, NULL, &vMax);CHKERRQ(ierr);
  if (vMax >= 0) {
    PetscInt pStart, pEnd, p, localSize = 0;

    ierr = PetscSectionGetChart(globalCoordSection, &pStart, &pEnd);CHKERRQ(ierr);
    pEnd = PetscMin(pEnd, vMax);
    for (p = pStart; p < pEnd; ++p) {
      PetscInt dof;

      ierr = PetscSectionGetDof(globalCoordSection, p, &dof);CHKERRQ(ierr);
      if (dof > 0) ++localSize;
    }
    ierr = PetscLayoutCreate(PetscObjectComm((PetscObject)dm), &vLayout);CHKERRQ(ierr);
    ierr = PetscLayoutSetLocalSize(vLayout, localSize);CHKERRQ(ierr);
    ierr = PetscLayoutSetBlockSize(vLayout, 1);CHKERRQ(ierr);
    ierr = PetscLayoutSetUp(vLayout);CHKERRQ(ierr);
  } else {
    ierr = PetscSectionGetPointLayout(PetscObjectComm((PetscObject)dm), globalCoordSection, &vLayout);CHKERRQ(ierr);
  }
  ierr = PetscLayoutGetSize(vLayout, &totVertices);CHKERRQ(ierr);
  ierr = PetscFPrintf(comm, fp, "POINTS %d double\n", totVertices);CHKERRQ(ierr);
  ierr = DMPlexVTKWriteSection_ASCII(dm, coordSection, globalCoordSection, coordinates, fp, 3, PETSC_DETERMINE, lengthScale);CHKERRQ(ierr);
  /* Cells */
  ierr = DMPlexVTKWriteCells_ASCII(dm, fp, &totCells);CHKERRQ(ierr);
  /* Vertex fields */
  for (link = vtk->link; link; link = link->next) {
    if ((link->ft == PETSC_VTK_POINT_FIELD) || (link->ft == PETSC_VTK_POINT_VECTOR_FIELD)) hasPoint = PETSC_TRUE;
    if ((link->ft == PETSC_VTK_CELL_FIELD)  || (link->ft == PETSC_VTK_CELL_VECTOR_FIELD))  hasCell  = PETSC_TRUE;
  }
  if (hasPoint) {
    ierr = PetscFPrintf(comm, fp, "POINT_DATA %d\n", totVertices);CHKERRQ(ierr);
    for (link = vtk->link; link; link = link->next) {
      Vec          X = (Vec) link->vec;
      DM           dmX;
      PetscSection section, globalSection, newSection = NULL;
      const char   *name;
      PetscInt     enforceDof = PETSC_DETERMINE;

      if ((link->ft != PETSC_VTK_POINT_FIELD) && (link->ft != PETSC_VTK_POINT_VECTOR_FIELD)) continue;
      if (link->ft == PETSC_VTK_POINT_VECTOR_FIELD) enforceDof = 3;
      ierr = PetscObjectGetName(link->vec, &name);CHKERRQ(ierr);
      ierr = VecGetDM(X, &dmX);CHKERRQ(ierr);
      if (dmX) {
        DMLabel  subpointMap, subpointMapX;
        PetscInt dim, dimX, pStart, pEnd, qStart, qEnd;

        ierr = DMGetDefaultSection(dmX, &section);CHKERRQ(ierr);
        /* Here is where we check whether dmX is a submesh of dm */
        ierr = DMGetDimension(dm,  &dim);CHKERRQ(ierr);
        ierr = DMGetDimension(dmX, &dimX);CHKERRQ(ierr);
        ierr = DMPlexGetChart(dm,  &pStart, &pEnd);CHKERRQ(ierr);
        ierr = DMPlexGetChart(dmX, &qStart, &qEnd);CHKERRQ(ierr);
        ierr = DMPlexGetSubpointMap(dm,  &subpointMap);CHKERRQ(ierr);
        ierr = DMPlexGetSubpointMap(dmX, &subpointMapX);CHKERRQ(ierr);
        if (((dim != dimX) || ((pEnd-pStart) < (qEnd-qStart))) && subpointMap && !subpointMapX) {
          const PetscInt *ind = NULL;
          IS              subpointIS;
          PetscInt        n = 0, q;

          ierr = PetscSectionGetChart(section, &qStart, &qEnd);CHKERRQ(ierr);
          ierr = DMPlexCreateSubpointIS(dm, &subpointIS);CHKERRQ(ierr);
          if (subpointIS) {
            ierr = ISGetLocalSize(subpointIS, &n);CHKERRQ(ierr);
            ierr = ISGetIndices(subpointIS, &ind);CHKERRQ(ierr);
          }
          ierr = PetscSectionCreate(comm, &newSection);CHKERRQ(ierr);
          ierr = PetscSectionSetChart(newSection, pStart, pEnd);CHKERRQ(ierr);
          for (q = qStart; q < qEnd; ++q) {
            PetscInt dof, off, p;

            ierr = PetscSectionGetDof(section, q, &dof);CHKERRQ(ierr);
            if (dof) {
              ierr = PetscFindInt(q, n, ind, &p);CHKERRQ(ierr);
              if (p >= pStart) {
                ierr = PetscSectionSetDof(newSection, p, dof);CHKERRQ(ierr);
                ierr = PetscSectionGetOffset(section, q, &off);CHKERRQ(ierr);
                ierr = PetscSectionSetOffset(newSection, p, off);CHKERRQ(ierr);
              }
            }
          }
          if (subpointIS) {
            ierr = ISRestoreIndices(subpointIS, &ind);CHKERRQ(ierr);
            ierr = ISDestroy(&subpointIS);CHKERRQ(ierr);
          }
          /* No need to setup section */
          section = newSection;
        }
      } else {
        ierr = PetscObjectQuery(link->vec, "section", (PetscObject*) &section);CHKERRQ(ierr);
        if (!section) SETERRQ1(PetscObjectComm((PetscObject)dm), PETSC_ERR_ARG_WRONG, "Vector %s had no PetscSection composed with it", name);
      }
      if (!section) SETERRQ1(PetscObjectComm((PetscObject)dm), PETSC_ERR_ARG_WRONG, "Vector %s had no PetscSection composed with it", name);
      ierr = PetscSectionCreateGlobalSection(section, dm->sf, PETSC_FALSE, PETSC_FALSE, &globalSection);CHKERRQ(ierr);
      ierr = DMPlexVTKWriteField_ASCII(dm, section, globalSection, X, name, fp, enforceDof, PETSC_DETERMINE, 1.0);CHKERRQ(ierr);
      ierr = PetscSectionDestroy(&globalSection);CHKERRQ(ierr);
      if (newSection) {ierr = PetscSectionDestroy(&newSection);CHKERRQ(ierr);}
    }
  }
  /* Cell Fields */
  ierr = PetscOptionsGetBool(((PetscObject) dm)->options,((PetscObject) dm)->prefix, "-dm_view_partition", &writePartition, NULL);CHKERRQ(ierr);
  if (hasCell || writePartition) {
    ierr = PetscFPrintf(comm, fp, "CELL_DATA %d\n", totCells);CHKERRQ(ierr);
    for (link = vtk->link; link; link = link->next) {
      Vec          X = (Vec) link->vec;
      DM           dmX;
      PetscSection section, globalSection;
      const char   *name;
      PetscInt     enforceDof = PETSC_DETERMINE;

      if ((link->ft != PETSC_VTK_CELL_FIELD) && (link->ft != PETSC_VTK_CELL_VECTOR_FIELD)) continue;
      if (link->ft == PETSC_VTK_CELL_VECTOR_FIELD) enforceDof = 3;
      ierr = PetscObjectGetName(link->vec, &name);CHKERRQ(ierr);
      ierr = VecGetDM(X, &dmX);CHKERRQ(ierr);
      if (dmX) {
        ierr = DMGetDefaultSection(dmX, &section);CHKERRQ(ierr);
      } else {
        PetscContainer c;

        ierr = PetscObjectQuery(link->vec, "section", (PetscObject*) &c);CHKERRQ(ierr);
        if (!c) SETERRQ1(PetscObjectComm((PetscObject)dm), PETSC_ERR_ARG_WRONG, "Vector %s had no PetscSection composed with it", name);
        ierr = PetscContainerGetPointer(c, (void**) &section);CHKERRQ(ierr);
      }
      if (!section) SETERRQ1(PetscObjectComm((PetscObject)dm), PETSC_ERR_ARG_WRONG, "Vector %s had no PetscSection composed with it", name);
      ierr = PetscSectionCreateGlobalSection(section, dm->sf, PETSC_FALSE, PETSC_FALSE, &globalSection);CHKERRQ(ierr);
      ierr = DMPlexVTKWriteField_ASCII(dm, section, globalSection, X, name, fp, enforceDof, PETSC_DETERMINE, 1.0);CHKERRQ(ierr);
      ierr = PetscSectionDestroy(&globalSection);CHKERRQ(ierr);
    }
    if (writePartition) {
      ierr = PetscFPrintf(comm, fp, "SCALARS partition int 1\n");CHKERRQ(ierr);
      ierr = PetscFPrintf(comm, fp, "LOOKUP_TABLE default\n");CHKERRQ(ierr);
      ierr = DMPlexVTKWritePartition_ASCII(dm, fp);CHKERRQ(ierr);
    }
  }
  /* Cleanup */
  ierr = PetscSectionDestroy(&globalCoordSection);CHKERRQ(ierr);
  ierr = PetscLayoutDestroy(&vLayout);CHKERRQ(ierr);
  ierr = PetscFClose(comm, fp);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMPlexVTKWriteAll"
/*@C
  DMPlexVTKWriteAll - Write a file containing all the fields that have been provided to the viewer

  Collective

  Input Arguments:
+ odm - The DMPlex specifying the mesh, passed as a PetscObject
- viewer - viewer of type VTK

  Level: developer

  Note:
  This function is a callback used by the VTK viewer to actually write the file.
  The reason for this odd model is that the VTK file format does not provide any way to write one field at a time.
  Instead, metadata for the entire file needs to be available up-front before you can start writing the file.

.seealso: PETSCVIEWERVTK
@*/
PetscErrorCode DMPlexVTKWriteAll(PetscObject odm, PetscViewer viewer)
{
  DM             dm = (DM) odm;
  PetscBool      isvtk;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm, DM_CLASSID, 1);
  PetscValidHeaderSpecific(viewer, PETSC_VIEWER_CLASSID, 2);
  ierr = PetscObjectTypeCompare((PetscObject) viewer, PETSCVIEWERVTK, &isvtk);CHKERRQ(ierr);
  if (!isvtk) SETERRQ1(PetscObjectComm((PetscObject)viewer), PETSC_ERR_ARG_INCOMP, "Cannot use viewer type %s", ((PetscObject)viewer)->type_name);
  switch (viewer->format) {
  case PETSC_VIEWER_ASCII_VTK:
    ierr = DMPlexVTKWriteAll_ASCII(dm, viewer);CHKERRQ(ierr);
    break;
  case PETSC_VIEWER_VTK_VTU:
    ierr = DMPlexVTKWriteAll_VTU(dm, viewer);CHKERRQ(ierr);
    break;
  default: SETERRQ1(PetscObjectComm((PetscObject)dm), PETSC_ERR_SUP, "No support for format '%s'", PetscViewerFormats[viewer->format]);
  }
  PetscFunctionReturn(0);
}
