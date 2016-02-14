#! /bin/bash


#======================================================================
# PETSc + GAMG; Nx = 2950; Ny = 2950;
#======================================================================
PART=short
TIME=00:30:00
TARGET=${PWD}/scripts/launchPoisson.sh
MODE=PETSc
ALGORITHM=GAMG

launchSbatch 1 16 ${TIME} ${PART} "${TARGET} ${MODE} ${ALGORITHM} 16 16 2950 2950 0"
launchSbatch 2 16 ${TIME} ${PART} "${TARGET} ${MODE} ${ALGORITHM} 32 16 2950 2950 0"
launchSbatch 4 16 ${TIME} ${PART} "${TARGET} ${MODE} ${ALGORITHM} 64 16 2950 2950 0"
launchSbatch 8 16 ${TIME} ${PART} "${TARGET} ${MODE} ${ALGORITHM} 128 16 2950 2950 0"
launchSbatch 16 16 ${TIME} ${PART} "${TARGET} ${MODE} ${ALGORITHM} 256 16 2950 2950 0"

