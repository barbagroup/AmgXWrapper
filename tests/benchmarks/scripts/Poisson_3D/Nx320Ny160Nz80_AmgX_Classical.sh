#! /bin/bash


#======================================================================
# AmgX + Classical; Nx = 320; Ny = 160; Nz = 80
#======================================================================
PART=ivygpu-noecc
TIME=00:02:00
TARGET=${PWD}/scripts/launchPoisson.sh
MODE=AmgX
ALGORITHM=Classical

launchSbatch 1 2 ${TIME} ${PART} "${TARGET} ${MODE} ${ALGORITHM} 1 2 320 160 80"
launchSbatch 1 2 ${TIME} ${PART} "${TARGET} ${MODE} ${ALGORITHM} 2 2 320 160 80"
launchSbatch 2 2 ${TIME} ${PART} "${TARGET} ${MODE} ${ALGORITHM} 4 2 320 160 80"
launchSbatch 4 2 ${TIME} ${PART} "${TARGET} ${MODE} ${ALGORITHM} 8 2 320 160 80"
launchSbatch 8 2 ${TIME} ${PART} "${TARGET} ${MODE} ${ALGORITHM} 16 2 320 160 80"
launchSbatch 16 2 ${TIME} ${PART} "${TARGET} ${MODE} ${ALGORITHM} 32 2 320 160 80"

