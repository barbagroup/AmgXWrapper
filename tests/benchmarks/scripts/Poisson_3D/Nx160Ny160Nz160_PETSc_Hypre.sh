#! /bin/bash


#======================================================================
# PETSc + Hypre; Nx = 160; Ny = 160; Nz = 160
#======================================================================
PART=short
TIME=00:05:00
TARGET=${PWD}/scripts/launchPoisson.sh
MODE=PETSc
ALGORITHM=Hypre

launchSbatch 1 16 ${TIME} ${PART} "${TARGET} ${MODE} ${ALGORITHM} 16 16 160 160 160"
launchSbatch 2 16 ${TIME} ${PART} "${TARGET} ${MODE} ${ALGORITHM} 32 16 160 160 160"
launchSbatch 4 16 ${TIME} ${PART} "${TARGET} ${MODE} ${ALGORITHM} 64 16 160 160 160"
launchSbatch 8 16 ${TIME} ${PART} "${TARGET} ${MODE} ${ALGORITHM} 128 16 160 160 160"
launchSbatch 16 16 ${TIME} ${PART} "${TARGET} ${MODE} ${ALGORITHM} 256 16 160 160 160"

