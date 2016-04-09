
/*
   Plots vectors obtained with DMDACreate2d()
*/

#include <petsc/private/dmdaimpl.h>      /*I  "petscdmda.h"   I*/
#include <petsc/private/vecimpl.h>
#include <petscdraw.h>
#include <petscviewerhdf5.h>

/*
        The data that is passed into the graphics callback
*/
typedef struct {
  PetscInt          m,n,step,k;
  PetscReal         min,max,scale;
  const PetscScalar *xy,*v;
  PetscBool         showgrid;
  const char        *name0,*name1;
} ZoomCtx;

/*
       This does the drawing for one particular field
    in one particular set of coordinates. It is a callback
    called from PetscDrawZoom()
*/
#undef __FUNCT__
#define __FUNCT__ "VecView_MPI_Draw_DA2d_Zoom"
PetscErrorCode VecView_MPI_Draw_DA2d_Zoom(PetscDraw draw,void *ctx)
{
  ZoomCtx           *zctx = (ZoomCtx*)ctx;
  PetscErrorCode    ierr;
  PetscInt          m,n,i,j,k,step,id,c1,c2,c3,c4;
  PetscReal         s,min,max,x1,x2,x3,x4,y_1,y2,y3,y4,xmin = PETSC_MAX_REAL,xmax = PETSC_MIN_REAL,ymin = PETSC_MAX_REAL,ymax = PETSC_MIN_REAL;
  PetscReal         xminf,xmaxf,yminf,ymaxf,w;
  const PetscScalar *v,*xy;
  char              value[16];
  size_t            len;

  PetscFunctionBegin;
  m    = zctx->m;
  n    = zctx->n;
  step = zctx->step;
  k    = zctx->k;
  v    = zctx->v;
  xy   = zctx->xy;
  s    = zctx->scale;
  min  = zctx->min;
  max  = zctx->max;

  /* PetscDraw the contour plot patch */
  for (j=0; j<n-1; j++) {
    for (i=0; i<m-1; i++) {
      id   = i+j*m;
      x1   = PetscRealPart(xy[2*id]);
      y_1  = PetscRealPart(xy[2*id+1]);
      c1   = (int)(PETSC_DRAW_BASIC_COLORS+s*(PetscClipInterval(PetscRealPart(v[k+step*id]),min,max)-min));
      xmin = PetscMin(xmin,x1);
      ymin = PetscMin(ymin,y_1);
      xmax = PetscMax(xmax,x1);
      ymax = PetscMax(ymax,y_1);

      id   = i+j*m+1;
      x2   = PetscRealPart(xy[2*id]);
      y2   = PetscRealPart(xy[2*id+1]);
      c2   = (int)(PETSC_DRAW_BASIC_COLORS+s*(PetscClipInterval(PetscRealPart(v[k+step*id]),min,max)-min));
      xmin = PetscMin(xmin,x2);
      ymin = PetscMin(ymin,y2);
      xmax = PetscMax(xmax,x2);
      ymax = PetscMax(ymax,y2);

      id   = i+j*m+1+m;
      x3   = PetscRealPart(xy[2*id]);
      y3   = PetscRealPart(xy[2*id+1]);
      c3   = (int)(PETSC_DRAW_BASIC_COLORS+s*(PetscClipInterval(PetscRealPart(v[k+step*id]),min,max)-min));
      xmin = PetscMin(xmin,x3);
      ymin = PetscMin(ymin,y3);
      xmax = PetscMax(xmax,x3);
      ymax = PetscMax(ymax,y3);

      id = i+j*m+m;
      x4 = PetscRealPart(xy[2*id]);
      y4 = PetscRealPart(xy[2*id+1]);
      c4 = (int)(PETSC_DRAW_BASIC_COLORS+s*(PetscClipInterval(PetscRealPart(v[k+step*id]),min,max)-min));
      xmin = PetscMin(xmin,x4);
      ymin = PetscMin(ymin,y4);
      xmax = PetscMax(xmax,x4);
      ymax = PetscMax(ymax,y4);

      ierr = PetscDrawTriangle(draw,x1,y_1,x2,y2,x3,y3,c1,c2,c3);CHKERRQ(ierr);
      ierr = PetscDrawTriangle(draw,x1,y_1,x3,y3,x4,y4,c1,c3,c4);CHKERRQ(ierr);
      if (zctx->showgrid) {
        ierr = PetscDrawLine(draw,x1,y_1,x2,y2,PETSC_DRAW_BLACK);CHKERRQ(ierr);
        ierr = PetscDrawLine(draw,x2,y2,x3,y3,PETSC_DRAW_BLACK);CHKERRQ(ierr);
        ierr = PetscDrawLine(draw,x3,y3,x4,y4,PETSC_DRAW_BLACK);CHKERRQ(ierr);
        ierr = PetscDrawLine(draw,x4,y4,x1,y_1,PETSC_DRAW_BLACK);CHKERRQ(ierr);
      }
    }
  }
  if (zctx->name0) {
    PetscReal xl,yl,xr,yr,x,y;
    ierr = PetscDrawGetCoordinates(draw,&xl,&yl,&xr,&yr);CHKERRQ(ierr);
    x    = xl + .3*(xr - xl);
    xl   = xl + .01*(xr - xl);
    y    = yr - .3*(yr - yl);
    yl   = yl + .01*(yr - yl);
    ierr = PetscDrawString(draw,x,yl,PETSC_DRAW_BLACK,zctx->name0);CHKERRQ(ierr);
    ierr = PetscDrawStringVertical(draw,xl,y,PETSC_DRAW_BLACK,zctx->name1);CHKERRQ(ierr);
  }
  /*
     Ideally we would use the PetscDrawAxis object to manage displaying the coordinate limits 
     but that may require some refactoring.
  */
  ierr = MPIU_Allreduce(&xmin,&xminf,1,MPIU_REAL,MPIU_MIN,PetscObjectComm((PetscObject)draw));CHKERRQ(ierr);
  ierr = MPIU_Allreduce(&xmax,&xmaxf,1,MPIU_REAL,MPIU_MAX,PetscObjectComm((PetscObject)draw));CHKERRQ(ierr);
  ierr = MPIU_Allreduce(&ymin,&yminf,1,MPIU_REAL,MPIU_MIN,PetscObjectComm((PetscObject)draw));CHKERRQ(ierr);
  ierr = MPIU_Allreduce(&ymax,&ymaxf,1,MPIU_REAL,MPIU_MAX,PetscObjectComm((PetscObject)draw));CHKERRQ(ierr);
  ierr = PetscSNPrintf(value,16,"%f",xminf);CHKERRQ(ierr);
  ierr = PetscDrawString(draw,xminf,yminf - .05*(ymaxf - yminf),PETSC_DRAW_BLACK,value);CHKERRQ(ierr);
  ierr = PetscSNPrintf(value,16,"%f",xmaxf);CHKERRQ(ierr);
  ierr = PetscStrlen(value,&len);CHKERRQ(ierr);
  ierr = PetscDrawStringGetSize(draw,&w,NULL);CHKERRQ(ierr);
  ierr = PetscDrawString(draw,xmaxf - len*w,yminf - .05*(ymaxf - yminf),PETSC_DRAW_BLACK,value);CHKERRQ(ierr);
  ierr = PetscSNPrintf(value,16,"%f",yminf);CHKERRQ(ierr);
  ierr = PetscDrawString(draw,xminf - .05*(xmaxf - xminf),yminf,PETSC_DRAW_BLACK,value);CHKERRQ(ierr);
  ierr = PetscSNPrintf(value,16,"%f",ymaxf);CHKERRQ(ierr);
  ierr = PetscDrawString(draw,xminf - .05*(xmaxf - xminf),ymaxf,PETSC_DRAW_BLACK,value);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecView_MPI_Draw_DA2d"
PetscErrorCode VecView_MPI_Draw_DA2d(Vec xin,PetscViewer viewer)
{
  DM                 da,dac,dag;
  PetscErrorCode     ierr;
  PetscMPIInt        rank;
  PetscInt           N,s,M,w,ncoors = 4;
  const PetscInt     *lx,*ly;
  PetscReal          coors[4],ymin,ymax,xmin,xmax;
  PetscDraw          draw,popup;
  PetscBool          isnull,useports = PETSC_FALSE;
  MPI_Comm           comm;
  Vec                xlocal,xcoor,xcoorl;
  DMBoundaryType     bx,by;
  DMDAStencilType    st;
  ZoomCtx            zctx;
  PetscDrawViewPorts *ports = NULL;
  PetscViewerFormat  format;
  PetscInt           *displayfields;
  PetscInt           ndisplayfields,i,nbounds;
  const PetscReal    *bounds;

  PetscFunctionBegin;
  zctx.showgrid = PETSC_FALSE;

  ierr = PetscViewerDrawGetDraw(viewer,0,&draw);CHKERRQ(ierr);
  ierr = PetscDrawIsNull(draw,&isnull);CHKERRQ(ierr); if (isnull) PetscFunctionReturn(0);
  ierr = PetscViewerDrawGetBounds(viewer,&nbounds,&bounds);CHKERRQ(ierr);

  ierr = VecGetDM(xin,&da);CHKERRQ(ierr);
  if (!da) SETERRQ(PetscObjectComm((PetscObject)xin),PETSC_ERR_ARG_WRONG,"Vector not generated from a DMDA");

  ierr = PetscObjectGetComm((PetscObject)xin,&comm);CHKERRQ(ierr);
  ierr = MPI_Comm_rank(comm,&rank);CHKERRQ(ierr);

  ierr = DMDAGetInfo(da,0,&M,&N,0,&zctx.m,&zctx.n,0,&w,&s,&bx,&by,0,&st);CHKERRQ(ierr);
  ierr = DMDAGetOwnershipRanges(da,&lx,&ly,NULL);CHKERRQ(ierr);

  /*
        Obtain a sequential vector that is going to contain the local values plus ONE layer of
     ghosted values to draw the graphics from. We also need its corresponding DMDA (dac) that will
     update the local values pluse ONE layer of ghost values.
  */
  ierr = PetscObjectQuery((PetscObject)da,"GraphicsGhosted",(PetscObject*)&xlocal);CHKERRQ(ierr);
  if (!xlocal) {
    if (bx !=  DM_BOUNDARY_NONE || by !=  DM_BOUNDARY_NONE || s != 1 || st != DMDA_STENCIL_BOX) {
      /*
         if original da is not of stencil width one, or periodic or not a box stencil then
         create a special DMDA to handle one level of ghost points for graphics
      */
      ierr = DMDACreate2d(comm,DM_BOUNDARY_NONE,DM_BOUNDARY_NONE,DMDA_STENCIL_BOX,M,N,zctx.m,zctx.n,w,1,lx,ly,&dac);CHKERRQ(ierr);
      ierr = PetscInfo(da,"Creating auxilary DMDA for managing graphics ghost points\n");CHKERRQ(ierr);
    } else {
      /* otherwise we can use the da we already have */
      dac = da;
    }
    /* create local vector for holding ghosted values used in graphics */
    ierr = DMCreateLocalVector(dac,&xlocal);CHKERRQ(ierr);
    if (dac != da) {
      /* don't keep any public reference of this DMDA, is is only available through xlocal */
      ierr = PetscObjectDereference((PetscObject)dac);CHKERRQ(ierr);
    } else {
      /* remove association between xlocal and da, because below we compose in the opposite
         direction and if we left this connect we'd get a loop, so the objects could
         never be destroyed */
      ierr = PetscObjectRemoveReference((PetscObject)xlocal,"__PETSc_dm");CHKERRQ(ierr);
    }
    ierr = PetscObjectCompose((PetscObject)da,"GraphicsGhosted",(PetscObject)xlocal);CHKERRQ(ierr);
    ierr = PetscObjectDereference((PetscObject)xlocal);CHKERRQ(ierr);
  } else {
    if (bx !=  DM_BOUNDARY_NONE || by !=  DM_BOUNDARY_NONE || s != 1 || st != DMDA_STENCIL_BOX) {
      ierr = VecGetDM(xlocal, &dac);CHKERRQ(ierr);
    } else {
      dac = da;
    }
  }

  /*
      Get local (ghosted) values of vector
  */
  ierr = DMGlobalToLocalBegin(dac,xin,INSERT_VALUES,xlocal);CHKERRQ(ierr);
  ierr = DMGlobalToLocalEnd(dac,xin,INSERT_VALUES,xlocal);CHKERRQ(ierr);
  ierr = VecGetArrayRead(xlocal,&zctx.v);CHKERRQ(ierr);

  /* get coordinates of nodes */
  ierr = DMGetCoordinates(da,&xcoor);CHKERRQ(ierr);
  if (!xcoor) {
    ierr = DMDASetUniformCoordinates(da,0.0,1.0,0.0,1.0,0.0,0.0);CHKERRQ(ierr);
    ierr = DMGetCoordinates(da,&xcoor);CHKERRQ(ierr);
  }

  /*
      Determine the min and max  coordinates in plot
  */
  ierr     = VecStrideMin(xcoor,0,NULL,&xmin);CHKERRQ(ierr);
  ierr     = VecStrideMax(xcoor,0,NULL,&xmax);CHKERRQ(ierr);
  ierr     = VecStrideMin(xcoor,1,NULL,&ymin);CHKERRQ(ierr);
  ierr     = VecStrideMax(xcoor,1,NULL,&ymax);CHKERRQ(ierr);
  coors[0] = xmin - .05*(xmax- xmin); coors[2] = xmax + .05*(xmax - xmin);
  coors[1] = ymin - .05*(ymax- ymin); coors[3] = ymax + .05*(ymax - ymin);
  ierr     = PetscInfo4(da,"Preparing DMDA 2d contour plot coordinates %g %g %g %g\n",(double)coors[0],(double)coors[1],(double)coors[2],(double)coors[3]);CHKERRQ(ierr);

  ierr = PetscOptionsGetRealArray(NULL,NULL,"-draw_coordinates",coors,&ncoors,NULL);CHKERRQ(ierr);

  /*
       get local ghosted version of coordinates
  */
  ierr = PetscObjectQuery((PetscObject)da,"GraphicsCoordinateGhosted",(PetscObject*)&xcoorl);CHKERRQ(ierr);
  if (!xcoorl) {
    /* create DMDA to get local version of graphics */
    ierr = DMDACreate2d(comm,DM_BOUNDARY_NONE,DM_BOUNDARY_NONE,DMDA_STENCIL_BOX,M,N,zctx.m,zctx.n,2,1,lx,ly,&dag);CHKERRQ(ierr);
    ierr = PetscInfo(dag,"Creating auxilary DMDA for managing graphics coordinates ghost points\n");CHKERRQ(ierr);
    ierr = DMCreateLocalVector(dag,&xcoorl);CHKERRQ(ierr);
    ierr = PetscObjectCompose((PetscObject)da,"GraphicsCoordinateGhosted",(PetscObject)xcoorl);CHKERRQ(ierr);
    ierr = PetscObjectDereference((PetscObject)dag);CHKERRQ(ierr);
    ierr = PetscObjectDereference((PetscObject)xcoorl);CHKERRQ(ierr);
  } else {
    ierr = VecGetDM(xcoorl,&dag);CHKERRQ(ierr);
  }
  ierr = DMGlobalToLocalBegin(dag,xcoor,INSERT_VALUES,xcoorl);CHKERRQ(ierr);
  ierr = DMGlobalToLocalEnd(dag,xcoor,INSERT_VALUES,xcoorl);CHKERRQ(ierr);
  ierr = VecGetArrayRead(xcoorl,&zctx.xy);CHKERRQ(ierr);

  /*
        Get information about size of area each processor must do graphics for
  */
  ierr = DMDAGetInfo(dac,0,&M,&N,0,0,0,0,&zctx.step,0,&bx,&by,0,0);CHKERRQ(ierr);
  ierr = DMDAGetGhostCorners(dac,0,0,0,&zctx.m,&zctx.n,0);CHKERRQ(ierr);

  ierr = PetscOptionsGetBool(NULL,NULL,"-draw_contour_grid",&zctx.showgrid,NULL);CHKERRQ(ierr);

  ierr = DMDASelectFields(da,&ndisplayfields,&displayfields);CHKERRQ(ierr);

  ierr = PetscViewerGetFormat(viewer,&format);CHKERRQ(ierr);
  ierr = PetscOptionsGetBool(NULL,NULL,"-draw_ports",&useports,NULL);CHKERRQ(ierr);
  if (useports || format == PETSC_VIEWER_DRAW_PORTS) {
    ierr       = PetscDrawSynchronizedClear(draw);CHKERRQ(ierr);
    ierr       = PetscDrawViewPortsCreate(draw,ndisplayfields,&ports);CHKERRQ(ierr);
    zctx.name0 = 0;
    zctx.name1 = 0;
  } else {
    ierr = DMDAGetCoordinateName(da,0,&zctx.name0);CHKERRQ(ierr);
    ierr = DMDAGetCoordinateName(da,1,&zctx.name1);CHKERRQ(ierr);
  }

  /*
     Loop over each field; drawing each in a different window
  */
  for (i=0; i<ndisplayfields; i++) {
    zctx.k = displayfields[i];
    if (useports) {
      ierr = PetscDrawViewPortsSet(ports,i);CHKERRQ(ierr);
    } else {
      ierr = PetscViewerDrawGetDraw(viewer,i,&draw);CHKERRQ(ierr);
    }

    /*
        Determine the min and max color in plot
    */
    ierr = VecStrideMin(xin,zctx.k,NULL,&zctx.min);CHKERRQ(ierr);
    ierr = VecStrideMax(xin,zctx.k,NULL,&zctx.max);CHKERRQ(ierr);
    if (zctx.k < nbounds) {
      zctx.min = bounds[2*zctx.k];
      zctx.max = bounds[2*zctx.k+1];
    }
    if (zctx.min == zctx.max) {
      zctx.min -= 1.e-12;
      zctx.max += 1.e-12;
    }

    if (!rank) {
      const char *title;

      ierr = DMDAGetFieldName(da,zctx.k,&title);CHKERRQ(ierr);
      if (title) {
        ierr = PetscDrawSetTitle(draw,title);CHKERRQ(ierr);
      }
    }
    ierr = PetscDrawSetCoordinates(draw,coors[0],coors[1],coors[2],coors[3]);CHKERRQ(ierr);
    ierr = PetscInfo2(da,"DMDA 2d contour plot min %g max %g\n",(double)zctx.min,(double)zctx.max);CHKERRQ(ierr);

    ierr = PetscDrawGetPopup(draw,&popup);CHKERRQ(ierr);
    if (popup) {ierr = PetscDrawScalePopup(popup,zctx.min,zctx.max);CHKERRQ(ierr);}

    zctx.scale = (245.0 - PETSC_DRAW_BASIC_COLORS)/(zctx.max - zctx.min);

    ierr = PetscDrawZoom(draw,VecView_MPI_Draw_DA2d_Zoom,&zctx);CHKERRQ(ierr);
  }
  ierr = PetscFree(displayfields);CHKERRQ(ierr);
  ierr = PetscDrawViewPortsDestroy(ports);CHKERRQ(ierr);

  ierr = VecRestoreArrayRead(xcoorl,&zctx.xy);CHKERRQ(ierr);
  ierr = VecRestoreArrayRead(xlocal,&zctx.v);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#if defined(PETSC_HAVE_HDF5)
#undef __FUNCT__
#define __FUNCT__ "VecGetHDF5ChunkSize"
static PetscErrorCode VecGetHDF5ChunkSize(DM_DA *da, Vec xin, PetscInt dimension, PetscInt timestep, hsize_t *chunkDims)
{
  PetscMPIInt    comm_size;
  PetscErrorCode ierr;
  hsize_t        chunk_size, target_size, dim;
  hsize_t        vec_size = sizeof(PetscScalar)*da->M*da->N*da->P*da->w;
  hsize_t        avg_local_vec_size,KiB = 1024,MiB = KiB*KiB,GiB = MiB*KiB,min_size = MiB;
  hsize_t        max_chunks = 64*KiB;                                              /* HDF5 internal limitation */
  hsize_t        max_chunk_size = 4*GiB;                                           /* HDF5 internal limitation */
  PetscInt       zslices=da->p, yslices=da->n, xslices=da->m;

  PetscFunctionBegin;
  ierr = MPI_Comm_size(PetscObjectComm((PetscObject)xin), &comm_size);CHKERRQ(ierr);
  avg_local_vec_size = (hsize_t) ceil(vec_size*1.0/comm_size);      /* we will attempt to use this as the chunk size */

  target_size = (hsize_t) ceil(PetscMin(vec_size,PetscMin(max_chunk_size,PetscMax(avg_local_vec_size,PetscMax(ceil(vec_size*1.0/max_chunks),min_size)))));
  /* following line uses sizeof(PetscReal) instead of sizeof(PetscScalar) because the last dimension of chunkDims[] captures the 2* when complex numbers are being used */
  chunk_size = (hsize_t) PetscMax(1,chunkDims[0])*PetscMax(1,chunkDims[1])*PetscMax(1,chunkDims[2])*PetscMax(1,chunkDims[3])*PetscMax(1,chunkDims[4])*PetscMax(1,chunkDims[5])*sizeof(PetscReal);

  /*
   if size/rank > max_chunk_size, we need radical measures: even going down to
   avg_local_vec_size is not enough, so we simply use chunk size of 4 GiB no matter
   what, composed in the most efficient way possible.
   N.B. this minimises the number of chunks, which may or may not be the optimal
   solution. In a BG, for example, the optimal solution is probably to make # chunks = #
   IO nodes involved, but this author has no access to a BG to figure out how to
   reliably find the right number. And even then it may or may not be enough.
   */
  if (avg_local_vec_size > max_chunk_size) {
    /* check if we can just split local z-axis: is that enough? */
    zslices = (PetscInt)ceil(vec_size*1.0/(da->p*max_chunk_size))*zslices;
    if (zslices > da->P) {
      /* lattice is too large in xy-directions, splitting z only is not enough */
      zslices = da->P;
      yslices= (PetscInt)ceil(vec_size*1.0/(zslices*da->n*max_chunk_size))*yslices;
      if (yslices > da->N) {
	/* lattice is too large in x-direction, splitting along z, y is not enough */
	yslices = da->N;
	xslices= (PetscInt)ceil(vec_size*1.0/(zslices*yslices*da->m*max_chunk_size))*xslices;
      }
    }
    dim = 0;
    if (timestep >= 0) {
      ++dim;
    }
    /* prefer to split z-axis, even down to planar slices */
    if (dimension == 3) {
      chunkDims[dim++] = (hsize_t) da->P/zslices;
      chunkDims[dim++] = (hsize_t) da->N/yslices;
      chunkDims[dim++] = (hsize_t) da->M/xslices;
    } else {
      /* This is a 2D world exceeding 4GiB in size; yes, I've seen them, even used myself */
      chunkDims[dim++] = (hsize_t) da->N/yslices;
      chunkDims[dim++] = (hsize_t) da->M/xslices;
    }
    chunk_size = (hsize_t) PetscMax(1,chunkDims[0])*PetscMax(1,chunkDims[1])*PetscMax(1,chunkDims[2])*PetscMax(1,chunkDims[3])*PetscMax(1,chunkDims[4])*PetscMax(1,chunkDims[5])*sizeof(double);
  } else {
    if (target_size < chunk_size) {
      /* only change the defaults if target_size < chunk_size */
      dim = 0;
      if (timestep >= 0) {
	++dim;
      }
      /* prefer to split z-axis, even down to planar slices */
      if (dimension == 3) {
	/* try splitting the z-axis to core-size bits, i.e. divide chunk size by # comm_size in z-direction */
	if (target_size >= chunk_size/da->p) {
	  /* just make chunks the size of <local_z>x<whole_world_y>x<whole_world_x>x<dof> */
	  chunkDims[dim] = (hsize_t) ceil(da->P*1.0/da->p);
	} else {
	  /* oops, just splitting the z-axis is NOT ENOUGH, need to split more; let's be
           radical and let everyone write all they've got */
	  chunkDims[dim++] = (hsize_t) ceil(da->P*1.0/da->p);
	  chunkDims[dim++] = (hsize_t) ceil(da->N*1.0/da->n);
	  chunkDims[dim++] = (hsize_t) ceil(da->M*1.0/da->m);
	}
      } else {
	/* This is a 2D world exceeding 4GiB in size; yes, I've seen them, even used myself */
	if (target_size >= chunk_size/da->n) {
	  /* just make chunks the size of <local_z>x<whole_world_y>x<whole_world_x>x<dof> */
	  chunkDims[dim] = (hsize_t) ceil(da->N*1.0/da->n);
	} else {
	  /* oops, just splitting the z-axis is NOT ENOUGH, need to split more; let's be
	   radical and let everyone write all they've got */
	  chunkDims[dim++] = (hsize_t) ceil(da->N*1.0/da->n);
	  chunkDims[dim++] = (hsize_t) ceil(da->M*1.0/da->m);
	}

      }
      chunk_size = (hsize_t) PetscMax(1,chunkDims[0])*PetscMax(1,chunkDims[1])*PetscMax(1,chunkDims[2])*PetscMax(1,chunkDims[3])*PetscMax(1,chunkDims[4])*PetscMax(1,chunkDims[5])*sizeof(double);
    } else {
      /* precomputed chunks are fine, we don't need to do anything */
    }
  }
  PetscFunctionReturn(0);
}
#endif

#if defined(PETSC_HAVE_HDF5)
#undef __FUNCT__
#define __FUNCT__ "VecView_MPI_HDF5_DA"
PetscErrorCode VecView_MPI_HDF5_DA(Vec xin,PetscViewer viewer)
{
  DM                dm;
  DM_DA             *da;
  hid_t             filespace;  /* file dataspace identifier */
  hid_t             chunkspace; /* chunk dataset property identifier */
  hid_t             plist_id;   /* property list identifier */
  hid_t             dset_id;    /* dataset identifier */
  hid_t             memspace;   /* memory dataspace identifier */
  hid_t             file_id;
  hid_t             group;
  hid_t             memscalartype; /* scalar type for mem (H5T_NATIVE_FLOAT or H5T_NATIVE_DOUBLE) */
  hid_t             filescalartype; /* scalar type for file (H5T_NATIVE_FLOAT or H5T_NATIVE_DOUBLE) */
  hsize_t           dim;
  hsize_t           maxDims[6]={0}, dims[6]={0}, chunkDims[6]={0}, count[6]={0}, offset[6]={0}; /* we depend on these being sane later on  */
  PetscInt          timestep, dimension;
  const PetscScalar *x;
  const char        *vecname;
  PetscErrorCode    ierr;
  PetscBool         dim2;
  PetscBool         spoutput;

  PetscFunctionBegin;
  ierr = PetscViewerHDF5OpenGroup(viewer, &file_id, &group);CHKERRQ(ierr);
  ierr = PetscViewerHDF5GetTimestep(viewer, &timestep);CHKERRQ(ierr);
  ierr = PetscViewerHDF5GetBaseDimension2(viewer,&dim2);CHKERRQ(ierr);
  ierr = PetscViewerHDF5GetSPOutput(viewer,&spoutput);CHKERRQ(ierr);

  ierr = VecGetDM(xin,&dm);CHKERRQ(ierr);
  if (!dm) SETERRQ(PetscObjectComm((PetscObject)xin),PETSC_ERR_ARG_WRONG,"Vector not generated from a DMDA");
  da = (DM_DA*)dm->data;
  ierr = DMGetDimension(dm, &dimension);CHKERRQ(ierr);

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
  if (dimension == 3) {
    ierr           = PetscHDF5IntCast(da->P,dims+dim);CHKERRQ(ierr);
    maxDims[dim]   = dims[dim];
    chunkDims[dim] = dims[dim];
    ++dim;
  }
  if (dimension > 1) {
    ierr           = PetscHDF5IntCast(da->N,dims+dim);CHKERRQ(ierr);
    maxDims[dim]   = dims[dim];
    chunkDims[dim] = dims[dim];
    ++dim;
  }
  ierr           = PetscHDF5IntCast(da->M,dims+dim);CHKERRQ(ierr);
  maxDims[dim]   = dims[dim];
  chunkDims[dim] = dims[dim];
  ++dim;
  if (da->w > 1 || dim2) {
    ierr           = PetscHDF5IntCast(da->w,dims+dim);CHKERRQ(ierr);
    maxDims[dim]   = dims[dim];
    chunkDims[dim] = dims[dim];
    ++dim;
  }
#if defined(PETSC_USE_COMPLEX)
  dims[dim]      = 2;
  maxDims[dim]   = dims[dim];
  chunkDims[dim] = dims[dim];
  ++dim;
#endif

  ierr = VecGetHDF5ChunkSize(da, xin, dimension, timestep, chunkDims); CHKERRQ(ierr);

  PetscStackCallHDF5Return(filespace,H5Screate_simple,(dim, dims, maxDims));

#if defined(PETSC_USE_REAL_SINGLE)
  memscalartype = H5T_NATIVE_FLOAT;
  filescalartype = H5T_NATIVE_FLOAT;
#elif defined(PETSC_USE_REAL___FLOAT128)
#error "HDF5 output with 128 bit floats not supported."
#else
  memscalartype = H5T_NATIVE_DOUBLE;
  if (spoutput == PETSC_TRUE) filescalartype = H5T_NATIVE_FLOAT;
  else filescalartype = H5T_NATIVE_DOUBLE;
#endif

  /* Create the dataset with default properties and close filespace */
  ierr = PetscObjectGetName((PetscObject)xin,&vecname);CHKERRQ(ierr);
  if (!H5Lexists(group, vecname, H5P_DEFAULT)) {
    /* Create chunk */
    PetscStackCallHDF5Return(chunkspace,H5Pcreate,(H5P_DATASET_CREATE));
    PetscStackCallHDF5(H5Pset_chunk,(chunkspace, dim, chunkDims));

#if (H5_VERS_MAJOR * 10000 + H5_VERS_MINOR * 100 + H5_VERS_RELEASE >= 10800)
    PetscStackCallHDF5Return(dset_id,H5Dcreate2,(group, vecname, filescalartype, filespace, H5P_DEFAULT, chunkspace, H5P_DEFAULT));
#else
    PetscStackCallHDF5Return(dset_id,H5Dcreate,(group, vecname, filescalartype, filespace, H5P_DEFAULT));
#endif
  } else {
    PetscStackCallHDF5Return(dset_id,H5Dopen2,(group, vecname, H5P_DEFAULT));
    PetscStackCallHDF5(H5Dset_extent,(dset_id, dims));
  }
  PetscStackCallHDF5(H5Sclose,(filespace));

  /* Each process defines a dataset and writes it to the hyperslab in the file */
  dim = 0;
  if (timestep >= 0) {
    offset[dim] = timestep;
    ++dim;
  }
  if (dimension == 3) {ierr = PetscHDF5IntCast(da->zs,offset + dim++);CHKERRQ(ierr);}
  if (dimension > 1)  {ierr = PetscHDF5IntCast(da->ys,offset + dim++);CHKERRQ(ierr);}
  ierr = PetscHDF5IntCast(da->xs/da->w,offset + dim++);CHKERRQ(ierr);
  if (da->w > 1 || dim2) offset[dim++] = 0;
#if defined(PETSC_USE_COMPLEX)
  offset[dim++] = 0;
#endif
  dim = 0;
  if (timestep >= 0) {
    count[dim] = 1;
    ++dim;
  }
  if (dimension == 3) {ierr = PetscHDF5IntCast(da->ze - da->zs,count + dim++);CHKERRQ(ierr);}
  if (dimension > 1)  {ierr = PetscHDF5IntCast(da->ye - da->ys,count + dim++);CHKERRQ(ierr);}
  ierr = PetscHDF5IntCast((da->xe - da->xs)/da->w,count + dim++);CHKERRQ(ierr);
  if (da->w > 1 || dim2) {ierr = PetscHDF5IntCast(da->w,count + dim++);CHKERRQ(ierr);}
#if defined(PETSC_USE_COMPLEX)
  count[dim++] = 2;
#endif
  PetscStackCallHDF5Return(memspace,H5Screate_simple,(dim, count, NULL));
  PetscStackCallHDF5Return(filespace,H5Dget_space,(dset_id));
  PetscStackCallHDF5(H5Sselect_hyperslab,(filespace, H5S_SELECT_SET, offset, NULL, count, NULL));

  /* Create property list for collective dataset write */
  PetscStackCallHDF5Return(plist_id,H5Pcreate,(H5P_DATASET_XFER));
#if defined(PETSC_HAVE_H5PSET_FAPL_MPIO)
  PetscStackCallHDF5(H5Pset_dxpl_mpio,(plist_id, H5FD_MPIO_COLLECTIVE));
#endif
  /* To write dataset independently use H5Pset_dxpl_mpio(plist_id, H5FD_MPIO_INDEPENDENT) */

  ierr   = VecGetArrayRead(xin, &x);CHKERRQ(ierr);
  PetscStackCallHDF5(H5Dwrite,(dset_id, memscalartype, memspace, filespace, plist_id, x));
  PetscStackCallHDF5(H5Fflush,(file_id, H5F_SCOPE_GLOBAL));
  ierr   = VecRestoreArrayRead(xin, &x);CHKERRQ(ierr);

  /* Close/release resources */
  if (group != file_id) {
    PetscStackCallHDF5(H5Gclose,(group));
  }
  PetscStackCallHDF5(H5Pclose,(plist_id));
  PetscStackCallHDF5(H5Sclose,(filespace));
  PetscStackCallHDF5(H5Sclose,(memspace));
  PetscStackCallHDF5(H5Dclose,(dset_id));
  ierr   = PetscInfo1(xin,"Wrote Vec object with name %s\n",vecname);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
#endif

extern PetscErrorCode VecView_MPI_Draw_DA1d(Vec,PetscViewer);

#if defined(PETSC_HAVE_MPIIO)
#undef __FUNCT__
#define __FUNCT__ "DMDAArrayMPIIO"
static PetscErrorCode DMDAArrayMPIIO(DM da,PetscViewer viewer,Vec xin,PetscBool write)
{
  PetscErrorCode    ierr;
  MPI_File          mfdes;
  PetscMPIInt       gsizes[4],lsizes[4],lstarts[4],asiz,dof;
  MPI_Datatype      view;
  const PetscScalar *array;
  MPI_Offset        off;
  MPI_Aint          ub,ul;
  PetscInt          type,rows,vecrows,tr[2];
  DM_DA             *dd = (DM_DA*)da->data;
  PetscBool         skipheader;

  PetscFunctionBegin;
  ierr = VecGetSize(xin,&vecrows);CHKERRQ(ierr);
  ierr = PetscViewerBinaryGetSkipHeader(viewer,&skipheader);CHKERRQ(ierr);
  if (!write) {
    /* Read vector header. */
    if (!skipheader) {
      ierr = PetscViewerBinaryRead(viewer,tr,2,NULL,PETSC_INT);CHKERRQ(ierr);
      type = tr[0];
      rows = tr[1];
      if (type != VEC_FILE_CLASSID) SETERRQ(PetscObjectComm((PetscObject)da),PETSC_ERR_ARG_WRONG,"Not vector next in file");
      if (rows != vecrows) SETERRQ(PetscObjectComm((PetscObject)da),PETSC_ERR_ARG_SIZ,"Vector in file not same size as DMDA vector");
    }
  } else {
    tr[0] = VEC_FILE_CLASSID;
    tr[1] = vecrows;
    if (!skipheader) {
      ierr  = PetscViewerBinaryWrite(viewer,tr,2,PETSC_INT,PETSC_TRUE);CHKERRQ(ierr);
    }
  }

  ierr       = PetscMPIIntCast(dd->w,&dof);CHKERRQ(ierr);
  gsizes[0]  = dof;
  ierr       = PetscMPIIntCast(dd->M,gsizes+1);CHKERRQ(ierr);
  ierr       = PetscMPIIntCast(dd->N,gsizes+2);CHKERRQ(ierr);
  ierr       = PetscMPIIntCast(dd->P,gsizes+3);CHKERRQ(ierr);
  lsizes[0]  = dof;
  ierr       = PetscMPIIntCast((dd->xe-dd->xs)/dof,lsizes+1);CHKERRQ(ierr);
  ierr       = PetscMPIIntCast(dd->ye-dd->ys,lsizes+2);CHKERRQ(ierr);
  ierr       = PetscMPIIntCast(dd->ze-dd->zs,lsizes+3);CHKERRQ(ierr);
  lstarts[0] = 0;
  ierr       = PetscMPIIntCast(dd->xs/dof,lstarts+1);CHKERRQ(ierr);
  ierr       = PetscMPIIntCast(dd->ys,lstarts+2);CHKERRQ(ierr);
  ierr       = PetscMPIIntCast(dd->zs,lstarts+3);CHKERRQ(ierr);
  ierr       = MPI_Type_create_subarray(da->dim+1,gsizes,lsizes,lstarts,MPI_ORDER_FORTRAN,MPIU_SCALAR,&view);CHKERRQ(ierr);
  ierr       = MPI_Type_commit(&view);CHKERRQ(ierr);

  ierr = PetscViewerBinaryGetMPIIODescriptor(viewer,&mfdes);CHKERRQ(ierr);
  ierr = PetscViewerBinaryGetMPIIOOffset(viewer,&off);CHKERRQ(ierr);
  ierr = MPI_File_set_view(mfdes,off,MPIU_SCALAR,view,(char*)"native",MPI_INFO_NULL);CHKERRQ(ierr);
  ierr = VecGetArrayRead(xin,&array);CHKERRQ(ierr);
  asiz = lsizes[1]*(lsizes[2] > 0 ? lsizes[2] : 1)*(lsizes[3] > 0 ? lsizes[3] : 1)*dof;
  if (write) {
    ierr = MPIU_File_write_all(mfdes,(PetscScalar*)array,asiz,MPIU_SCALAR,MPI_STATUS_IGNORE);CHKERRQ(ierr);
  } else {
    ierr = MPIU_File_read_all(mfdes,(PetscScalar*)array,asiz,MPIU_SCALAR,MPI_STATUS_IGNORE);CHKERRQ(ierr);
  }
  ierr = MPI_Type_get_extent(view,&ul,&ub);CHKERRQ(ierr);
  ierr = PetscViewerBinaryAddMPIIOOffset(viewer,ub);CHKERRQ(ierr);
  ierr = VecRestoreArrayRead(xin,&array);CHKERRQ(ierr);
  ierr = MPI_Type_free(&view);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}
#endif

#undef __FUNCT__
#define __FUNCT__ "VecView_MPI_DA"
PetscErrorCode  VecView_MPI_DA(Vec xin,PetscViewer viewer)
{
  DM                da;
  PetscErrorCode    ierr;
  PetscInt          dim;
  Vec               natural;
  PetscBool         isdraw,isvtk;
#if defined(PETSC_HAVE_HDF5)
  PetscBool         ishdf5;
#endif
  const char        *prefix,*name;
  PetscViewerFormat format;

  PetscFunctionBegin;
  ierr = VecGetDM(xin,&da);CHKERRQ(ierr);
  if (!da) SETERRQ(PetscObjectComm((PetscObject)xin),PETSC_ERR_ARG_WRONG,"Vector not generated from a DMDA");
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERDRAW,&isdraw);CHKERRQ(ierr);
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERVTK,&isvtk);CHKERRQ(ierr);
#if defined(PETSC_HAVE_HDF5)
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERHDF5,&ishdf5);CHKERRQ(ierr);
#endif
  if (isdraw) {
    ierr = DMDAGetInfo(da,&dim,0,0,0,0,0,0,0,0,0,0,0,0);CHKERRQ(ierr);
    if (dim == 1) {
      ierr = VecView_MPI_Draw_DA1d(xin,viewer);CHKERRQ(ierr);
    } else if (dim == 2) {
      ierr = VecView_MPI_Draw_DA2d(xin,viewer);CHKERRQ(ierr);
    } else SETERRQ1(PetscObjectComm((PetscObject)da),PETSC_ERR_SUP,"Cannot graphically view vector associated with this dimensional DMDA %D",dim);
  } else if (isvtk) {           /* Duplicate the Vec and hold a reference to the DM */
    Vec Y;
    ierr = PetscObjectReference((PetscObject)da);CHKERRQ(ierr);
    ierr = VecDuplicate(xin,&Y);CHKERRQ(ierr);
    if (((PetscObject)xin)->name) {
      /* If xin was named, copy the name over to Y. The duplicate names are safe because nobody else will ever see Y. */
      ierr = PetscObjectSetName((PetscObject)Y,((PetscObject)xin)->name);CHKERRQ(ierr);
    }
    ierr = VecCopy(xin,Y);CHKERRQ(ierr);
    ierr = PetscViewerVTKAddField(viewer,(PetscObject)da,DMDAVTKWriteAll,PETSC_VTK_POINT_FIELD,(PetscObject)Y);CHKERRQ(ierr);
#if defined(PETSC_HAVE_HDF5)
  } else if (ishdf5) {
    ierr = VecView_MPI_HDF5_DA(xin,viewer);CHKERRQ(ierr);
#endif
  } else {
#if defined(PETSC_HAVE_MPIIO)
    PetscBool isbinary,isMPIIO;

    ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERBINARY,&isbinary);CHKERRQ(ierr);
    if (isbinary) {
      ierr = PetscViewerBinaryGetUseMPIIO(viewer,&isMPIIO);CHKERRQ(ierr);
      if (isMPIIO) {
        ierr = DMDAArrayMPIIO(da,viewer,xin,PETSC_TRUE);CHKERRQ(ierr);
        PetscFunctionReturn(0);
      }
    }
#endif

    /* call viewer on natural ordering */
    ierr = PetscObjectGetOptionsPrefix((PetscObject)xin,&prefix);CHKERRQ(ierr);
    ierr = DMDACreateNaturalVector(da,&natural);CHKERRQ(ierr);
    ierr = PetscObjectSetOptionsPrefix((PetscObject)natural,prefix);CHKERRQ(ierr);
    ierr = DMDAGlobalToNaturalBegin(da,xin,INSERT_VALUES,natural);CHKERRQ(ierr);
    ierr = DMDAGlobalToNaturalEnd(da,xin,INSERT_VALUES,natural);CHKERRQ(ierr);
    ierr = PetscObjectGetName((PetscObject)xin,&name);CHKERRQ(ierr);
    ierr = PetscObjectSetName((PetscObject)natural,name);CHKERRQ(ierr);

    ierr = PetscViewerGetFormat(viewer,&format);CHKERRQ(ierr);
    if (format == PETSC_VIEWER_BINARY_MATLAB) {
      /* temporarily remove viewer format so it won't trigger in the VecView() */
      ierr = PetscViewerPushFormat(viewer,PETSC_VIEWER_DEFAULT);CHKERRQ(ierr);
    }

    ierr = VecView(natural,viewer);CHKERRQ(ierr);

    if (format == PETSC_VIEWER_BINARY_MATLAB) {
      MPI_Comm    comm;
      FILE        *info;
      const char  *fieldname;
      char        fieldbuf[256];
      PetscInt    dim,ni,nj,nk,pi,pj,pk,dof,n;

      /* set the viewer format back into the viewer */
      ierr = PetscViewerPopFormat(viewer);CHKERRQ(ierr);
      ierr = PetscObjectGetComm((PetscObject)viewer,&comm);CHKERRQ(ierr);
      ierr = PetscViewerBinaryGetInfoPointer(viewer,&info);CHKERRQ(ierr);
      ierr = DMDAGetInfo(da,&dim,&ni,&nj,&nk,&pi,&pj,&pk,&dof,0,0,0,0,0);CHKERRQ(ierr);
      ierr = PetscFPrintf(comm,info,"#--- begin code written by PetscViewerBinary for MATLAB format ---#\n");CHKERRQ(ierr);
      ierr = PetscFPrintf(comm,info,"#$$ tmp = PetscBinaryRead(fd); \n");CHKERRQ(ierr);
      if (dim == 1) { ierr = PetscFPrintf(comm,info,"#$$ tmp = reshape(tmp,%d,%d);\n",dof,ni);CHKERRQ(ierr); }
      if (dim == 2) { ierr = PetscFPrintf(comm,info,"#$$ tmp = reshape(tmp,%d,%d,%d);\n",dof,ni,nj);CHKERRQ(ierr); }
      if (dim == 3) { ierr = PetscFPrintf(comm,info,"#$$ tmp = reshape(tmp,%d,%d,%d,%d);\n",dof,ni,nj,nk);CHKERRQ(ierr); }

      for (n=0; n<dof; n++) {
        ierr = DMDAGetFieldName(da,n,&fieldname);CHKERRQ(ierr);
        if (!fieldname || !fieldname[0]) {
          ierr = PetscSNPrintf(fieldbuf,sizeof fieldbuf,"field%D",n);CHKERRQ(ierr);
          fieldname = fieldbuf;
        }
        if (dim == 1) { ierr = PetscFPrintf(comm,info,"#$$ Set.%s.%s = squeeze(tmp(%d,:))';\n",name,fieldname,n+1);CHKERRQ(ierr); }
        if (dim == 2) { ierr = PetscFPrintf(comm,info,"#$$ Set.%s.%s = squeeze(tmp(%d,:,:))';\n",name,fieldname,n+1);CHKERRQ(ierr); }
        if (dim == 3) { ierr = PetscFPrintf(comm,info,"#$$ Set.%s.%s = permute(squeeze(tmp(%d,:,:,:)),[2 1 3]);\n",name,fieldname,n+1);CHKERRQ(ierr);}
      }
      ierr = PetscFPrintf(comm,info,"#$$ clear tmp; \n");CHKERRQ(ierr);
      ierr = PetscFPrintf(comm,info,"#--- end code written by PetscViewerBinary for MATLAB format ---#\n\n");CHKERRQ(ierr);
    }

    ierr = VecDestroy(&natural);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#if defined(PETSC_HAVE_HDF5)
#undef __FUNCT__
#define __FUNCT__ "VecLoad_HDF5_DA"
PetscErrorCode VecLoad_HDF5_DA(Vec xin, PetscViewer viewer)
{
  DM             da;
  PetscErrorCode ierr;
  hsize_t        dim,rdim;
  hsize_t        dims[6]={0},count[6]={0},offset[6]={0};
  PetscInt       dimension,timestep,dofInd;
  PetscScalar    *x;
  const char     *vecname;
  hid_t          filespace; /* file dataspace identifier */
  hid_t          plist_id;  /* property list identifier */
  hid_t          dset_id;   /* dataset identifier */
  hid_t          memspace;  /* memory dataspace identifier */
  hid_t          file_id,group;
  hid_t          scalartype; /* scalar type (H5T_NATIVE_FLOAT or H5T_NATIVE_DOUBLE) */
  DM_DA          *dd;
  PetscBool      dim2 = PETSC_FALSE;

  PetscFunctionBegin;
#if defined(PETSC_USE_REAL_SINGLE)
  scalartype = H5T_NATIVE_FLOAT;
#elif defined(PETSC_USE_REAL___FLOAT128)
#error "HDF5 output with 128 bit floats not supported."
#else
  scalartype = H5T_NATIVE_DOUBLE;
#endif

  ierr = PetscViewerHDF5OpenGroup(viewer, &file_id, &group);CHKERRQ(ierr);
  ierr = PetscViewerHDF5GetTimestep(viewer, &timestep);CHKERRQ(ierr);
  ierr = PetscObjectGetName((PetscObject)xin,&vecname);CHKERRQ(ierr);  
  ierr = VecGetDM(xin,&da);CHKERRQ(ierr);
  dd   = (DM_DA*)da->data;
  ierr = DMGetDimension(da, &dimension);CHKERRQ(ierr);

  /* Open dataset */
#if (H5_VERS_MAJOR * 10000 + H5_VERS_MINOR * 100 + H5_VERS_RELEASE >= 10800)
  PetscStackCallHDF5Return(dset_id,H5Dopen2,(group, vecname, H5P_DEFAULT));
#else
  PetscStackCallHDF5Return(dset_id,H5Dopen,(group, vecname));
#endif  

  /* Retrieve the dataspace for the dataset */
  PetscStackCallHDF5Return(filespace,H5Dget_space,(dset_id));
  PetscStackCallHDF5Return(rdim,H5Sget_simple_extent_dims,(filespace, dims, NULL));

  /* Expected dimension for holding the dof's */
#if defined(PETSC_USE_COMPLEX)
  dofInd = rdim-2;
#else
  dofInd = rdim-1;
#endif

  /* The expected number of dimensions, assuming basedimension2 = false */
  dim = dimension;
  if (dd->w > 1) ++dim;
  if (timestep >= 0) ++dim;
#if defined(PETSC_USE_COMPLEX)
  ++dim;
#endif

  /* In this case the input dataset have one extra, unexpected dimension. */
  if (rdim == dim+1) {
    /* In this case the block size unity */
    if (dd->w == 1 && dims[dofInd] == 1) dim2 = PETSC_TRUE;

    /* Special error message for the case where dof does not match the input file */
    else if (dd->w != (PetscInt) dims[dofInd]) SETERRQ2(PETSC_COMM_SELF,PETSC_ERR_FILE_UNEXPECTED, "Number of dofs in file is %D, not %D as expected",(PetscInt)dims[dofInd],dd->w);

  /* Other cases where rdim != dim cannot be handled currently */
  } else if (rdim != dim) SETERRQ3(PETSC_COMM_SELF,PETSC_ERR_FILE_UNEXPECTED, "Dimension of array in file is %d, not %d as expected with dof = %D",rdim,dim,dd->w);

  /* Set up the hyperslab size */
  dim = 0;
  if (timestep >= 0) {
    offset[dim] = timestep;
    count[dim] = 1;
    ++dim;
  }
  if (dimension == 3) {
    ierr = PetscHDF5IntCast(dd->zs,offset + dim);CHKERRQ(ierr);
    ierr = PetscHDF5IntCast(dd->ze - dd->zs,count + dim);CHKERRQ(ierr);
    ++dim;
  }
  if (dimension > 1) {
    ierr = PetscHDF5IntCast(dd->ys,offset + dim);CHKERRQ(ierr);
    ierr = PetscHDF5IntCast(dd->ye - dd->ys,count + dim);CHKERRQ(ierr);
    ++dim;
  }
  ierr = PetscHDF5IntCast(dd->xs/dd->w,offset + dim);CHKERRQ(ierr);
  ierr = PetscHDF5IntCast((dd->xe - dd->xs)/dd->w,count + dim);CHKERRQ(ierr);
  ++dim;
  if (dd->w > 1 || dim2) {
    offset[dim] = 0;
    ierr = PetscHDF5IntCast(dd->w,count + dim);CHKERRQ(ierr);
    ++dim;
  }
#if defined(PETSC_USE_COMPLEX)
  offset[dim] = 0;
  count[dim] = 2;
  ++dim;
#endif

  /* Create the memory and filespace */
  PetscStackCallHDF5Return(memspace,H5Screate_simple,(dim, count, NULL));
  PetscStackCallHDF5(H5Sselect_hyperslab,(filespace, H5S_SELECT_SET, offset, NULL, count, NULL));

  /* Create property list for collective dataset write */
  PetscStackCallHDF5Return(plist_id,H5Pcreate,(H5P_DATASET_XFER));
#if defined(PETSC_HAVE_H5PSET_FAPL_MPIO)
  PetscStackCallHDF5(H5Pset_dxpl_mpio,(plist_id, H5FD_MPIO_COLLECTIVE));
#endif
  /* To read dataset independently use H5Pset_dxpl_mpio(plist_id, H5FD_MPIO_INDEPENDENT) */

  ierr   = VecGetArray(xin, &x);CHKERRQ(ierr);
  PetscStackCallHDF5(H5Dread,(dset_id, scalartype, memspace, filespace, plist_id, x));
  ierr   = VecRestoreArray(xin, &x);CHKERRQ(ierr);

  /* Close/release resources */
  if (group != file_id) {
    PetscStackCallHDF5(H5Gclose,(group));
  }
  PetscStackCallHDF5(H5Pclose,(plist_id));
  PetscStackCallHDF5(H5Sclose,(filespace));
  PetscStackCallHDF5(H5Sclose,(memspace));
  PetscStackCallHDF5(H5Dclose,(dset_id));
  PetscFunctionReturn(0);
}
#endif

#undef __FUNCT__
#define __FUNCT__ "VecLoad_Binary_DA"
PetscErrorCode VecLoad_Binary_DA(Vec xin, PetscViewer viewer)
{
  DM             da;
  PetscErrorCode ierr;
  Vec            natural;
  const char     *prefix;
  PetscInt       bs;
  PetscBool      flag;
  DM_DA          *dd;
#if defined(PETSC_HAVE_MPIIO)
  PetscBool isMPIIO;
#endif

  PetscFunctionBegin;
  ierr = VecGetDM(xin,&da);CHKERRQ(ierr);
  dd   = (DM_DA*)da->data;
#if defined(PETSC_HAVE_MPIIO)
  ierr = PetscViewerBinaryGetUseMPIIO(viewer,&isMPIIO);CHKERRQ(ierr);
  if (isMPIIO) {
    ierr = DMDAArrayMPIIO(da,viewer,xin,PETSC_FALSE);CHKERRQ(ierr);
    PetscFunctionReturn(0);
  }
#endif

  ierr = PetscObjectGetOptionsPrefix((PetscObject)xin,&prefix);CHKERRQ(ierr);
  ierr = DMDACreateNaturalVector(da,&natural);CHKERRQ(ierr);
  ierr = PetscObjectSetName((PetscObject)natural,((PetscObject)xin)->name);CHKERRQ(ierr);
  ierr = PetscObjectSetOptionsPrefix((PetscObject)natural,prefix);CHKERRQ(ierr);
  ierr = VecLoad(natural,viewer);CHKERRQ(ierr);
  ierr = DMDANaturalToGlobalBegin(da,natural,INSERT_VALUES,xin);CHKERRQ(ierr);
  ierr = DMDANaturalToGlobalEnd(da,natural,INSERT_VALUES,xin);CHKERRQ(ierr);
  ierr = VecDestroy(&natural);CHKERRQ(ierr);
  ierr = PetscInfo(xin,"Loading vector from natural ordering into DMDA\n");CHKERRQ(ierr);
  ierr = PetscOptionsGetInt(NULL,((PetscObject)xin)->prefix,"-vecload_block_size",&bs,&flag);CHKERRQ(ierr);
  if (flag && bs != dd->w) {
    ierr = PetscInfo2(xin,"Block size in file %D not equal to DMDA's dof %D\n",bs,dd->w);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "VecLoad_Default_DA"
PetscErrorCode  VecLoad_Default_DA(Vec xin, PetscViewer viewer)
{
  PetscErrorCode ierr;
  DM             da;
  PetscBool      isbinary;
#if defined(PETSC_HAVE_HDF5)
  PetscBool ishdf5;
#endif

  PetscFunctionBegin;
  ierr = VecGetDM(xin,&da);CHKERRQ(ierr);
  if (!da) SETERRQ(PetscObjectComm((PetscObject)xin),PETSC_ERR_ARG_WRONG,"Vector not generated from a DMDA");

#if defined(PETSC_HAVE_HDF5)
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERHDF5,&ishdf5);CHKERRQ(ierr);
#endif
  ierr = PetscObjectTypeCompare((PetscObject)viewer,PETSCVIEWERBINARY,&isbinary);CHKERRQ(ierr);

  if (isbinary) {
    ierr = VecLoad_Binary_DA(xin,viewer);CHKERRQ(ierr);
#if defined(PETSC_HAVE_HDF5)
  } else if (ishdf5) {
    ierr = VecLoad_HDF5_DA(xin,viewer);CHKERRQ(ierr);
#endif
  } else SETERRQ1(PETSC_COMM_SELF,PETSC_ERR_SUP,"Viewer type %s not supported for vector loading", ((PetscObject)viewer)->type_name);
  PetscFunctionReturn(0);
}
