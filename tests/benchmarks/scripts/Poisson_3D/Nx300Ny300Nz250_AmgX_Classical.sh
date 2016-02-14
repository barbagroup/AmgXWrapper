#! /bin/bash


#======================================================================
# AmgX + Classical; Nx = 300; Ny = 300; Nz = 250
#======================================================================
PART=ivygpu-noecc
TIME=00:10:00
TARGET=${PWD}/scripts/launchPoisson.sh
MODE=AmgX
ALGORITHM=Classical

launchSbatch 4 2 ${TIME} ${PART} "${TARGET} ${MODE} ${ALGORITHM} 8 2 300 300 250"
launchSbatch 8 2 ${TIME} ${PART} "${TARGET} ${MODE} ${ALGORITHM} 16 2 300 300 250"
launchSbatch 16 2 ${TIME} ${PART} "${TARGET} ${MODE} ${ALGORITHM} 32 2 300 300 250"

