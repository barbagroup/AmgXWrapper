/*
   Second step toward adjoint ERK solver
   Features:
     1. checkpointed data stored in the RK context;
     2. adjoint solver is implemented inside TS;
     3. support arrays of lambda and mu.
*/
static char help[] = "Solves the van der Pol equation.\n\
Input parameters include:\n\
      -mu : stiffness parameter\n\n";

/*
   Concepts: TS^time-dependent nonlinear problems
   Concepts: TS^van der Pol equation
   Processors: 1
*/
/* ------------------------------------------------------------------------

   This program solves the van der Pol equation
       y'' - \mu (1-y^2)*y' + y = 0        (1)
   on the domain 0 <= x <= 1, with the boundary conditions
       y(0) = 2, y'(0) = 0,
   This is a nonlinear equation.

   Notes:
   This code demonstrates the TS solver interface to two variants of
   linear problems, u_t = f(u,t), namely turning (1) into a system of
   first order differential equations,

   [ y' ] = [          z          ]
   [ z' ]   [ \mu (1 - y^2) z - y ]

   which then we can write as a vector equation

   [ u_1' ] = [             u_2           ]  (2)
   [ u_2' ]   [ \mu (1 - u_1^2) u_2 - u_1 ]

   which is now in the desired form of u_t = f(u,t). 
  ------------------------------------------------------------------------- */

#include <petscts.h>
#include <petscmat.h>
typedef struct _n_User *User;
struct _n_User {
  PetscReal mu;
  PetscReal next_output;
  PetscReal tprev;
};

/*
*  User-defined routines
*/
#undef __FUNCT__
#define __FUNCT__ "RHSFunction"
static PetscErrorCode RHSFunction(TS ts,PetscReal t,Vec X,Vec F,void *ctx)
{
  PetscErrorCode    ierr;
  User              user = (User)ctx;
  PetscScalar       *f;
  const PetscScalar *x;

  PetscFunctionBeginUser;
  ierr = VecGetArrayRead(X,&x);CHKERRQ(ierr);
  ierr = VecGetArray(F,&f);CHKERRQ(ierr);
  f[0] = x[1];
  f[1] = user->mu*(1.-x[0]*x[0])*x[1]-x[0];
  ierr = VecRestoreArrayRead(X,&x);CHKERRQ(ierr);
  ierr = VecRestoreArray(F,&f);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "RHSJacobian"
static PetscErrorCode RHSJacobian(TS ts,PetscReal t,Vec X,Mat A,Mat B,void *ctx)
{
  PetscErrorCode    ierr;
  User              user = (User)ctx;
  PetscReal         mu   = user->mu;
  PetscInt          rowcol[] = {0,1};
  PetscScalar       J[2][2];
  const PetscScalar *x;

  PetscFunctionBeginUser;
  ierr = VecGetArrayRead(X,&x);CHKERRQ(ierr);
  J[0][0] = 0;
  J[1][0] = -2.*mu*x[1]*x[0]-1.;
  J[0][1] = 1.0;
  J[1][1] = mu*(1.0-x[0]*x[0]);
  ierr    = MatSetValues(A,2,rowcol,2,rowcol,&J[0][0],INSERT_VALUES);CHKERRQ(ierr);
  ierr = MatAssemblyBegin(A,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(A,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  if (A != B) {
    ierr = MatAssemblyBegin(B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
    ierr = MatAssemblyEnd(B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  }
  ierr = VecRestoreArrayRead(X,&x);CHKERRQ(ierr);  
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "RHSJacobianP"
static PetscErrorCode RHSJacobianP(TS ts,PetscReal t,Vec X,Mat A,void *ctx)
{
  PetscErrorCode    ierr;
  PetscInt          row[] = {0,1},col[]={0};
  PetscScalar       J[2][1];
  const PetscScalar *x;

  PetscFunctionBeginUser;
  ierr = VecGetArrayRead(X,&x);CHKERRQ(ierr);
  J[0][0] = 0;
  J[1][0] = (1.-x[0]*x[0])*x[1];
  ierr    = MatSetValues(A,2,row,1,col,&J[0][0],INSERT_VALUES);CHKERRQ(ierr);
  ierr = MatAssemblyBegin(A,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(A,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = VecRestoreArrayRead(X,&x);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "Monitor"
/* Monitor timesteps and use interpolation to output at integer multiples of 0.1 */
static PetscErrorCode Monitor(TS ts,PetscInt step,PetscReal t,Vec X,void *ctx)
{
  PetscErrorCode    ierr;
  const PetscScalar *x;
  PetscReal         tfinal, dt, tprev;
  User              user = (User)ctx;

  PetscFunctionBeginUser;
  ierr = TSGetTimeStep(ts,&dt);CHKERRQ(ierr);
  ierr = TSGetDuration(ts,NULL,&tfinal);CHKERRQ(ierr);
  ierr = TSGetPrevTime(ts,&tprev);CHKERRQ(ierr);
  ierr = VecGetArrayRead(X,&x);CHKERRQ(ierr);
  ierr = PetscPrintf(PETSC_COMM_WORLD,"[%.1f] %D TS %.6f (dt = %.6f) X % 12.6e % 12.6e\n",(double)user->next_output,step,(double)t,(double)dt,(double)PetscRealPart(x[0]),(double)PetscRealPart(x[1]));CHKERRQ(ierr);
  ierr = PetscPrintf(PETSC_COMM_WORLD,"t %.6f (tprev = %.6f) \n",(double)t,(double)tprev);CHKERRQ(ierr);
  ierr = VecRestoreArrayRead(X,&x);CHKERRQ(ierr);  
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **argv)
{
  TS             ts;            /* nonlinear solver */
  Vec            x;             /* solution, residual vectors */
  Mat            A;             /* Jacobian matrix */
  Mat            Jacp;          /* JacobianP matrix */
  PetscInt       steps;
  PetscReal      ftime   =0.5;
  PetscBool      monitor = PETSC_FALSE;
  PetscScalar    *x_ptr;
  PetscMPIInt    size;
  struct _n_User user;
  PetscErrorCode ierr;
  Vec            lambda[2],mu[2];

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Initialize program
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  PetscInitialize(&argc,&argv,NULL,help);

  ierr = MPI_Comm_size(PETSC_COMM_WORLD,&size);CHKERRQ(ierr);
  if (size != 1) SETERRQ(PETSC_COMM_SELF,1,"This is a uniprocessor example only!");

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Set runtime options
    - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  user.mu          = 1;
  user.next_output = 0.0;


  ierr = PetscOptionsGetReal(NULL,NULL,"-mu",&user.mu,NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetBool(NULL,NULL,"-monitor",&monitor,NULL);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Create necessary matrix and vectors, solve same ODE on every process
    - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = MatCreate(PETSC_COMM_WORLD,&A);CHKERRQ(ierr);
  ierr = MatSetSizes(A,PETSC_DECIDE,PETSC_DECIDE,2,2);CHKERRQ(ierr);
  ierr = MatSetFromOptions(A);CHKERRQ(ierr);
  ierr = MatSetUp(A);CHKERRQ(ierr);
  ierr = MatCreateVecs(A,&x,NULL);CHKERRQ(ierr);

  ierr = MatCreate(PETSC_COMM_WORLD,&Jacp);CHKERRQ(ierr);
  ierr = MatSetSizes(Jacp,PETSC_DECIDE,PETSC_DECIDE,2,1);CHKERRQ(ierr);
  ierr = MatSetFromOptions(Jacp);CHKERRQ(ierr);
  ierr = MatSetUp(Jacp);CHKERRQ(ierr);
 
  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Create timestepping solver context
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = TSCreate(PETSC_COMM_WORLD,&ts);CHKERRQ(ierr);
  ierr = TSSetType(ts,TSRK);CHKERRQ(ierr);
  ierr = TSSetRHSFunction(ts,NULL,RHSFunction,&user);CHKERRQ(ierr);
  ierr = TSSetDuration(ts,PETSC_DEFAULT,ftime);CHKERRQ(ierr);
  ierr = TSSetExactFinalTime(ts,TS_EXACTFINALTIME_MATCHSTEP);CHKERRQ(ierr);
  if (monitor) {
    ierr = TSMonitorSet(ts,Monitor,&user,NULL);CHKERRQ(ierr);
  }
  
  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Set initial conditions
   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = VecGetArray(x,&x_ptr);CHKERRQ(ierr);

  x_ptr[0] = 2;   x_ptr[1] = 0.66666654321;
  ierr = VecRestoreArray(x,&x_ptr);CHKERRQ(ierr);
  ierr = TSSetInitialTimeStep(ts,0.0,.001);CHKERRQ(ierr);

  /*
    Have the TS save its trajectory so that TSAdjointSolve() may be used
  */
  ierr = TSSetSaveTrajectory(ts);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Set runtime options
   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = TSSetFromOptions(ts);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Solve nonlinear system
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = TSSolve(ts,x);CHKERRQ(ierr);
  ierr = TSGetSolveTime(ts,&ftime);CHKERRQ(ierr);
  ierr = TSGetTimeStepNumber(ts,&steps);CHKERRQ(ierr);
  ierr = PetscPrintf(PETSC_COMM_WORLD,"mu %g, steps %D, ftime %g\n",(double)user.mu,steps,(double)ftime);CHKERRQ(ierr);
  ierr = VecView(x,PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Start the Adjoint model
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = MatCreateVecs(A,&lambda[0],NULL);CHKERRQ(ierr);
  ierr = MatCreateVecs(A,&lambda[1],NULL);CHKERRQ(ierr);
  /*   Reset initial conditions for the adjoint integration */
  ierr = VecGetArray(lambda[0],&x_ptr);CHKERRQ(ierr);
  x_ptr[0] = 1.0;   x_ptr[1] = 0.0;
  ierr = VecRestoreArray(lambda[0],&x_ptr);CHKERRQ(ierr);
  ierr = VecGetArray(lambda[1],&x_ptr);CHKERRQ(ierr);
  x_ptr[0] = 0.0;   x_ptr[1] = 1.0;
  ierr = VecRestoreArray(lambda[1],&x_ptr);CHKERRQ(ierr);

  ierr = MatCreateVecs(Jacp,&mu[0],NULL);CHKERRQ(ierr);
  ierr = MatCreateVecs(Jacp,&mu[1],NULL);CHKERRQ(ierr);
  ierr = VecGetArray(mu[0],&x_ptr);CHKERRQ(ierr);
  x_ptr[0] = 0.0;
  ierr = VecRestoreArray(mu[0],&x_ptr);CHKERRQ(ierr);
  ierr = VecGetArray(mu[1],&x_ptr);CHKERRQ(ierr);
  x_ptr[0] = 0.0;
  ierr = VecRestoreArray(mu[1],&x_ptr);CHKERRQ(ierr);
  ierr = TSSetCostGradients(ts,2,lambda,mu);CHKERRQ(ierr);

  /*   Set RHS Jacobian for the adjoint integration */
  ierr = TSSetRHSJacobian(ts,A,A,RHSJacobian,&user);CHKERRQ(ierr);

  /*   Set RHS JacobianP */
  ierr = TSAdjointSetRHSJacobian(ts,Jacp,RHSJacobianP,&user);CHKERRQ(ierr);

  ierr = TSAdjointSolve(ts);CHKERRQ(ierr);

  ierr = VecView(lambda[0],PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);
  ierr = VecView(lambda[1],PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);
  ierr = VecView(mu[0],PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);
  ierr = VecView(mu[1],PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Free work space.  All PETSc objects should be destroyed when they
     are no longer needed.
   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = MatDestroy(&A);CHKERRQ(ierr);
  ierr = MatDestroy(&Jacp);CHKERRQ(ierr);
  ierr = VecDestroy(&x);CHKERRQ(ierr);
  ierr = VecDestroy(&lambda[0]);CHKERRQ(ierr);
  ierr = VecDestroy(&lambda[1]);CHKERRQ(ierr);
  ierr = VecDestroy(&mu[0]);CHKERRQ(ierr);
  ierr = VecDestroy(&mu[1]);CHKERRQ(ierr);
  ierr = TSDestroy(&ts);CHKERRQ(ierr);

  ierr = PetscFinalize();
  PetscFunctionReturn(0);
}
