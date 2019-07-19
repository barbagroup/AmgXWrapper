# Dependencies

So far the, the following sets of dependencies and versions have been tested and
worked with AmgXWrapper:

#### Using AmgX GitHub Repository -- commit aba9132119fd9efde679f41369628c04e3452a14

* [OpenMPI v4.0.0](https://www.open-mpi.org/software/ompi/v4.0/)
* [CUDA v10.0.130](https://developer.nvidia.com/cuda-10.0-download-archive)
* [PETSc v3.10.4](https://www.mcs.anl.gov/petsc/download/index.html)
* [AmgX GitHub commit aba9132](https://github.com/NVIDIA/AMGX/commit/aba9132119fd9efde679f41369628c04e3452a14)

The C and C++ compilers for this set of dependencies are GCC 7.

### AmgX v1.0 (closed source)

This is the last version before NVIDIA made AmgX open-sourced. Some HPC clusters
may still be using this version of AmgX.

* [OpenMPI v1.8](https://www.open-mpi.org/software/ompi/v1.8/)
* [CUDA v6.5](https://developer.nvidia.com/cuda-toolkit-65)
* [PETSc v3.5](https://www.mcs.anl.gov/petsc/download/index.html)
* [AmgX v1.0](https://developer.nvidia.com/rdp/assets/amgx-trial-download)

The C and C++ compilers for this set of dependencies are GCC 4.9. Note that this
version of AmgX requires a valid license file.

### Note

1. [Doxygen](http://www.stack.nl/~dimitri/doxygen/) is optional if a user wants to 
build API manual.
2. The performances of the GPU solvers highly depends on the optimization flags. 
If using AmgX or PETSc that is built/compiled by other people, remember to check 
the compiler flags they used. Mistakenly using the `-g` flag can be dramatically
slower than the `-O3 -DNDEBUG` flag by 50x.
