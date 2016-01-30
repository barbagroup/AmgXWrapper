#if !defined(__PETSCDMLABEL_H)
#define __PETSCDMLABEL_H
#include <petscis.h>

/*S
  DMLabel - Object which encapsulates a subset of the mesh from this DM

  Level: developer

  Concepts: grids, grid refinement

.seealso:  DM, DMPlexCreate(), DMPlexCreateLabel()
S*/
typedef struct _n_DMLabel *DMLabel;
PETSC_EXTERN PetscErrorCode DMLabelCreate(const char [], DMLabel *);
PETSC_EXTERN PetscErrorCode DMLabelView(DMLabel, PetscViewer);
PETSC_EXTERN PetscErrorCode DMLabelDestroy(DMLabel *);
PETSC_EXTERN PetscErrorCode DMLabelGetState(DMLabel, PetscObjectState *);
PETSC_EXTERN PetscErrorCode DMLabelDuplicate(DMLabel, DMLabel *);
PETSC_EXTERN PetscErrorCode DMLabelGetName(DMLabel, const char **);
PETSC_EXTERN PetscErrorCode DMLabelGetValue(DMLabel, PetscInt, PetscInt *);
PETSC_EXTERN PetscErrorCode DMLabelSetValue(DMLabel, PetscInt, PetscInt);
PETSC_EXTERN PetscErrorCode DMLabelClearValue(DMLabel, PetscInt, PetscInt);
PETSC_EXTERN PetscErrorCode DMLabelAddStratum(DMLabel, PetscInt);
PETSC_EXTERN PetscErrorCode DMLabelInsertIS(DMLabel, IS, PetscInt);
PETSC_EXTERN PetscErrorCode DMLabelGetNumValues(DMLabel, PetscInt *);
PETSC_EXTERN PetscErrorCode DMLabelGetStratumBounds(DMLabel, PetscInt, PetscInt *, PetscInt *);
PETSC_EXTERN PetscErrorCode DMLabelGetValueIS(DMLabel, IS *);
PETSC_EXTERN PetscErrorCode DMLabelStratumHasPoint(DMLabel, PetscInt, PetscInt, PetscBool *);
PETSC_EXTERN PetscErrorCode DMLabelGetStratumSize(DMLabel, PetscInt, PetscInt *);
PETSC_EXTERN PetscErrorCode DMLabelGetStratumIS(DMLabel, PetscInt, IS *);
PETSC_EXTERN PetscErrorCode DMLabelClearStratum(DMLabel, PetscInt);
PETSC_EXTERN PetscErrorCode DMLabelCreateIndex(DMLabel, PetscInt, PetscInt);
PETSC_EXTERN PetscErrorCode DMLabelDestroyIndex(DMLabel);
PETSC_EXTERN PetscErrorCode DMLabelHasValue(DMLabel, PetscInt, PetscBool *);
PETSC_EXTERN PetscErrorCode DMLabelHasPoint(DMLabel, PetscInt, PetscBool *);
PETSC_EXTERN PetscErrorCode DMLabelFilter(DMLabel, PetscInt, PetscInt);
PETSC_EXTERN PetscErrorCode DMLabelPermute(DMLabel, IS, DMLabel *);
PETSC_EXTERN PetscErrorCode DMLabelDistribute(DMLabel, PetscSF, DMLabel *);
PETSC_EXTERN PetscErrorCode DMLabelConvertToSection(DMLabel, PetscSection *, IS *);

#endif
