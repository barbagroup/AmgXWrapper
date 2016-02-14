#! /bin/bash


#======================================================================
# AmgX + Aggregation; Nx = 160; Ny = 160; Nz = 160
#======================================================================
PART=ivygpu-noecc
TIME=00:02:00
TARGET=${PWD}/scripts/launchPoisson.sh
MODE=AmgX
ALGORITHM=AGG

launchSbatch 1 2 ${TIME} ${PART} "${TARGET} ${MODE} ${ALGORITHM} 1 2 160 160 160"
launchSbatch 1 2 ${TIME} ${PART} "${TARGET} ${MODE} ${ALGORITHM} 2 2 160 160 160"
launchSbatch 2 2 ${TIME} ${PART} "${TARGET} ${MODE} ${ALGORITHM} 4 2 160 160 160"
launchSbatch 4 2 ${TIME} ${PART} "${TARGET} ${MODE} ${ALGORITHM} 8 2 160 160 160"
launchSbatch 8 2 ${TIME} ${PART} "${TARGET} ${MODE} ${ALGORITHM} 16 2 160 160 160"
launchSbatch 16 2 ${TIME} ${PART} "${TARGET} ${MODE} ${ALGORITHM} 32 2 160 160 160"

