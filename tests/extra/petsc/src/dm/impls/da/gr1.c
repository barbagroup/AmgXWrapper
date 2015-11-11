
/*
   Plots vectors obtained with DMDACreate1d()
*/

#include <petsc/private/dmdaimpl.h>      /*I  "petscdmda.h"   I*/

#undef __FUNCT__
#define __FUNCT__ "DMDASetUniformCoordinates"
/*@
    DMDASetUniformCoordinates - Sets a DMDA coordinates to be a uniform grid

  Collective on DMDA

  Input Parameters:
+  da - the distributed array object
.  xmin,xmax - extremes in the x direction
.  ymin,ymax - extremes in the y direction (value ignored for 1 dimensional problems)
-  zmin,zmax - extremes in the z direction (value ignored for 1 or 2 dimensional problems)

  Level: beginner

.seealso: DMSetCoordinates(), DMGetCoordinates(), DMDACreate1d(), DMDACreate2d(), DMDACreate3d()

@*/
PetscErrorCode  DMDASetUniformCoordinates(DM da,PetscReal xmin,PetscReal xmax,PetscReal ymin,PetscReal ymax,PetscReal zmin,PetscReal zmax)
{
  MPI_Comm         comm;
  PetscSection     section;
  DM               cda;
  DMBoundaryType   bx,by,bz;
  Vec              xcoor;
  PetscScalar      *coors;
  PetscReal        hx,hy,hz_;
  PetscInt         i,j,k,M,N,P,istart,isize,jstart,jsize,kstart,ksize,dim,cnt;
  PetscErrorCode   ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(da,DM_CLASSID,1);
  ierr = DMDAGetInfo(da,&dim,&M,&N,&P,0,0,0,0,0,&bx,&by,&bz,0);CHKERRQ(ierr);
  if (xmax < xmin) SETERRQ2(PetscObjectComm((PetscObject)da),PETSC_ERR_ARG_INCOMP,"xmax must be larger than xmin %g %g",(double)xmin,(double)xmax);
  if ((ymax < ymin) && (dim > 1)) SETERRQ2(PetscObjectComm((PetscObject)da),PETSC_ERR_ARG_INCOMP,"ymax must be larger than ymin %g %g",(double)ymin,(double)ymax);
  if ((zmax < zmin) && (dim > 2)) SETERRQ2(PetscObjectComm((PetscObject)da),PETSC_ERR_ARG_INCOMP,"zmax must be larger than zmin %g %g",(double)zmin,(double)zmax);
  ierr = PetscObjectGetComm((PetscObject)da,&comm);CHKERRQ(ierr);
  ierr = DMGetDefaultSection(da,&section);CHKERRQ(ierr);
  ierr = DMDAGetCorners(da,&istart,&jstart,&kstart,&isize,&jsize,&ksize);CHKERRQ(ierr);
  ierr = DMGetCoordinateDM(da, &cda);CHKERRQ(ierr);
  if (section) {
    /* This would be better as a vector, but this is compatible */
    PetscInt numComp[3]      = {1, 1, 1};
    PetscInt numVertexDof[3] = {1, 1, 1};

    ierr = DMDASetFieldName(cda, 0, "x");CHKERRQ(ierr);
    if (dim > 1) {ierr = DMDASetFieldName(cda, 1, "y");CHKERRQ(ierr);}
    if (dim > 2) {ierr = DMDASetFieldName(cda, 2, "z");CHKERRQ(ierr);}
    ierr = DMDACreateSection(cda, numComp, numVertexDof, NULL, NULL);CHKERRQ(ierr);
  }
  ierr = DMCreateGlobalVector(cda, &xcoor);CHKERRQ(ierr);
  if (section) {
    PetscSection csection;
    PetscInt     vStart, vEnd;

    ierr = DMGetDefaultGlobalSection(cda,&csection);CHKERRQ(ierr);
    ierr = VecGetArray(xcoor,&coors);CHKERRQ(ierr);
    ierr = DMDAGetHeightStratum(da, dim, &vStart, &vEnd);CHKERRQ(ierr);
    if (bx == DM_BOUNDARY_PERIODIC) hx  = (xmax-xmin)/(M+1);
    else                              hx  = (xmax-xmin)/(M ? M : 1);
    if (by == DM_BOUNDARY_PERIODIC) hy  = (ymax-ymin)/(N+1);
    else                              hy  = (ymax-ymin)/(N ? N : 1);
    if (bz == DM_BOUNDARY_PERIODIC) hz_ = (zmax-zmin)/(P+1);
    else                              hz_ = (zmax-zmin)/(P ? P : 1);
    switch (dim) {
    case 1:
      for (i = 0; i < isize+1; ++i) {
        PetscInt v = i+vStart, dof, off;

        ierr = PetscSectionGetDof(csection, v, &dof);CHKERRQ(ierr);
        ierr = PetscSectionGetOffset(csection, v, &off);CHKERRQ(ierr);
        if (off >= 0) {
          coors[off] = xmin + hx*(i+istart);
        }
      }
      break;
    case 2:
      for (j = 0; j < jsize+1; ++j) {
        for (i = 0; i < isize+1; ++i) {
          PetscInt v = j*(isize+1)+i+vStart, dof, off;

          ierr = PetscSectionGetDof(csection, v, &dof);CHKERRQ(ierr);
          ierr = PetscSectionGetOffset(csection, v, &off);CHKERRQ(ierr);
          if (off >= 0) {
            coors[off+0] = xmin + hx*(i+istart);
            coors[off+1] = ymin + hy*(j+jstart);
          }
        }
      }
      break;
    case 3:
      for (k = 0; k < ksize+1; ++k) {
        for (j = 0; j < jsize+1; ++j) {
          for (i = 0; i < isize+1; ++i) {
            PetscInt v = (k*(jsize+1)+j)*(isize+1)+i+vStart, dof, off;

            ierr = PetscSectionGetDof(csection, v, &dof);CHKERRQ(ierr);
            ierr = PetscSectionGetOffset(csection, v, &off);CHKERRQ(ierr);
            if (off >= 0) {
              coors[off+0] = xmin + hx*(i+istart);
              coors[off+1] = ymin + hy*(j+jstart);
              coors[off+2] = zmin + hz_*(k+kstart);
            }
          }
        }
      }
      break;
    default:
      SETERRQ1(PetscObjectComm((PetscObject)da),PETSC_ERR_SUP,"Cannot create uniform coordinates for this dimension %D\n",dim);
    }
    ierr = VecRestoreArray(xcoor,&coors);CHKERRQ(ierr);
    ierr = DMSetCoordinates(da,xcoor);CHKERRQ(ierr);
    ierr = PetscLogObjectParent((PetscObject)da,(PetscObject)xcoor);CHKERRQ(ierr);
    ierr = VecDestroy(&xcoor);CHKERRQ(ierr);
    PetscFunctionReturn(0);
  }
  if (dim == 1) {
    if (bx == DM_BOUNDARY_PERIODIC) hx = (xmax-xmin)/M;
    else hx = (xmax-xmin)/(M-1);
    ierr = VecGetArray(xcoor,&coors);CHKERRQ(ierr);
    for (i=0; i<isize; i++) {
      coors[i] = xmin + hx*(i+istart);
    }
    ierr = VecRestoreArray(xcoor,&coors);CHKERRQ(ierr);
  } else if (dim == 2) {
    if (bx == DM_BOUNDARY_PERIODIC) hx = (xmax-xmin)/(M);
    else hx = (xmax-xmin)/(M-1);
    if (by == DM_BOUNDARY_PERIODIC) hy = (ymax-ymin)/(N);
    else hy = (ymax-ymin)/(N-1);
    ierr = VecGetArray(xcoor,&coors);CHKERRQ(ierr);
    cnt  = 0;
    for (j=0; j<jsize; j++) {
      for (i=0; i<isize; i++) {
        coors[cnt++] = xmin + hx*(i+istart);
        coors[cnt++] = ymin + hy*(j+jstart);
      }
    }
    ierr = VecRestoreArray(xcoor,&coors);CHKERRQ(ierr);
  } else if (dim == 3) {
    if (bx == DM_BOUNDARY_PERIODIC) hx = (xmax-xmin)/(M);
    else hx = (xmax-xmin)/(M-1);
    if (by == DM_BOUNDARY_PERIODIC) hy = (ymax-ymin)/(N);
    else hy = (ymax-ymin)/(N-1);
    if (bz == DM_BOUNDARY_PERIODIC) hz_ = (zmax-zmin)/(P);
    else hz_ = (zmax-zmin)/(P-1);
    ierr = VecGetArray(xcoor,&coors);CHKERRQ(ierr);
    cnt  = 0;
    for (k=0; k<ksize; k++) {
      for (j=0; j<jsize; j++) {
        for (i=0; i<isize; i++) {
          coors[cnt++] = xmin + hx*(i+istart);
          coors[cnt++] = ymin + hy*(j+jstart);
          coors[cnt++] = zmin + hz_*(k+kstart);
        }
      }
    }
    ierr = VecRestoreArray(xcoor,&coors);CHKERRQ(ierr);
  } else SETERRQ1(PetscObjectComm((PetscObject)da),PETSC_ERR_SUP,"Cannot create uniform coordinates for this dimension %D\n",dim);
  ierr = DMSetCoordinates(da,xcoor);CHKERRQ(ierr);
  ierr = PetscLogObjectParent((PetscObject)da,(PetscObject)xcoor);CHKERRQ(ierr);
  ierr = VecDestroy(&xcoor);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "DMDASelectFields"
/*
    Allows a user to select a subset of the fields to be drawn by VecView() when the vector comes from a DMDA
*/
PetscErrorCode DMDASelectFields(DM da,PetscInt *outfields,PetscInt **fields)
{
  PetscErrorCode ierr;
  PetscInt       step,ndisplayfields,*displayfields,k,j;
  PetscBool      flg;

  PetscFunctionBegin;
  ierr = DMDAGetInfo(da,0,0,0,0,0,0,0,&step,0,0,0,0,0);CHKERRQ(ierr);
  ierr = PetscMalloc1(step,&displayfields);CHKERRQ(ierr);
  for (k=0; k<step; k++) displayfields[k] = k;
  ndisplayfields = step;
  ierr           = PetscOptionsGetIntArray(NULL,NULL,"-draw_fields",displayfields,&ndisplayfields,&flg);CHKERRQ(ierr);
  if (!ndisplayfields) ndisplayfields = step;
  if (!flg) {
    char       **fields;
    const char *fieldname;
    PetscInt   nfields = step;
    ierr = PetscMalloc1(step,&fields);CHKERRQ(ierr);
    ierr = PetscOptionsGetStringArray(NULL,NULL,"-draw_fields_by_name",fields,&nfields,&flg);CHKERRQ(ierr);
    if (flg) {
      ndisplayfields = 0;
      for (k=0; k<nfields;k++) {
        for (j=0; j<step; j++) {
          ierr = DMDAGetFieldName(da,j,&fieldname);CHKERRQ(ierr);
          ierr = PetscStrcmp(fieldname,fields[k],&flg);CHKERRQ(ierr);
          if (flg) {
            goto found;
          }
        }
        SETERRQ1(PetscObjectComm((PetscObject)da),PETSC_ERR_USER,"Unknown fieldname %s",fields[k]);
found:  displayfields[ndisplayfields++] = j;
      }
    }
    for (k=0; k<nfields; k++) {
      ierr = PetscFree(fields[k]);CHKERRQ(ierr);
    }
    ierr = PetscFree(fields);CHKERRQ(ierr);
  }
  *fields    = displayfields;
  *outfields = ndisplayfields;
  PetscFunctionReturn(0);
}

#include <petscdraw.h>
#if defined(PETSC_HAVE_SETJMP_H) && defined(PETSC_HAVE_X)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <setjmp.h>
static jmp_buf PetscXIOErrorJumpBuf;
static void PetscXIOHandler(Display *dpy)
{
  longjmp(PetscXIOErrorJumpBuf, 1);
}
#endif

#undef __FUNCT__
#define __FUNCT__ "VecView_MPI_Draw_DA1d"
PetscErrorCode VecView_MPI_Draw_DA1d(Vec xin,PetscViewer v)
{
  DM                da;
  PetscErrorCode    ierr;
  PetscMPIInt       rank,size,tag1,tag2;
  PetscInt          i,n,N,step,istart,isize,j,nbounds;
  MPI_Status        status;
  PetscReal         coors[4],ymin,ymax,min,max,xmin = 0.0,xmax = 0.0,tmp = 0.0,xgtmp = 0.0;
  const PetscScalar *array,*xg;
  PetscDraw         draw;
  PetscBool         isnull,showmarkers = PETSC_FALSE;
  MPI_Comm          comm;
  PetscDrawAxis     axis;
  Vec               xcoor;
  DMBoundaryType    bx;
  const PetscReal   *bounds;
  PetscInt          *displayfields;
  PetscInt          k,ndisplayfields;
  PetscBool         hold;

  PetscFunctionBegin;
  ierr = PetscViewerDrawGetDraw(v,0,&draw);CHKERRQ(ierr);
  ierr = PetscDrawIsNull(draw,&isnull);CHKERRQ(ierr); if (isnull) PetscFunctionReturn(0);
  ierr = PetscViewerDrawGetBounds(v,&nbounds,&bounds);CHKERRQ(ierr);

  ierr = VecGetDM(xin,&da);CHKERRQ(ierr);
  if (!da) SETERRQ(PetscObjectComm((PetscObject)xin),PETSC_ERR_ARG_WRONG,"Vector not generated from a DMDA");

  ierr = PetscOptionsGetBool(NULL,NULL,"-draw_vec_use_markers",&showmarkers,NULL);CHKERRQ(ierr);

  ierr = DMDAGetInfo(da,0,&N,0,0,0,0,0,&step,0,&bx,0,0,0);CHKERRQ(ierr);
  ierr = DMDAGetCorners(da,&istart,0,0,&isize,0,0);CHKERRQ(ierr);
  ierr = VecGetArrayRead(xin,&array);CHKERRQ(ierr);
  ierr = VecGetLocalSize(xin,&n);CHKERRQ(ierr);
  n    = n/step;

  /* get coordinates of nodes */
  ierr = DMGetCoordinates(da,&xcoor);CHKERRQ(ierr);
  if (!xcoor) {
    ierr = DMDASetUniformCoordinates(da,0.0,1.0,0.0,0.0,0.0,0.0);CHKERRQ(ierr);
    ierr = DMGetCoordinates(da,&xcoor);CHKERRQ(ierr);
  }
  ierr = VecGetArrayRead(xcoor,&xg);CHKERRQ(ierr);

  ierr = PetscObjectGetComm((PetscObject)xin,&comm);CHKERRQ(ierr);
  ierr = MPI_Comm_size(comm,&size);CHKERRQ(ierr);
  ierr = MPI_Comm_rank(comm,&rank);CHKERRQ(ierr);

  /*
      Determine the min and max x coordinate in plot
  */
  if (!rank) {
    xmin = PetscRealPart(xg[0]);
  }
  if (rank == size-1) {
    xmax = PetscRealPart(xg[n-1]);
  }
  ierr = MPI_Bcast(&xmin,1,MPIU_REAL,0,comm);CHKERRQ(ierr);
  ierr = MPI_Bcast(&xmax,1,MPIU_REAL,size-1,comm);CHKERRQ(ierr);

  ierr = DMDASelectFields(da,&ndisplayfields,&displayfields);CHKERRQ(ierr);
#if defined(PETSC_HAVE_SETJMP_H) && defined(PETSC_HAVE_X)
  if (!setjmp(PetscXIOErrorJumpBuf)) XSetIOErrorHandler((XIOErrorHandler)PetscXIOHandler);
  else {
    XSetIOErrorHandler(NULL);
    ierr = PetscDrawSetType(draw,PETSC_DRAW_NULL);CHKERRQ(ierr);
    PetscFunctionReturn(0);
  }
#endif
  for (k=0; k<ndisplayfields; k++) {
    j    = displayfields[k];
    ierr = PetscViewerDrawGetDraw(v,k,&draw);CHKERRQ(ierr);
    ierr = PetscDrawCheckResizedWindow(draw);CHKERRQ(ierr);

    /*
        Determine the min and max y coordinate in plot
    */
    min = 1.e20; max = -1.e20;
    for (i=0; i<n; i++) {
      if (PetscRealPart(array[j+i*step]) < min) min = PetscRealPart(array[j+i*step]);
      if (PetscRealPart(array[j+i*step]) > max) max = PetscRealPart(array[j+i*step]);
    }
    if (min + 1.e-10 > max) {
      min -= 1.e-5;
      max += 1.e-5;
    }
    if (j < nbounds) {
      min = PetscMin(min,bounds[2*j]);
      max = PetscMax(max,bounds[2*j+1]);
    }

    ierr = MPI_Reduce(&min,&ymin,1,MPIU_REAL,MPIU_MIN,0,comm);CHKERRQ(ierr);
    ierr = MPI_Reduce(&max,&ymax,1,MPIU_REAL,MPIU_MAX,0,comm);CHKERRQ(ierr);

    ierr = PetscViewerDrawGetHold(v,&hold);CHKERRQ(ierr);
    if (!hold) {
      ierr = PetscDrawSynchronizedClear(draw);CHKERRQ(ierr);
    }
    ierr = PetscViewerDrawGetDrawAxis(v,k,&axis);CHKERRQ(ierr);
    ierr = PetscLogObjectParent((PetscObject)draw,(PetscObject)axis);CHKERRQ(ierr);
    if (!rank) {
      const char *title;

      ierr = PetscDrawAxisSetLimits(axis,xmin,xmax,ymin,ymax);CHKERRQ(ierr);
      ierr = PetscDrawAxisDraw(axis);CHKERRQ(ierr);
      ierr = PetscDrawGetCoordinates(draw,coors,coors+1,coors+2,coors+3);CHKERRQ(ierr);
      ierr = DMDAGetFieldName(da,j,&title);CHKERRQ(ierr);
      if (title) {ierr = PetscDrawSetTitle(draw,title);CHKERRQ(ierr);}
    }
    ierr = MPI_Bcast(coors,4,MPIU_REAL,0,comm);CHKERRQ(ierr);
    if (rank) {
      ierr = PetscDrawSetCoordinates(draw,coors[0],coors[1],coors[2],coors[3]);CHKERRQ(ierr);
    }

    /* draw local part of vector */
    ierr = PetscObjectGetNewTag((PetscObject)xin,&tag1);CHKERRQ(ierr);
    ierr = PetscObjectGetNewTag((PetscObject)xin,&tag2);CHKERRQ(ierr);
    if (rank < size-1) { /*send value to right */
      ierr = MPI_Send((void*)&array[j+(n-1)*step],1,MPIU_REAL,rank+1,tag1,comm);CHKERRQ(ierr);
      ierr = MPI_Send((void*)&xg[n-1],1,MPIU_REAL,rank+1,tag1,comm);CHKERRQ(ierr);
    }
    if (!rank && bx == DM_BOUNDARY_PERIODIC && size > 1) { /* first processor sends first value to last */
      ierr = MPI_Send((void*)&array[j],1,MPIU_REAL,size-1,tag2,comm);CHKERRQ(ierr);
    }

    for (i=1; i<n; i++) {
      ierr = PetscDrawLine(draw,PetscRealPart(xg[i-1]),PetscRealPart(array[j+step*(i-1)]),PetscRealPart(xg[i]),PetscRealPart(array[j+step*i]),PETSC_DRAW_RED);CHKERRQ(ierr);
      if (showmarkers) {
        ierr = PetscDrawMarker(draw,PetscRealPart(xg[i-1]),PetscRealPart(array[j+step*(i-1)]),PETSC_DRAW_BLACK);CHKERRQ(ierr);
      }
    }
    if (rank) { /* receive value from left */
      ierr = MPI_Recv(&tmp,1,MPIU_REAL,rank-1,tag1,comm,&status);CHKERRQ(ierr);
      ierr = MPI_Recv(&xgtmp,1,MPIU_REAL,rank-1,tag1,comm,&status);CHKERRQ(ierr);
      ierr = PetscDrawLine(draw,xgtmp,tmp,PetscRealPart(xg[0]),PetscRealPart(array[j]),PETSC_DRAW_RED);CHKERRQ(ierr);
      if (showmarkers) {
        ierr = PetscDrawPoint(draw,xgtmp,tmp,PETSC_DRAW_BLACK);CHKERRQ(ierr);
      }
    }
    if (rank == size-1 && bx == DM_BOUNDARY_PERIODIC && size > 1) {
      ierr = MPI_Recv(&tmp,1,MPIU_REAL,0,tag2,comm,&status);CHKERRQ(ierr);
      /* If the mesh is not uniform we do not know the mesh spacing between the last point on the right and the first ghost point */
      ierr = PetscDrawLine(draw,PetscRealPart(xg[n-1]),PetscRealPart(array[j+step*(n-1)]),PetscRealPart(xg[n-1]+(xg[n-1]-xg[n-2])),tmp,PETSC_DRAW_RED);CHKERRQ(ierr);
      if (showmarkers) {
        ierr = PetscDrawMarker(draw,PetscRealPart(xg[n-2]),PetscRealPart(array[j+step*(n-1)]),PETSC_DRAW_BLACK);CHKERRQ(ierr);
      }
    }
    ierr = PetscDrawSynchronizedFlush(draw);CHKERRQ(ierr);
    ierr = PetscDrawPause(draw);CHKERRQ(ierr);
  }
#if defined(PETSC_HAVE_SETJMP_H) && defined(PETSC_HAVE_X)
  XSetIOErrorHandler(NULL);
#endif
  ierr = PetscFree(displayfields);CHKERRQ(ierr);
  ierr = VecRestoreArrayRead(xcoor,&xg);CHKERRQ(ierr);
  ierr = VecRestoreArrayRead(xin,&array);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

