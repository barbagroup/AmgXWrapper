#! /bin/bash

# MODE, ALGORITHM, NPROC, NPERNODE, Nx, Ny, Nz
CONF=${CONFIG_DIR}/${1}_SolverOptions_${2}.info
CASE=Nx${5}Ny${6}Nz${7}_${1}_${2}_${3}

nvidia-smi >> ${RESULT_DIR}/${CASE}.results

mpiexec \
	-n ${3} \
	-map-by ppr:${4}:node \
	-display-map \
	${PWD}/bin/Poisson \
		-caseName ${CASE} \
		-mode ${1} \
		-Nx ${5} \
		-Ny ${6} \
		-Nz ${7} \
		-cfgFileName ${CONF} \
		-optFileName ${LOG_DIR}/${CASE} \
	1>>${RESULT_DIR}/${CASE}.results \
	2>>${ERROR_DIR}/${CASE}.errors

