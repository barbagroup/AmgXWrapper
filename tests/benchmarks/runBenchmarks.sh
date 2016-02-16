#!/bin/bash

# create the folder "results" if it doesn't exist
if [ ! -d "results" ]
then
	mkdir results
fi

# create the folder "errors" if it doesn't exist
if [ ! -d "errors" ]
then
	mkdir errors
fi

# create the folder "logs" if it doesn't exist
if [ ! -d "logs" ]
then
	mkdir logs
fi


# load necessary module
source moduleLoad.sh

# set AmgX license file
export LM_LICENSE_FILE=/groups/barbalab/pychuang/AmgX/amgx_trial-exp-Mar-31-2016.lic

# append the location of AmgX libs to LD_LIBRARY_PATH
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/groups/barbalab/pychuang/AmgX/lib

# set variables for scripts of cases
export RESULT_DIR=${PWD}/results
export ERROR_DIR=${PWD}/errors
export LOG_DIR=${PWD}/logs
export CONFIG_DIR=${PWD}/configs


# simplify the launching process of sbatch
launchSbatch(){
	sbatch \
		--nodes=${1} \
		--ntasks-per-node=${2} \
		--time=${3} \
		--partition=${4} \
		${5}
}

#source ./scripts/debug_gpu.sh
#source ./scripts/debug_cpu.sh

source ./scripts/Poisson_3D/Nx160Ny160Nz160_PETSc_Hypre.sh
source ./scripts/Poisson_3D/Nx160Ny160Nz160_PETSc_GAMG.sh
source ./scripts/Poisson_3D/Nx160Ny160Nz160_AmgX_Classical.sh
source ./scripts/Poisson_3D/Nx160Ny160Nz160_AmgX_AGG.sh

source ./scripts/Poisson_3D/Nx320Ny160Nz80_PETSc_Hypre.sh
source ./scripts/Poisson_3D/Nx320Ny160Nz80_PETSc_GAMG.sh
source ./scripts/Poisson_3D/Nx320Ny160Nz80_AmgX_Classical.sh
source ./scripts/Poisson_3D/Nx320Ny160Nz80_AmgX_AGG.sh

source ./scripts/Poisson_3D/Nx200Ny200Nz150_PETSc_Hypre.sh
source ./scripts/Poisson_3D/Nx200Ny200Nz150_PETSc_GAMG.sh
source ./scripts/Poisson_3D/Nx200Ny200Nz150_AmgX_Classical.sh
source ./scripts/Poisson_3D/Nx200Ny200Nz150_AmgX_AGG.sh

source ./scripts/Poisson_3D/Nx300Ny300Nz250_PETSc_Hypre.sh
source ./scripts/Poisson_3D/Nx300Ny300Nz250_PETSc_GAMG.sh
source ./scripts/Poisson_3D/Nx300Ny300Nz250_AmgX_Classical.sh
source ./scripts/Poisson_3D/Nx300Ny300Nz250_AmgX_AGG.sh

source ./scripts/Poisson_2D/Nx3000Ny3000_PETSc_Hypre.sh
source ./scripts/Poisson_2D/Nx3000Ny3000_PETSc_GAMG.sh
source ./scripts/Poisson_2D/Nx3000Ny3000_AmgX_Classical.sh
source ./scripts/Poisson_2D/Nx3000Ny3000_AmgX_AGG.sh
