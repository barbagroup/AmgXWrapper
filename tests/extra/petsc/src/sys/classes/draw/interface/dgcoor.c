
/*
       Provides the calling sequences for all the basic PetscDraw routines.
*/
#include <petsc/private/drawimpl.h>  /*I "petscdraw.h" I*/

#undef __FUNCT__
#define __FUNCT__ "PetscDrawGetCoordinates"
/*@
   PetscDrawGetCoordinates - Gets the application coordinates of the corners of
   the window (or page).

   Not Collective

   Input Parameter:
.  draw - the drawing object

   Level: advanced

   Ouput Parameters:
.  xl,yl,xr,yr - the coordinates of the lower left corner and upper
                 right corner of the drawing region.

   Concepts: drawing^coordinates
   Concepts: graphics^coordinates

.seealso: PetscDrawSetCoordinates()

@*/
PetscErrorCode  PetscDrawGetCoordinates(PetscDraw draw,PetscReal *xl,PetscReal *yl,PetscReal *xr,PetscReal *yr)
{
  PetscFunctionBegin;
  PetscValidHeaderSpecific(draw,PETSC_DRAW_CLASSID,1);
  PetscValidRealPointer(xl,2);
  PetscValidRealPointer(yl,3);
  PetscValidRealPointer(xr,4);
  PetscValidRealPointer(yr,5);
  *xl = draw->coor_xl; *yl = draw->coor_yl;
  *xr = draw->coor_xr; *yr = draw->coor_yr;
  PetscFunctionReturn(0);
}
