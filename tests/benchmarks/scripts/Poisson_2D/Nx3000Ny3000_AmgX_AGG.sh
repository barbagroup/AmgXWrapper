#! /bin/bash


#======================================================================
# AmgX + Aggregation; Nx = 3000; Ny = 3000;
#======================================================================
PART=ivygpu-noecc
TIME=00:15:00
TARGET=${PWD}/scripts/launchPoisson.sh
MODE=AmgX
ALGORITHM=AGG

launchSbatch 1 2 ${TIME} ${PART} "${TARGET} ${MODE} ${ALGORITHM} 1 2 3000 3000 0"
launchSbatch 1 2 ${TIME} ${PART} "${TARGET} ${MODE} ${ALGORITHM} 2 2 3000 3000 0"
launchSbatch 2 2 ${TIME} ${PART} "${TARGET} ${MODE} ${ALGORITHM} 4 2 3000 3000 0"
launchSbatch 4 2 ${TIME} ${PART} "${TARGET} ${MODE} ${ALGORITHM} 8 2 3000 3000 0"
launchSbatch 8 2 ${TIME} ${PART} "${TARGET} ${MODE} ${ALGORITHM} 16 2 3000 3000 0"
launchSbatch 16 2 ${TIME} ${PART} "${TARGET} ${MODE} ${ALGORITHM} 32 2 3000 3000 0"

