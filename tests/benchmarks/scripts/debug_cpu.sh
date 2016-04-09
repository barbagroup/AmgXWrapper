#! /bin/bash


#======================================================================
# PETSc + Hypre; Nx = 3000; Ny = 3000;
#======================================================================
PART=debug-cpu
TIME=00:05:00
TARGET=${PWD}/scripts/launchPoisson.sh
MODE=PETSc
ALGORITHM=Hypre

launchSbatch 1 16 ${TIME} ${PART} "${TARGET} ${MODE} ${ALGORITHM} 16 16 3000 3000 0"

