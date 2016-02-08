#!/bin/bash

echo "Start to configure environment"


#===============================================================================
#===============================================================================
echo "Loading neccessary modules/libraries..."

sh ./moduleLoad.sh


#===============================================================================
#===============================================================================
echo "Checking Python version..."

case "$(python --version 2>&1)" in
    *"Python 2."*)
        echo "Python OK!"
        ;;
    *"Python 3."*)
        echo "PETSc currently doesn't support Python 3!!" 1>&2
        exit 1
        ;;
esac



#===============================================================================
#===============================================================================
echo "Configuring PETSc..."

cd ../extra/petsc
export PETSC_DIR=${PWD}
export PETSC_ARCH=forBenchmarks
./configure                                             \
    --with-petsc-arch=${PETSC_ARCH}                     \
    --with-debugging=0                                  \
    --COPTFLAGS='-O3 -m64'                              \
    --CXXOPTFLAGS='-O3 -m64'                            \
    --with-fc=0                                         \
    --with-shared-libraries=0                           \
    --with-x=0                                          \
    --with-precision=double                             \
    --with-mpi=1                                        \
    --download-openmpi=no                               \
    --download-f2cblaslapack=yes                        \
    --download-hypre=yes

echo "Building PETSc..."

make --silent PETSC_DIR=${PETSC_DIR} PETSC_ARCH=${PETSC_ARCH} all

echo "Running PETSc tests..."

make PETSC_DIR=${PETSC_DIR} PETSC_ARCH=${PETSC_ARCH} test

cd ../../benchmarks



#===============================================================================
#===============================================================================
echo "Building Poisson test exectuable file..."

export CC=gcc
export CXX=g++

rm bin cmake_install.cmake CMakeFiles/ CMakeCache.txt Makefile -rf

#cmake ../Poisson                                       \
#    -DAMGX_DIR=/groups/barbalab/pychuang/AmgX     \
#    -DCUDA_TOOLKIT_ROOT_DIR=/c1/apps/cuda/toolkit/6.5.14
cmake ../Poisson                                        \
        -DAMGX_DIR=/opt/AmgX                            \
        -DCUDA_TOOLKIT_ROOT_DIR=/opt/cuda65
make



#===============================================================================
#===============================================================================
echo "Configuration finished."
