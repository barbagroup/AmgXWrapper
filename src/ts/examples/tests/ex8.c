static char help[] = "Solves DAE with integrator only on non-algebraic terms \n";

#include <petscts.h>

/*
        \dot{U} = f(U,V)
        F(U,V)  = 0

    Same as ex6.c and ex7.c except calls the ARKIMEX integrator on the entire DAE
*/


#undef __FUNCT__
#define __FUNCT__ "f"
/*
   f(U,V) = U + V

*/
PetscErrorCode f(PetscReal t,Vec UV,Vec F)
{
  PetscErrorCode    ierr;
  const PetscScalar *u,*v;
  PetscScalar       *f;
  PetscInt          n,i;

  PetscFunctionBeginUser;
  ierr = VecGetLocalSize(UV,&n);CHKERRQ(ierr);
  n    = n/2;
  ierr = VecGetArrayRead(UV,&u);CHKERRQ(ierr);
  v    = u + n;
  ierr = VecGetArray(F,&f);CHKERRQ(ierr);
  for (i=0; i<n; i++) f[i] = u[i] + v[i];
  ierr = VecRestoreArrayRead(UV,&u);CHKERRQ(ierr);
  ierr = VecRestoreArray(F,&f);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "F"
/*
   F(U,V) = U - V

*/
PetscErrorCode F(PetscReal t,Vec UV,Vec F)
{
  PetscErrorCode    ierr;
  const PetscScalar *u,*v;
  PetscScalar       *f;
  PetscInt          n,i;

  PetscFunctionBeginUser;
  ierr = VecGetLocalSize(UV,&n);CHKERRQ(ierr);
  n    = n/2;
  ierr = VecGetArrayRead(UV,&u);CHKERRQ(ierr);
  v    = u + n;
  ierr = VecGetArray(F,&f);CHKERRQ(ierr);
  f    = f + n;
  for (i=0; i<n; i++) f[i] = u[i] - v[i];
  f    = f - n;
  ierr = VecRestoreArrayRead(UV,&u);CHKERRQ(ierr);
  ierr = VecRestoreArray(F,&f);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

typedef struct {
  PetscErrorCode (*f)(PetscReal,Vec,Vec);
  PetscErrorCode (*F)(PetscReal,Vec,Vec);
} AppCtx;

extern PetscErrorCode TSFunctionRHS(TS,PetscReal,Vec,Vec,void*);
extern PetscErrorCode TSFunctionI(TS,PetscReal,Vec,Vec,Vec,void*);

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **argv)
{
  PetscErrorCode ierr;
  AppCtx         ctx;
  TS             ts;
  Vec            tsrhs,UV;

  PetscInitialize(&argc,&argv,(char*)0,help);
  ierr = TSCreate(PETSC_COMM_WORLD,&ts);CHKERRQ(ierr);
  ierr = TSSetProblemType(ts,TS_NONLINEAR);CHKERRQ(ierr);
  ierr = TSSetType(ts,TSROSW);CHKERRQ(ierr);
  ierr = TSSetFromOptions(ts);CHKERRQ(ierr);
  ierr = VecCreateMPI(PETSC_COMM_WORLD,2,PETSC_DETERMINE,&tsrhs);CHKERRQ(ierr);
  ierr = VecCreateMPI(PETSC_COMM_WORLD,2,PETSC_DETERMINE,&UV);CHKERRQ(ierr);
  ierr = TSSetRHSFunction(ts,tsrhs,TSFunctionRHS,&ctx);CHKERRQ(ierr);
  ierr = TSSetIFunction(ts,NULL,TSFunctionI,&ctx);CHKERRQ(ierr);
  ctx.f = f;
  ctx.F = F;

  ierr = VecSet(UV,1.0);CHKERRQ(ierr);
  ierr = TSSolve(ts,UV);CHKERRQ(ierr);
  ierr = VecDestroy(&tsrhs);CHKERRQ(ierr);
  ierr = VecDestroy(&UV);CHKERRQ(ierr);
  ierr = TSDestroy(&ts);CHKERRQ(ierr);
  PetscFinalize();
  return 0;
}

#undef __FUNCT__
#define __FUNCT__ "TSFunctionRHS"
/*
   Defines the RHS function that is passed to the time-integrator. 



*/
PetscErrorCode TSFunctionRHS(TS ts,PetscReal t,Vec UV,Vec F,void *actx)
{
  AppCtx         *ctx = (AppCtx*)actx;
  PetscErrorCode ierr;

  PetscFunctionBeginUser;
  ierr = VecSet(F,0.0);CHKERRQ(ierr);
  ierr = (*ctx->f)(t,UV,F);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "TSFunctionI"
/*
   Defines the nonlinear function that is passed to the time-integrator

*/
PetscErrorCode TSFunctionI(TS ts,PetscReal t,Vec UV,Vec UVdot,Vec F,void *actx)
{
  AppCtx         *ctx = (AppCtx*)actx;
  PetscErrorCode ierr;

  PetscFunctionBeginUser;
  ierr = VecCopy(UVdot,F);CHKERRQ(ierr);
  ierr = (*ctx->F)(t,UV,F);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}


