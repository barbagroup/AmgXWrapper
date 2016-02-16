#! /bin/bash


#======================================================================
# AmgX + Classical; Nx = 500; Ny = 500; Nz = 400
#======================================================================
PART=ivygpu-noecc
TIME=00:10:00
TARGET=${PWD}/scripts/launchPoisson.sh
MODE=AmgX
ALGORITHM=Classical

launchSbatch 4 2 ${TIME} ${PART} "${TARGET} ${MODE} ${ALGORITHM} 8 2 500 500 400"
launchSbatch 8 2 ${TIME} ${PART} "${TARGET} ${MODE} ${ALGORITHM} 16 2 500 500 400"
launchSbatch 16 2 ${TIME} ${PART} "${TARGET} ${MODE} ${ALGORITHM} 32 2 500 500 400"

