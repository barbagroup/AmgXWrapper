#!/bin/bash

echo "Start to configure environment"
echo "Loading neccessary modules/libraries..."

sh ./moduleLoad.sh

echo "Configuring PETSc..."

cd ../../extra/petsc
export PETSC_DIR=${PWD}
export PETSC_ARCH=forBenchmarks
./configure                             		\
    --with-petsc-arch=${PETSC_ARCH}
    --with-mpi=1                                        \
    --download-openmpi=no                               \
    --with-debugging=0                                  \
    --COPTFLAGS='-O3 -m64'                              \
    --CXXOPTFLAGS='-O3 -m64'                            \
    --FOPTFLAGS='-O3 -m64'                              \
    --with-shared-libraries=0 				\
    --with-valgrind=1 					\
    --with-valgrind-dir=/c1/apps/valgrind/3.10.0 	\
    --with-blas-lib=/c1/apps/blas/gcc/1/lib64/libblas.a \
    --with-lapack-lib=/c1/apps/lapack/gcc/3.4.1/lib/liblapack.a \
    --with-x=0 						\
    --with-precision=double                             \
    --download-hypre=yes				\
    --with-boost=1					\
    --with-boost-dir=/c1/libs/boost/boost_1_56_0        \
    --download-sowing=yes

echo "Building PETSc..."

make --silent PETSC_DIR=${PETSC_DIR} PETSC_ARCH=${PETSC_ARCH} all

echo "Running PETSc tests..."

make PETSC_DIR=${PETSC_DIR} PETSC_ARCH=${PETSC_ARCH} test

echo "Building Poisson3D test exectuable file..."

export CC=gcc
export CXX=g++
cd ../../Poisson3D
rm bin cmake_install.cmake CMakeFiles/ CMakeCache.txt Makefile -rf
cmake . -DAMGX_DIR=/groups/barbalab/pychuang/AmgX
make

cd benchmarks 

echo "Configuration finished."
