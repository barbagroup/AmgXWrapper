/*
   User interface for the timestepping package. This package
   is for use in solving time-dependent PDEs.
*/
#if !defined(__PETSCTS_H)
#define __PETSCTS_H
#include <petscsnes.h>

/*S
     TS - Abstract PETSc object that manages all time-steppers (ODE integrators)

   Level: beginner

  Concepts: ODE solvers

.seealso:  TSCreate(), TSSetType(), TSType, SNES, KSP, PC, TSDestroy()
S*/
typedef struct _p_TS* TS;

/*J
    TSType - String with the name of a PETSc TS method.

   Level: beginner

.seealso: TSSetType(), TS, TSRegister()
J*/
typedef const char* TSType;
#define TSEULER           "euler"
#define TSBEULER          "beuler"
#define TSPSEUDO          "pseudo"
#define TSCN              "cn"
#define TSSUNDIALS        "sundials"
#define TSRK              "rk"
#define TSPYTHON          "python"
#define TSTHETA           "theta"
#define TSALPHA           "alpha"
#define TSGL              "gl"
#define TSSSP             "ssp"
#define TSARKIMEX         "arkimex"
#define TSROSW            "rosw"
#define TSEIMEX           "eimex"
#define TSMIMEX           "mimex"
/*E
    TSProblemType - Determines the type of problem this TS object is to be used to solve

   Level: beginner

.seealso: TSCreate()
E*/
typedef enum {TS_LINEAR,TS_NONLINEAR} TSProblemType;

/*E
   TSEquationType - type of TS problem that is solved

   Level: beginner

   Developer Notes: this must match petsc/finclude/petscts.h

   Supported types are:
     TS_EQ_UNSPECIFIED (default)
     TS_EQ_EXPLICIT {ODE and DAE index 1, 2, 3, HI}: F(t,U,U_t) := M(t) U_t - G(U,t) = 0
     TS_EQ_IMPLICIT {ODE and DAE index 1, 2, 3, HI}: F(t,U,U_t) = 0

.seealso: TSGetEquationType(), TSSetEquationType()
E*/
typedef enum {
  TS_EQ_UNSPECIFIED               = -1,
  TS_EQ_EXPLICIT                  = 0,
  TS_EQ_ODE_EXPLICIT              = 1,
  TS_EQ_DAE_SEMI_EXPLICIT_INDEX1  = 100,
  TS_EQ_DAE_SEMI_EXPLICIT_INDEX2  = 200,
  TS_EQ_DAE_SEMI_EXPLICIT_INDEX3  = 300,
  TS_EQ_DAE_SEMI_EXPLICIT_INDEXHI = 500,
  TS_EQ_IMPLICIT                  = 1000,
  TS_EQ_ODE_IMPLICIT              = 1001,
  TS_EQ_DAE_IMPLICIT_INDEX1       = 1100,
  TS_EQ_DAE_IMPLICIT_INDEX2       = 1200,
  TS_EQ_DAE_IMPLICIT_INDEX3       = 1300,
  TS_EQ_DAE_IMPLICIT_INDEXHI      = 1500
} TSEquationType;
PETSC_EXTERN const char *const*TSEquationTypes;

/*E
   TSConvergedReason - reason a TS method has converged or not

   Level: beginner

   Developer Notes: this must match petsc/finclude/petscts.h

   Each reason has its own manual page.

.seealso: TSGetConvergedReason()
E*/
typedef enum {
  TS_CONVERGED_ITERATING      = 0,
  TS_CONVERGED_TIME           = 1,
  TS_CONVERGED_ITS            = 2,
  TS_CONVERGED_USER           = 3,
  TS_CONVERGED_EVENT          = 4,
  TS_CONVERGED_PSEUDO_FATOL   = 5,
  TS_CONVERGED_PSEUDO_FRTOL   = 6,
  TS_DIVERGED_NONLINEAR_SOLVE = -1,
  TS_DIVERGED_STEP_REJECTED   = -2
} TSConvergedReason;
PETSC_EXTERN const char *const*TSConvergedReasons;

/*MC
   TS_CONVERGED_ITERATING - this only occurs if TSGetConvergedReason() is called during the TSSolve()

   Level: beginner

.seealso: TSSolve(), TSGetConvergedReason(), TSGetAdapt()
M*/

/*MC
   TS_CONVERGED_TIME - the final time was reached

   Level: beginner

.seealso: TSSolve(), TSGetConvergedReason(), TSGetAdapt(), TSSetDuration(), TSGetSolveTime()
M*/

/*MC
   TS_CONVERGED_ITS - the maximum number of iterations (time-steps) was reached prior to the final time

   Level: beginner

.seealso: TSSolve(), TSGetConvergedReason(), TSGetAdapt(), TSSetDuration()
M*/

/*MC
   TS_CONVERGED_USER - user requested termination

   Level: beginner

.seealso: TSSolve(), TSGetConvergedReason(), TSSetConvergedReason(), TSSetDuration()
M*/

/*MC
   TS_CONVERGED_EVENT - user requested termination on event detection

   Level: beginner

.seealso: TSSolve(), TSGetConvergedReason(), TSSetConvergedReason(), TSSetDuration()
M*/

/*MC
   TS_CONVERGED_PSEUDO_FRTOL - stops when function norm decreased by a set amount, used only for TSPSEUDO

   Level: beginner

   Options Database:
.   -ts_pseudo_frtol <rtol>

.seealso: TSSolve(), TSGetConvergedReason(), TSSetConvergedReason(), TSSetDuration(), TS_CONVERGED_PSEUDO_FATOL
M*/

/*MC
   TS_CONVERGED_PSEUDO_FATOL - stops when function norm decreases below a set amount, used only for TSPSEUDO

   Level: beginner

   Options Database:
.   -ts_pseudo_fatol <atol>

.seealso: TSSolve(), TSGetConvergedReason(), TSSetConvergedReason(), TSSetDuration(), TS_CONVERGED_PSEUDO_FRTOL
M*/

/*MC
   TS_DIVERGED_NONLINEAR_SOLVE - too many nonlinear solves failed

   Level: beginner

   Notes: See TSSetMaxSNESFailures() for how to allow more nonlinear solver failures.

.seealso: TSSolve(), TSGetConvergedReason(), TSGetAdapt(), TSGetSNES(), SNESGetConvergedReason(), TSSetMaxSNESFailures()
M*/

/*MC
   TS_DIVERGED_STEP_REJECTED - too many steps were rejected

   Level: beginner

   Notes: See TSSetMaxStepRejections() for how to allow more step rejections.

.seealso: TSSolve(), TSGetConvergedReason(), TSGetAdapt(), TSSetMaxStepRejections()
M*/

/*E
   TSExactFinalTimeOption - option for handling of final time step

   Level: beginner

   Developer Notes: this must match petsc/finclude/petscts.h

$  TS_EXACTFINALTIME_STEPOVER    - Don't do anything if final time is exceeded
$  TS_EXACTFINALTIME_INTERPOLATE - Interpolate back to final time
$  TS_EXACTFINALTIME_MATCHSTEP - Adapt final time step to match the final time
.seealso: TSGetConvergedReason(), TSSetExactFinalTime()

E*/
typedef enum {TS_EXACTFINALTIME_STEPOVER=0,TS_EXACTFINALTIME_INTERPOLATE=1,TS_EXACTFINALTIME_MATCHSTEP=2} TSExactFinalTimeOption;
PETSC_EXTERN const char *const TSExactFinalTimeOptions[];


/* Logging support */
PETSC_EXTERN PetscClassId TS_CLASSID;
PETSC_EXTERN PetscClassId DMTS_CLASSID;

PETSC_EXTERN PetscErrorCode TSInitializePackage(void);

PETSC_EXTERN PetscErrorCode TSCreate(MPI_Comm,TS*);
PETSC_EXTERN PetscErrorCode TSClone(TS,TS*);
PETSC_EXTERN PetscErrorCode TSDestroy(TS*);

PETSC_EXTERN PetscErrorCode TSSetProblemType(TS,TSProblemType);
PETSC_EXTERN PetscErrorCode TSGetProblemType(TS,TSProblemType*);
PETSC_EXTERN PetscErrorCode TSMonitor(TS,PetscInt,PetscReal,Vec);
PETSC_EXTERN PetscErrorCode TSMonitorSet(TS,PetscErrorCode(*)(TS,PetscInt,PetscReal,Vec,void*),void *,PetscErrorCode (*)(void**));
PETSC_EXTERN PetscErrorCode TSMonitorCancel(TS);

PETSC_EXTERN PetscErrorCode TSAdjointMonitor(TS,PetscInt,PetscReal,Vec,PetscInt,Vec*,Vec*);
PETSC_EXTERN PetscErrorCode TSAdjointMonitorSet(TS,PetscErrorCode(*)(TS,PetscInt,PetscReal,Vec,PetscInt,Vec*,Vec*,void*),void *,PetscErrorCode (*)(void**));
PETSC_EXTERN PetscErrorCode TSAdjointMonitorCancel(TS);


PETSC_EXTERN PetscErrorCode TSSetOptionsPrefix(TS,const char[]);
PETSC_EXTERN PetscErrorCode TSAppendOptionsPrefix(TS,const char[]);
PETSC_EXTERN PetscErrorCode TSGetOptionsPrefix(TS,const char *[]);
PETSC_EXTERN PetscErrorCode TSSetFromOptions(TS);
PETSC_EXTERN PetscErrorCode TSSetUp(TS);
PETSC_EXTERN PetscErrorCode TSReset(TS);

PETSC_EXTERN PetscErrorCode TSSetSolution(TS,Vec);
PETSC_EXTERN PetscErrorCode TSGetSolution(TS,Vec*);

/*S
     TSTrajectory - Abstract PETSc object that storing the trajectory (solution of ODE/ADE at each time step and stage)

   Level: advanced

  Concepts: ODE solvers, adjoint

.seealso:  TSCreate(), TSSetType(), TSType, SNES, KSP, PC, TSDestroy()
S*/
typedef struct _p_TSTrajectory* TSTrajectory;

/*J
    TSTrajectoryType - String with the name of a PETSc TS trajectory storage method

   Level: intermediate

.seealso: TSSetType(), TS, TSRegister(), TSTrajectoryCreate(), TSTrajectorySetType()
J*/
typedef const char* TSTrajectoryType;
#define TSTRAJECTORYBASIC      "basic"
#define TSTRAJECTORYSINGLEFILE "singlefile"
#define TSTRAJECTORYMEMORY     "memory"

PETSC_EXTERN PetscFunctionList TSTrajectoryList;
PETSC_EXTERN PetscClassId      TSTRAJECTORY_CLASSID;
PETSC_EXTERN PetscBool         TSTrajectoryRegisterAllCalled;

PETSC_EXTERN PetscErrorCode TSSetSaveTrajectory(TS);

PETSC_EXTERN PetscErrorCode TSTrajectoryCreate(MPI_Comm,TSTrajectory*);
PETSC_EXTERN PetscErrorCode TSTrajectoryDestroy(TSTrajectory*);
PETSC_EXTERN PetscErrorCode TSTrajectorySetType(TSTrajectory,TS,const TSTrajectoryType);
PETSC_EXTERN PetscErrorCode TSTrajectorySet(TSTrajectory,TS,PetscInt,PetscReal,Vec);
PETSC_EXTERN PetscErrorCode TSTrajectoryGet(TSTrajectory,TS,PetscInt,PetscReal*);
PETSC_EXTERN PetscErrorCode TSTrajectorySetFromOptions(TSTrajectory,TS);
PETSC_EXTERN PetscErrorCode TSTrajectoryRegisterAll(void);
PETSC_EXTERN PetscErrorCode TSTrajectorySetUp(TSTrajectory,TS);

PETSC_EXTERN PetscErrorCode TSSetCostGradients(TS,PetscInt,Vec*,Vec*);
PETSC_EXTERN PetscErrorCode TSGetCostGradients(TS,PetscInt*,Vec**,Vec**);
PETSC_EXTERN PetscErrorCode TSSetCostIntegrand(TS,PetscInt,PetscErrorCode (*)(TS,PetscReal,Vec,Vec,void*),PetscErrorCode (*)(TS,PetscReal,Vec,Vec*,void*),PetscErrorCode (*)(TS,PetscReal,Vec,Vec*,void*),void*);
PETSC_EXTERN PetscErrorCode TSGetCostIntegral(TS,Vec*);

PETSC_EXTERN PetscErrorCode TSAdjointSetRHSJacobian(TS,Mat,PetscErrorCode(*)(TS,PetscReal,Vec,Mat,void*),void*);
PETSC_EXTERN PetscErrorCode TSAdjointSolve(TS);
PETSC_EXTERN PetscErrorCode TSAdjointSetSteps(TS,PetscInt);

PETSC_EXTERN PetscErrorCode TSAdjointComputeRHSJacobian(TS,PetscReal,Vec,Mat);
PETSC_EXTERN PetscErrorCode TSAdjointStep(TS);
PETSC_EXTERN PetscErrorCode TSAdjointSetUp(TS);
PETSC_EXTERN PetscErrorCode TSAdjointComputeDRDPFunction(TS,PetscReal,Vec,Vec*);
PETSC_EXTERN PetscErrorCode TSAdjointComputeDRDYFunction(TS,PetscReal,Vec,Vec*);
PETSC_EXTERN PetscErrorCode TSAdjointComputeCostIntegrand(TS,PetscReal,Vec,Vec);

PETSC_EXTERN PetscErrorCode TSSetDuration(TS,PetscInt,PetscReal);
PETSC_EXTERN PetscErrorCode TSGetDuration(TS,PetscInt*,PetscReal*);
PETSC_EXTERN PetscErrorCode TSSetExactFinalTime(TS,TSExactFinalTimeOption);

PETSC_EXTERN PetscErrorCode TSMonitorDefault(TS,PetscInt,PetscReal,Vec,void*);

typedef struct _n_TSMonitorDrawCtx*  TSMonitorDrawCtx;
PETSC_EXTERN PetscErrorCode TSMonitorDrawCtxCreate(MPI_Comm,const char[],const char[],int,int,int,int,PetscInt,TSMonitorDrawCtx *);
PETSC_EXTERN PetscErrorCode TSMonitorDrawCtxDestroy(TSMonitorDrawCtx*);
PETSC_EXTERN PetscErrorCode TSMonitorDrawSolution(TS,PetscInt,PetscReal,Vec,void*);
PETSC_EXTERN PetscErrorCode TSMonitorDrawSolutionPhase(TS,PetscInt,PetscReal,Vec,void*);
PETSC_EXTERN PetscErrorCode TSMonitorDrawError(TS,PetscInt,PetscReal,Vec,void*);

PETSC_EXTERN PetscErrorCode TSAdjointMonitorDefault(TS,PetscInt,PetscReal,Vec,PetscInt,Vec*,Vec*,void *);
PETSC_EXTERN PetscErrorCode TSAdjointMonitorDrawSensi(TS,PetscInt,PetscReal,Vec,PetscInt,Vec*,Vec*,void*);

PETSC_EXTERN PetscErrorCode TSMonitorSolution(TS,PetscInt,PetscReal,Vec,void*);
PETSC_EXTERN PetscErrorCode TSMonitorSolutionVTK(TS,PetscInt,PetscReal,Vec,void*);
PETSC_EXTERN PetscErrorCode TSMonitorSolutionVTKDestroy(void*);

PETSC_EXTERN PetscErrorCode TSStep(TS);
PETSC_EXTERN PetscErrorCode TSEvaluateStep(TS,PetscInt,Vec,PetscBool*);
PETSC_EXTERN PetscErrorCode TSSolve(TS,Vec);
PETSC_EXTERN PetscErrorCode TSGetEquationType(TS,TSEquationType*);
PETSC_EXTERN PetscErrorCode TSSetEquationType(TS,TSEquationType);
PETSC_EXTERN PetscErrorCode TSGetConvergedReason(TS,TSConvergedReason*);
PETSC_EXTERN PetscErrorCode TSSetConvergedReason(TS,TSConvergedReason);
PETSC_EXTERN PetscErrorCode TSGetSolveTime(TS,PetscReal*);
PETSC_EXTERN PetscErrorCode TSGetSNESIterations(TS,PetscInt*);
PETSC_EXTERN PetscErrorCode TSGetKSPIterations(TS,PetscInt*);
PETSC_EXTERN PetscErrorCode TSGetStepRejections(TS,PetscInt*);
PETSC_EXTERN PetscErrorCode TSSetMaxStepRejections(TS,PetscInt);
PETSC_EXTERN PetscErrorCode TSGetSNESFailures(TS,PetscInt*);
PETSC_EXTERN PetscErrorCode TSSetMaxSNESFailures(TS,PetscInt);
PETSC_EXTERN PetscErrorCode TSSetErrorIfStepFails(TS,PetscBool);
PETSC_EXTERN PetscErrorCode TSRollBack(TS);
PETSC_EXTERN PetscErrorCode TSGetTotalSteps(TS,PetscInt*);

PETSC_EXTERN PetscErrorCode TSGetStages(TS,PetscInt*,Vec**);

PETSC_EXTERN PetscErrorCode TSSetInitialTimeStep(TS,PetscReal,PetscReal);
PETSC_EXTERN PetscErrorCode TSGetTimeStep(TS,PetscReal*);
PETSC_EXTERN PetscErrorCode TSGetTime(TS,PetscReal*);
PETSC_EXTERN PetscErrorCode TSSetTime(TS,PetscReal);
PETSC_EXTERN PetscErrorCode TSGetTimeStepNumber(TS,PetscInt*);
PETSC_EXTERN PetscErrorCode TSSetTimeStep(TS,PetscReal);
PETSC_EXTERN PetscErrorCode TSGetPrevTime(TS,PetscReal*);

PETSC_EXTERN_TYPEDEF typedef PetscErrorCode (*TSRHSFunction)(TS,PetscReal,Vec,Vec,void*);
PETSC_EXTERN_TYPEDEF typedef PetscErrorCode (*TSRHSJacobian)(TS,PetscReal,Vec,Mat,Mat,void*);
PETSC_EXTERN PetscErrorCode TSSetRHSFunction(TS,Vec,TSRHSFunction,void*);
PETSC_EXTERN PetscErrorCode TSGetRHSFunction(TS,Vec*,TSRHSFunction*,void**);
PETSC_EXTERN PetscErrorCode TSSetRHSJacobian(TS,Mat,Mat,TSRHSJacobian,void*);
PETSC_EXTERN PetscErrorCode TSGetRHSJacobian(TS,Mat*,Mat*,TSRHSJacobian*,void**);
PETSC_EXTERN PetscErrorCode TSRHSJacobianSetReuse(TS,PetscBool);

PETSC_EXTERN_TYPEDEF typedef PetscErrorCode (*TSSolutionFunction)(TS,PetscReal,Vec,void*);
PETSC_EXTERN PetscErrorCode TSSetSolutionFunction(TS,TSSolutionFunction,void*);
PETSC_EXTERN PetscErrorCode TSSetForcingFunction(TS,PetscErrorCode (*)(TS,PetscReal,Vec,void*),void*);

PETSC_EXTERN_TYPEDEF typedef PetscErrorCode (*TSIFunction)(TS,PetscReal,Vec,Vec,Vec,void*);
PETSC_EXTERN_TYPEDEF typedef PetscErrorCode (*TSIJacobian)(TS,PetscReal,Vec,Vec,PetscReal,Mat,Mat,void*);
PETSC_EXTERN PetscErrorCode TSSetIFunction(TS,Vec,TSIFunction,void*);
PETSC_EXTERN PetscErrorCode TSGetIFunction(TS,Vec*,TSIFunction*,void**);
PETSC_EXTERN PetscErrorCode TSSetIJacobian(TS,Mat,Mat,TSIJacobian,void*);
PETSC_EXTERN PetscErrorCode TSGetIJacobian(TS,Mat*,Mat*,TSIJacobian*,void**);

PETSC_EXTERN PetscErrorCode TSComputeRHSFunctionLinear(TS,PetscReal,Vec,Vec,void*);
PETSC_EXTERN PetscErrorCode TSComputeRHSJacobianConstant(TS,PetscReal,Vec,Mat,Mat,void*);
PETSC_EXTERN PetscErrorCode TSComputeIFunctionLinear(TS,PetscReal,Vec,Vec,Vec,void*);
PETSC_EXTERN PetscErrorCode TSComputeIJacobianConstant(TS,PetscReal,Vec,Vec,PetscReal,Mat,Mat,void*);
PETSC_EXTERN PetscErrorCode TSComputeSolutionFunction(TS,PetscReal,Vec);
PETSC_EXTERN PetscErrorCode TSComputeForcingFunction(TS,PetscReal,Vec);
PETSC_EXTERN PetscErrorCode TSComputeIJacobianDefaultColor(TS,PetscReal,Vec,Vec,PetscReal,Mat,Mat,void*);

PETSC_EXTERN PetscErrorCode TSSetPreStep(TS, PetscErrorCode (*)(TS));
PETSC_EXTERN PetscErrorCode TSSetPreStage(TS, PetscErrorCode (*)(TS,PetscReal));
PETSC_EXTERN PetscErrorCode TSSetPostStage(TS, PetscErrorCode (*)(TS,PetscReal,PetscInt,Vec*));
PETSC_EXTERN PetscErrorCode TSSetPostStep(TS, PetscErrorCode (*)(TS));
PETSC_EXTERN PetscErrorCode TSPreStep(TS);
PETSC_EXTERN PetscErrorCode TSPreStage(TS,PetscReal);
PETSC_EXTERN PetscErrorCode TSPostStage(TS,PetscReal,PetscInt,Vec*);
PETSC_EXTERN PetscErrorCode TSPostStep(TS);
PETSC_EXTERN PetscErrorCode TSSetRetainStages(TS,PetscBool);
PETSC_EXTERN PetscErrorCode TSInterpolate(TS,PetscReal,Vec);
PETSC_EXTERN PetscErrorCode TSSetTolerances(TS,PetscReal,Vec,PetscReal,Vec);
PETSC_EXTERN PetscErrorCode TSGetTolerances(TS,PetscReal*,Vec*,PetscReal*,Vec*);
PETSC_EXTERN PetscErrorCode TSErrorWeightedNormInfinity(TS,Vec,Vec,PetscReal*);
PETSC_EXTERN PetscErrorCode TSErrorWeightedNorm2(TS,Vec,Vec,PetscReal*);
PETSC_EXTERN PetscErrorCode TSErrorWeightedNorm(TS,Vec,Vec,NormType,PetscReal*);
PETSC_EXTERN PetscErrorCode TSSetCFLTimeLocal(TS,PetscReal);
PETSC_EXTERN PetscErrorCode TSGetCFLTime(TS,PetscReal*);
PETSC_EXTERN PetscErrorCode TSSetFunctionDomainError(TS, PetscErrorCode (*)(TS,PetscReal,Vec,PetscBool*));
PETSC_EXTERN PetscErrorCode TSFunctionDomainError(TS,PetscReal,Vec,PetscBool*);

PETSC_EXTERN PetscErrorCode TSPseudoSetTimeStep(TS,PetscErrorCode(*)(TS,PetscReal*,void*),void*);
PETSC_EXTERN PetscErrorCode TSPseudoTimeStepDefault(TS,PetscReal*,void*);
PETSC_EXTERN PetscErrorCode TSPseudoComputeTimeStep(TS,PetscReal *);
PETSC_EXTERN PetscErrorCode TSPseudoSetMaxTimeStep(TS,PetscReal);

PETSC_EXTERN PetscErrorCode TSPseudoSetVerifyTimeStep(TS,PetscErrorCode(*)(TS,Vec,void*,PetscReal*,PetscBool *),void*);
PETSC_EXTERN PetscErrorCode TSPseudoVerifyTimeStepDefault(TS,Vec,void*,PetscReal*,PetscBool *);
PETSC_EXTERN PetscErrorCode TSPseudoVerifyTimeStep(TS,Vec,PetscReal*,PetscBool *);
PETSC_EXTERN PetscErrorCode TSPseudoSetTimeStepIncrement(TS,PetscReal);
PETSC_EXTERN PetscErrorCode TSPseudoIncrementDtFromInitialDt(TS);

PETSC_EXTERN PetscErrorCode TSPythonSetType(TS,const char[]);

PETSC_EXTERN PetscErrorCode TSComputeRHSFunction(TS,PetscReal,Vec,Vec);
PETSC_EXTERN PetscErrorCode TSComputeRHSJacobian(TS,PetscReal,Vec,Mat,Mat);
PETSC_EXTERN PetscErrorCode TSComputeIFunction(TS,PetscReal,Vec,Vec,Vec,PetscBool);
PETSC_EXTERN PetscErrorCode TSComputeIJacobian(TS,PetscReal,Vec,Vec,PetscReal,Mat,Mat,PetscBool);
PETSC_EXTERN PetscErrorCode TSComputeLinearStability(TS,PetscReal,PetscReal,PetscReal*,PetscReal*);

PETSC_EXTERN PetscErrorCode TSVISetVariableBounds(TS,Vec,Vec);

PETSC_EXTERN PetscErrorCode DMTSSetRHSFunction(DM,TSRHSFunction,void*);
PETSC_EXTERN PetscErrorCode DMTSGetRHSFunction(DM,TSRHSFunction*,void**);
PETSC_EXTERN PetscErrorCode DMTSSetRHSJacobian(DM,TSRHSJacobian,void*);
PETSC_EXTERN PetscErrorCode DMTSGetRHSJacobian(DM,TSRHSJacobian*,void**);
PETSC_EXTERN PetscErrorCode DMTSSetIFunction(DM,TSIFunction,void*);
PETSC_EXTERN PetscErrorCode DMTSGetIFunction(DM,TSIFunction*,void**);
PETSC_EXTERN PetscErrorCode DMTSSetIJacobian(DM,TSIJacobian,void*);
PETSC_EXTERN PetscErrorCode DMTSGetIJacobian(DM,TSIJacobian*,void**);
PETSC_EXTERN PetscErrorCode DMTSSetSolutionFunction(DM,TSSolutionFunction,void*);
PETSC_EXTERN PetscErrorCode DMTSGetSolutionFunction(DM,TSSolutionFunction*,void**);
PETSC_EXTERN PetscErrorCode DMTSSetForcingFunction(DM,PetscErrorCode (*)(TS,PetscReal,Vec,void*),void*);
PETSC_EXTERN PetscErrorCode DMTSGetForcingFunction(DM,PetscErrorCode (**)(TS,PetscReal,Vec,void*),void**);
PETSC_EXTERN PetscErrorCode DMTSGetMinRadius(DM,PetscReal*);
PETSC_EXTERN PetscErrorCode DMTSSetMinRadius(DM,PetscReal);
PETSC_EXTERN PetscErrorCode DMTSCheckFromOptions(TS, Vec, PetscErrorCode (**)(PetscInt, PetscReal, const PetscReal[], PetscInt, PetscScalar *, void *), void **);

PETSC_EXTERN PetscErrorCode DMTSSetIFunctionLocal(DM,PetscErrorCode (*)(DM,PetscReal,Vec,Vec,Vec,void*),void*);
PETSC_EXTERN PetscErrorCode DMTSSetIJacobianLocal(DM,PetscErrorCode (*)(DM,PetscReal,Vec,Vec,PetscReal,Mat,Mat,void*),void*);
PETSC_EXTERN PetscErrorCode DMTSSetRHSFunctionLocal(DM,PetscErrorCode (*)(DM,PetscReal,Vec,Vec,void*),void*);

PETSC_EXTERN PetscErrorCode DMTSSetIFunctionSerialize(DM,PetscErrorCode (*)(void*,PetscViewer),PetscErrorCode (*)(void**,PetscViewer));
PETSC_EXTERN PetscErrorCode DMTSSetIJacobianSerialize(DM,PetscErrorCode (*)(void*,PetscViewer),PetscErrorCode (*)(void**,PetscViewer));

PETSC_EXTERN_TYPEDEF typedef PetscErrorCode (*DMDATSRHSFunctionLocal)(DMDALocalInfo*,PetscReal,void*,void*,void*);
PETSC_EXTERN_TYPEDEF typedef PetscErrorCode (*DMDATSRHSJacobianLocal)(DMDALocalInfo*,PetscReal,void*,Mat,Mat,void*);
PETSC_EXTERN_TYPEDEF typedef PetscErrorCode (*DMDATSIFunctionLocal)(DMDALocalInfo*,PetscReal,void*,void*,void*,void*);
PETSC_EXTERN_TYPEDEF typedef PetscErrorCode (*DMDATSIJacobianLocal)(DMDALocalInfo*,PetscReal,void*,void*,PetscReal,Mat,Mat,void*);

PETSC_EXTERN PetscErrorCode DMDATSSetRHSFunctionLocal(DM,InsertMode,PetscErrorCode (*)(DMDALocalInfo*,PetscReal,void*,void*,void*),void *);
PETSC_EXTERN PetscErrorCode DMDATSSetRHSJacobianLocal(DM,PetscErrorCode (*)(DMDALocalInfo*,PetscReal,void*,Mat,Mat,void*),void *);
PETSC_EXTERN PetscErrorCode DMDATSSetIFunctionLocal(DM,InsertMode,PetscErrorCode (*)(DMDALocalInfo*,PetscReal,void*,void*,void*,void*),void *);
PETSC_EXTERN PetscErrorCode DMDATSSetIJacobianLocal(DM,PetscErrorCode (*)(DMDALocalInfo*,PetscReal,void*,void*,PetscReal,Mat,Mat,void*),void *);

PETSC_EXTERN PetscErrorCode DMPlexTSGetGeometryFVM(DM, Vec *, Vec *, PetscReal *);

typedef struct _n_TSMonitorLGCtx*  TSMonitorLGCtx;
typedef struct {
  Vec            ray;
  VecScatter     scatter;
  PetscViewer    viewer;
  TSMonitorLGCtx lgctx;
} TSMonitorDMDARayCtx;
PETSC_EXTERN PetscErrorCode TSMonitorDMDARayDestroy(void**);
PETSC_EXTERN PetscErrorCode TSMonitorDMDARay(TS,PetscInt,PetscReal,Vec,void*);
PETSC_EXTERN PetscErrorCode TSMonitorLGDMDARay(TS,PetscInt,PetscReal,Vec,void*);


/* Dynamic creation and loading functions */
PETSC_EXTERN PetscFunctionList TSList;
PETSC_EXTERN PetscErrorCode TSGetType(TS,TSType*);
PETSC_EXTERN PetscErrorCode TSSetType(TS,TSType);
PETSC_EXTERN PetscErrorCode TSRegister(const char[], PetscErrorCode (*)(TS));

PETSC_EXTERN PetscErrorCode TSGetSNES(TS,SNES*);
PETSC_EXTERN PetscErrorCode TSSetSNES(TS,SNES);
PETSC_EXTERN PetscErrorCode TSGetKSP(TS,KSP*);

PETSC_EXTERN PetscErrorCode TSView(TS,PetscViewer);
PETSC_EXTERN PetscErrorCode TSLoad(TS,PetscViewer);
PETSC_STATIC_INLINE PetscErrorCode TSViewFromOptions(TS A,PetscObject obj,const char name[]) {return PetscObjectViewFromOptions((PetscObject)A,obj,name);}
PETSC_STATIC_INLINE PetscErrorCode TSTrajectoryViewFromOptions(TSTrajectory A,PetscObject obj,const char name[]) {return PetscObjectViewFromOptions((PetscObject)A,obj,name);}

#define TS_FILE_CLASSID 1211225

PETSC_EXTERN PetscErrorCode TSSetApplicationContext(TS,void *);
PETSC_EXTERN PetscErrorCode TSGetApplicationContext(TS,void *);

PETSC_EXTERN PetscErrorCode TSMonitorLGCtxCreate(MPI_Comm,const char[],const char[],int,int,int,int,PetscInt,TSMonitorLGCtx *);
PETSC_EXTERN PetscErrorCode TSMonitorLGCtxDestroy(TSMonitorLGCtx*);
PETSC_EXTERN PetscErrorCode TSMonitorLGTimeStep(TS,PetscInt,PetscReal,Vec,void *);
PETSC_EXTERN PetscErrorCode TSMonitorLGSolution(TS,PetscInt,PetscReal,Vec,void *);
PETSC_EXTERN PetscErrorCode TSMonitorLGSetVariableNames(TS,const char * const*);
PETSC_EXTERN PetscErrorCode TSMonitorLGGetVariableNames(TS,const char *const **);
PETSC_EXTERN PetscErrorCode TSMonitorLGCtxSetVariableNames(TSMonitorLGCtx,const char * const *);
PETSC_EXTERN PetscErrorCode TSMonitorLGSetDisplayVariables(TS,const char * const*);
PETSC_EXTERN PetscErrorCode TSMonitorLGCtxSetDisplayVariables(TSMonitorLGCtx,const char * const*);
PETSC_EXTERN PetscErrorCode TSMonitorLGSetTransform(TS,PetscErrorCode (*)(void*,Vec,Vec*),PetscErrorCode (*)(void*),void*);
PETSC_EXTERN PetscErrorCode TSMonitorLGCtxSetTransform(TSMonitorLGCtx,PetscErrorCode (*)(void*,Vec,Vec*),PetscErrorCode (*)(void*),void *);
PETSC_EXTERN PetscErrorCode TSMonitorLGError(TS,PetscInt,PetscReal,Vec,void *);
PETSC_EXTERN PetscErrorCode TSMonitorLGSNESIterations(TS,PetscInt,PetscReal,Vec,void *);
PETSC_EXTERN PetscErrorCode TSMonitorLGKSPIterations(TS,PetscInt,PetscReal,Vec,void *);

typedef struct _n_TSMonitorEnvelopeCtx*  TSMonitorEnvelopeCtx;
PETSC_EXTERN PetscErrorCode TSMonitorEnvelopeCtxCreate(TS,TSMonitorEnvelopeCtx*);
PETSC_EXTERN PetscErrorCode TSMonitorEnvelope(TS,PetscInt,PetscReal,Vec,void*);
PETSC_EXTERN PetscErrorCode TSMonitorEnvelopeGetBounds(TS,Vec*,Vec*);
PETSC_EXTERN PetscErrorCode TSMonitorEnvelopeCtxDestroy(TSMonitorEnvelopeCtx*);

typedef struct _n_TSMonitorSPEigCtx*  TSMonitorSPEigCtx;
PETSC_EXTERN PetscErrorCode TSMonitorSPEigCtxCreate(MPI_Comm,const char[],const char[],int,int,int,int,PetscInt,TSMonitorSPEigCtx *);
PETSC_EXTERN PetscErrorCode TSMonitorSPEigCtxDestroy(TSMonitorSPEigCtx*);
PETSC_EXTERN PetscErrorCode TSMonitorSPEig(TS,PetscInt,PetscReal,Vec,void *);

PETSC_EXTERN PetscErrorCode TSSetEventMonitor(TS,PetscInt,PetscInt*,PetscBool*,PetscErrorCode (*)(TS,PetscReal,Vec,PetscScalar*,void*),PetscErrorCode (*)(TS,PetscInt,PetscInt[],PetscReal,Vec,PetscBool,void*),void*);
PETSC_EXTERN PetscErrorCode TSSetEventTolerances(TS,PetscReal,PetscReal*);
/*J
   TSSSPType - string with the name of TSSSP scheme.

   Level: beginner

.seealso: TSSSPSetType(), TS
J*/
typedef const char* TSSSPType;
#define TSSSPRKS2  "rks2"
#define TSSSPRKS3  "rks3"
#define TSSSPRK104 "rk104"

PETSC_EXTERN PetscErrorCode TSSSPSetType(TS,TSSSPType);
PETSC_EXTERN PetscErrorCode TSSSPGetType(TS,TSSSPType*);
PETSC_EXTERN PetscErrorCode TSSSPSetNumStages(TS,PetscInt);
PETSC_EXTERN PetscErrorCode TSSSPGetNumStages(TS,PetscInt*);
PETSC_EXTERN PetscErrorCode TSSSPFinalizePackage(void);
PETSC_EXTERN PetscErrorCode TSSSPInitializePackage(void);

/*S
   TSAdapt - Abstract object that manages time-step adaptivity

   Level: beginner

.seealso: TS, TSAdaptCreate(), TSAdaptType
S*/
typedef struct _p_TSAdapt *TSAdapt;

/*E
    TSAdaptType - String with the name of TSAdapt scheme.

   Level: beginner

.seealso: TSAdaptSetType(), TS
E*/
typedef const char *TSAdaptType;
#define TSADAPTBASIC "basic"
#define TSADAPTNONE  "none"
#define TSADAPTCFL   "cfl"

PETSC_EXTERN PetscErrorCode TSGetAdapt(TS,TSAdapt*);
PETSC_EXTERN PetscErrorCode TSAdaptRegister(const char[],PetscErrorCode (*)(TSAdapt));
PETSC_EXTERN PetscErrorCode TSAdaptInitializePackage(void);
PETSC_EXTERN PetscErrorCode TSAdaptFinalizePackage(void);
PETSC_EXTERN PetscErrorCode TSAdaptCreate(MPI_Comm,TSAdapt*);
PETSC_EXTERN PetscErrorCode TSAdaptSetType(TSAdapt,TSAdaptType);
PETSC_EXTERN PetscErrorCode TSAdaptSetOptionsPrefix(TSAdapt,const char[]);
PETSC_EXTERN PetscErrorCode TSAdaptCandidatesClear(TSAdapt);
PETSC_EXTERN PetscErrorCode TSAdaptCandidateAdd(TSAdapt,const char[],PetscInt,PetscInt,PetscReal,PetscReal,PetscBool);
PETSC_EXTERN PetscErrorCode TSAdaptCandidatesGet(TSAdapt,PetscInt*,const PetscInt**,const PetscInt**,const PetscReal**,const PetscReal**);
PETSC_EXTERN PetscErrorCode TSAdaptChoose(TSAdapt,TS,PetscReal,PetscInt*,PetscReal*,PetscBool*);
PETSC_EXTERN PetscErrorCode TSAdaptCheckStage(TSAdapt,TS,PetscReal,Vec,PetscBool*);
PETSC_EXTERN PetscErrorCode TSAdaptView(TSAdapt,PetscViewer);
PETSC_EXTERN PetscErrorCode TSAdaptLoad(TSAdapt,PetscViewer);
PETSC_EXTERN PetscErrorCode TSAdaptSetFromOptions(PetscOptionItems*,TSAdapt);
PETSC_EXTERN PetscErrorCode TSAdaptReset(TSAdapt);
PETSC_EXTERN PetscErrorCode TSAdaptDestroy(TSAdapt*);
PETSC_EXTERN PetscErrorCode TSAdaptSetMonitor(TSAdapt,PetscBool);
PETSC_EXTERN PetscErrorCode TSAdaptSetStepLimits(TSAdapt,PetscReal,PetscReal);
PETSC_EXTERN PetscErrorCode TSAdaptSetCheckStage(TSAdapt,PetscErrorCode(*)(TSAdapt,TS,PetscReal,Vec,PetscBool*));

/*S
   TSGLAdapt - Abstract object that manages time-step adaptivity

   Level: beginner

   Developer Notes:
   This functionality should be replaced by the TSAdapt.

.seealso: TSGL, TSGLAdaptCreate(), TSGLAdaptType
S*/
typedef struct _p_TSGLAdapt *TSGLAdapt;

/*J
    TSGLAdaptType - String with the name of TSGLAdapt scheme

   Level: beginner

.seealso: TSGLAdaptSetType(), TS
J*/
typedef const char *TSGLAdaptType;
#define TSGLADAPT_NONE "none"
#define TSGLADAPT_SIZE "size"
#define TSGLADAPT_BOTH "both"

PETSC_EXTERN PetscErrorCode TSGLAdaptRegister(const char[],PetscErrorCode (*)(TSGLAdapt));
PETSC_EXTERN PetscErrorCode TSGLAdaptInitializePackage(void);
PETSC_EXTERN PetscErrorCode TSGLAdaptFinalizePackage(void);
PETSC_EXTERN PetscErrorCode TSGLAdaptCreate(MPI_Comm,TSGLAdapt*);
PETSC_EXTERN PetscErrorCode TSGLAdaptSetType(TSGLAdapt,TSGLAdaptType);
PETSC_EXTERN PetscErrorCode TSGLAdaptSetOptionsPrefix(TSGLAdapt,const char[]);
PETSC_EXTERN PetscErrorCode TSGLAdaptChoose(TSGLAdapt,PetscInt,const PetscInt[],const PetscReal[],const PetscReal[],PetscInt,PetscReal,PetscReal,PetscInt*,PetscReal*,PetscBool *);
PETSC_EXTERN PetscErrorCode TSGLAdaptView(TSGLAdapt,PetscViewer);
PETSC_EXTERN PetscErrorCode TSGLAdaptSetFromOptions(PetscOptionItems*,TSGLAdapt);
PETSC_EXTERN PetscErrorCode TSGLAdaptDestroy(TSGLAdapt*);

/*J
    TSGLAcceptType - String with the name of TSGLAccept scheme

   Level: beginner

.seealso: TSGLSetAcceptType(), TS
J*/
typedef const char *TSGLAcceptType;
#define TSGLACCEPT_ALWAYS "always"

PETSC_EXTERN_TYPEDEF typedef PetscErrorCode (*TSGLAcceptFunction)(TS,PetscReal,PetscReal,const PetscReal[],PetscBool *);
PETSC_EXTERN PetscErrorCode TSGLAcceptRegister(const char[],TSGLAcceptFunction);

/*J
  TSGLType - family of time integration method within the General Linear class

  Level: beginner

.seealso: TSGLSetType(), TSGLRegister()
J*/
typedef const char* TSGLType;
#define TSGL_IRKS   "irks"

PETSC_EXTERN PetscErrorCode TSGLRegister(const char[],PetscErrorCode(*)(TS));
PETSC_EXTERN PetscErrorCode TSGLInitializePackage(void);
PETSC_EXTERN PetscErrorCode TSGLFinalizePackage(void);
PETSC_EXTERN PetscErrorCode TSGLSetType(TS,TSGLType);
PETSC_EXTERN PetscErrorCode TSGLGetAdapt(TS,TSGLAdapt*);
PETSC_EXTERN PetscErrorCode TSGLSetAcceptType(TS,TSGLAcceptType);

/*J
    TSEIMEXType - String with the name of an Extrapolated IMEX method.

   Level: beginner

.seealso: TSEIMEXSetType(), TS, TSEIMEX, TSEIMEXRegister()
J*/
#define TSEIMEXType   char*

PETSC_EXTERN PetscErrorCode TSEIMEXSetMaxRows(TS ts,PetscInt);
PETSC_EXTERN PetscErrorCode TSEIMEXSetRowCol(TS ts,PetscInt,PetscInt);
PETSC_EXTERN PetscErrorCode TSEIMEXSetOrdAdapt(TS,PetscBool);

/*J
    TSRKType - String with the name of a Runge-Kutta method.

   Level: beginner

.seealso: TSRKSetType(), TS, TSRK, TSRKRegister()
J*/
typedef const char* TSRKType;
#define TSRK1FE   "1fe"
#define TSRK2A    "2a"
#define TSRK3     "3"
#define TSRK3BS   "3bs"
#define TSRK4     "4"
#define TSRK5F    "5f"
#define TSRK5DP   "5dp"
PETSC_EXTERN PetscErrorCode TSRKGetType(TS ts,TSRKType*);
PETSC_EXTERN PetscErrorCode TSRKSetType(TS ts,TSRKType);
PETSC_EXTERN PetscErrorCode TSRKSetFullyImplicit(TS,PetscBool);
PETSC_EXTERN PetscErrorCode TSRKRegister(TSRKType,PetscInt,PetscInt,const PetscReal[],const PetscReal[],const PetscReal[],const PetscReal[],PetscInt,const PetscReal[]);
PETSC_EXTERN PetscErrorCode TSRKFinalizePackage(void);
PETSC_EXTERN PetscErrorCode TSRKInitializePackage(void);
PETSC_EXTERN PetscErrorCode TSRKRegisterDestroy(void);

/*J
    TSARKIMEXType - String with the name of an Additive Runge-Kutta IMEX method.

   Level: beginner

.seealso: TSARKIMEXSetType(), TS, TSARKIMEX, TSARKIMEXRegister()
J*/
typedef const char* TSARKIMEXType;
#define TSARKIMEX1BEE   "1bee"
#define TSARKIMEXA2     "a2"
#define TSARKIMEXL2     "l2"
#define TSARKIMEXARS122 "ars122"
#define TSARKIMEX2C     "2c"
#define TSARKIMEX2D     "2d"
#define TSARKIMEX2E     "2e"
#define TSARKIMEXPRSSP2 "prssp2"
#define TSARKIMEX3      "3"
#define TSARKIMEXBPR3   "bpr3"
#define TSARKIMEXARS443 "ars443"
#define TSARKIMEX4      "4"
#define TSARKIMEX5      "5"
PETSC_EXTERN PetscErrorCode TSARKIMEXGetType(TS ts,TSARKIMEXType*);
PETSC_EXTERN PetscErrorCode TSARKIMEXSetType(TS ts,TSARKIMEXType);
PETSC_EXTERN PetscErrorCode TSARKIMEXSetFullyImplicit(TS,PetscBool);
PETSC_EXTERN PetscErrorCode TSARKIMEXRegister(TSARKIMEXType,PetscInt,PetscInt,const PetscReal[],const PetscReal[],const PetscReal[],const PetscReal[],const PetscReal[],const PetscReal[],const PetscReal[],const PetscReal[],PetscInt,const PetscReal[],const PetscReal[]);
PETSC_EXTERN PetscErrorCode TSARKIMEXFinalizePackage(void);
PETSC_EXTERN PetscErrorCode TSARKIMEXInitializePackage(void);
PETSC_EXTERN PetscErrorCode TSARKIMEXRegisterDestroy(void);

/*J
    TSRosWType - String with the name of a Rosenbrock-W method.

   Level: beginner

.seealso: TSRosWSetType(), TS, TSROSW, TSRosWRegister()
J*/
typedef const char* TSRosWType;
#define TSROSW2M          "2m"
#define TSROSW2P          "2p"
#define TSROSWRA3PW       "ra3pw"
#define TSROSWRA34PW2     "ra34pw2"
#define TSROSWRODAS3      "rodas3"
#define TSROSWSANDU3      "sandu3"
#define TSROSWASSP3P3S1C  "assp3p3s1c"
#define TSROSWLASSP3P4S2C "lassp3p4s2c"
#define TSROSWLLSSP3P4S2C "llssp3p4s2c"
#define TSROSWARK3        "ark3"
#define TSROSWTHETA1      "theta1"
#define TSROSWTHETA2      "theta2"
#define TSROSWGRK4T       "grk4t"
#define TSROSWSHAMP4      "shamp4"
#define TSROSWVELDD4      "veldd4"
#define TSROSW4L          "4l"

PETSC_EXTERN PetscErrorCode TSRosWGetType(TS ts,TSRosWType*);
PETSC_EXTERN PetscErrorCode TSRosWSetType(TS ts,TSRosWType);
PETSC_EXTERN PetscErrorCode TSRosWSetRecomputeJacobian(TS,PetscBool);
PETSC_EXTERN PetscErrorCode TSRosWRegister(TSRosWType,PetscInt,PetscInt,const PetscReal[],const PetscReal[],const PetscReal[],const PetscReal[],PetscInt,const PetscReal[]);
PETSC_EXTERN PetscErrorCode TSRosWRegisterRos4(TSRosWType,PetscReal,PetscReal,PetscReal,PetscReal,PetscReal);
PETSC_EXTERN PetscErrorCode TSRosWFinalizePackage(void);
PETSC_EXTERN PetscErrorCode TSRosWInitializePackage(void);
PETSC_EXTERN PetscErrorCode TSRosWRegisterDestroy(void);

/*
       PETSc interface to Sundials
*/
#ifdef PETSC_HAVE_SUNDIALS
typedef enum { SUNDIALS_ADAMS=1,SUNDIALS_BDF=2} TSSundialsLmmType;
PETSC_EXTERN const char *const TSSundialsLmmTypes[];
typedef enum { SUNDIALS_MODIFIED_GS = 1,SUNDIALS_CLASSICAL_GS = 2 } TSSundialsGramSchmidtType;
PETSC_EXTERN const char *const TSSundialsGramSchmidtTypes[];
PETSC_EXTERN PetscErrorCode TSSundialsSetType(TS,TSSundialsLmmType);
PETSC_EXTERN PetscErrorCode TSSundialsGetPC(TS,PC*);
PETSC_EXTERN PetscErrorCode TSSundialsSetTolerance(TS,PetscReal,PetscReal);
PETSC_EXTERN PetscErrorCode TSSundialsSetMinTimeStep(TS,PetscReal);
PETSC_EXTERN PetscErrorCode TSSundialsSetMaxTimeStep(TS,PetscReal);
PETSC_EXTERN PetscErrorCode TSSundialsGetIterations(TS,PetscInt *,PetscInt *);
PETSC_EXTERN PetscErrorCode TSSundialsSetGramSchmidtType(TS,TSSundialsGramSchmidtType);
PETSC_EXTERN PetscErrorCode TSSundialsSetGMRESRestart(TS,PetscInt);
PETSC_EXTERN PetscErrorCode TSSundialsSetLinearTolerance(TS,PetscReal);
PETSC_EXTERN PetscErrorCode TSSundialsMonitorInternalSteps(TS,PetscBool );
PETSC_EXTERN PetscErrorCode TSSundialsGetParameters(TS,PetscInt *,long*[],double*[]);
PETSC_EXTERN PetscErrorCode TSSundialsSetMaxl(TS,PetscInt);
#endif

PETSC_EXTERN PetscErrorCode TSThetaSetTheta(TS,PetscReal);
PETSC_EXTERN PetscErrorCode TSThetaGetTheta(TS,PetscReal*);
PETSC_EXTERN PetscErrorCode TSThetaGetEndpoint(TS,PetscBool*);
PETSC_EXTERN PetscErrorCode TSThetaSetEndpoint(TS,PetscBool);

PETSC_EXTERN PetscErrorCode TSAlphaSetAdapt(TS,PetscErrorCode(*)(TS,PetscReal,Vec,Vec,PetscReal*,PetscBool*,void*),void*);
PETSC_EXTERN PetscErrorCode TSAlphaAdaptDefault(TS,PetscReal,Vec,Vec,PetscReal*,PetscBool*,void*);
PETSC_EXTERN PetscErrorCode TSAlphaSetRadius(TS,PetscReal);
PETSC_EXTERN PetscErrorCode TSAlphaSetParams(TS,PetscReal,PetscReal,PetscReal);
PETSC_EXTERN PetscErrorCode TSAlphaGetParams(TS,PetscReal*,PetscReal*,PetscReal*);

PETSC_EXTERN PetscErrorCode TSSetDM(TS,DM);
PETSC_EXTERN PetscErrorCode TSGetDM(TS,DM*);

PETSC_EXTERN PetscErrorCode SNESTSFormFunction(SNES,Vec,Vec,void*);
PETSC_EXTERN PetscErrorCode SNESTSFormJacobian(SNES,Vec,Mat,Mat,void*);

#endif
