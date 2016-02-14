#! /bin/bash


#======================================================================
# AmgX + Classical; Nx = 200; Ny = 200; Nz = 150
#======================================================================
PART=ivygpu-noecc
TIME=00:05:00
TARGET=${PWD}/scripts/launchPoisson.sh
MODE=AmgX
ALGORITHM=Classical

launchSbatch 1 2 ${TIME} ${PART} "${TARGET} ${MODE} ${ALGORITHM} 2 2 200 200 150"
launchSbatch 2 2 ${TIME} ${PART} "${TARGET} ${MODE} ${ALGORITHM} 4 2 200 200 150"
launchSbatch 4 2 ${TIME} ${PART} "${TARGET} ${MODE} ${ALGORITHM} 8 2 200 200 150"
launchSbatch 8 2 ${TIME} ${PART} "${TARGET} ${MODE} ${ALGORITHM} 16 2 200 200 150"
launchSbatch 16 2 ${TIME} ${PART} "${TARGET} ${MODE} ${ALGORITHM} 32 2 200 200 150"

