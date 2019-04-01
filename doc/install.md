# Build and install

We suggest two different ways to build or install AmgXWrapper.

## 1. Compile the source code together with users' applications
-------------------------------------------------------------

Since AmgXWrapper doesn't have a lot of code or complex hierarchy, it's very 
easy to include the source code in the building process of your applications.
The `poisson` and `solveFromFiles` in the folder `example` belong to this use 
case.

## 2. Build and install AmgXWrapper with CMake
------------------------------------------------

AmgXWrapper can be compiled and built as a library and can be installed to a 
user's preferred path. We provide `CMakeLists.txt` for this purpose.
Our CFD solver, [PetIBM](https://github.com/barbagroup/petibm) is an example
of using AmgXWrapper as a library.

### Step 1

At any location you like, make a temporary folder to held compiled files and 
then go into that folder. For example,

```bash
$ mkdir ${HOME}/build
$ cd ${HOME}/build
```

### Step 2

Run CMake.

```bash
$ cmake \
    -D CMAKE_INSTALL_PREFIX=${PATH_TO_WHERE_YOU_WANT_TO_INSTALL_AmgXWrapper} \
    -D PETSC_DIR=${PATH_TO_PETSC} \
    -D PETSC_ARCH=${THE_BUILD_OF_PETSC_YOU_WANT_TO_USE} \
    -D CUDA_DIR=${PATH_TO_CUDA} \
    -D AMGX_DIR=${PATH_TO_AMGX} \
    ${PATH_TO_AmgXWrapper_SOURCE}
```

Other optional CMake parameters are:

* `CUDA_HOST_COMPILER`: the underlying C compiler for CUDA compiler `nvcc`. If 
  the default C compiler is too new for CUDA, users may need to use this 
  parameter to choose an older version of C compiler.

* `CMAKE_C_COMPILER`: the default is `mpicc`. This is the C compiler used to 
  compile C codes in AmgXWrapper. If a user wants to change the underlying C 
  compiler used by MPI compiler, he/she should instead use the environment
  variable `OMPI_MPICC` for this purpose. See 
  [Compiling MPI applications](https://www.open-mpi.org/faq/?category=mpi-apps).

* `CMAKE_CXX_COMPILER`: the default is `mpicc`. This is the C++ compiler used to 
  compile C++ codes in AmgXWrapper. If a user wants to change the underlying C++ 
  compiler used by MPI compiler, he/she should instead use the environment
  variable `OMPI_MPICXX` for this purpose. See 
  [Compiling MPI applications](https://www.open-mpi.org/faq/?category=mpi-apps).

* `CMAKE_BUILD_TYPE`: the default is `RELEASE`. `RELEASE` mode will use the flags
  `-O3 -DNDEBUG`. Another option is `DEBUG`, which is using flag `-g`.

* `CMAKE_CXX_FLAGS` and `CMAKE_C_FLAGS`: set customized flags here if desired.

* `BUILD_SHARED_LIBS`: the default is `ON`. This will create a shared library 
  `libAmgXWrapper.so`. To create a static library (libAmgXWrapper.a) only, set
  this argument to `OFF`.

### Step 3

Build the library.

```bash
$ make -j <number of threads used to compiling>
```

### Step 4 (optional)

If Doxygen exists, and if CMake is able to find Doxygen, the API documentation
can be built with

```bash
$ make doc
```

This will build a HTML version of API documentation. The homepage is at 
`doc/html/index.html` under the build directory before installation.

### Step 5

Install the library, header, and documents to your preferred location.

```bash
$ make install
```

After installation, the build directory can be safely deleted. For example,
here we can delete `${HOME}/build`.

Under the installation location, `lib` (or `lib64`, depends on the system)
contains the library, `libAmgXWrapper.so` or `libAmgXWrapper.a`. The header file 
`AmgXSolver.hpp` will be in the folder `include`, and the documents (if available)
will be in the folder `doc`.
