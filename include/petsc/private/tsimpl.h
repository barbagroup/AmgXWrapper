
#ifndef __TSIMPL_H
#define __TSIMPL_H

#include <petscts.h>
#include <petsc/private/petscimpl.h>

/*
    Timesteping context.
      General DAE: F(t,U,U_t) = 0, required Jacobian is G'(U) where G(U) = F(t,U,U0+a*U)
      General ODE: U_t = F(t,U) <-- the right-hand-side function
      Linear  ODE: U_t = A(t) U <-- the right-hand-side matrix
      Linear (no time) ODE: U_t = A U <-- the right-hand-side matrix
*/

/*
     Maximum number of monitors you can run with a single TS
*/
#define MAXTSMONITORS 10

PETSC_EXTERN PetscBool TSRegisterAllCalled;
PETSC_EXTERN PetscErrorCode TSRegisterAll(void);

typedef struct _TSOps *TSOps;

struct _TSOps {
  PetscErrorCode (*snesfunction)(SNES,Vec,Vec,TS);
  PetscErrorCode (*snesjacobian)(SNES,Vec,Mat,Mat,TS);
  PetscErrorCode (*setup)(TS);
  PetscErrorCode (*step)(TS);
  PetscErrorCode (*solve)(TS);
  PetscErrorCode (*interpolate)(TS,PetscReal,Vec);
  PetscErrorCode (*evaluatestep)(TS,PetscInt,Vec,PetscBool*);
  PetscErrorCode (*setfromoptions)(PetscOptionItems*,TS);
  PetscErrorCode (*destroy)(TS);
  PetscErrorCode (*view)(TS,PetscViewer);
  PetscErrorCode (*reset)(TS);
  PetscErrorCode (*linearstability)(TS,PetscReal,PetscReal,PetscReal*,PetscReal*);
  PetscErrorCode (*load)(TS,PetscViewer);
  PetscErrorCode (*rollback)(TS);
  PetscErrorCode (*getstages)(TS,PetscInt*,Vec**);
  PetscErrorCode (*adjointstep)(TS);
  PetscErrorCode (*adjointsetup)(TS);
};

/* 
   TSEvent - Abstract object to handle event monitoring
*/
typedef struct _p_TSEvent *TSEvent;

typedef struct _TSTrajectoryOps *TSTrajectoryOps;

struct _TSTrajectoryOps {
  PetscErrorCode (*view)(TSTrajectory,PetscViewer);
  PetscErrorCode (*destroy)(TSTrajectory);
  PetscErrorCode (*set)(TSTrajectory,TS,PetscInt,PetscReal,Vec);
  PetscErrorCode (*get)(TSTrajectory,TS,PetscInt,PetscReal*);
  PetscErrorCode (*setfromoptions)(PetscOptionItems*,TSTrajectory);
  PetscErrorCode (*setup)(TSTrajectory,TS);
};

struct _p_TSTrajectory {
  PETSCHEADER(struct _TSTrajectoryOps);
  PetscInt setupcalled;             /* true if setup has been called */
  PetscInt recomps;                 /* counter for recomputations in the adjoint run */
  void *data;
};

struct _p_TS {
  PETSCHEADER(struct _TSOps);
  DM            dm;
  TSProblemType problem_type;
  Vec           vec_sol;
  TSAdapt       adapt;
  TSEvent       event;

  /* ---------------- User (or PETSc) Provided stuff ---------------------*/
  PetscErrorCode (*monitor[MAXTSMONITORS])(TS,PetscInt,PetscReal,Vec,void*); /* returns control to user after */
  PetscErrorCode (*monitordestroy[MAXTSMONITORS])(void**);
  void *monitorcontext[MAXTSMONITORS];                 /* residual calculation, allows user */
  PetscInt  numbermonitors;                                 /* to, for instance, print residual norm, etc. */
  PetscErrorCode (*adjointmonitor[MAXTSMONITORS])(TS,PetscInt,PetscReal,Vec,PetscInt,Vec*,Vec*,void*);
  PetscErrorCode (*adjointmonitordestroy[MAXTSMONITORS])(void**);
  void *adjointmonitorcontext[MAXTSMONITORS];
  PetscInt  numberadjointmonitors;

  PetscErrorCode (*prestep)(TS);
  PetscErrorCode (*prestage)(TS,PetscReal);
  PetscErrorCode (*poststage)(TS,PetscReal,PetscInt,Vec*);
  PetscErrorCode (*poststep)(TS);
  PetscErrorCode (*functiondomainerror)(TS,PetscReal,Vec,PetscBool*);

  /* ---------------------- Sensitivity Analysis support -----------------*/
  TSTrajectory trajectory;   /* All solutions are kept here for the entire time integration process */
  Vec       *vecs_sensi;             /* one vector for each cost function */
  Vec       *vecs_sensip;
  PetscInt  numcost;                 /* number of cost functions */
  Vec       vec_costintegral;
  PetscInt  adjointsetupcalled;
  PetscInt  adjoint_max_steps;
  PetscBool adjoint_solve;          /* immediately call TSAdjointSolve() after TSSolve() is complete */
  PetscBool costintegralfwd;        /* cost integral is evaluated in the forward run if true */
  /* workspace for Adjoint computations */
  Vec       vec_costintegrand;
  Mat       Jacp;
  void      *rhsjacobianpctx;
  void      *costintegrandctx;
  Vec       *vecs_drdy;
  Vec       *vecs_drdp;

  PetscErrorCode (*rhsjacobianp)(TS,PetscReal,Vec,Mat,void*);
  PetscErrorCode (*costintegrand)(TS,PetscReal,Vec,Vec,void*);
  PetscErrorCode (*drdyfunction)(TS,PetscReal,Vec,Vec*,void*);
  PetscErrorCode (*drdpfunction)(TS,PetscReal,Vec,Vec*,void*);

  /* ---------------------- IMEX support ---------------------------------*/
  /* These extra slots are only used when the user provides both Implicit and RHS */
  Mat Arhs;     /* Right hand side matrix */
  Mat Brhs;     /* Right hand side preconditioning matrix */
  Vec Frhs;     /* Right hand side function value */

  /* This is a general caching scheme to avoid recomputing the Jacobian at a place that has been previously been evaluated.
   * The present use case is that TSComputeRHSFunctionLinear() evaluates the Jacobian once and we don't want it to be immeditely re-evaluated.
   */
  struct {
    PetscReal time;             /* The time at which the matrices were last evaluated */
    Vec X;                      /* Solution vector at which the Jacobian was last evaluated */
    PetscObjectState Xstate;    /* State of the solution vector */
    MatStructure mstructure;    /* The structure returned */
    /* Flag to unshift Jacobian before calling the IJacobian or RHSJacobian functions.  This is useful
     * if the user would like to reuse (part of) the Jacobian from the last evaluation. */
    PetscBool reuse;
    PetscReal scale,shift;
  } rhsjacobian;

  struct {
    PetscReal shift;            /* The derivative of the lhs wrt to Xdot */
  } ijacobian;

  /* ---------------------Nonlinear Iteration------------------------------*/
  SNES  snes;

  /* --- Data that is unique to each particular solver --- */
  PetscInt setupcalled;             /* true if setup has been called */
  void     *data;                   /* implementationspecific data */
  void     *user;                   /* user context */

  /* ------------------  Parameters -------------------------------------- */
  PetscInt  max_steps;              /* max number of steps */
  PetscReal max_time;               /* max time allowed */
  PetscReal time_step;              /* current/completed time increment */
  PetscReal time_step_prev;         /* previous time step  */

  /*
     these are temporary to support increasing the time step if nonlinear solver convergence remains good
     and time_step was previously cut due to failed nonlinear solver
  */
  PetscReal time_step_orig;            /* original time step requested by user */
  PetscInt  time_steps_since_decrease; /* number of timesteps since timestep was decreased due to lack of convergence */
  /* ----------------------------------------------------------------------------------------------------------------*/

  PetscBool steprollback;           /* Is the current step rolled back? */
  PetscInt  steps;                  /* steps taken so far in latest call to TSSolve() */
  PetscInt  total_steps;            /* steps taken in all calls to TSSolve() since the TS was created or since TSSetUp() was called */
  PetscReal ptime;                  /* time at the start of the current step (stage time is internal if it exists) */
  PetscReal ptime_prev;             /* time at the start of the previous step */
  PetscReal solvetime;              /* time at the conclusion of TSSolve() */
  PetscInt  ksp_its;                /* total number of linear solver iterations */
  PetscInt  snes_its;               /* total number of nonlinear solver iterations */

  PetscInt num_snes_failures;
  PetscInt max_snes_failures;
  TSConvergedReason reason;
  TSEquationType equation_type;
  PetscBool errorifstepfailed;
  TSExactFinalTimeOption  exact_final_time;
  PetscBool retain_stages;
  PetscInt reject,max_reject;

  PetscReal atol,rtol;          /* Relative and absolute tolerance for local truncation error */
  Vec       vatol,vrtol;        /* Relative and absolute tolerance in vector form */
  PetscReal cfltime,cfltime_local;

  /* ------------------- Default work-area management ------------------ */
  PetscInt nwork;
  Vec      *work;
};

struct _TSAdaptOps {
  PetscErrorCode (*choose)(TSAdapt,TS,PetscReal,PetscInt*,PetscReal*,PetscBool*,PetscReal*);
  PetscErrorCode (*destroy)(TSAdapt);
  PetscErrorCode (*reset)(TSAdapt);
  PetscErrorCode (*view)(TSAdapt,PetscViewer);
  PetscErrorCode (*setfromoptions)(PetscOptionItems*,TSAdapt);
  PetscErrorCode (*load)(TSAdapt,PetscViewer);
};

struct _p_TSAdapt {
  PETSCHEADER(struct _TSAdaptOps);
  void *data;
  PetscErrorCode (*checkstage)(TSAdapt,TS,PetscReal,Vec,PetscBool*);
  struct {
    PetscInt   n;                /* number of candidate schemes, including the one currently in use */
    PetscBool  inuse_set;        /* the current scheme has been set */
    const char *name[16];        /* name of the scheme */
    PetscInt   order[16];        /* classical order of each scheme */
    PetscInt   stageorder[16];   /* stage order of each scheme */
    PetscReal  ccfl[16];         /* stability limit relative to explicit Euler */
    PetscReal  cost[16];         /* relative measure of the amount of work required for each scheme */
  } candidates;
  PetscReal   dt_min,dt_max;
  PetscReal   scale_solve_failed; /* Scale step by this factor if solver (linear or nonlinear) fails. */
  PetscViewer monitor;
  NormType    wnormtype;
};

typedef struct _p_DMTS *DMTS;
typedef struct _DMTSOps *DMTSOps;
struct _DMTSOps {
  TSRHSFunction rhsfunction;
  TSRHSJacobian rhsjacobian;

  TSIFunction ifunction;
  PetscErrorCode (*ifunctionview)(void*,PetscViewer);
  PetscErrorCode (*ifunctionload)(void**,PetscViewer);

  TSIJacobian ijacobian;
  PetscErrorCode (*ijacobianview)(void*,PetscViewer);
  PetscErrorCode (*ijacobianload)(void**,PetscViewer);

  TSSolutionFunction solution;
  PetscErrorCode (*forcing)(TS,PetscReal,Vec,void*);

  PetscErrorCode (*destroy)(DMTS);
  PetscErrorCode (*duplicate)(DMTS,DMTS);
};

struct _p_DMTS {
  PETSCHEADER(struct _DMTSOps);
  void *rhsfunctionctx;
  void *rhsjacobianctx;

  void *ifunctionctx;
  void *ijacobianctx;

  void *solutionctx;
  void *forcingctx;

  void *data;

  /* This is NOT reference counted. The DM on which this context was first created is cached here to implement one-way
   * copy-on-write. When DMGetDMTSWrite() sees a request using a different DM, it makes a copy. Thus, if a user
   * only interacts directly with one level, e.g., using TSSetIFunction(), then coarse levels of a multilevel item
   * integrator are built, then the user changes the routine with another call to TSSetIFunction(), it automatically
   * propagates to all the levels. If instead, they get out a specific level and set the function on that level,
   * subsequent changes to the original level will no longer propagate to that level.
   */
  DM originaldm;
};

PETSC_EXTERN PetscErrorCode DMGetDMTS(DM,DMTS*);
PETSC_EXTERN PetscErrorCode DMGetDMTSWrite(DM,DMTS*);
PETSC_EXTERN PetscErrorCode DMCopyDMTS(DM,DM);
PETSC_EXTERN PetscErrorCode DMTSView(DMTS,PetscViewer);
PETSC_EXTERN PetscErrorCode DMTSLoad(DMTS,PetscViewer);

typedef enum {TSEVENT_NONE,TSEVENT_LOCATED_INTERVAL,TSEVENT_PROCESSING,TSEVENT_ZERO,TSEVENT_RESET_NEXTSTEP} TSEventStatus;

/* Maximum number of event times that can be recorded */
#define MAXEVENTRECORDERS 24

struct _p_TSEvent {
  PetscScalar    *fvalue;          /* value of event function at the end of the step*/
  PetscScalar    *fvalue_prev;     /* value of event function at start of the step */
  PetscReal       ptime;           /* time at step end */
  PetscReal       ptime_prev;      /* time at step start */
  PetscErrorCode  (*monitor)(TS,PetscReal,Vec,PetscScalar*,void*); /* User event monitor function */
  PetscErrorCode  (*postevent)(TS,PetscInt,PetscInt[],PetscReal,Vec,PetscBool,void*); /* User post event function */
  PetscBool      *terminate;        /* 1 -> Terminate time stepping, 0 -> continue */
  PetscInt       *direction;        /* Zero crossing direction: 1 -> Going positive, -1 -> Going negative, 0 -> Any */ 
  PetscInt        nevents;          /* Number of events to handle */
  PetscInt        nevents_zero;     /* Number of event zero detected */
  PetscInt        *events_zero;      /* List of events that have reached zero */
  void           *monitorcontext;
  PetscReal      *vtol;             /* Vector tolerances for event zero check */
  TSEventStatus   status;           /* Event status */
  PetscReal       tstepend;         /* End time of step */
  PetscReal       initial_timestep; /* Initial time step */
  PetscViewer     mon;
  /* Struct to record the events */
  struct {
    PetscInt  ctr;                          /* recorder counter */
    PetscReal time[MAXEVENTRECORDERS];      /* Event times */
    PetscInt  stepnum[MAXEVENTRECORDERS];   /* Step numbers */
    PetscInt  nevents[MAXEVENTRECORDERS];   /* Number of events occuring at the event times */
    PetscInt  *eventidx[MAXEVENTRECORDERS]; /* Local indices of the events in the event list */
  } recorder;
};

PETSC_EXTERN PetscErrorCode TSEventMonitor(TS);
PETSC_EXTERN PetscErrorCode TSEventMonitorDestroy(TSEvent*);
PETSC_EXTERN PetscErrorCode TSAdjointEventMonitor(TS);
PETSC_EXTERN PetscErrorCode TSEventMonitorInitialize(TS);

PETSC_EXTERN PetscLogEvent TS_AdjointStep, TS_Step, TS_PseudoComputeTimeStep, TS_FunctionEval, TS_JacobianEval;

typedef enum {TS_STEP_INCOMPLETE, /* vec_sol, ptime, etc point to beginning of step */
              TS_STEP_PENDING,    /* vec_sol advanced, but step has not been accepted yet */
              TS_STEP_COMPLETE    /* step accepted and ptime, steps, etc have been advanced */
} TSStepStatus;

struct _n_TSMonitorLGCtx {
  PetscDrawLG    lg;
  PetscInt       howoften;  /* when > 0 uses step % howoften, when negative only final solution plotted */
  PetscInt       ksp_its,snes_its;
  char           **names;
  char           **displaynames;
  PetscInt       ndisplayvariables;
  PetscInt       *displayvariables;
  PetscReal      *displayvalues;
  PetscErrorCode (*transform)(void*,Vec,Vec*);
  PetscErrorCode (*transformdestroy)(void*);
  void           *transformctx;
};

struct _n_TSMonitorEnvelopeCtx {
  Vec max,min;
};

PETSC_EXTERN PetscLogEvent TSTrajectory_Set, TSTrajectory_Get, Disk_Write, Disk_Read;

#endif
