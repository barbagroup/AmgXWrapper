# Example: poisson

This example solves a Poisson equation in 2D and 3D. We use standard central
difference with a uniform grid in each direction. The exact solutions are
known, so `poisson` can also serve as a kind of tests.

The Poisson equation is
<center>Δu = -8π cos(2πx) cos(2πy)</center>
for 2D. And for 3D, it is
<center>Δu = -12π cos(2πx) cos(2πy) cos(2πz)</center>

The exact solutions are
<center>Δu = cos(2πx) cos(2πy)</center>
for 2D, and
<center>Δu = cos(2πx) cos(2πy) cos(2πz)</center>
for 3D.

The boundary condition is an all-Neumann BC, except that we pin a point as a
reference point (i.e., we apply a Dirichlet BC to that point) to avoid a singular
matrix. We choose the point that is represented by the first row in matrix A
as our reference point.

Users can solve other Poisson equations with known exact solutions by modifying
the hard-coded equation in the functions `generateRHS` and `generateExt`. The
computational domain is hard-coded as `Lx=Ly=Lz=1`.

Most of the code is regarding setting up the Poisson problem with PETSc. If users
are already familiar with PETSc, they should be able to quickly find out how to
solve PETSc linear systems with AmgXWrapper.

## Build
---------

The example is built and installed along with the main AmgXWrapper library. It will be at
`<installation prefix>/share/AmgXWrapper/example/poisson`.


## Running `poisson`
--------------------------

Assume we are now in `<installation prefix>/share/AmgXWrapper/example/poisson`. To see all command
line arguments for `poisson`, use

```bash
$ bin/poisson -print
```

Note, if using `-help` instead of `-print`, all PETSc arguments will be printed.

To run a test with PETSc and with 4 CPU cores to solve the 2D Poisson equation, for example,

```bash
$ mpiexec -n 4 bin/poisson \
    -caseName ${CASE_NAME} \
    -mode PETSc \
    -cfgFileName ${PATH_TO_KSP_SETTING_FILE} \
    -Nx ${NUMBER_OF_CELL_IN_X_DIRECTION} \
    -Ny ${NUMBER_OF_CELL_IN_Y_DIRECTION}
```

1. `${CASE_NAME}` is the name of this run. It's only purpose is to be printed on
    the monitor and can serve as a keyword when parsing the results in post-processing.
2. `${PATH_TO_KSP_SETTING_FILE}` is the file containing the setting to KSP solver.

Note, for argument `-mode`, available options are `PETSc`, `AmgX_GPU`, and `AmgX_CSR`.
CPU version of AmgX solver is not supported.

To run a test with AmgX and with 4 CPU cores plus GPUs to solve the 3D Poisson,
for example,

```bash
$ mpiexec -n 4 bin/poisson \
    -caseName ${CASE_NAME} \
    -mode AmgX_GPU \
    -cfgFileName ${PATH_TO_KSP_SETTING_FILE} \
    -Nx ${NUMBER_OF_CELL_IN_X_DIRECTION} \
    -Ny ${NUMBER_OF_CELL_IN_Y_DIRECTION} \
    -Nz ${NUMBER_OF_CELL_IN_Z_DIRECTION}
```

The number of GPUs used will depend on the available GPUs on the machines. To
limit the number of GPUs used, use the environmental variable `CUDA_VISIBLE_DEVICES`.
For example, to only use the first two GPUs on the machine, do

```bash
$ export CUDA_VISIBLE_DEVICES=0,1
```

before running the program.
