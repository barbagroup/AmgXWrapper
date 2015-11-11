
/*
       Provides the calling sequences for all the basic PetscDraw routines.
*/
#include <petsc/private/drawimpl.h>  /*I "petscdraw.h" I*/

PETSC_EXTERN PetscErrorCode PetscDrawCreate_TikZ(PetscDraw);
#if defined(PETSC_HAVE_X)
PETSC_EXTERN PetscErrorCode PetscDrawCreate_X(PetscDraw);
#endif
#if defined(PETSC_HAVE_GLUT)
PETSC_EXTERN PetscErrorCode PetscDrawCreate_GLUT(PetscDraw);
#endif
#if defined(PETSC_HAVE_OPENGLES)
PETSC_EXTERN PetscErrorCode PetscDrawCreate_OpenGLES(PetscDraw);
#endif
PETSC_EXTERN PetscErrorCode PetscDrawCreate_Null(PetscDraw);
#if defined(PETSC_USE_WINDOWS_GRAPHICS)
PETSC_EXTERN PetscErrorCode PetscDrawCreate_Win32(PetscDraw);
#endif

PetscBool PetscDrawRegisterAllCalled = PETSC_FALSE;

#undef __FUNCT__
#define __FUNCT__ "PetscDrawRegisterAll"
/*@C
  PetscDrawRegisterAll - Registers all of the graphics methods in the PetscDraw package.

  Not Collective

  Level: developer

.seealso:  PetscDrawRegisterDestroy()
@*/
PetscErrorCode  PetscDrawRegisterAll(void)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  if (PetscDrawRegisterAllCalled) PetscFunctionReturn(0);
  PetscDrawRegisterAllCalled = PETSC_TRUE;

  ierr = PetscDrawRegister(PETSC_DRAW_TIKZ,     PetscDrawCreate_TikZ);CHKERRQ(ierr);
#if defined(PETSC_HAVE_OPENGLES)
  ierr = PetscDrawRegister(PETSC_DRAW_OPENGLES, PetscDrawCreate_OpenGLES);CHKERRQ(ierr);
#endif
#if defined(PETSC_HAVE_GLUT)
  ierr = PetscDrawRegister(PETSC_DRAW_GLUT,     PetscDrawCreate_GLUT);CHKERRQ(ierr);
#endif
#if defined(PETSC_HAVE_X)
  ierr = PetscDrawRegister(PETSC_DRAW_X,        PetscDrawCreate_X);CHKERRQ(ierr);
#elif defined(PETSC_USE_WINDOWS_GRAPHICS)
  ierr = PetscDrawRegister(PETSC_DRAW_WIN32,    PetscDrawCreate_Win32);CHKERRQ(ierr);
#endif
  ierr = PetscDrawRegister(PETSC_DRAW_NULL,     PetscDrawCreate_Null);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

