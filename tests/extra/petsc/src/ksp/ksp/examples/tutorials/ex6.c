static char help[] = "Working out corner cases of the ASM preconditioner.\n\n";

#include <petscksp.h>

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **args)
{
  KSP            ksp;
  PC             pc;
  Mat            A;
  Vec            u, x, b;
  PetscReal      error;
  PetscMPIInt    rank, size, sized;
  PetscInt       M = 8, N = 8, m, n, rstart, rend, r;
  PetscBool      userSubdomains = PETSC_FALSE;
  PetscErrorCode ierr;

  ierr = PetscInitialize(&argc, &args, NULL, help);CHKERRQ(ierr);
  ierr = PetscOptionsGetInt(NULL,NULL, "-M", &M, NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetInt(NULL,NULL, "-N", &N, NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetBool(NULL,NULL, "-user_subdomains", &userSubdomains, NULL);CHKERRQ(ierr);
  /* Do parallel decomposition */
  ierr = MPI_Comm_rank(PETSC_COMM_WORLD, &rank);CHKERRQ(ierr);
  ierr = MPI_Comm_size(PETSC_COMM_WORLD, &size);CHKERRQ(ierr);
  sized = (PetscMPIInt) PetscSqrtReal((PetscReal) size);
  if (PetscSqr(sized) != size) SETERRQ1(PETSC_COMM_WORLD, PETSC_ERR_ARG_WRONG, "This test may only be run on a nubmer of processes which is a perfect square, not %d", (int) size);
  if (M % sized) SETERRQ2(PETSC_COMM_WORLD, PETSC_ERR_ARG_WRONG, "The number of x-vertices %D does not divide the number of x-processes %d", M, (int) sized);
  if (N % sized) SETERRQ2(PETSC_COMM_WORLD, PETSC_ERR_ARG_WRONG, "The number of y-vertices %D does not divide the number of y-processes %d", N, (int) sized);
  /* Assemble the matrix for the five point stencil, YET AGAIN
       Every other process will be empty */
  ierr = MatCreate(PETSC_COMM_WORLD, &A);CHKERRQ(ierr);
  m    = (sized > 1) ? (rank % 2) ? 0 : 2*M/sized : M;
  n    = N/sized;
  ierr = MatSetSizes(A, m*n, m*n, M*N, M*N);CHKERRQ(ierr);
  ierr = MatSetFromOptions(A);CHKERRQ(ierr);
  ierr = MatSetUp(A);CHKERRQ(ierr);
  ierr = MatGetOwnershipRange(A, &rstart, &rend);CHKERRQ(ierr);
  for (r = rstart; r < rend; ++r) {
    const PetscScalar diag = 4.0, offdiag = -1.0;
    const PetscInt    i    = r/N;
    const PetscInt    j    = r - i*N;
    PetscInt          c;

    if (i > 0)   {c = r - n; ierr = MatSetValues(A, 1, &r, 1, &c, &offdiag, INSERT_VALUES);CHKERRQ(ierr);}
    if (i < M-1) {c = r + n; ierr = MatSetValues(A, 1, &r, 1, &c, &offdiag, INSERT_VALUES);CHKERRQ(ierr);}
    if (j > 0)   {c = r - 1; ierr = MatSetValues(A, 1, &r, 1, &c, &offdiag, INSERT_VALUES);CHKERRQ(ierr);}
    if (j < N-1) {c = r + 1; ierr = MatSetValues(A, 1, &r, 1, &c, &offdiag, INSERT_VALUES);CHKERRQ(ierr);}
    ierr = MatSetValues(A, 1, &r, 1, &r, &diag, INSERT_VALUES);CHKERRQ(ierr);
  }
  ierr = MatAssemblyBegin(A,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(A,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  /* Setup Solve */
  ierr = VecCreate(PETSC_COMM_WORLD, &b);CHKERRQ(ierr);
  ierr = VecSetSizes(b, m*n, PETSC_DETERMINE);CHKERRQ(ierr);
  ierr = VecSetFromOptions(b);CHKERRQ(ierr);
  ierr = VecDuplicate(b, &u);CHKERRQ(ierr);
  ierr = VecDuplicate(b, &x);CHKERRQ(ierr);
  ierr = VecSet(u, 1.0);CHKERRQ(ierr);
  ierr = MatMult(A, u, b);CHKERRQ(ierr);
  ierr = KSPCreate(PETSC_COMM_WORLD, &ksp);CHKERRQ(ierr);
  ierr = KSPSetOperators(ksp, A, A);CHKERRQ(ierr);
  ierr = KSPGetPC(ksp, &pc);CHKERRQ(ierr);
  ierr = PCSetType(pc, PCASM);CHKERRQ(ierr);
  /* Setup ASM by hand */
  if (userSubdomains) {
    IS        is;
    PetscInt *rows;

    /* Use no overlap for now */
    ierr = PetscMalloc1(rend-rstart, &rows);CHKERRQ(ierr);
    for (r = rstart; r < rend; ++r) rows[r-rstart] = r;
    ierr = ISCreateGeneral(PETSC_COMM_SELF, rend-rstart, rows, PETSC_OWN_POINTER, &is);CHKERRQ(ierr);
    ierr = PCASMSetLocalSubdomains(pc, 1, &is, &is);CHKERRQ(ierr);
    ierr = ISDestroy(&is);CHKERRQ(ierr);
  }
  ierr = KSPSetFromOptions(ksp);CHKERRQ(ierr);
  /* Solve and Compare */
  ierr = KSPSolve(ksp, b, x);CHKERRQ(ierr);
  ierr = VecAXPY(x, -1.0, u);CHKERRQ(ierr);
  ierr = VecNorm(x, NORM_INFINITY, &error);CHKERRQ(ierr);
  ierr = PetscPrintf(PETSC_COMM_WORLD, "Infinity norm of the error: %g\n", (double) error);CHKERRQ(ierr);
  /* Cleanup */
  ierr = KSPDestroy(&ksp);CHKERRQ(ierr);
  ierr = MatDestroy(&A);CHKERRQ(ierr);
  ierr = VecDestroy(&u);CHKERRQ(ierr);
  ierr = VecDestroy(&x);CHKERRQ(ierr);
  ierr = VecDestroy(&b);CHKERRQ(ierr);
  ierr = PetscFinalize();
  return 0;
}
