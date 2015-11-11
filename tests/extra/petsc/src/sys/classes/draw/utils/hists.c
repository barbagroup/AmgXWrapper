
/*
  Contains the data structure for plotting a histogram in a window with an axis.
*/
#include <petscdraw.h>         /*I "petscdraw.h" I*/
#include <petsc/private/petscimpl.h>         /*I "petscsys.h" I*/
#include <petscviewer.h>         /*I "petscviewer.h" I*/

PetscClassId PETSC_DRAWHG_CLASSID = 0;

struct _p_PetscDrawHG {
  PETSCHEADER(int);
  PetscErrorCode (*destroy)(PetscDrawSP);
  PetscErrorCode (*view)(PetscDrawSP,PetscViewer);
  PetscDraw      win;
  PetscDrawAxis  axis;
  PetscReal      xmin,xmax;
  PetscReal      ymin,ymax;
  int            numBins;
  int            maxBins;
  PetscReal      *bins;
  int            numValues;
  int            maxValues;
  PetscReal      *values;
  int            color;
  PetscBool      calcStats;
  PetscBool      integerBins;
};

#define CHUNKSIZE 100

#undef __FUNCT__
#define __FUNCT__ "PetscDrawHGCreate"
/*@C
   PetscDrawHGCreate - Creates a histogram data structure.

   Collective over PetscDraw

   Input Parameters:
+  draw  - The window where the graph will be made
-  bins - The number of bins to use

   Output Parameters:
.  hist - The histogram context

   Level: intermediate

   Concepts: histogram^creating

.seealso: PetscDrawHGDestroy()

@*/
PetscErrorCode  PetscDrawHGCreate(PetscDraw draw, int bins, PetscDrawHG *hist)
{
  PetscBool      isnull;
  PetscDrawHG    h;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(draw, PETSC_DRAW_CLASSID,1);
  PetscValidLogicalCollectiveInt(draw,bins,2);
  PetscValidPointer(hist,3);

  ierr = PetscDrawIsNull(draw,&isnull);CHKERRQ(ierr);
  if (isnull) {*hist = NULL; PetscFunctionReturn(0);}

  ierr = PetscHeaderCreate(h, PETSC_DRAWHG_CLASSID,  "PetscDrawHG", "Histogram", "Draw", PetscObjectComm((PetscObject)draw), PetscDrawHGDestroy, NULL);CHKERRQ(ierr);
  ierr = PetscLogObjectParent((PetscObject)draw,(PetscObject)h);CHKERRQ(ierr);

  ierr = PetscObjectReference((PetscObject)draw);CHKERRQ(ierr);
  h->win = draw;

  h->view        = NULL;
  h->destroy     = NULL;
  h->color       = PETSC_DRAW_GREEN;
  h->xmin        = PETSC_MAX_REAL;
  h->xmax        = PETSC_MIN_REAL;
  h->ymin        = 0.;
  h->ymax        = 1.;
  h->numBins     = bins;
  h->maxBins     = bins;

  ierr = PetscMalloc1(h->maxBins,&h->bins);CHKERRQ(ierr);

  h->numValues   = 0;
  h->maxValues   = CHUNKSIZE;
  h->calcStats   = PETSC_FALSE;
  h->integerBins = PETSC_FALSE;

  ierr = PetscMalloc1(h->maxValues,&h->values);CHKERRQ(ierr);
  ierr = PetscLogObjectMemory((PetscObject)h,(h->maxBins + h->maxValues)*sizeof(PetscReal));CHKERRQ(ierr);

  ierr = PetscDrawAxisCreate(draw,&h->axis);CHKERRQ(ierr);
  ierr = PetscLogObjectParent((PetscObject)h,(PetscObject)h->axis);CHKERRQ(ierr);

  *hist = h;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDrawHGSetNumberBins"
/*@
   PetscDrawHGSetNumberBins - Change the number of bins that are to be drawn.

   Not Collective (ignored except on processor 0 of PetscDrawHG)

   Input Parameter:
+  hist - The histogram context.
-  bins  - The number of bins.

   Level: intermediate

   Concepts: histogram^setting number of bins

@*/
PetscErrorCode  PetscDrawHGSetNumberBins(PetscDrawHG hist, int bins)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (!hist) PetscFunctionReturn(0);
  PetscValidHeaderSpecific(hist,PETSC_DRAWHG_CLASSID,1);
  PetscValidLogicalCollectiveInt(hist,bins,2);

  if (hist->maxBins < bins) {
    ierr = PetscFree(hist->bins);CHKERRQ(ierr);
    ierr = PetscMalloc1(bins, &hist->bins);CHKERRQ(ierr);
    ierr = PetscLogObjectMemory((PetscObject)hist, (bins - hist->maxBins) * sizeof(PetscReal));CHKERRQ(ierr);
    hist->maxBins = bins;
  }
  hist->numBins = bins;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDrawHGReset"
/*@
  PetscDrawHGReset - Clears histogram to allow for reuse with new data.

  Not Collective (ignored except on processor 0 of PetscDrawHG)

  Input Parameter:
. hist - The histogram context.

  Level: intermediate

  Concepts: histogram^resetting
@*/
PetscErrorCode  PetscDrawHGReset(PetscDrawHG hist)
{
  PetscFunctionBegin;
  if (!hist) PetscFunctionReturn(0);
  PetscValidHeaderSpecific(hist,PETSC_DRAWHG_CLASSID,1);

  hist->xmin      = PETSC_MAX_REAL;
  hist->xmax      = PETSC_MIN_REAL;
  hist->ymin      = 0.0;
  hist->ymax      = 0.0;
  hist->numValues = 0;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDrawHGDestroy"
/*@C
  PetscDrawHGDestroy - Frees all space taken up by histogram data structure.

  Collective over PetscDrawHG

  Input Parameter:
. hist - The histogram context

  Level: intermediate

.seealso:  PetscDrawHGCreate()
@*/
PetscErrorCode  PetscDrawHGDestroy(PetscDrawHG *hist)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (!*hist) PetscFunctionReturn(0);
  PetscValidHeaderSpecific(*hist,PETSC_DRAWHG_CLASSID,1);
  if (--((PetscObject)(*hist))->refct > 0) {*hist = NULL; PetscFunctionReturn(0);}

  ierr = PetscFree((*hist)->bins);CHKERRQ(ierr);
  ierr = PetscFree((*hist)->values);CHKERRQ(ierr);
  ierr = PetscDrawAxisDestroy(&(*hist)->axis);CHKERRQ(ierr);
  ierr = PetscDrawDestroy(&(*hist)->win);CHKERRQ(ierr);
  ierr = PetscHeaderDestroy(hist);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDrawHGAddValue"
/*@
  PetscDrawHGAddValue - Adds another value to the histogram.

  Not Collective (ignored except on processor 0 of PetscDrawHG)

  Input Parameters:
+ hist  - The histogram
- value - The value

  Level: intermediate

  Concepts: histogram^adding values

.seealso: PetscDrawHGAddValues()
@*/
PetscErrorCode  PetscDrawHGAddValue(PetscDrawHG hist, PetscReal value)
{
  PetscFunctionBegin;
  if (!hist) PetscFunctionReturn(0);
  PetscValidHeaderSpecific(hist,PETSC_DRAWHG_CLASSID,1);

  /* Allocate more memory if necessary */
  if (hist->numValues >= hist->maxValues) {
    PetscReal      *tmp;
    PetscErrorCode ierr;

    ierr = PetscMalloc1(hist->maxValues+CHUNKSIZE, &tmp);CHKERRQ(ierr);
    ierr = PetscLogObjectMemory((PetscObject)hist, CHUNKSIZE * sizeof(PetscReal));CHKERRQ(ierr);
    ierr = PetscMemcpy(tmp, hist->values, hist->maxValues * sizeof(PetscReal));CHKERRQ(ierr);
    ierr = PetscFree(hist->values);CHKERRQ(ierr);

    hist->values     = tmp;
    hist->maxValues += CHUNKSIZE;
  }
  /* I disagree with the original Petsc implementation here. There should be no overshoot, but rather the
     stated convention of using half-open intervals (always the way to go) */
  if (!hist->numValues) {
    hist->xmin = value;
    hist->xmax = value;
#if 1
  } else {
    /* Update limits */
    if (value > hist->xmax) hist->xmax = value;
    if (value < hist->xmin) hist->xmin = value;
#else
  } else if (hist->numValues == 1) {
    /* Update limits -- We need to overshoot the largest value somewhat */
    if (value > hist->xmax) hist->xmax = value + 0.001*(value - hist->xmin)/hist->numBins;
    if (value < hist->xmin) {
      hist->xmin = value;
      hist->xmax = hist->xmax + 0.001*(hist->xmax - hist->xmin)/hist->numBins;
    }
  } else {
    /* Update limits -- We need to overshoot the largest value somewhat */
    if (value > hist->xmax) hist->xmax = value + 0.001*(hist->xmax - hist->xmin)/hist->numBins;
    if (value < hist->xmin) hist->xmin = value;
#endif
  }

  hist->values[hist->numValues++] = value;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDrawHGDraw"
/*@
  PetscDrawHGDraw - Redraws a histogram.

  Collective, but ignored by all processors except processor 0 in PetscDrawHG

  Input Parameter:
. hist - The histogram context

  Level: intermediate

@*/
PetscErrorCode  PetscDrawHGDraw(PetscDrawHG hist)
{
  PetscDraw      draw;
  PetscBool      isnull;
  PetscReal      xmin,xmax,ymin,ymax,*bins,*values,binSize,binLeft,binRight,maxHeight,mean,var;
  char           title[256];
  char           xlabel[256];
  PetscInt       numBins,numBinsOld,numValues,initSize,i,p,bcolor,color;
  PetscMPIInt    rank;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (!hist) PetscFunctionReturn(0);
  PetscValidHeaderSpecific(hist,PETSC_DRAWHG_CLASSID,1);

  draw = hist->win;
  ierr = PetscDrawIsNull(draw,&isnull);CHKERRQ(ierr);
  if (isnull) PetscFunctionReturn(0);
  if ((hist->xmin >= hist->xmax) || (hist->ymin >= hist->ymax)) PetscFunctionReturn(0);
  if (hist->numValues < 1) PetscFunctionReturn(0);
  ierr = MPI_Comm_rank(PetscObjectComm((PetscObject)hist),&rank);CHKERRQ(ierr);

  color = hist->color;
  if (color == PETSC_DRAW_ROTATE) bcolor = 2;
  else bcolor = color;

  xmin      = hist->xmin;
  xmax      = hist->xmax;
  ymin      = hist->ymin;
  ymax      = hist->ymax;
  numValues = hist->numValues;
  values    = hist->values;
  mean      = 0.0;
  var       = 0.0;

  ierr = PetscDrawCheckResizedWindow(draw);CHKERRQ(ierr);
  ierr = PetscDrawSynchronizedClear(draw);CHKERRQ(ierr);
  ierr = PetscDrawCollectiveBegin(draw);CHKERRQ(ierr);

  if (xmin == xmax) {
    /* Calculate number of points in each bin */
    bins    = hist->bins;
    bins[0] = 0.;
    for (p = 0; p < numValues; p++) {
      if (values[p] == xmin) bins[0]++;
      mean += values[p];
      var  += values[p]*values[p];
    }
    maxHeight = bins[0];
    if (maxHeight > ymax) ymax = hist->ymax = maxHeight;
    xmax = xmin + 1;
    ierr = PetscDrawAxisSetLimits(hist->axis, xmin, xmax, ymin, ymax);CHKERRQ(ierr);
    if (hist->calcStats) {
      mean /= numValues;
      if (numValues > 1) var = (var - numValues*mean*mean) / (numValues-1);
      else var = 0.0;
      ierr = PetscSNPrintf(title, 256, "Mean: %g  Var: %g", (double)mean, (double)var);CHKERRQ(ierr);
      ierr = PetscSNPrintf(xlabel,256, "Total: %D", numValues);CHKERRQ(ierr);
      ierr = PetscDrawAxisSetLabels(hist->axis, title, xlabel, NULL);CHKERRQ(ierr);
    }
    ierr = PetscDrawAxisDraw(hist->axis);CHKERRQ(ierr);
    if (!rank) { /* Draw bins */
      binLeft  = xmin;
      binRight = xmax;
      ierr = PetscDrawRectangle(draw,binLeft,ymin,binRight,bins[0],bcolor,bcolor,bcolor,bcolor);CHKERRQ(ierr);
      ierr = PetscDrawLine(draw,binLeft,ymin,binLeft,bins[0],PETSC_DRAW_BLACK);CHKERRQ(ierr);
      ierr = PetscDrawLine(draw,binRight,ymin,binRight,bins[0],PETSC_DRAW_BLACK);CHKERRQ(ierr);
      ierr = PetscDrawLine(draw,binLeft,bins[0],binRight,bins[0],PETSC_DRAW_BLACK);CHKERRQ(ierr);
      if (color == PETSC_DRAW_ROTATE && bins[0] != 0.0) bcolor++;
      if (bcolor > PETSC_DRAW_BASIC_COLORS-1) bcolor = 2;
    }
  } else {
    numBins    = hist->numBins;
    numBinsOld = hist->numBins;
    if (hist->integerBins && (((int) xmax - xmin) + 1.0e-05 > xmax - xmin)) {
      initSize = (int) ((int) xmax - xmin)/numBins;
      while (initSize*numBins != (int) xmax - xmin) {
        initSize = PetscMax(initSize - 1, 1);
        numBins  = (int) ((int) xmax - xmin)/initSize;
        ierr     = PetscDrawHGSetNumberBins(hist, numBins);CHKERRQ(ierr);
      }
    }
    binSize = (xmax - xmin)/numBins;
    bins    = hist->bins;

    ierr = PetscMemzero(bins, numBins * sizeof(PetscReal));CHKERRQ(ierr);

    maxHeight = 0.0;
    for (i = 0; i < numBins; i++) {
      binLeft  = xmin + binSize*i;
      binRight = xmin + binSize*(i+1);
      for (p = 0; p < numValues; p++) {
        if ((values[p] >= binLeft) && (values[p] < binRight)) bins[i]++;
        /* Handle last bin separately */
        if ((i == numBins-1) && (values[p] == binRight)) bins[i]++;
        if (!i) {
          mean += values[p];
          var  += values[p]*values[p];
        }
      }
      maxHeight = PetscMax(maxHeight, bins[i]);
    }
    if (maxHeight > ymax) ymax = hist->ymax = maxHeight;

    ierr = PetscDrawAxisSetLimits(hist->axis, xmin, xmax, ymin, ymax);CHKERRQ(ierr);
    if (hist->calcStats) {
      mean /= numValues;
      if (numValues > 1) var = (var - numValues*mean*mean) / (numValues-1);
      else var = 0.0;

      ierr = PetscSNPrintf(title, 256,"Mean: %g  Var: %g", (double)mean, (double)var);CHKERRQ(ierr);
      ierr = PetscSNPrintf(xlabel,256, "Total: %D", numValues);CHKERRQ(ierr);
      ierr = PetscDrawAxisSetLabels(hist->axis, title, xlabel, NULL);CHKERRQ(ierr);
    }
    ierr = PetscDrawAxisDraw(hist->axis);CHKERRQ(ierr);
    if (!rank) { /* Draw bins */
      for (i = 0; i < numBins; i++) {
        binLeft  = xmin + binSize*i;
        binRight = xmin + binSize*(i+1);
        ierr = PetscDrawRectangle(draw,binLeft,ymin,binRight,bins[i],bcolor,bcolor,bcolor,bcolor);CHKERRQ(ierr);
        ierr = PetscDrawLine(draw,binLeft,ymin,binLeft,bins[i],PETSC_DRAW_BLACK);CHKERRQ(ierr);
        ierr = PetscDrawLine(draw,binRight,ymin,binRight,bins[i],PETSC_DRAW_BLACK);CHKERRQ(ierr);
        ierr = PetscDrawLine(draw,binLeft,bins[i],binRight,bins[i],PETSC_DRAW_BLACK);CHKERRQ(ierr);
        if (color == PETSC_DRAW_ROTATE && bins[i]) bcolor++;
        if (bcolor > PETSC_DRAW_BASIC_COLORS-1) bcolor = 2;
      }
    }
    ierr = PetscDrawHGSetNumberBins(hist, numBinsOld);CHKERRQ(ierr);
  }

  ierr = PetscDrawCollectiveEnd(draw);CHKERRQ(ierr);
  ierr = PetscDrawSynchronizedFlush(draw);CHKERRQ(ierr);
  ierr = PetscDrawPause(draw);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDrawHGView"
/*@
  PetscDrawHGView - Prints the histogram information.

  Not collective

  Input Parameter:
. hist - The histogram context

  Level: beginner

.keywords:  draw, histogram
@*/
PetscErrorCode  PetscDrawHGView(PetscDrawHG hist,PetscViewer viewer)
{
  PetscReal      xmax,xmin,*bins,*values,binSize,binLeft,binRight,mean,var;
  PetscErrorCode ierr;
  PetscInt       numBins,numBinsOld,numValues,initSize,i,p;

  PetscFunctionBegin;
  if (!hist) PetscFunctionReturn(0);
  PetscValidHeaderSpecific(hist,PETSC_DRAWHG_CLASSID,1);

  if ((hist->xmin > hist->xmax) || (hist->ymin >= hist->ymax)) PetscFunctionReturn(0);
  if (hist->numValues < 1) PetscFunctionReturn(0);

  if (!viewer){
    ierr = PetscViewerASCIIGetStdout(PetscObjectComm((PetscObject)hist),&viewer);CHKERRQ(ierr);
  }
  ierr = PetscObjectPrintClassNamePrefixType((PetscObject)hist,viewer);CHKERRQ(ierr);
  xmax      = hist->xmax;
  xmin      = hist->xmin;
  numValues = hist->numValues;
  values    = hist->values;
  mean      = 0.0;
  var       = 0.0;
  if (xmax == xmin) {
    /* Calculate number of points in the bin */
    bins    = hist->bins;
    bins[0] = 0.;
    for (p = 0; p < numValues; p++) {
      if (values[p] == xmin) bins[0]++;
      mean += values[p];
      var  += values[p]*values[p];
    }
    /* Draw bins */
    ierr = PetscViewerASCIIPrintf(viewer, "Bin %2d (%6.2g - %6.2g): %.0g\n", 0, (double)xmin, (double)xmax, (double)bins[0]);CHKERRQ(ierr);
  } else {
    numBins    = hist->numBins;
    numBinsOld = hist->numBins;
    if (hist->integerBins && (((int) xmax - xmin) + 1.0e-05 > xmax - xmin)) {
      initSize = (int) ((int) xmax - xmin)/numBins;
      while (initSize*numBins != (int) xmax - xmin) {
        initSize = PetscMax(initSize - 1, 1);
        numBins  = (int) ((int) xmax - xmin)/initSize;
        ierr     = PetscDrawHGSetNumberBins(hist, numBins);CHKERRQ(ierr);
      }
    }
    binSize = (xmax - xmin)/numBins;
    bins    = hist->bins;

    /* Calculate number of points in each bin */
    ierr = PetscMemzero(bins, numBins * sizeof(PetscReal));CHKERRQ(ierr);
    for (i = 0; i < numBins; i++) {
      binLeft  = xmin + binSize*i;
      binRight = xmin + binSize*(i+1);
      for (p = 0; p < numValues; p++) {
        if ((values[p] >= binLeft) && (values[p] < binRight)) bins[i]++;
        /* Handle last bin separately */
        if ((i == numBins-1) && (values[p] == binRight)) bins[i]++;
        if (!i) {
          mean += values[p];
          var  += values[p]*values[p];
        }
      }
    }
    /* Draw bins */
    for (i = 0; i < numBins; i++) {
      binLeft  = xmin + binSize*i;
      binRight = xmin + binSize*(i+1);
      ierr = PetscViewerASCIIPrintf(viewer, "Bin %2d (%6.2g - %6.2g): %.0g\n", (int)i, (double)binLeft, (double)binRight, (double)bins[i]);CHKERRQ(ierr);
    }
    ierr = PetscDrawHGSetNumberBins(hist, numBinsOld);CHKERRQ(ierr);
  }

  if (hist->calcStats) {
    mean /= numValues;
    if (numValues > 1) var = (var - numValues*mean*mean) / (numValues-1);
    else var = 0.0;
    ierr = PetscViewerASCIIPrintf(viewer, "Mean: %g  Var: %g\n", (double)mean, (double)var);CHKERRQ(ierr);
    ierr = PetscViewerASCIIPrintf(viewer, "Total: %D\n", numValues);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDrawHGSetColor"
/*@
  PetscDrawHGSetColor - Sets the color the bars will be drawn with.

  Not Collective (ignored except on processor 0 of PetscDrawHG)

  Input Parameters:
+ hist - The histogram context
- color - one of the colors defined in petscdraw.h or PETSC_DRAW_ROTATE to make each bar a
          different color

  Level: intermediate

@*/
PetscErrorCode  PetscDrawHGSetColor(PetscDrawHG hist, int color)
{
  PetscFunctionBegin;
  if (!hist) PetscFunctionReturn(0);
  PetscValidHeaderSpecific(hist,PETSC_DRAWHG_CLASSID,1);

  hist->color = color;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDrawHGSetLimits"
/*@
  PetscDrawHGSetLimits - Sets the axis limits for a histogram. If more
  points are added after this call, the limits will be adjusted to
  include those additional points.

  Not Collective (ignored except on processor 0 of PetscDrawHG)

  Input Parameters:
+ hist - The histogram context
- x_min,x_max,y_min,y_max - The limits

  Level: intermediate

  Concepts: histogram^setting axis
@*/
PetscErrorCode  PetscDrawHGSetLimits(PetscDrawHG hist, PetscReal x_min, PetscReal x_max, int y_min, int y_max)
{
  PetscFunctionBegin;
  if (!hist) PetscFunctionReturn(0);
  PetscValidHeaderSpecific(hist,PETSC_DRAWHG_CLASSID,1);

  hist->xmin = x_min;
  hist->xmax = x_max;
  hist->ymin = y_min;
  hist->ymax = y_max;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDrawHGCalcStats"
/*@
  PetscDrawHGCalcStats - Turns on calculation of descriptive statistics

  Not collective

  Input Parameters:
+ hist - The histogram context
- calc - Flag for calculation

  Level: intermediate

.keywords:  draw, histogram, statistics

@*/
PetscErrorCode  PetscDrawHGCalcStats(PetscDrawHG hist, PetscBool calc)
{
  PetscFunctionBegin;
  if (!hist) PetscFunctionReturn(0);
  PetscValidHeaderSpecific(hist,PETSC_DRAWHG_CLASSID,1);

  hist->calcStats = calc;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDrawHGIntegerBins"
/*@
  PetscDrawHGIntegerBins - Turns on integer width bins

  Not collective

  Input Parameters:
+ hist - The histogram context
- ints - Flag for integer width bins

  Level: intermediate

.keywords:  draw, histogram, statistics
@*/
PetscErrorCode  PetscDrawHGIntegerBins(PetscDrawHG hist, PetscBool ints)
{
  PetscFunctionBegin;
  if (!hist) PetscFunctionReturn(0);
  PetscValidHeaderSpecific(hist,PETSC_DRAWHG_CLASSID,1);

  hist->integerBins = ints;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDrawHGGetAxis"
/*@C
  PetscDrawHGGetAxis - Gets the axis context associated with a histogram.
  This is useful if one wants to change some axis property, such as
  labels, color, etc. The axis context should not be destroyed by the
  application code.

  Not Collective (ignored except on processor 0 of PetscDrawHG)

  Input Parameter:
. hist - The histogram context

  Output Parameter:
. axis - The axis context

  Level: intermediate

@*/
PetscErrorCode  PetscDrawHGGetAxis(PetscDrawHG hist, PetscDrawAxis *axis)
{
  PetscFunctionBegin;
  PetscValidPointer(axis,2);
  if (!hist) {*axis = NULL; PetscFunctionReturn(0);}
  PetscValidHeaderSpecific(hist,PETSC_DRAWHG_CLASSID,1);

  *axis = hist->axis;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDrawHGGetDraw"
/*@C
  PetscDrawHGGetDraw - Gets the draw context associated with a histogram.

  Not Collective, PetscDraw is parallel if PetscDrawHG is parallel

  Input Parameter:
. hist - The histogram context

  Output Parameter:
. draw  - The draw context

  Level: intermediate

@*/
PetscErrorCode  PetscDrawHGGetDraw(PetscDrawHG hist, PetscDraw *draw)
{
  PetscFunctionBegin;
  PetscValidPointer(draw,2);
  if (!hist) {*draw = NULL; PetscFunctionReturn(0);}
  PetscValidHeaderSpecific(hist,PETSC_DRAWHG_CLASSID,1);

  *draw = hist->win;
  PetscFunctionReturn(0);
}

