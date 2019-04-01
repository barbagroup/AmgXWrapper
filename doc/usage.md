# Usage

AmgXWrapper usage is simple. Here is a simple example usage:

## Step 1

In your PETSc application, at any point after `PetscInitialize(...)`, declare 
an instance of `AmgXSolver`.

```c++
AmgXSolver      solver;
```

## Step 2

Next, at any point you like, initialize this instance.

```c++
solver.initialize(comm, mode, configFile);
```

1. `comm` is the preferred MPI communicator. Typically, this is `MPI_COMM_WORLD`
or `PETSC_COMM_WORLD`.

2. `mode` is a `std::string` indicating AmgX solver mode. The `mode` string is 
composed of four letters. The first letter (lowercase) indicates if AmgX
solver will run on GPU (`d`) or CPU (`h`). The second letter (uppercase) indicates
the precision of matrix entries: float (`F`) or double (`D`). The third one
(uppercase) indicates the precision of the vectors. The last one indicates the
integer type of the indices of matrix and vector entries (currently only 32bit
integer is supported, so only `I` is available). For example, `mode = dDDI` means
the AmgX solver will run on GPUs, using double precision floating numbers 
for matrix and vector entries, and using 32-bit integers for indices.

3. `configFile` is a `std::string` indicating the path to AmgX configuration file. 
Please see AmgX Reference Manual for writing configuration files. For example, 
[here](../example/poisson/configs/AmgX_SolverOptions_Classical.info) 
is a typical solver and preconditioner settings we used in PetIBM simulations.

The return values of all AmgXWrapper functions are `PetscErrorCode`, so one can
call these functions and check the returns in PETSc style:

```c++
ierr = solver.initialize(comm, mode, configFile); CHKERRQ(ierr);
```

It's also to combine the first step and this step together:

```c++
AmgXSolver      solver(comm, mode, configFile);
```

## Step 3

After done creating the coefficient matrix using PETSc, upload the 
matrix to GPUs and bind to the `solver` with

```c++
ierr = solver.setA(A); CHKERRQ(ierr);
```

`A` is the coefficient matrix with data type `Mat` from PETSc.

No matter how many GPUs and how many CPU cores or nodes being used, `setA` 
will handle data gathering/scattering automatically.

## Step 4

After creating the right-hand-side vector, the system can be solved through:

```c++
ierr = solver.solve(lhs, rhs); CHKERRQ(ierr);
```

`lhs` is a PETSc `Vec` type vector holding the initial guess of solution. Also,
`lhs` will holding the final solution in the end. `rhs` is a PETSc `Vec` type 
vector for the right-hand-side. Matrix `A`, `lhs`, and `rhs` must be created 
with the same MPI communicator.

For time-marching problems, the same linear system may be solved again and again
with different right-hand-side values. In this case, simply call `solver.solve`
again with updated right-hand-side vector.

## Step 5 (optional)

If interested in the number of iterations used in a solve, use

```c++
int     iters;
ierr = solver.getIters(iters); CHKERRQ(ierr);
```

The `iters` is the number of iterations in the last solve.

`solver.getResidual(...)` can be used to obtain the residual at any iteration
in the last solve. For example, to get the residual at the second iteration,
use

```c++
double      res;
ierr = solver.getResidual(2, res); CHKERRQ(ierr);
```

## Step 6

Finalization can be done manually:

```c++
ierr = solver.finalize(); CHKERRQ(ierr);
```

The destructor of the class `AmgXSolver` also does the same thing. This benefits
when using pointer. For example,

```c++
AmgXSolver  *solver = new AmgXSolver(comm, mode, configFile);

solver->setA(A);

solver->solve(lhs, rhs);

delete solver;
```

**Note:**  
If memory leak is a concern, all `AmgXSolver` instances must be finalized
before calling `PetscFinalize()`. This is because there are some PETSc data
in `AmgXSolver` instances.
