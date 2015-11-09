#include <petsc/private/dmdaimpl.h>
#include <../src/sys/classes/viewer/impls/vtk/vtkvimpl.h>

#undef __FUNCT__
#define __FUNCT__ "DMDAVTKWriteAll_VTS"
static PetscErrorCode DMDAVTKWriteAll_VTS(DM da,PetscViewer viewer)
{
#if defined(PETSC_USE_REAL_SINGLE)
  const char precision[] = "Float32";
#elif defined(PETSC_USE_REAL_DOUBLE)
  const char precision[] = "Float64";
#else
  const char precision[] = "UnknownPrecision";
#endif
  MPI_Comm                 comm;
  Vec                      Coords;
  PetscViewer_VTK          *vtk = (PetscViewer_VTK*)viewer->data;
  PetscViewerVTKObjectLink link;
  FILE                     *fp;
  PetscMPIInt              rank,size,tag;
  DMDALocalInfo            info;
  PetscInt                 dim,mx,my,mz,cdim,bs,boffset,maxnnodes,i,j,k,f,r;
  PetscInt                 rloc[6],(*grloc)[6] = NULL;
  PetscScalar              *array,*array2;
  PetscReal                gmin[3],gmax[3];
  PetscErrorCode           ierr;

  PetscFunctionBegin;
  ierr = PetscObjectGetComm((PetscObject)da,&comm);CHKERRQ(ierr);
#if defined(PETSC_USE_COMPLEX)
  SETERRQ(comm,PETSC_ERR_SUP,"Complex values not supported");
#endif
  ierr = MPI_Comm_size(comm,&size);CHKERRQ(ierr);
  ierr = MPI_Comm_rank(comm,&rank);CHKERRQ(ierr);
  ierr = DMDAGetInfo(da,&dim, &mx,&my,&mz, 0,0,0, &bs,0,0,0,0,0);CHKERRQ(ierr);
  ierr = DMDAGetLocalInfo(da,&info);CHKERRQ(ierr);
  ierr = DMDAGetBoundingBox(da,gmin,gmax);CHKERRQ(ierr);
  ierr = DMGetCoordinates(da,&Coords);CHKERRQ(ierr);
  if (Coords) {
    PetscInt csize;
    ierr = VecGetSize(Coords,&csize);CHKERRQ(ierr);
    if (csize % (mx*my*mz)) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_PLIB,"Coordinate vector size mismatch");
    cdim = csize/(mx*my*mz);
    if (cdim < dim || cdim > 3) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_PLIB,"Coordinate vector size mismatch");
  } else {
    cdim = dim;
  }

  ierr = PetscFOpen(comm,vtk->filename,"wb",&fp);CHKERRQ(ierr);
  ierr = PetscFPrintf(comm,fp,"<?xml version=\"1.0\"?>\n");CHKERRQ(ierr);
#if defined(PETSC_WORDS_BIGENDIAN)
  ierr = PetscFPrintf(comm,fp,"<VTKFile type=\"StructuredGrid\" version=\"0.1\" byte_order=\"BigEndian\">\n");CHKERRQ(ierr);
#else
  ierr = PetscFPrintf(comm,fp,"<VTKFile type=\"StructuredGrid\" version=\"0.1\" byte_order=\"LittleEndian\">\n");CHKERRQ(ierr);
#endif
  ierr = PetscFPrintf(comm,fp,"  <StructuredGrid WholeExtent=\"%D %D %D %D %D %D\">\n",0,mx-1,0,my-1,0,mz-1);CHKERRQ(ierr);

  if (!rank) {ierr = PetscMalloc1(size*6,&grloc);CHKERRQ(ierr);}
  rloc[0] = info.xs;
  rloc[1] = info.xm;
  rloc[2] = info.ys;
  rloc[3] = info.ym;
  rloc[4] = info.zs;
  rloc[5] = info.zm;
  ierr    = MPI_Gather(rloc,6,MPIU_INT,&grloc[0][0],6,MPIU_INT,0,comm);CHKERRQ(ierr);

  /* Write XML header */
  maxnnodes = 0;                /* Used for the temporary array size on rank 0 */
  boffset   = 0;                /* Offset into binary file */
  for (r=0; r<size; r++) {
    PetscInt xs=-1,xm=-1,ys=-1,ym=-1,zs=-1,zm=-1,nnodes = 0;
    if (!rank) {
      xs     = grloc[r][0];
      xm     = grloc[r][1];
      ys     = grloc[r][2];
      ym     = grloc[r][3];
      zs     = grloc[r][4];
      zm     = grloc[r][5];
      nnodes = xm*ym*zm;
    }
    maxnnodes = PetscMax(maxnnodes,nnodes);
#if 0
    switch (dim) {
    case 1:
      ierr = PetscFPrintf(comm,fp,"    <Piece Extent=\"%D %D %D %D %D %D\">\n",xs,xs+xm-1,0,0,0,0);CHKERRQ(ierr);
      break;
    case 2:
      ierr = PetscFPrintf(comm,fp,"    <Piece Extent=\"%D %D %D %D %D %D\">\n",xs,xs+xm,ys+ym-1,xs,xs+xm-1,0,0);CHKERRQ(ierr);
      break;
    case 3:
      ierr = PetscFPrintf(comm,fp,"    <Piece Extent=\"%D %D %D %D %D %D\">\n",xs,xs+xm-1,ys,ys+ym-1,zs,zs+zm-1);CHKERRQ(ierr);
      break;
    default: SETERRQ1(PetscObjectComm((PetscObject)da),PETSC_ERR_SUP,"No support for dimension %D",dim);
    }
#endif
    ierr     = PetscFPrintf(comm,fp,"    <Piece Extent=\"%D %D %D %D %D %D\">\n",xs,xs+xm-1,ys,ys+ym-1,zs,zs+zm-1);CHKERRQ(ierr);
    ierr     = PetscFPrintf(comm,fp,"      <Points>\n");CHKERRQ(ierr);
    ierr     = PetscFPrintf(comm,fp,"        <DataArray type=\"%s\" Name=\"Position\" NumberOfComponents=\"3\" format=\"appended\" offset=\"%D\" />\n",precision,boffset);CHKERRQ(ierr);
    boffset += 3*nnodes*sizeof(PetscScalar) + sizeof(int);
    ierr     = PetscFPrintf(comm,fp,"      </Points>\n");CHKERRQ(ierr);

    ierr = PetscFPrintf(comm,fp,"      <PointData Scalars=\"ScalarPointData\">\n");CHKERRQ(ierr);
    for (link=vtk->link; link; link=link->next) {
      Vec        X        = (Vec)link->vec;
      const char *vecname = "";
      if (((PetscObject)X)->name || link != vtk->link) { /* If the object is already named, use it. If it is past the first link, name it to disambiguate. */
        ierr = PetscObjectGetName((PetscObject)X,&vecname);CHKERRQ(ierr);
      }
      for (i=0; i<bs; i++) {
        char       buf[256];
        const char *fieldname;
        ierr = DMDAGetFieldName(da,i,&fieldname);CHKERRQ(ierr);
        if (!fieldname) {
          ierr      = PetscSNPrintf(buf,sizeof(buf),"Unnamed%D",i);CHKERRQ(ierr);
          fieldname = buf;
        }
        ierr     = PetscFPrintf(comm,fp,"        <DataArray type=\"%s\" Name=\"%s%s\" NumberOfComponents=\"1\" format=\"appended\" offset=\"%D\" />\n",precision,vecname,fieldname,boffset);CHKERRQ(ierr);
        boffset += nnodes*sizeof(PetscScalar) + sizeof(int);
      }
    }
    ierr = PetscFPrintf(comm,fp,"      </PointData>\n");CHKERRQ(ierr);
    ierr = PetscFPrintf(comm,fp,"    </Piece>\n");CHKERRQ(ierr);
  }
  ierr = PetscFPrintf(comm,fp,"  </StructuredGrid>\n");CHKERRQ(ierr);
  ierr = PetscFPrintf(comm,fp,"  <AppendedData encoding=\"raw\">\n");CHKERRQ(ierr);
  ierr = PetscFPrintf(comm,fp,"_");CHKERRQ(ierr);

  /* Now write the arrays. */
  tag  = ((PetscObject)viewer)->tag;
  ierr = PetscMalloc2(maxnnodes*PetscMax(3,bs),&array,maxnnodes*3,&array2);CHKERRQ(ierr);
  for (r=0; r<size; r++) {
    MPI_Status status;
    PetscInt   xs=-1,xm=-1,ys=-1,ym=-1,zs=-1,zm=-1,nnodes = 0;
    if (!rank) {
      xs     = grloc[r][0];
      xm     = grloc[r][1];
      ys     = grloc[r][2];
      ym     = grloc[r][3];
      zs     = grloc[r][4];
      zm     = grloc[r][5];
      nnodes = xm*ym*zm;
    } else if (r == rank) {
      nnodes = info.xm*info.ym*info.zm;
    }

    /* Write the coordinates */
    if (Coords) {
      const PetscScalar *coords;
      ierr = VecGetArrayRead(Coords,&coords);CHKERRQ(ierr);
      if (!rank) {
        if (r) {
          PetscMPIInt nn;
          ierr = MPI_Recv(array,nnodes*cdim,MPIU_SCALAR,r,tag,comm,&status);CHKERRQ(ierr);
          ierr = MPI_Get_count(&status,MPIU_SCALAR,&nn);CHKERRQ(ierr);
          if (nn != nnodes*cdim) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_PLIB,"Array size mismatch");
        } else {
          ierr = PetscMemcpy(array,coords,nnodes*cdim*sizeof(PetscScalar));CHKERRQ(ierr);
        }
        /* Transpose coordinates to VTK (C-style) ordering */
        for (k=0; k<zm; k++) {
          for (j=0; j<ym; j++) {
            for (i=0; i<xm; i++) {
              PetscInt Iloc = i+xm*(j+ym*k);
              array2[Iloc*3+0] = array[Iloc*cdim + 0];
              array2[Iloc*3+1] = cdim > 1 ? array[Iloc*cdim + 1] : 0.0;
              array2[Iloc*3+2] = cdim > 2 ? array[Iloc*cdim + 2] : 0.0;
            }
          }
        }
      } else if (r == rank) {
        ierr = MPI_Send((void*)coords,nnodes*cdim,MPIU_SCALAR,0,tag,comm);CHKERRQ(ierr);
      }
      ierr = VecRestoreArrayRead(Coords,&coords);CHKERRQ(ierr);
    } else {       /* Fabricate some coordinates using grid index */
      for (k=0; k<zm; k++) {
        for (j=0; j<ym; j++) {
          for (i=0; i<xm; i++) {
            PetscInt Iloc = i+xm*(j+ym*k);
            array2[Iloc*3+0] = xs+i;
            array2[Iloc*3+1] = ys+j;
            array2[Iloc*3+2] = zs+k;
          }
        }
      }
    }
    ierr = PetscViewerVTKFWrite(viewer,fp,array2,nnodes*3,PETSC_SCALAR);CHKERRQ(ierr);

    /* Write each of the objects queued up for this file */
    for (link=vtk->link; link; link=link->next) {
      Vec               X = (Vec)link->vec;
      const PetscScalar *x;

      ierr = VecGetArrayRead(X,&x);CHKERRQ(ierr);
      if (!rank) {
        if (r) {
          PetscMPIInt nn;
          ierr = MPI_Recv(array,nnodes*bs,MPIU_SCALAR,r,tag,comm,&status);CHKERRQ(ierr);
          ierr = MPI_Get_count(&status,MPIU_SCALAR,&nn);CHKERRQ(ierr);
          if (nn != nnodes*bs) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_PLIB,"Array size mismatch receiving from rank %D",r);
        } else {
          ierr = PetscMemcpy(array,x,nnodes*bs*sizeof(PetscScalar));CHKERRQ(ierr);
        }
        for (f=0; f<bs; f++) {
          /* Extract and transpose the f'th field */
          for (k=0; k<zm; k++) {
            for (j=0; j<ym; j++) {
              for (i=0; i<xm; i++) {
                PetscInt Iloc = i+xm*(j+ym*k);
                array2[Iloc] = array[Iloc*bs + f];
              }
            }
          }
          ierr = PetscViewerVTKFWrite(viewer,fp,array2,nnodes,PETSC_SCALAR);CHKERRQ(ierr);
        }
      } else if (r == rank) {
        ierr = MPI_Send((void*)x,nnodes*bs,MPIU_SCALAR,0,tag,comm);CHKERRQ(ierr);
      }
      ierr = VecRestoreArrayRead(X,&x);CHKERRQ(ierr);
    }
  }
  ierr = PetscFree2(array,array2);CHKERRQ(ierr);
  ierr = PetscFree(grloc);CHKERRQ(ierr);

  ierr = PetscFPrintf(comm,fp,"\n </AppendedData>\n");CHKERRQ(ierr);
  ierr = PetscFPrintf(comm,fp,"</VTKFile>\n");CHKERRQ(ierr);
  ierr = PetscFClose(comm,fp);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}


#undef __FUNCT__
#define __FUNCT__ "DMDAVTKWriteAll_VTR"
static PetscErrorCode DMDAVTKWriteAll_VTR(DM da,PetscViewer viewer)
{
#if defined(PETSC_USE_REAL_SINGLE)
  const char precision[] = "Float32";
#elif defined(PETSC_USE_REAL_DOUBLE)
  const char precision[] = "Float64";
#else
  const char precision[] = "UnknownPrecision";
#endif
  MPI_Comm                 comm;
  PetscViewer_VTK          *vtk = (PetscViewer_VTK*)viewer->data;
  PetscViewerVTKObjectLink link;
  FILE                     *fp;
  PetscMPIInt              rank,size,tag;
  DMDALocalInfo            info;
  PetscInt                 dim,mx,my,mz,bs,boffset,maxnnodes,i,j,k,f,r;
  PetscInt                 rloc[6],(*grloc)[6] = NULL;
  PetscScalar              *array,*array2;
  PetscReal                gmin[3],gmax[3];
  PetscErrorCode           ierr;

  PetscFunctionBegin;
  ierr = PetscObjectGetComm((PetscObject)da,&comm);CHKERRQ(ierr);
#if defined(PETSC_USE_COMPLEX)
  SETERRQ(comm,PETSC_ERR_SUP,"Complex values not supported");
#endif
  ierr = MPI_Comm_size(comm,&size);CHKERRQ(ierr);
  ierr = MPI_Comm_rank(comm,&rank);CHKERRQ(ierr);
  ierr = DMDAGetInfo(da,&dim, &mx,&my,&mz, 0,0,0, &bs,0,0,0,0,0);CHKERRQ(ierr);
  ierr = DMDAGetLocalInfo(da,&info);CHKERRQ(ierr);
  ierr = DMDAGetBoundingBox(da,gmin,gmax);CHKERRQ(ierr);
  ierr = PetscFOpen(comm,vtk->filename,"wb",&fp);CHKERRQ(ierr);
  ierr = PetscFPrintf(comm,fp,"<?xml version=\"1.0\"?>\n");CHKERRQ(ierr);
#if defined(PETSC_WORDS_BIGENDIAN)
  ierr = PetscFPrintf(comm,fp,"<VTKFile type=\"RectilinearGrid\" version=\"0.1\" byte_order=\"BigEndian\">\n");CHKERRQ(ierr);
#else
  ierr = PetscFPrintf(comm,fp,"<VTKFile type=\"RectilinearGrid\" version=\"0.1\" byte_order=\"LittleEndian\">\n");CHKERRQ(ierr);
#endif
  ierr = PetscFPrintf(comm,fp,"  <RectilinearGrid WholeExtent=\"%D %D %D %D %D %D\">\n",0,mx-1,0,my-1,0,mz-1);CHKERRQ(ierr);

  if (!rank) {ierr = PetscMalloc1(size*6,&grloc);CHKERRQ(ierr);}
  rloc[0] = info.xs;
  rloc[1] = info.xm;
  rloc[2] = info.ys;
  rloc[3] = info.ym;
  rloc[4] = info.zs;
  rloc[5] = info.zm;
  ierr    = MPI_Gather(rloc,6,MPIU_INT,&grloc[0][0],6,MPIU_INT,0,comm);CHKERRQ(ierr);

  /* Write XML header */
  maxnnodes = 0;                /* Used for the temporary array size on rank 0 */
  boffset   = 0;                /* Offset into binary file */
  for (r=0; r<size; r++) {
    PetscInt xs=-1,xm=-1,ys=-1,ym=-1,zs=-1,zm=-1,nnodes = 0;
    if (!rank) {
      xs     = grloc[r][0];
      xm     = grloc[r][1];
      ys     = grloc[r][2];
      ym     = grloc[r][3];
      zs     = grloc[r][4];
      zm     = grloc[r][5];
      nnodes = xm*ym*zm;
    }
    maxnnodes = PetscMax(maxnnodes,nnodes);
    ierr     = PetscFPrintf(comm,fp,"    <Piece Extent=\"%D %D %D %D %D %D\">\n",xs,xs+xm-1,ys,ys+ym-1,zs,zs+zm-1);CHKERRQ(ierr);
    ierr     = PetscFPrintf(comm,fp,"      <Coordinates>\n");CHKERRQ(ierr);
    ierr     = PetscFPrintf(comm,fp,"        <DataArray type=\"%s\" Name=\"Xcoord\"  format=\"appended\"  offset=\"%D\" />\n",precision,boffset);CHKERRQ(ierr);
    boffset += xm*sizeof(PetscScalar) + sizeof(int);
    ierr     = PetscFPrintf(comm,fp,"        <DataArray type=\"%s\" Name=\"Ycoord\"  format=\"appended\"  offset=\"%D\" />\n",precision,boffset);CHKERRQ(ierr);
    boffset += ym*sizeof(PetscScalar) + sizeof(int);
    ierr     = PetscFPrintf(comm,fp,"        <DataArray type=\"%s\" Name=\"Zcoord\"  format=\"appended\"  offset=\"%D\" />\n",precision,boffset);CHKERRQ(ierr);
    boffset += zm*sizeof(PetscScalar) + sizeof(int);
    ierr     = PetscFPrintf(comm,fp,"      </Coordinates>\n");CHKERRQ(ierr);
    ierr = PetscFPrintf(comm,fp,"      <PointData Scalars=\"ScalarPointData\">\n");CHKERRQ(ierr);
    for (link=vtk->link; link; link=link->next) {
      Vec        X        = (Vec)link->vec;
      const char *vecname = "";
      if (((PetscObject)X)->name || link != vtk->link) { /* If the object is already named, use it. If it is past the first link, name it to disambiguate. */
        ierr = PetscObjectGetName((PetscObject)X,&vecname);CHKERRQ(ierr);
      }
      for (i=0; i<bs; i++) {
        char       buf[256];
        const char *fieldname;
        ierr = DMDAGetFieldName(da,i,&fieldname);CHKERRQ(ierr);
        if (!fieldname) {
          ierr      = PetscSNPrintf(buf,sizeof(buf),"Unnamed%D",i);CHKERRQ(ierr);
          fieldname = buf;
        }
        ierr     = PetscFPrintf(comm,fp,"        <DataArray type=\"%s\" Name=\"%s%s\" NumberOfComponents=\"1\" format=\"appended\" offset=\"%D\" />\n",precision,vecname,fieldname,boffset);CHKERRQ(ierr);
        boffset += nnodes*sizeof(PetscScalar) + sizeof(int);
      }
    }
    ierr = PetscFPrintf(comm,fp,"      </PointData>\n");CHKERRQ(ierr);
    ierr = PetscFPrintf(comm,fp,"    </Piece>\n");CHKERRQ(ierr);
  }
  ierr = PetscFPrintf(comm,fp,"  </RectilinearGrid>\n");CHKERRQ(ierr);
  ierr = PetscFPrintf(comm,fp,"  <AppendedData encoding=\"raw\">\n");CHKERRQ(ierr);
  ierr = PetscFPrintf(comm,fp,"_");CHKERRQ(ierr);

  /* Now write the arrays. */
  tag  = ((PetscObject)viewer)->tag;
  ierr = PetscMalloc2(PetscMax(maxnnodes*bs,info.xm+info.ym+info.zm),&array,PetscMax(maxnnodes*3,info.xm+info.ym+info.zm),&array2);CHKERRQ(ierr);
  for (r=0; r<size; r++) {
    MPI_Status status;
    PetscInt   xs=-1,xm=-1,ys=-1,ym=-1,zs=-1,zm=-1,nnodes = 0;
    if (!rank) {
      xs     = grloc[r][0];
      xm     = grloc[r][1];
      ys     = grloc[r][2];
      ym     = grloc[r][3];
      zs     = grloc[r][4];
      zm     = grloc[r][5];
      nnodes = xm*ym*zm;
    } else if (r == rank) {
      nnodes = info.xm*info.ym*info.zm;
    }
    {                           /* Write the coordinates */
      Vec Coords;
      ierr = DMGetCoordinates(da,&Coords);CHKERRQ(ierr);
      if (Coords) {
        const PetscScalar *coords;
        ierr = VecGetArrayRead(Coords,&coords);CHKERRQ(ierr);
        if (!rank) {
          if (r) {
            PetscMPIInt nn;
            ierr = MPI_Recv(array,xm+ym+zm,MPIU_SCALAR,r,tag,comm,&status);CHKERRQ(ierr);
            ierr = MPI_Get_count(&status,MPIU_SCALAR,&nn);CHKERRQ(ierr);
            if (nn != xm+ym+zm) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_PLIB,"Array size mismatch");
          } else {
            /* x coordinates */
            for (j=0, k=0, i=0; i<xm; i++) {
              PetscInt Iloc = i+xm*(j+ym*k);
              array[i] = coords[Iloc*dim + 0];
            }
            /* y coordinates */
            for (i=0, k=0, j=0; j<ym; j++) {
              PetscInt Iloc = i+xm*(j+ym*k);
              array[j+xm] = dim > 1 ? coords[Iloc*dim + 1] : 0;
            }
            /* z coordinates */
            for (i=0, j=0, k=0; k<zm; k++) {
              PetscInt Iloc = i+xm*(j+ym*k);
              array[k+xm+ym] = dim > 2 ? coords[Iloc*dim + 2] : 0;
            }
          }
        } else if (r == rank) {
          xm = info.xm;
          ym = info.ym;
          zm = info.zm;
          for (j=0, k=0, i=0; i<xm; i++) {
            PetscInt Iloc = i+xm*(j+ym*k);
            array2[i] = coords[Iloc*dim + 0];
          }
          for (i=0, k=0, j=0; j<ym; j++) {
            PetscInt Iloc = i+xm*(j+ym*k);
            array2[j+xm] = dim > 1 ? coords[Iloc*dim + 1] : 0;
          }
          for (i=0, j=0, k=0; k<zm; k++) {
            PetscInt Iloc = i+xm*(j+ym*k);
            array2[k+xm+ym] = dim > 2 ? coords[Iloc*dim + 2] : 0;
          }
          ierr = MPI_Send((void*)array2,xm+ym+zm,MPIU_SCALAR,0,tag,comm);CHKERRQ(ierr);
        }
        ierr = VecRestoreArrayRead(Coords,&coords);CHKERRQ(ierr);
      } else {       /* Fabricate some coordinates using grid index */
        for (j=0, k=0, i=0; i<xm; i++) {
          array[i] = xs+i;
        }
        for (i=0, k=0, j=0; j<ym; j++) {
          array[j+xm] = ys+j;
        }
        for (i=0, j=0, k=0; k<zm; k++) {
          array[k+xm+ym] = zs+k;
        }
      }
      if (!rank) {
        ierr = PetscViewerVTKFWrite(viewer,fp,&(array[0]),xm,PETSC_SCALAR);CHKERRQ(ierr);
        ierr = PetscViewerVTKFWrite(viewer,fp,&(array[xm]),ym,PETSC_SCALAR);CHKERRQ(ierr);
        ierr = PetscViewerVTKFWrite(viewer,fp,&(array[xm+ym]),zm,PETSC_SCALAR);CHKERRQ(ierr);
      }
    }

    /* Write each of the objects queued up for this file */
    for (link=vtk->link; link; link=link->next) {
      Vec               X = (Vec)link->vec;
      const PetscScalar *x;

      ierr = VecGetArrayRead(X,&x);CHKERRQ(ierr);
      if (!rank) {
        if (r) {
          PetscMPIInt nn;
          ierr = MPI_Recv(array,nnodes*bs,MPIU_SCALAR,r,tag,comm,&status);CHKERRQ(ierr);
          ierr = MPI_Get_count(&status,MPIU_SCALAR,&nn);CHKERRQ(ierr);
          if (nn != nnodes*bs) SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_PLIB,"Array size mismatch receiving from rank %D",r);
        } else {
          ierr = PetscMemcpy(array,x,nnodes*bs*sizeof(PetscScalar));CHKERRQ(ierr);
        }
        for (f=0; f<bs; f++) {
          /* Extract and transpose the f'th field */
          for (k=0; k<zm; k++) {
            for (j=0; j<ym; j++) {
              for (i=0; i<xm; i++) {
                PetscInt Iloc = i+xm*(j+ym*k);
                array2[Iloc] = array[Iloc*bs + f];
              }
            }
          }
          ierr = PetscViewerVTKFWrite(viewer,fp,array2,nnodes,PETSC_SCALAR);CHKERRQ(ierr);
        }
      } else if (r == rank) {
        ierr = MPI_Send((void*)x,nnodes*bs,MPIU_SCALAR,0,tag,comm);CHKERRQ(ierr);
      }
      ierr = VecRestoreArrayRead(X,&x);CHKERRQ(ierr);
    }
  }
  ierr = PetscFree2(array,array2);CHKERRQ(ierr);
  ierr = PetscFree(grloc);CHKERRQ(ierr);

  ierr = PetscFPrintf(comm,fp,"\n </AppendedData>\n");CHKERRQ(ierr);
  ierr = PetscFPrintf(comm,fp,"</VTKFile>\n");CHKERRQ(ierr);
  ierr = PetscFClose(comm,fp);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMDAVTKWriteAll"
/*@C
   DMDAVTKWriteAll - Write a file containing all the fields that have been provided to the viewer

   Collective

   Input Arguments:
   odm - DM specifying the grid layout, passed as a PetscObject
   viewer - viewer of type VTK

   Level: developer

   Note:
   This function is a callback used by the VTK viewer to actually write the file.
   The reason for this odd model is that the VTK file format does not provide any way to write one field at a time.
   Instead, metadata for the entire file needs to be available up-front before you can start writing the file.

.seealso: PETSCVIEWERVTK
@*/
PetscErrorCode DMDAVTKWriteAll(PetscObject odm,PetscViewer viewer)
{
  DM             dm = (DM)odm;
  PetscBool      isvtk;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(dm,DM_CLASSID,1);
  PetscValidHeaderSpecific(viewer,PETSC_VIEWER_CLASSID,2);
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERVTK,&isvtk);CHKERRQ(ierr);
  if (!isvtk) SETERRQ1(PetscObjectComm((PetscObject)viewer),PETSC_ERR_ARG_INCOMP,"Cannot use viewer type %s",((PetscObject)viewer)->type_name);
  switch (viewer->format) {
  case PETSC_VIEWER_VTK_VTS:
    ierr = DMDAVTKWriteAll_VTS(dm,viewer);CHKERRQ(ierr);
    break;
  case PETSC_VIEWER_VTK_VTR:
    ierr = DMDAVTKWriteAll_VTR(dm,viewer);CHKERRQ(ierr);
    break;
  default: SETERRQ1(PetscObjectComm((PetscObject)dm),PETSC_ERR_SUP,"No support for format '%s'",PetscViewerFormats[viewer->format]);
  }
  PetscFunctionReturn(0);
}
