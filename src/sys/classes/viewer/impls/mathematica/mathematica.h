/*
   This is the definition of the Mathematica viewer structure.
*/

#include <petsc/private/viewerimpl.h>   /*I  "petscsys.h"  I*/
#if defined(PETSC_HAVE_MATHEMATICA)
#include <mathlink.h>
#endif

typedef enum {MATHEMATICA_LINK_CREATE, MATHEMATICA_LINK_CONNECT, MATHEMATICA_LINK_LAUNCH} LinkMode;

typedef struct {
#if defined(PETSC_HAVE_MATHEMATICA)
  MLINK        link;         /* The link to Mathematica */
#endif
  char        *linkname;     /* The name to link to Mathematica on (usually a port) */
  char        *linkhost;     /* The host to link to Mathematica on */
  LinkMode     linkmode;     /* The link mode */
  const char  *objName;      /* The name for the next object passed to Mathematica */
} PetscViewer_Mathematica;

extern PetscErrorCode PetscViewerMathematicaSetFromOptions(PetscViewer);
extern PetscErrorCode PetscViewerMathematicaSetLinkName(PetscViewer,const char*);
extern PetscErrorCode PetscViewerMathematicaSetLinkPort(PetscViewer,int);
extern PetscErrorCode PetscViewerMathematicaSetLinkHost(PetscViewer,const char*);
extern PetscErrorCode PetscViewerMathematicaSetLinkMode(PetscViewer,LinkMode);
