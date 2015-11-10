#!/bin/bash

echo "Start to configure environment"
echo "Loading neccessary modules/libraries..."

sh ./moduleLoad.sh

echo "Configuring PETSc..."

cd ../../extra/petsc
export PETSC_DIR=${PWD}
export PETSC_ARCH=forBenchmarks
./configure                             		\
    --with-petsc-arch=${PETSC_ARCH} 			\
    --with-debugging=0                                  \
    --COPTFLAGS='-O3 -m64'                              \
    --CXXOPTFLAGS='-O3 -m64'                            \
    --FOPTFLAGS='-O3 -m64'                              \
    --with-shared-libraries=0 				\
    --with-x=0 						\
    --with-precision=double                             \
    \
    --with-mpi=1                                        \
    --download-openmpi=no                               \
    \
    --with-valgrind=1 					\
    --with-valgrind-dir=/c1/apps/valgrind/3.10.0 	\
    \
    --download-fblaslapack=yes 				\
    \
    --download-f2cblaslapack=yes 			\
    \
    --download-hypre=yes				\
    \
    --download-boost=yes 				\
    \
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
cmake . 						\
	-DAMGX_DIR=/groups/barbalab/pychuang/AmgX 	\
	-DBOOST_ROOT=${PETSC_DIR}/${PETSC_ARCH}		\
	-DCUDA_TOOLKIT_ROOT_DIR=/c1/apps/cuda/toolkit/6.5.14
make

cd benchmarks 

echo "Configuration finished."
