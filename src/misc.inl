/**
 * \file misc.cpp
 * \brief definition of some member functions of the class AmgXSolver.
 * \author Pi-Yueh Chuang (pychuang@gwu.edu)
 * \date 2016-01-08
 */


// AmgXWrapper
# include "AmgXSolver.hpp"


/** \copydoc AmgXSolver::getMode. */
PetscErrorCode AmgXSolver::setMode(const std::string &_mode)
{
    PetscFunctionBeginUser;

    if (_mode == "hDDI")
        mode = AMGX_mode_hDDI;
    else if (_mode == "hDFI")
        mode = AMGX_mode_hDFI;
    else if (_mode == "hFFI")
        mode = AMGX_mode_hFFI;
    else if (_mode == "dDDI")
        mode = AMGX_mode_dDDI;
    else if (_mode == "dDFI")
        mode = AMGX_mode_dDFI;
    else if (_mode == "dFFI")
        mode = AMGX_mode_dFFI;
    else
        SETERRQ1(MPI_COMM_WORLD, PETSC_ERR_ARG_WRONG,
                "%s is not an available mode! Available modes are: "
                "hDDI, hDFI, hFFI, dDDI, dDFI, dFFI.\n", _mode.c_str());

    PetscFunctionReturn(0);
}


/** \copydoc AmgXSolver::getIters. */
PetscErrorCode AmgXSolver::getIters(int &iter)
{
    // only processes using AmgX will try to get # of iterations
    if (gpuProc == 0)
        AMGX_solver_get_iterations_number(solver, &iter); 

    return iter;
}


/** \copydoc AmgXSolver::getResidual. */
PetscErrorCode AmgXSolver::getResidual(const int &iter, double &res)
{
    // only processes using AmgX will try to get residual
    if (gpuProc == 0)
        AMGX_solver_get_iteration_residual(solver, iter, 0, &res);

    return res;
}
