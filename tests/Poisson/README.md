To build:

```
$ cmake . -DCUDA_TOOLKIT_ROOT_DIR=<installation of CUDA 6.5> -DAMGX_DIR=<installation of AmgX> -DPETSC_DIR=<installation of PETSc>

$ make
```

Exectuable file will be located in the folder `bin`. To run, for example, a 100x100x100 3D Poisson problem using AmgX solver and two GPUs on one node:

```
$ mpiexec -map-by ppr:2:node -n 2 bin/Poisson3D -caseName test -mode GPU -Nx 100 -Ny 100 -Nz 100 -cfgFileName configs/AmgX_V_HMIS_LU_0.8.info -optFileName performance.txt
```

Or if PETSc KSP solver and 16 CPUs are preferred:

```
$ mpiexec -map-by ppr:16:node -n 16 bin/Poisson3D -caseName test -mode PETSc -Nx 100 -Ny 100 -Nz 100 -cfgFileName configs/PETSc_SolverOptions.info -optFileName performance.txt
```

