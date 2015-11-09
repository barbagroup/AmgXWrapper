
/*
       Provides the calling sequences for all the basic PetscDraw routines.
*/
#include <petsc/private/drawimpl.h>  /*I "petscdraw.h" I*/

#undef __FUNCT__
#define __FUNCT__ "PetscDrawStringVertical"
/*@C
   PetscDrawStringVertical - PetscDraws text onto a drawable.

   Not Collective

   Input Parameters:
+  draw - the drawing context
.  xl,yl - the coordinates of upper left corner of text
.  cl - the color of the text
-  text - the text to draw

   Level: beginner

   Concepts: string^drawing vertical

.seealso: PetscDrawString()

@*/
PetscErrorCode  PetscDrawStringVertical(PetscDraw draw,PetscReal xl,PetscReal yl,int cl,const char text[])
{
  PetscErrorCode ierr;
  PetscBool      isnull;

  PetscFunctionBegin;
  PetscValidHeaderSpecific(draw,PETSC_DRAW_CLASSID,1);
  PetscValidCharPointer(text,5);
  ierr = PetscObjectTypeCompare((PetscObject)draw,PETSC_DRAW_NULL,&isnull);CHKERRQ(ierr);
  if (isnull) PetscFunctionReturn(0);
  if (!draw->ops->stringvertical) SETERRQ(PetscObjectComm((PetscObject)draw),PETSC_ERR_SUP,"No support for vertical strings");
  ierr = (*draw->ops->stringvertical)(draw,xl,yl,cl,text);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

