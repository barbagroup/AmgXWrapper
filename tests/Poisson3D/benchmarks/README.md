This folder contains benchmarks run on [GW Colonial One](http://ots.columbian.gwu.edu/colonial-one-high-performance-computing-initiative). 

To reproduce the results, first run the shell script `config_build.sh` on Colonial One to build PETSc and the exectuable file of Poisson 3D test. And then, run the shell script `runScripts.sh` to launch all slurm jobs. All output will be saved into the folder `results`, and error messages, if any, will be saved into `errors` folder. The `logs` folder contains PETSc log file for each sub-case.

To show the wall times of benchmark cases, run the python scrip `grabData.py` (use Python 3, please).

Note, the wall times are only for solve phase, i.e. the time used on `solver.solve(...)` for AmgX solvers and `KSPSolve(...)` for PETSc KSP solvers.
