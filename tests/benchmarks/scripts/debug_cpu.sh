#! /bin/bash


#======================================================================
# PETSc + Hypre; Nx = 2950; Ny = 2950;
#======================================================================
PART=debug-cpu
TIME=00:05:00
TARGET=${PWD}/scripts/launchPoisson.sh
MODE=PETSc
ALGORITHM=Hypre

launchSbatch 1 16 ${TIME} ${PART} "${TARGET} ${MODE} ${ALGORITHM} 16 16 2950 2950 0"

