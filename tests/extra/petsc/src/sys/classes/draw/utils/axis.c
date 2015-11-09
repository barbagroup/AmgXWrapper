
#include <../src/sys/classes/draw/utils/axisimpl.h>  /*I   "petscdraw.h"  I*/

#undef __FUNCT__
#define __FUNCT__ "PetscDrawAxisSetLimits"
/*@
    PetscDrawAxisSetLimits -  Sets the limits (in user coords) of the axis

    Not Collective (ignored on all processors except processor 0 of PetscDrawAxis)

    Input Parameters:
+   axis - the axis
.   xmin,xmax - limits in x
-   ymin,ymax - limits in y

    Options Database:
.   -drawaxis_hold - hold the initial set of axis limits for future plotting

    Level: advanced

.seealso:  PetscDrawAxisSetHoldLimits()

@*/
PetscErrorCode  PetscDrawAxisSetLimits(PetscDrawAxis axis,PetscReal xmin,PetscReal xmax,PetscReal ymin,PetscReal ymax)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (!axis) PetscFunctionReturn(0);
  PetscValidHeaderSpecific(axis,PETSC_DRAWAXIS_CLASSID,1);
  if (axis->hold) PetscFunctionReturn(0);
  axis->xlow = xmin;
  axis->xhigh= xmax;
  axis->ylow = ymin;
  axis->yhigh= ymax;
  ierr = PetscOptionsHasName(((PetscObject)axis)->options,((PetscObject)axis)->prefix,"-drawaxis_hold",&axis->hold);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDrawAxisGetLimits"
/*@
    PetscDrawAxisGetLimits -  Gets the limits (in user coords) of the axis

    Not Collective (ignored on all processors except processor 0 of PetscDrawAxis)

    Input Parameters:
+   axis - the axis
.   xmin,xmax - limits in x
-   ymin,ymax - limits in y

    Level: advanced

.seealso:  PetscDrawAxisSetLimits()

@*/
PetscErrorCode  PetscDrawAxisGetLimits(PetscDrawAxis axis,PetscReal *xmin,PetscReal *xmax,PetscReal *ymin,PetscReal *ymax)
{
  PetscFunctionBegin;
  if (!axis) PetscFunctionReturn(0);
  PetscValidHeaderSpecific(axis,PETSC_DRAWAXIS_CLASSID,1);
  *xmin = axis->xlow;
  *xmax = axis->xhigh;
  *ymin = axis->ylow;
  *ymax = axis->yhigh;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscADefLabel"
/*
   val is the label value.  sep is the separation to the next (or previous)
   label; this is useful in determining how many significant figures to
   keep.
 */
PetscErrorCode PetscADefLabel(PetscReal val,PetscReal sep,char **p)
{
  static char    buf[40];
  PetscErrorCode ierr;

  PetscFunctionBegin;
  /* Find the string */
  if (PetscAbsReal(val)/sep <  1.e-4) {
    buf[0] = '0'; buf[1] = 0;
  } else {
    sprintf(buf,"%0.1e",(double)val);
    ierr = PetscStripZerosPlus(buf);CHKERRQ(ierr);
    ierr = PetscStripe0(buf);CHKERRQ(ierr);
    ierr = PetscStripInitialZero(buf);CHKERRQ(ierr);
    ierr = PetscStripAllZeros(buf);CHKERRQ(ierr);
    ierr = PetscStripTrailingZeros(buf);CHKERRQ(ierr);
  }
  *p =buf;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscADefTicks"
/* Finds "nice" locations for the ticks */
PetscErrorCode PetscADefTicks(PetscReal low,PetscReal high,int num,int *ntick,PetscReal * tickloc,int maxtick)
{
  PetscErrorCode ierr;
  int            i,power;
  PetscReal      x = 0.0,base=0.0;

  PetscFunctionBegin;
  ierr = PetscAGetBase(low,high,num,&base,&power);CHKERRQ(ierr);
  ierr = PetscAGetNice(low,base,-1,&x);CHKERRQ(ierr);

  /* Values are of the form j * base */
  /* Find the starting value */
  if (x < low) x += base;

  i = 0;
  while (i < maxtick && x <= high) {
    tickloc[i++] = x;
    x           += base;
  }
  *ntick = i;

  if (i < 2 && num < 10) {
    ierr = PetscADefTicks(low,high,num+1,ntick,tickloc,maxtick);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#define EPS 1.e-6

#undef __FUNCT__
#define __FUNCT__ "PetscExp10"
PetscErrorCode PetscExp10(PetscReal d,PetscReal *result)
{
  PetscFunctionBegin;
  *result = PetscPowReal((PetscReal)10.0,d);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscMod"
PetscErrorCode PetscMod(PetscReal x,PetscReal y,PetscReal *result)
{
  int i;

  PetscFunctionBegin;
  if (y == 1) {
    *result = 0.0;
    PetscFunctionReturn(0);
  }
  i = ((int)x) / ((int)y);
  x = x - i * y;
  while (x > y) x -= y;
  *result = x;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscCopysign"
PetscErrorCode PetscCopysign(PetscReal a,PetscReal b,PetscReal *result)
{
  PetscFunctionBegin;
  if (b >= 0) *result = a;
  else        *result = -a;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscAGetNice"
/*
    Given a value "in" and a "base", return a nice value.
    based on "sign", extend up (+1) or down (-1)
 */
PetscErrorCode PetscAGetNice(PetscReal in,PetscReal base,int sign,PetscReal *result)
{
  PetscReal      etmp,s,s2,m;
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr    = PetscCopysign (0.5,(double)sign,&s);CHKERRQ(ierr);
  etmp    = in / base + 0.5 + s;
  ierr    = PetscCopysign (0.5,etmp,&s);CHKERRQ(ierr);
  ierr    = PetscCopysign (EPS * etmp,(double)sign,&s2);CHKERRQ(ierr);
  etmp    = etmp - 0.5 + s - s2;
  ierr    = PetscMod(etmp,1.0,&m);CHKERRQ(ierr);
  etmp    = base * (etmp -  m);
  *result = etmp;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscAGetBase"
PetscErrorCode PetscAGetBase(PetscReal vmin,PetscReal vmax,int num,PetscReal *Base,int *power)
{
  PetscReal        base,ftemp,e10;
  static PetscReal base_try[5] = {10.0,5.0,2.0,1.0,0.5};
  PetscErrorCode   ierr;
  int              i;

  PetscFunctionBegin;
  /* labels of the form n * BASE */
  /* get an approximate value for BASE */
  base = (vmax - vmin) / (double)(num + 1);

  /* make it of form   m x 10^power,  m in [1.0, 10) */
  if (base <= 0.0) {
    base = PetscAbsReal(vmin);
    if (base < 1.0) base = 1.0;
  }
  ftemp = PetscLog10Real((1.0 + EPS) * base);
  if (ftemp < 0.0) ftemp -= 1.0;
  *power = (int)ftemp;
  ierr   = PetscExp10((double)-*power,&e10);CHKERRQ(ierr);
  base   = base * e10;
  if (base < 1.0) base = 1.0;
  /* now reduce it to one of 1, 2, or 5 */
  for (i=1; i<5; i++) {
    if (base >= base_try[i]) {
      ierr = PetscExp10((double)*power,&e10);CHKERRQ(ierr);
      base = base_try[i-1] * e10;
      if (i == 1) *power = *power + 1;
      break;
    }
  }
  *Base = base;
  PetscFunctionReturn(0);
}






