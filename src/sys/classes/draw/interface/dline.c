
/*
       Provides the calling sequences for all the basic PetscDraw routines.
*/
#include <petsc/private/drawimpl.h>  /*I "petscdraw.h" I*/

#undef __FUNCT__
#define __FUNCT__ "PetscDrawGetBoundingBox"
/*@
   PetscDrawGetBoundingBox - Gets the bounding box of all PetscDrawStringBoxed() commands

   Not collective

   Input Parameter:
.  draw - the drawing context

   Output Parameters:
.   xl,yl,xr,yr - coordinates of lower left and upper right corners of bounding box

   Level: intermediate

.seealso:  PetscDrawPushCurrentPoint(), PetscDrawPopCurrentPoint(), PetscDrawSetCurrentPoint()
@*/
PetscErrorCode  PetscDrawGetBoundingBox(PetscDraw draw,PetscReal *xl,PetscReal *yl,PetscReal *xr,PetscReal *yr)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(draw,PETSC_DRAW_CLASSID,1);
  if (xl) *xl = draw->boundbox_xl;
  if (yl) *yl = draw->boundbox_yl;
  if (xr) *xr = draw->boundbox_xr;
  if (yr) *yr = draw->boundbox_yr;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDrawGetCurrentPoint"
/*@
   PetscDrawGetCurrentPoint - Gets the current draw point, some codes use this point to determine where to draw next

   Not collective

   Input Parameter:
.  draw - the drawing context

   Output Parameters:
.   x,y - the current point

   Level: intermediate

.seealso:  PetscDrawPushCurrentPoint(), PetscDrawPopCurrentPoint(), PetscDrawSetCurrentPoint()
@*/
PetscErrorCode  PetscDrawGetCurrentPoint(PetscDraw draw,PetscReal *x,PetscReal *y)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(draw,PETSC_DRAW_CLASSID,1);
  *x = draw->currentpoint_x[draw->currentpoint];
  *y = draw->currentpoint_y[draw->currentpoint];
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDrawSetCurrentPoint"
/*@
   PetscDrawSetCurrentPoint - Sets the current draw point, some codes use this point to determine where to draw next

   Not collective

   Input Parameters:
+  draw - the drawing context
-  x,y - the location of the current point

   Level: intermediate

.seealso:  PetscDrawPushCurrentPoint(), PetscDrawPopCurrentPoint(), PetscDrawGetCurrentPoint()
@*/
PetscErrorCode  PetscDrawSetCurrentPoint(PetscDraw draw,PetscReal x,PetscReal y)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(draw,PETSC_DRAW_CLASSID,1);
  draw->currentpoint_x[draw->currentpoint] = x;
  draw->currentpoint_y[draw->currentpoint] = y;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDrawPushCurrentPoint"
/*@
   PetscDrawPushCurrentPoint - Pushes a new current draw point, retaining the old one, some codes use this point to determine where to draw next

   Not collective

   Input Parameters:
+  draw - the drawing context
-  x,y - the location of the current point

   Level: intermediate

.seealso:  PetscDrawPushCurrentPoint(), PetscDrawPopCurrentPoint(), PetscDrawGetCurrentPoint()
@*/
PetscErrorCode  PetscDrawPushCurrentPoint(PetscDraw draw,PetscReal x,PetscReal y)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(draw,PETSC_DRAW_CLASSID,1);
  if (draw->currentpoint > 19) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SUP,"You have pushed too many current points");
  draw->currentpoint_x[++draw->currentpoint] = x;
  draw->currentpoint_y[draw->currentpoint]   = y;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDrawPopCurrentPoint"
/*@
   PetscDrawPopCurrentPoint - Pops a current draw point (discarding it)

   Not collective

   Input Parameter:
.  draw - the drawing context

   Level: intermediate

.seealso:  PetscDrawPushCurrentPoint(), PetscDrawSetCurrentPoint(), PetscDrawGetCurrentPoint()
@*/
PetscErrorCode  PetscDrawPopCurrentPoint(PetscDraw draw)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(draw,PETSC_DRAW_CLASSID,1);
  if (draw->currentpoint-- == 0) SETERRQ(PETSC_COMM_SELF,PETSC_ERR_SUP,"You have popped too many current points");
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDrawLine"
/*@
   PetscDrawLine - PetscDraws a line onto a drawable.

   Not collective

   Input Parameters:
+  draw - the drawing context
.  xl,yl,xr,yr - the coordinates of the line endpoints
-  cl - the colors of the endpoints

   Level: beginner

   Concepts: line^drawing
   Concepts: drawing^line

@*/
PetscErrorCode  PetscDrawLine(PetscDraw draw,PetscReal xl,PetscReal yl,PetscReal xr,PetscReal yr,int cl)
{
  PetscErrorCode ierr;
  PetscBool      isdrawnull;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(draw,PETSC_DRAW_CLASSID,1);
  ierr = PetscObjectTypeCompare((PetscObject)draw,PETSC_DRAW_NULL,&isdrawnull);CHKERRQ(ierr);
  if (isdrawnull) PetscFunctionReturn(0);
  if (!draw->ops->line) SETERRQ(PetscObjectComm((PetscObject)draw),PETSC_ERR_SUP,"No support for drawing lines");
  ierr = (*draw->ops->line)(draw,xl,yl,xr,yr,cl);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PetscDrawArrow"
/*@
   PetscDrawArrow - PetscDraws a line with arrow head at end if the line is long enough

   Not collective

   Input Parameters:
+  draw - the drawing context
.  xl,yl,xr,yr - the coordinates of the line endpoints
-  cl - the colors of the endpoints

   Level: beginner

   Concepts: line^drawing
   Concepts: drawing^line

@*/
PetscErrorCode  PetscDrawArrow(PetscDraw draw,PetscReal xl,PetscReal yl,PetscReal xr,PetscReal yr,int cl)
{
  PetscErrorCode ierr;
  PetscBool      isdrawnull;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(draw,PETSC_DRAW_CLASSID,1);
  ierr = PetscObjectTypeCompare((PetscObject)draw,PETSC_DRAW_NULL,&isdrawnull);CHKERRQ(ierr);
  if (isdrawnull) PetscFunctionReturn(0);
  if (!draw->ops->arrow) SETERRQ(PetscObjectComm((PetscObject)draw),PETSC_ERR_SUP,"No support for drawing arrows");
  ierr = (*draw->ops->arrow)(draw,xl,yl,xr,yr,cl);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

