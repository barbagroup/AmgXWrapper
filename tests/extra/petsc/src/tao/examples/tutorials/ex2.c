static char help[] = "One-Shot Multigrid for Parameter Estimation Problem for the Poisson Equation.\n\
Using the Interior Point Method.\n\n\n";

/*F
  We are solving the parameter estimation problem for the Laplacian. We will ask to minimize a Lagrangian
function over $y$ and $u$, given by
\begin{align}
  L(u, a, \lambda) = \frac{1}{2} || Qu - d_A ||^2 || Qu - d_B ||^2 + \frac{\beta}{2} || L (a - a_r) ||^2 + \lambda F(u; a)
\end{align}
where $Q$ is a sampling operator, $L$ is a regularization operator, $F$ defines the PDE.

Currently, we have perfect information, meaning $Q = I$, and then we need no regularization, $L = I$. We
also give the null vector for the reference control $a_r$. Right now $\beta = 1$.

The PDE will be the Laplace equation with homogeneous boundary conditions
\begin{align}
  -Delta u = a
\end{align}

F*/

#include <petsc.h>
#include <petscfe.h>

typedef enum {RUN_FULL, RUN_TEST} RunType;

typedef struct {
  RunType           runType;        /* Whether to run tests, or solve the full problem */
  PetscBool         useDualPenalty; /* Penalize deviation from both goals */
  PetscErrorCode (**exactFuncs)(PetscInt dim, const PetscReal x[], PetscInt Nf, PetscScalar *u, void *ctx);
} AppCtx;

#undef __FUNCT__
#define __FUNCT__ "ProcessOptions"
static PetscErrorCode ProcessOptions(MPI_Comm comm, AppCtx *options)
{
  const char    *runTypes[2] = {"full", "test"};
  PetscInt       run;
  PetscErrorCode ierr;

  PetscFunctionBeginUser;
  options->runType        = RUN_FULL;
  options->useDualPenalty = PETSC_FALSE;

  ierr = PetscOptionsBegin(comm, "", "Inverse Problem Options", "DMPLEX");CHKERRQ(ierr);
  run  = options->runType;
  ierr = PetscOptionsEList("-run_type", "The run type", "ex2.c", runTypes, 2, runTypes[options->runType], &run, NULL);CHKERRQ(ierr);
  options->runType = (RunType) run;
  ierr = PetscOptionsBool("-use_dual_penalty", "Penalize deviation from both goals", "ex2.c", options->useDualPenalty, &options->useDualPenalty, NULL);CHKERRQ(ierr);
  ierr = PetscOptionsEnd();
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "CreateMesh"
static PetscErrorCode CreateMesh(MPI_Comm comm, AppCtx *user, DM *dm)
{
  DM             distributedMesh = NULL;
  PetscErrorCode ierr;

  PetscFunctionBeginUser;
  ierr = DMPlexCreateBoxMesh(comm, 2, PETSC_TRUE, dm);CHKERRQ(ierr);
  ierr = PetscObjectSetName((PetscObject) *dm, "Mesh");CHKERRQ(ierr);
  ierr = DMPlexDistribute(*dm, 0, NULL, &distributedMesh);CHKERRQ(ierr);
  if (distributedMesh) {
    ierr = DMDestroy(dm);CHKERRQ(ierr);
    *dm  = distributedMesh;
  }
  ierr = DMSetFromOptions(*dm);CHKERRQ(ierr);
  ierr = DMViewFromOptions(*dm, NULL, "-dm_view");CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

void f0_u(PetscInt dim, PetscInt Nf, PetscInt NfAux,
          const PetscInt uOff[], const PetscInt uOff_x[], const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[],
          const PetscInt aOff[], const PetscInt aOff_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[],
          PetscReal t, const PetscReal x[], PetscScalar f0[])
{
  f0[0] = (u[0] - (x[0]*x[0] + x[1]*x[1]));
}
void f0_u_full(PetscInt dim, PetscInt Nf, PetscInt NfAux,
          const PetscInt uOff[], const PetscInt uOff_x[], const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[],
          const PetscInt aOff[], const PetscInt aOff_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[],
          PetscReal t, const PetscReal x[], PetscScalar f0[])
{
  f0[0] = (u[0] - (x[0]*x[0] + x[1]*x[1]))*PetscSqr(u[0] - (sin(2.0*PETSC_PI*x[0]) * sin(2.0*PETSC_PI*x[1]))) +
    PetscSqr(u[0] - (x[0]*x[0] + x[1]*x[1]))*(u[0] - (sin(2.0*PETSC_PI*x[0]) * sin(2.0*PETSC_PI*x[1])));
}
void f1_u(PetscInt dim, PetscInt Nf, PetscInt NfAux,
          const PetscInt uOff[], const PetscInt uOff_x[], const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[],
          const PetscInt aOff[], const PetscInt aOff_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[],
          PetscReal t, const PetscReal x[], PetscScalar f1[])
{
  PetscInt d;
  for (d = 0; d < dim; ++d) f1[d] = u_x[dim*2+d];
}
void g0_uu(PetscInt dim, PetscInt Nf, PetscInt NfAux,
           const PetscInt uOff[], const PetscInt uOff_x[], const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[],
           const PetscInt aOff[], const PetscInt aOff_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[],
           PetscReal t, PetscReal u_tShift, const PetscReal x[], PetscScalar g0[])
{
  g0[0] = 1.0;
}
void g0_uu_full(PetscInt dim, PetscInt Nf, PetscInt NfAux,
                const PetscInt uOff[], const PetscInt uOff_x[], const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[],
                const PetscInt aOff[], const PetscInt aOff_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[],
                PetscReal t, PetscReal u_tShift, const PetscReal x[], PetscScalar g0[])
{
  g0[0] = PetscSqr(u[0] - sin(2.0*PETSC_PI*x[0]) * sin(2.0*PETSC_PI*x[1]))
    + PetscSqr(u[0] - (x[0]*x[0] + x[1]*x[1]))
    - 2.0*((x[0]*x[0] + x[1]*x[1]) + (sin(2.0*PETSC_PI*x[0]) * sin(2.0*PETSC_PI*x[1])))*u[0]
    + 4.0*(x[0]*x[0] + x[1]*x[1])*(sin(2.0*PETSC_PI*x[0]) * sin(2.0*PETSC_PI*x[1]));
}
void g3_ul(PetscInt dim, PetscInt Nf, PetscInt NfAux,
           const PetscInt uOff[], const PetscInt uOff_x[], const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[],
           const PetscInt aOff[], const PetscInt aOff_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[],
           PetscReal t, PetscReal u_tShift, const PetscReal x[], PetscScalar g3[])
{
  PetscInt d;
  for (d = 0; d < dim; ++d) g3[d*dim+d] = 1.0;
}

void f0_a(PetscInt dim, PetscInt Nf, PetscInt NfAux,
          const PetscInt uOff[], const PetscInt uOff_x[], const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[],
          const PetscInt aOff[], const PetscInt aOff_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[],
          PetscReal t, const PetscReal x[], PetscScalar f0[])
{
  f0[0] = u[1] - 4.0 /* 0.0 */ + u[2];
}
void g0_aa(PetscInt dim, PetscInt Nf, PetscInt NfAux,
           const PetscInt uOff[], const PetscInt uOff_x[], const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[],
           const PetscInt aOff[], const PetscInt aOff_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[],
           PetscReal t, PetscReal u_tShift, const PetscReal x[], PetscScalar g0[])
{
  g0[0] = 1.0;
}
void g0_al(PetscInt dim, PetscInt Nf, PetscInt NfAux,
           const PetscInt uOff[], const PetscInt uOff_x[], const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[],
           const PetscInt aOff[], const PetscInt aOff_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[],
           PetscReal t, PetscReal u_tShift, const PetscReal x[], PetscScalar g0[])
{
  g0[0] = 1.0;
}

void f0_l(PetscInt dim, PetscInt Nf, PetscInt NfAux,
          const PetscInt uOff[], const PetscInt uOff_x[], const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[],
          const PetscInt aOff[], const PetscInt aOff_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[],
          PetscReal t, const PetscReal x[], PetscScalar f0[])
{
  f0[0] = u[1];
}
void f1_l(PetscInt dim, PetscInt Nf, PetscInt NfAux,
          const PetscInt uOff[], const PetscInt uOff_x[], const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[],
          const PetscInt aOff[], const PetscInt aOff_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[],
          PetscReal t, const PetscReal x[], PetscScalar f1[])
{
  PetscInt d;
  for (d = 0; d < dim; ++d) f1[d] = u_x[d];
}
void g0_la(PetscInt dim, PetscInt Nf, PetscInt NfAux,
           const PetscInt uOff[], const PetscInt uOff_x[], const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[],
           const PetscInt aOff[], const PetscInt aOff_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[],
           PetscReal t, PetscReal u_tShift, const PetscReal x[], PetscScalar g0[])
{
  g0[0] = 1.0;
}
void g3_lu(PetscInt dim, PetscInt Nf, PetscInt NfAux,
           const PetscInt uOff[], const PetscInt uOff_x[], const PetscScalar u[], const PetscScalar u_t[], const PetscScalar u_x[],
           const PetscInt aOff[], const PetscInt aOff_x[], const PetscScalar a[], const PetscScalar a_t[], const PetscScalar a_x[],
           PetscReal t, PetscReal u_tShift, const PetscReal x[], PetscScalar g3[])
{
  PetscInt d;
  for (d = 0; d < dim; ++d) g3[d*dim+d] = 1.0;
}

/*
  In 2D for Dirichlet conditions with a variable coefficient, we use exact solution:

    u   = x^2 + y^2
    a   = 4
    d_A = 4
    d_B = sin(2*pi*x[0]) * sin(2*pi*x[1])

  so that

    -\Delta u + a = -4 + 4 = 0
*/
PetscErrorCode quadratic_u_2d(PetscInt dim, const PetscReal x[], PetscInt Nf, PetscScalar *u, void *ctx)
{
  *u = x[0]*x[0] + x[1]*x[1];
  return 0;
}
PetscErrorCode constant_a_2d(PetscInt dim, const PetscReal x[], PetscInt Nf, PetscScalar *a, void *ctx)
{
  *a = 4;
  return 0;
}
PetscErrorCode zero(PetscInt dim, const PetscReal x[], PetscInt Nf, PetscScalar *l, void *ctx)
{
  *l = 0.0;
  return 0;
}

#undef __FUNCT__
#define __FUNCT__ "SetupProblem"
PetscErrorCode SetupProblem(DM dm, AppCtx *user)
{
  PetscDS        prob;
  PetscErrorCode ierr;

  PetscFunctionBeginUser;
  ierr = DMGetDS(dm, &prob);CHKERRQ(ierr);
  ierr = PetscDSSetResidual(prob, 0, user->useDualPenalty == PETSC_TRUE ? f0_u_full : f0_u, f1_u);CHKERRQ(ierr);
  ierr = PetscDSSetResidual(prob, 1, f0_a, NULL);CHKERRQ(ierr);
  ierr = PetscDSSetResidual(prob, 2, f0_l, f1_l);CHKERRQ(ierr);
  ierr = PetscDSSetJacobian(prob, 0, 0, user->useDualPenalty == PETSC_TRUE ? g0_uu_full : g0_uu, NULL, NULL, NULL);CHKERRQ(ierr);
  ierr = PetscDSSetJacobian(prob, 0, 2, NULL, NULL, NULL, g3_ul);CHKERRQ(ierr);
  ierr = PetscDSSetJacobian(prob, 1, 1, g0_aa, NULL, NULL, NULL);CHKERRQ(ierr);
  ierr = PetscDSSetJacobian(prob, 1, 2, g0_al, NULL, NULL, NULL);CHKERRQ(ierr);
  ierr = PetscDSSetJacobian(prob, 2, 1, g0_la, NULL, NULL, NULL);CHKERRQ(ierr);
  ierr = PetscDSSetJacobian(prob, 2, 0, NULL, NULL, NULL, g3_lu);CHKERRQ(ierr);

  user->exactFuncs[0] = quadratic_u_2d;
  user->exactFuncs[1] = constant_a_2d;
  user->exactFuncs[2] = zero;
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "SetupDiscretization"
PetscErrorCode SetupDiscretization(DM dm, AppCtx *user)
{
  DM              cdm = dm;
  const PetscInt  dim = 2;
  PetscFE         fe[3];
  PetscQuadrature q;
  PetscInt        f;
  PetscErrorCode  ierr;

  PetscFunctionBeginUser;
  /* Create finite element */
  ierr = PetscFECreateDefault(dm, dim, 1, PETSC_TRUE, "potential_", -1, &fe[0]);CHKERRQ(ierr);
  ierr = PetscObjectSetName((PetscObject) fe[0], "potential");CHKERRQ(ierr);
  ierr = PetscFEGetQuadrature(fe[0], &q);CHKERRQ(ierr);
  ierr = PetscFECreateDefault(dm, dim, 1, PETSC_TRUE, "charge_", -1, &fe[1]);CHKERRQ(ierr);
  ierr = PetscObjectSetName((PetscObject) fe[1], "charge");CHKERRQ(ierr);
  ierr = PetscFESetQuadrature(fe[1], q);CHKERRQ(ierr);
  ierr = PetscFECreateDefault(dm, dim, 1, PETSC_TRUE, "multiplier_", -1, &fe[2]);CHKERRQ(ierr);
  ierr = PetscObjectSetName((PetscObject) fe[2], "multiplier");CHKERRQ(ierr);
  ierr = PetscFESetQuadrature(fe[2], q);CHKERRQ(ierr);
  /* Set discretization and boundary conditions for each mesh */
  while (cdm) {
    const PetscInt id = 1;
    PetscDS        prob;

    ierr = DMGetDS(cdm, &prob);CHKERRQ(ierr);
    for (f = 0; f < 3; ++f) {ierr = PetscDSSetDiscretization(prob, f, (PetscObject) fe[f]);CHKERRQ(ierr);}
    ierr = SetupProblem(cdm, user);CHKERRQ(ierr);
    ierr = DMPlexAddBoundary(cdm, PETSC_TRUE, "wall", "marker", 0, 0, NULL, (void (*)()) user->exactFuncs[0], 1, &id, user);CHKERRQ(ierr);
    ierr = DMPlexAddBoundary(cdm, PETSC_TRUE, "wall", "marker", 1, 0, NULL, (void (*)()) user->exactFuncs[1], 1, &id, user);CHKERRQ(ierr);
    ierr = DMPlexAddBoundary(cdm, PETSC_TRUE, "wall", "marker", 2, 0, NULL, (void (*)()) user->exactFuncs[2], 1, &id, user);CHKERRQ(ierr);
    ierr = DMPlexGetCoarseDM(cdm, &cdm);CHKERRQ(ierr);
  }
  for (f = 0; f < 3; ++f) {ierr = PetscFEDestroy(&fe[f]);CHKERRQ(ierr);}
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc, char **argv)
{
  DM             dm;
  SNES           snes;
  Vec            u, r;
  AppCtx         user;
  PetscErrorCode ierr;

  ierr = PetscInitialize(&argc, &argv, NULL, help);CHKERRQ(ierr);
  ierr = ProcessOptions(PETSC_COMM_WORLD, &user);CHKERRQ(ierr);
  ierr = SNESCreate(PETSC_COMM_WORLD, &snes);CHKERRQ(ierr);
  ierr = CreateMesh(PETSC_COMM_WORLD, &user, &dm);CHKERRQ(ierr);
  ierr = SNESSetDM(snes, dm);CHKERRQ(ierr);

  ierr = PetscMalloc(3 * sizeof(void (*)()), &user.exactFuncs);CHKERRQ(ierr);
  ierr = SetupDiscretization(dm, &user);CHKERRQ(ierr);

  ierr = DMCreateGlobalVector(dm, &u);CHKERRQ(ierr);
  ierr = PetscObjectSetName((PetscObject) u, "solution");CHKERRQ(ierr);
  ierr = VecDuplicate(u, &r);CHKERRQ(ierr);
  ierr = DMSNESSetFunctionLocal(dm, DMPlexSNESComputeResidualFEM, &user);CHKERRQ(ierr);
  ierr = DMSNESSetJacobianLocal(dm, DMPlexSNESComputeJacobianFEM, &user);CHKERRQ(ierr);
  ierr = SNESSetFromOptions(snes);CHKERRQ(ierr);

  ierr = DMPlexProjectFunction(dm, user.exactFuncs, NULL, INSERT_ALL_VALUES, u);CHKERRQ(ierr);
  ierr = DMSNESCheckFromOptions(snes, u, user.exactFuncs, NULL);CHKERRQ(ierr);
  if (user.runType == RUN_FULL) {
    PetscErrorCode (*initialGuess[3])(PetscInt dim, const PetscReal x[], PetscInt Nf, PetscScalar u[], void *ctx);
    PetscReal        error;

    initialGuess[0] = zero;
    initialGuess[1] = zero;
    initialGuess[2] = zero;
    ierr = DMPlexProjectFunction(dm, initialGuess, NULL, INSERT_VALUES, u);CHKERRQ(ierr);
    ierr = VecViewFromOptions(u, "initial_", "-vec_view");CHKERRQ(ierr);
    ierr = DMPlexComputeL2Diff(dm, user.exactFuncs, NULL, u, &error);CHKERRQ(ierr);
    if (error < 1.0e-11) {ierr = PetscPrintf(PETSC_COMM_WORLD, "Initial L_2 Error: < 1.0e-11\n");CHKERRQ(ierr);}
    else                 {ierr = PetscPrintf(PETSC_COMM_WORLD, "Initial L_2 Error: %g\n", error);CHKERRQ(ierr);}
    ierr = SNESSolve(snes, NULL, u);CHKERRQ(ierr);
    ierr = DMPlexComputeL2Diff(dm, user.exactFuncs, NULL, u, &error);CHKERRQ(ierr);
    if (error < 1.0e-11) {ierr = PetscPrintf(PETSC_COMM_WORLD, "Final L_2 Error: < 1.0e-11\n");CHKERRQ(ierr);}
    else                 {ierr = PetscPrintf(PETSC_COMM_WORLD, "Final L_2 Error: %g\n", error);CHKERRQ(ierr);}
  }
  ierr = VecViewFromOptions(u, "sol_", "-vec_view");CHKERRQ(ierr);

  ierr = VecDestroy(&u);CHKERRQ(ierr);
  ierr = VecDestroy(&r);CHKERRQ(ierr);
  ierr = SNESDestroy(&snes);CHKERRQ(ierr);
  ierr = DMDestroy(&dm);CHKERRQ(ierr);
  ierr = PetscFree(user.exactFuncs);CHKERRQ(ierr);
  ierr = PetscFinalize();
  return 0;
}
