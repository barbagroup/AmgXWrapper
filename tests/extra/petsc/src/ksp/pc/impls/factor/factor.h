/*
   Private data structure for ILU/ICC/LU/Cholesky preconditioners.
*/
#if !defined(__FACTOR_H)
#define __FACTOR_H

#include <petsc/private/pcimpl.h>
#include <petsc/private/matimpl.h>

typedef struct {
  Mat              fact;              /* factored matrix */
  MatFactorInfo    info;
  MatOrderingType  ordering;          /* matrix reordering */
  MatSolverPackage solvertype;
  MatFactorType    factortype;
} PC_Factor;

PETSC_INTERN PetscErrorCode PCFactorGetMatrix_Factor(PC,Mat*);

PETSC_INTERN PetscErrorCode PCFactorSetZeroPivot_Factor(PC,PetscReal);
PETSC_INTERN PetscErrorCode PCFactorSetShiftType_Factor(PC,MatFactorShiftType);
PETSC_INTERN PetscErrorCode PCFactorSetShiftAmount_Factor(PC,PetscReal);
PETSC_INTERN PetscErrorCode PCFactorSetDropTolerance_Factor(PC,PetscReal,PetscReal,PetscInt);
PETSC_INTERN PetscErrorCode PCFactorSetFill_Factor(PC,PetscReal);
PETSC_INTERN PetscErrorCode PCFactorSetMatOrderingType_Factor(PC,MatOrderingType);
PETSC_INTERN PetscErrorCode PCFactorGetLevels_Factor(PC,PetscInt*);
PETSC_INTERN PetscErrorCode PCFactorSetLevels_Factor(PC,PetscInt);
PETSC_INTERN PetscErrorCode PCFactorSetAllowDiagonalFill_Factor(PC,PetscBool);
PETSC_INTERN PetscErrorCode PCFactorGetAllowDiagonalFill_Factor(PC,PetscBool*);
PETSC_INTERN PetscErrorCode PCFactorSetPivotInBlocks_Factor(PC,PetscBool);
PETSC_INTERN PetscErrorCode PCFactorSetMatSolverPackage_Factor(PC,const MatSolverPackage);
PETSC_INTERN PetscErrorCode PCFactorSetUpMatSolverPackage_Factor(PC);
PETSC_INTERN PetscErrorCode PCFactorGetMatSolverPackage_Factor(PC,const MatSolverPackage*);
PETSC_INTERN PetscErrorCode PCFactorSetColumnPivot_Factor(PC,PetscReal);
PETSC_INTERN PetscErrorCode PCSetFromOptions_Factor(PetscOptionItems *PetscOptionsObject,PC);
PETSC_INTERN PetscErrorCode PCView_Factor(PC,PetscViewer);

#endif
