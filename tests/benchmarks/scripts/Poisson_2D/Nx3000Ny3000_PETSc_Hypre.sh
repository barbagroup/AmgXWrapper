#! /bin/bash


#======================================================================
# PETSc + Hypre; Nx = 3000; Ny = 3000;
#======================================================================
PART=short
TIME=00:05:00
TARGET=${PWD}/scripts/launchPoisson.sh
MODE=PETSc
ALGORITHM=Hypre

launchSbatch 1 16 ${TIME} ${PART} "${TARGET} ${MODE} ${ALGORITHM} 16 16 3000 3000 0"
launchSbatch 2 16 ${TIME} ${PART} "${TARGET} ${MODE} ${ALGORITHM} 32 16 3000 3000 0"
launchSbatch 4 16 ${TIME} ${PART} "${TARGET} ${MODE} ${ALGORITHM} 64 16 3000 3000 0"
launchSbatch 8 16 ${TIME} ${PART} "${TARGET} ${MODE} ${ALGORITHM} 128 16 3000 3000 0"
launchSbatch 16 16 ${TIME} ${PART} "${TARGET} ${MODE} ${ALGORITHM} 256 16 3000 3000 0"

