#! /bin/bash


#======================================================================
# AmgX + Classical; Nx = 160; Ny = 160; Nz = 160
#======================================================================
PART=debug-gpu
TIME=00:02:00
TARGET=${PWD}/scripts/launchPoisson.sh
MODE=AmgX
ALGORITHM=Classical

launchSbatch 1 2 ${TIME} ${PART} "${TARGET} ${MODE} ${ALGORITHM} 1 2 160 160 160"
launchSbatch 1 2 ${TIME} ${PART} "${TARGET} ${MODE} ${ALGORITHM} 2 2 160 160 160"

