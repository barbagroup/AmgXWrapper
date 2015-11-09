
static char help[] = "Basic equation for generator stability analysis.\n";

/*F

\begin{eqnarray}
                 \frac{d \theta}{dt} = \omega_b (\omega - \omega_s)
                 \frac{2 H}{\omega_s}\frac{d \omega}{dt} & = & P_m - P_max \sin(\theta) -D(\omega - \omega_s)\\
\end{eqnarray}



  Ensemble of initial conditions
   ./ex2 -ensemble -ts_monitor_draw_solution_phase -1,-3,3,3      -ts_adapt_dt_max .01  -ts_monitor -ts_type rosw -pc_type lu -ksp_type preonly

  Fault at .1 seconds
   ./ex2           -ts_monitor_draw_solution_phase .42,.95,.6,1.05 -ts_adapt_dt_max .01  -ts_monitor -ts_type rosw -pc_type lu -ksp_type preonly

  Initial conditions same as when fault is ended
   ./ex2 -u 0.496792,1.00932 -ts_monitor_draw_solution_phase .42,.95,.6,1.05  -ts_adapt_dt_max .01  -ts_monitor -ts_type rosw -pc_type lu -ksp_type preonly 


F*/

/*
   Include "petscts.h" so that we can use TS solvers.  Note that this
   file automatically includes:
     petscsys.h       - base PETSc routines   petscvec.h - vectors
     petscmat.h - matrices
     petscis.h     - index sets            petscksp.h - Krylov subspace methods
     petscviewer.h - viewers               petscpc.h  - preconditioners
     petscksp.h   - linear solvers
*/
#include <petscts.h>

typedef struct {
  PetscScalar H,D,omega_b,omega_s,Pmax,Pmax_ini,Pm,E,V,X;
  PetscReal   tf,tcl;
} AppCtx;

/* Event check */
#undef __FUNCT__
#define __FUNCT__ "EventFunction"
PetscErrorCode EventFunction(TS ts,PetscReal t,Vec X,PetscScalar *fvalue,void *ctx)
{
  AppCtx        *user=(AppCtx*)ctx;

  PetscFunctionBegin;
  /* Event for fault-on time */
  fvalue[0] = t - user->tf;
  /* Event for fault-off time */
  fvalue[1] = t - user->tcl;

  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PostEventFunction"
PetscErrorCode PostEventFunction(TS ts,PetscInt nevents,PetscInt event_list[],PetscReal t,Vec X,PetscBool forwardsolve,void* ctx)
{
  AppCtx *user=(AppCtx*)ctx;
  
  PetscFunctionBegin;

  if (event_list[0] == 0) user->Pmax = 0.0; /* Apply disturbance - this is done by setting Pmax = 0 */
  else if(event_list[0] == 1) user->Pmax = user->Pmax_ini; /* Remove the fault  - this is done by setting Pmax = Pmax_ini */
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "IFunction"
/*
     Defines the ODE passed to the ODE solver
*/
static PetscErrorCode IFunction(TS ts,PetscReal t,Vec U,Vec Udot,Vec F,AppCtx *ctx)
{
  PetscErrorCode    ierr;
  const PetscScalar *u,*udot;
  PetscScalar       *f,Pmax;

  PetscFunctionBegin;
  /*  The next three lines allow us to access the entries of the vectors directly */
  ierr = VecGetArrayRead(U,&u);CHKERRQ(ierr);
  ierr = VecGetArrayRead(Udot,&udot);CHKERRQ(ierr);
  ierr = VecGetArray(F,&f);CHKERRQ(ierr);
  Pmax = ctx->Pmax;

  f[0] = udot[0] - ctx->omega_b*(u[1] - ctx->omega_s);
  f[1] = 2.0*ctx->H/ctx->omega_s*udot[1] +  Pmax*PetscSinScalar(u[0]) + ctx->D*(u[1] - ctx->omega_s)- ctx->Pm;

  ierr = VecRestoreArrayRead(U,&u);CHKERRQ(ierr);
  ierr = VecRestoreArrayRead(Udot,&udot);CHKERRQ(ierr);
  ierr = VecRestoreArray(F,&f);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "IJacobian"
/*
     Defines the Jacobian of the ODE passed to the ODE solver. See TSSetIJacobian() for the meaning of a and the Jacobian.
*/
static PetscErrorCode IJacobian(TS ts,PetscReal t,Vec U,Vec Udot,PetscReal a,Mat A,Mat B,AppCtx *ctx)
{
  PetscErrorCode    ierr;
  PetscInt          rowcol[] = {0,1};
  PetscScalar       J[2][2],Pmax;
  const PetscScalar *u,*udot;

  PetscFunctionBegin;
  ierr = VecGetArrayRead(U,&u);CHKERRQ(ierr);
  ierr = VecGetArrayRead(Udot,&udot);CHKERRQ(ierr);
  Pmax = ctx->Pmax;

  J[0][0] = a;                                    J[0][1] = -ctx->omega_b;
  J[1][1] = 2.0*ctx->H/ctx->omega_s*a + ctx->D;   J[1][0] = Pmax*PetscCosScalar(u[0]);

  ierr    = MatSetValues(B,2,rowcol,2,rowcol,&J[0][0],INSERT_VALUES);CHKERRQ(ierr);
  ierr    = VecRestoreArrayRead(U,&u);CHKERRQ(ierr);
  ierr    = VecRestoreArrayRead(Udot,&udot);CHKERRQ(ierr);

  ierr = MatAssemblyBegin(A,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(A,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  if (A != B) {
    ierr = MatAssemblyBegin(B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
    ierr = MatAssemblyEnd(B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **argv)
{
  TS             ts;            /* ODE integrator */
  Vec            U;             /* solution will be stored here */
  Mat            A;             /* Jacobian matrix */
  PetscErrorCode ierr;
  PetscMPIInt    size;
  PetscInt       n = 2;
  AppCtx         ctx;
  PetscScalar    *u;
  PetscReal      du[2] = {0.0,0.0};
  PetscBool      ensemble = PETSC_FALSE,flg1,flg2;
  PetscInt       direction[2];
  PetscBool      terminate[2];

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Initialize program
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = PetscInitialize(&argc,&argv,(char*)0,help);CHKERRQ(ierr);
  ierr = MPI_Comm_size(PETSC_COMM_WORLD,&size);CHKERRQ(ierr);
  if (size > 1) SETERRQ(PETSC_COMM_WORLD,PETSC_ERR_SUP,"Only for sequential runs");

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Create necessary matrix and vectors
    - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = MatCreate(PETSC_COMM_WORLD,&A);CHKERRQ(ierr);
  ierr = MatSetSizes(A,n,n,PETSC_DETERMINE,PETSC_DETERMINE);CHKERRQ(ierr);
  ierr = MatSetType(A,MATDENSE);CHKERRQ(ierr);
  ierr = MatSetFromOptions(A);CHKERRQ(ierr);
  ierr = MatSetUp(A);CHKERRQ(ierr);

  ierr = MatCreateVecs(A,&U,NULL);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Set runtime options
    - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = PetscOptionsBegin(PETSC_COMM_WORLD,NULL,"Swing equation options","");CHKERRQ(ierr);
  {
    ctx.omega_b = 1.0;
    ctx.omega_s = 2.0*PETSC_PI*60.0;
    ctx.H       = 5.0;
    ierr        = PetscOptionsScalar("-Inertia","","",ctx.H,&ctx.H,NULL);CHKERRQ(ierr);
    ctx.D       = 5.0;
    ierr        = PetscOptionsScalar("-D","","",ctx.D,&ctx.D,NULL);CHKERRQ(ierr);
    ctx.E       = 1.1378;
    ctx.V       = 1.0;
    ctx.X       = 0.545;
    ctx.Pmax    = ctx.E*ctx.V/ctx.X;
    ctx.Pmax_ini = ctx.Pmax;
    ierr        = PetscOptionsScalar("-Pmax","","",ctx.Pmax,&ctx.Pmax,NULL);CHKERRQ(ierr);
    ctx.Pm      = 0.9;
    ierr        = PetscOptionsScalar("-Pm","","",ctx.Pm,&ctx.Pm,NULL);CHKERRQ(ierr);
    ctx.tf      = 1.0;
    ctx.tcl     = 1.05;
    ierr        = PetscOptionsReal("-tf","Time to start fault","",ctx.tf,&ctx.tf,NULL);CHKERRQ(ierr);
    ierr        = PetscOptionsReal("-tcl","Time to end fault","",ctx.tcl,&ctx.tcl,NULL);CHKERRQ(ierr);
    ierr        = PetscOptionsBool("-ensemble","Run ensemble of different initial conditions","",ensemble,&ensemble,NULL);CHKERRQ(ierr);
    if (ensemble) {
      ctx.tf      = -1;
      ctx.tcl     = -1;
    }

    ierr = VecGetArray(U,&u);CHKERRQ(ierr);
    u[0] = PetscAsinScalar(ctx.Pm/ctx.Pmax);
    u[1] = 1.0;
    ierr = PetscOptionsRealArray("-u","Initial solution","",u,&n,&flg1);CHKERRQ(ierr);
    n    = 2;
    ierr = PetscOptionsRealArray("-du","Perturbation in initial solution","",du,&n,&flg2);CHKERRQ(ierr);
    u[0] += du[0];
    u[1] += du[1];
    ierr = VecRestoreArray(U,&u);CHKERRQ(ierr);
    if (flg1 || flg2) {
      ctx.tf      = -1;
      ctx.tcl     = -1;
    }
  }
  ierr = PetscOptionsEnd();CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Create timestepping solver context
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = TSCreate(PETSC_COMM_WORLD,&ts);CHKERRQ(ierr);
  ierr = TSSetProblemType(ts,TS_NONLINEAR);CHKERRQ(ierr);
  ierr = TSSetType(ts,TSTHETA);CHKERRQ(ierr);
  ierr = TSSetEquationType(ts,TS_EQ_IMPLICIT);CHKERRQ(ierr);
  ierr = TSARKIMEXSetFullyImplicit(ts,PETSC_TRUE);CHKERRQ(ierr);
  ierr = TSSetIFunction(ts,NULL,(TSIFunction) IFunction,&ctx);CHKERRQ(ierr);
  ierr = TSSetIJacobian(ts,A,A,(TSIJacobian)IJacobian,&ctx);CHKERRQ(ierr);
  ierr = TSSetExactFinalTime(ts,TS_EXACTFINALTIME_MATCHSTEP);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Set initial conditions
   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = TSSetSolution(ts,U);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Set solver options
   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = TSSetDuration(ts,100000,35.0);CHKERRQ(ierr);
  ierr = TSSetInitialTimeStep(ts,0.0,.01);CHKERRQ(ierr);
  ierr = TSSetFromOptions(ts);CHKERRQ(ierr);

  direction[0] = direction[1] = 1;
  terminate[0] = terminate[1] = PETSC_FALSE;

  ierr = TSSetEventMonitor(ts,2,direction,terminate,EventFunction,PostEventFunction,(void*)&ctx);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Solve nonlinear system
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  if (ensemble) {
    for (du[1] = -2.5; du[1] <= .01; du[1] += .1) {
      ierr = VecGetArray(U,&u);CHKERRQ(ierr);
      u[0] = PetscAsinScalar(ctx.Pm/ctx.Pmax);
      u[1] = ctx.omega_s;
      u[0] += du[0];
      u[1] += du[1];
      ierr = VecRestoreArray(U,&u);CHKERRQ(ierr);
      ierr = TSSetInitialTimeStep(ts,0.0,.01);CHKERRQ(ierr);
      ierr = TSSolve(ts,U);CHKERRQ(ierr);
    }
  } else {
    ierr = TSSolve(ts,U);CHKERRQ(ierr);
  }
  ierr = VecView(U,PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);
  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Free work space.  All PETSc objects should be destroyed when they are no longer needed.
   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = MatDestroy(&A);CHKERRQ(ierr);
  ierr = VecDestroy(&U);CHKERRQ(ierr);
  ierr = TSDestroy(&ts);CHKERRQ(ierr);

  ierr = PetscFinalize();
  return(0);
}
