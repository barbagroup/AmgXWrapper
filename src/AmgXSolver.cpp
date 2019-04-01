/**
 * \file AmgXSolver.cpp
 * \brief Definition of member functions of the class AmgXSolver.
 * \author Pi-Yueh Chuang (pychuang@gwu.edu)
 * \date 2015-09-01
 * \copyright Copyright (c) 2015-2019 Pi-Yueh Chuang, Lorena A. Barba.
 *            This project is released under MIT License.
 */


// AmgXWrapper
# include "AmgXSolver.hpp"


// initialize AmgXSolver::count to 0
int AmgXSolver::count = 0;

// initialize AmgXSolver::rsrc to nullptr;
AMGX_resources_handle AmgXSolver::rsrc = nullptr;
