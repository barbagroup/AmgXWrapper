#! /bin/bash


#======================================================================
# PETSc + Hypre; Nx = 500; Ny = 500; Nz = 400
#======================================================================
PART=short
TIME=00:30:00
TARGET=${PWD}/scripts/launchPoisson.sh
MODE=PETSc
ALGORITHM=Hypre

launchSbatch 1 16 01:00:00 ${PART} "${TARGET} ${MODE} ${ALGORITHM} 16 16 500 500 400"
launchSbatch 2 16 01:00:00 ${PART} "${TARGET} ${MODE} ${ALGORITHM} 32 16 500 500 400"
launchSbatch 4 16 00:30:00 ${PART} "${TARGET} ${MODE} ${ALGORITHM} 64 16 500 500 400"
launchSbatch 8 16 00:30:00 ${PART} "${TARGET} ${MODE} ${ALGORITHM} 128 16 500 500 400"
launchSbatch 16 16 00:30:00 ${PART} "${TARGET} ${MODE} ${ALGORITHM} 256 16 500 500 400"

