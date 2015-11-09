#include <../src/sys/classes/draw/utils/axisimpl.h>  /*I   "petscdraw.h"  I*/

PetscClassId PETSC_DRAWAXIS_CLASSID = 0;

#undef __FUNCT__
#define __FUNCT__ "PetscDrawAxisCreate"
/*@
   PetscDrawAxisCreate - Generate the axis data structure.

   Collective over PetscDraw

   Input Parameters:
.  win - PetscDraw object where axis to to be made

   Ouput Parameters:
.  axis - the axis datastructure

   Level: advanced

@*/
PetscErrorCode  PetscDrawAxisCreate(PetscDraw draw,PetscDrawAxis *axis)
{
  PetscBool      isnull;
  PetscDrawAxis  ad;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(draw,PETSC_DRAW_CLASSID,1);
  PetscValidPointer(axis,2);

  ierr = PetscDrawIsNull(draw,&isnull);CHKERRQ(ierr);
  if (isnull) {*axis = NULL; PetscFunctionReturn(0);}

  ierr = PetscHeaderCreate(ad,PETSC_DRAWAXIS_CLASSID,"PetscDrawAxis","Draw Axis","Draw",PetscObjectComm((PetscObject)draw),PetscDrawAxisDestroy,NULL);CHKERRQ(ierr);
  ierr = PetscLogObjectParent((PetscObject)draw,(PetscObject)ad);CHKERRQ(ierr);

  ierr = PetscObjectReference((PetscObject)draw);CHKERRQ(ierr);
  ad->win = draw;

  ad->xticks    = PetscADefTicks;
  ad->yticks    = PetscADefTicks;
  ad->xlabelstr = PetscADefLabel;
  ad->ylabelstr = PetscADefLabel;
  ad->ac        = PETSC_DRAW_BLACK;
  ad->tc        = PETSC_DRAW_BLACK;
  ad->cc        = PETSC_DRAW_BLACK;
  ad->xlabel    = 0;
  ad->ylabel    = 0;
  ad->toplabel  = 0;

  *axis = ad;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDrawAxisDestroy"
/*@
    PetscDrawAxisDestroy - Frees the space used by an axis structure.

    Collective over PetscDrawAxis

    Input Parameters:
.   axis - the axis context

    Level: advanced

@*/
PetscErrorCode  PetscDrawAxisDestroy(PetscDrawAxis *axis)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (!*axis) PetscFunctionReturn(0);
  PetscValidHeaderSpecific(*axis,PETSC_DRAWAXIS_CLASSID,1);
  if (--((PetscObject)(*axis))->refct > 0) {*axis = NULL; PetscFunctionReturn(0);}

  ierr = PetscFree((*axis)->toplabel);CHKERRQ(ierr);
  ierr = PetscFree((*axis)->xlabel);CHKERRQ(ierr);
  ierr = PetscFree((*axis)->ylabel);CHKERRQ(ierr);
  ierr = PetscDrawDestroy(&(*axis)->win);CHKERRQ(ierr);
  ierr = PetscHeaderDestroy(axis);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDrawAxisSetColors"
/*@
    PetscDrawAxisSetColors -  Sets the colors to be used for the axis,
                         tickmarks, and text.

    Not Collective (ignored on all processors except processor 0 of PetscDrawAxis)

    Input Parameters:
+   axis - the axis
.   ac - the color of the axis lines
.   tc - the color of the tick marks
-   cc - the color of the text strings

    Level: advanced

@*/
PetscErrorCode  PetscDrawAxisSetColors(PetscDrawAxis axis,int ac,int tc,int cc)
{
  PetscFunctionBegin;
  if (!axis) PetscFunctionReturn(0);
  PetscValidHeaderSpecific(axis,PETSC_DRAWAXIS_CLASSID,1);
  axis->ac = ac; axis->tc = tc; axis->cc = cc;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDrawAxisSetLabels"
/*@C
    PetscDrawAxisSetLabels -  Sets the x and y axis labels.

    Not Collective (ignored on all processors except processor 0 of PetscDrawAxis)

    Input Parameters:
+   axis - the axis
.   top - the label at the top of the image
-   xlabel,ylabel - the labes for the x and y axis

    Notes: Must be called before PetscDrawAxisDraw() or PetscDrawLGDraw()
           There should be no newlines in the arguments

    Level: advanced

@*/
PetscErrorCode  PetscDrawAxisSetLabels(PetscDrawAxis axis,const char top[],const char xlabel[],const char ylabel[])
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (!axis) PetscFunctionReturn(0);
  PetscValidHeaderSpecific(axis,PETSC_DRAWAXIS_CLASSID,1);
  ierr = PetscFree(axis->xlabel);CHKERRQ(ierr);
  ierr = PetscFree(axis->ylabel);CHKERRQ(ierr);
  ierr = PetscFree(axis->toplabel);CHKERRQ(ierr);
  ierr = PetscStrallocpy(xlabel,&axis->xlabel);CHKERRQ(ierr);
  ierr = PetscStrallocpy(ylabel,&axis->ylabel);CHKERRQ(ierr);
  ierr = PetscStrallocpy(top,&axis->toplabel);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDrawAxisSetHoldLimits"
/*@
    PetscDrawAxisSetHoldLimits -  Causes an axis to keep the same limits until this is called
        again

    Not Collective (ignored on all processors except processor 0 of PetscDrawAxis)

    Input Parameters:
+   axis - the axis
-   hold - PETSC_TRUE - hold current limits, PETSC_FALSE allow limits to be changed

    Level: advanced

    Notes:
        Once this has been called with PETSC_TRUE the limits will not change if you call
     PetscDrawAxisSetLimits() until you call this with PETSC_FALSE

.seealso:  PetscDrawAxisSetLimits()

@*/
PetscErrorCode  PetscDrawAxisSetHoldLimits(PetscDrawAxis axis,PetscBool hold)
{
  PetscFunctionBegin;
  if (!axis) PetscFunctionReturn(0);
  PetscValidHeaderSpecific(axis,PETSC_DRAWAXIS_CLASSID,1);
  axis->hold = hold;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDrawAxisDraw"
/*@
    PetscDrawAxisDraw - PetscDraws an axis.

    Not Collective (ignored on all processors except processor 0 of PetscDrawAxis)

    Input Parameter:
.   axis - Axis structure

    Level: advanced

    Note:
    This draws the actual axis.  The limits etc have already been set.
    By picking special routines for the ticks and labels, special
    effects may be generated.  These routines are part of the Axis
    structure (axis).
@*/
PetscErrorCode  PetscDrawAxisDraw(PetscDrawAxis axis)
{
  int            i,ntick,numx,numy,ac,tc,cc;
  PetscMPIInt    rank;
  size_t         len;
  PetscReal      tickloc[MAXSEGS],sep,h,w,tw,th,xl,xr,yl,yr;
  char           *p;
  PetscDraw      draw;
  PetscBool      isnull;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (!axis) PetscFunctionReturn(0);
  PetscValidHeaderSpecific(axis,PETSC_DRAWAXIS_CLASSID,1);

  draw = axis->win;
  ierr = PetscDrawIsNull(draw,&isnull);CHKERRQ(ierr);
  if (isnull) PetscFunctionReturn(0);

  ierr = MPI_Comm_rank(PetscObjectComm((PetscObject)axis),&rank);CHKERRQ(ierr);
  if (rank) PetscFunctionReturn(0);

  ac = axis->ac; tc = axis->tc; cc = axis->cc;
  if (axis->xlow == axis->xhigh) {axis->xlow -= .5; axis->xhigh += .5;}
  if (axis->ylow == axis->yhigh) {axis->ylow -= .5; axis->yhigh += .5;}
  xl = axis->xlow; xr = axis->xhigh; yl = axis->ylow; yr = axis->yhigh;

  ierr = PetscDrawSetCoordinates(draw,xl,yl,xr,yr);CHKERRQ(ierr);
  ierr = PetscDrawStringGetSize(draw,&tw,&th);CHKERRQ(ierr);
  numx = (int)(.15*(xr-xl)/tw); if (numx > 6) numx = 6;if (numx< 2) numx = 2;
  numy = (int)(.50*(yr-yl)/th); if (numy > 6) numy = 6;if (numy< 2) numy = 2;
  xl  -= 11*tw; xr += 2*tw; yl -= 2.5*th; yr += 2*th;
  if (axis->xlabel) yl -= 2*th;
  if (axis->ylabel) xl -= 2*tw;
  ierr = PetscDrawSetCoordinates(draw,xl,yl,xr,yr);CHKERRQ(ierr);
  ierr = PetscDrawStringGetSize(draw,&tw,&th);CHKERRQ(ierr);

  ierr = PetscDrawLine(draw,axis->xlow,axis->ylow,axis->xhigh,axis->ylow,ac);CHKERRQ(ierr);
  ierr = PetscDrawLine(draw,axis->xlow,axis->ylow,axis->xlow,axis->yhigh,ac);CHKERRQ(ierr);

  if (axis->toplabel) {
    h    = axis->yhigh;
    ierr = PetscDrawStringCentered(draw,.5*(xl+xr),axis->yhigh,cc,axis->toplabel);CHKERRQ(ierr);
  }

  /* PetscDraw the ticks and labels */
  if (axis->xticks) {
    ierr = (*axis->xticks)(axis->xlow,axis->xhigh,numx,&ntick,tickloc,MAXSEGS);CHKERRQ(ierr);
    /* PetscDraw in tick marks */
    for (i=0; i<ntick; i++) {
      ierr = PetscDrawLine(draw,tickloc[i],axis->ylow-.5*th,tickloc[i],axis->ylow+.5*th,tc);CHKERRQ(ierr);
    }
    /* label ticks */
    for (i=0; i<ntick; i++) {
      if (axis->xlabelstr) {
        if (i < ntick - 1) sep = tickloc[i+1] - tickloc[i];
        else if (i > 0)    sep = tickloc[i]   - tickloc[i-1];
        else               sep = 0.0;
        ierr = (*axis->xlabelstr)(tickloc[i],sep,&p);CHKERRQ(ierr);
        ierr = PetscDrawStringCentered(draw,tickloc[i],axis->ylow-1.2*th,cc,p);CHKERRQ(ierr);
      }
    }
  }
  if (axis->xlabel) {
    h    = axis->ylow - 2.5*th;
    ierr = PetscDrawStringCentered(draw,.5*(xl + xr),h,cc,axis->xlabel);CHKERRQ(ierr);
  }
  if (axis->yticks) {
    ierr = (*axis->yticks)(axis->ylow,axis->yhigh,numy,&ntick,tickloc,MAXSEGS);CHKERRQ(ierr);
    /* PetscDraw in tick marks */
    for (i=0; i<ntick; i++) {
      ierr = PetscDrawLine(draw,axis->xlow -.5*tw,tickloc[i],axis->xlow+.5*tw,tickloc[i],tc);CHKERRQ(ierr);
    }
    /* label ticks */
    for (i=0; i<ntick; i++) {
      if (axis->ylabelstr) {
        if (i < ntick - 1) sep = tickloc[i+1] - tickloc[i];
        else if (i > 0)    sep = tickloc[i]   - tickloc[i-1];
        else               sep = 0.0;
        ierr = (*axis->xlabelstr)(tickloc[i],sep,&p);CHKERRQ(ierr);
        ierr = PetscStrlen(p,&len);CHKERRQ(ierr);
        w    = axis->xlow - len * tw - 1.2*tw;
        ierr = PetscDrawString(draw,w,tickloc[i]-.5*th,cc,p);CHKERRQ(ierr);
      }
    }
  }
  if (axis->ylabel) {
    ierr = PetscStrlen(axis->ylabel,&len);CHKERRQ(ierr);
    h    = yl + .5*(yr - yl) + .5*len*th;
    w    = xl + 1.5*tw;
    ierr = PetscDrawStringVertical(draw,w,h,cc,axis->ylabel);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscStripe0"
/*
    Removes all zeros but one from .0000
*/
PetscErrorCode PetscStripe0(char *buf)
{
  PetscErrorCode ierr;
  size_t         n;
  PetscBool      flg;
  char           *str;

  PetscFunctionBegin;
  ierr = PetscStrlen(buf,&n);CHKERRQ(ierr);
  ierr = PetscStrendswith(buf,"e00",&flg);CHKERRQ(ierr);
  if (flg) buf[n-3] = 0;
  ierr = PetscStrstr(buf,"e0",&str);CHKERRQ(ierr);
  if (str) {
    buf[n-2] = buf[n-1];
    buf[n-1] = 0;
  }
  ierr = PetscStrstr(buf,"e-0",&str);CHKERRQ(ierr);
  if (str) {
    buf[n-2] = buf[n-1];
    buf[n-1] = 0;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscStripAllZeros"
/*
    Removes all zeros but one from .0000
*/
PetscErrorCode PetscStripAllZeros(char *buf)
{
  PetscErrorCode ierr;
  size_t         i,n;

  PetscFunctionBegin;
  ierr = PetscStrlen(buf,&n);CHKERRQ(ierr);
  if (buf[0] != '.') PetscFunctionReturn(0);
  for (i=1; i<n; i++) {
    if (buf[i] != '0') PetscFunctionReturn(0);
  }
  buf[0] = '0';
  buf[1] = 0;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscStripTrailingZeros"
/*
    Removes trailing zeros
*/
PetscErrorCode PetscStripTrailingZeros(char *buf)
{
  PetscErrorCode ierr;
  char           *found;
  size_t         i,n,m = PETSC_MAX_INT;

  PetscFunctionBegin;
  /* if there is an e in string DO NOT strip trailing zeros */
  ierr = PetscStrchr(buf,'e',&found);CHKERRQ(ierr);
  if (found) PetscFunctionReturn(0);

  ierr = PetscStrlen(buf,&n);CHKERRQ(ierr);
  /* locate decimal point */
  for (i=0; i<n; i++) {
    if (buf[i] == '.') {m = i; break;}
  }
  /* if not decimal point then no zeros to remove */
  if (m == PETSC_MAX_INT) PetscFunctionReturn(0);
  /* start at right end of string removing 0s */
  for (i=n-1; i>m; i++) {
    if (buf[i] != '0') PetscFunctionReturn(0);
    buf[i] = 0;
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscStripInitialZero"
/*
    Removes leading 0 from 0.22 or -0.22
*/
PetscErrorCode PetscStripInitialZero(char *buf)
{
  PetscErrorCode ierr;
  size_t         i,n;

  PetscFunctionBegin;
  ierr = PetscStrlen(buf,&n);CHKERRQ(ierr);
  if (buf[0] == '0') {
    for (i=0; i<n; i++) buf[i] = buf[i+1];
  } else if (buf[0] == '-' && buf[1] == '0') {
    for (i=1; i<n; i++) buf[i] = buf[i+1];
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscStripZeros"
/*
     Removes the extraneous zeros in numbers like 1.10000e6
*/
PetscErrorCode PetscStripZeros(char *buf)
{
  PetscErrorCode ierr;
  size_t         i,j,n;

  PetscFunctionBegin;
  ierr = PetscStrlen(buf,&n);CHKERRQ(ierr);
  if (n<5) PetscFunctionReturn(0);
  for (i=1; i<n-1; i++) {
    if (buf[i] == 'e' && buf[i-1] == '0') {
      for (j=i; j<n+1; j++) buf[j-1] = buf[j];
      ierr = PetscStripZeros(buf);CHKERRQ(ierr);
      PetscFunctionReturn(0);
    }
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscStripZerosPlus"
/*
      Removes the plus in something like 1.1e+2 or 1.1e+02
*/
PetscErrorCode PetscStripZerosPlus(char *buf)
{
  PetscErrorCode ierr;
  size_t         i,j,n;

  PetscFunctionBegin;
  ierr = PetscStrlen(buf,&n);CHKERRQ(ierr);
  if (n<5) PetscFunctionReturn(0);
  for (i=1; i<n-2; i++) {
    if (buf[i] == '+') {
      if (buf[i+1] == '0') {
        for (j=i+1; j<n; j++) buf[j-1] = buf[j+1];
        PetscFunctionReturn(0);
      } else {
        for (j=i+1; j<n+1; j++) buf[j-1] = buf[j];
        PetscFunctionReturn(0);
      }
    } else if (buf[i] == '-') {
      if (buf[i+1] == '0') {
        for (j=i+1; j<n; j++) buf[j] = buf[j+1];
        PetscFunctionReturn(0);
      }
    }
  }
  PetscFunctionReturn(0);
}
