/**
 * @file headers.hpp
 * @brief headers of Poisson3D test
 * @author Pi-Yueh Chuang (pychuang@gwu.edu)
 * @version alpha
 * @date 2015-11-06
 */
# pragma once

# include <iostream>
# include <algorithm>
# include <string>
# include <vector>
# include <cmath>
# include <cstring>

# include <petscsys.h>
# include <petscksp.h>
# include <petscdmda.h>

# include <boost/timer/timer.hpp>

# include "StructArgs.hpp"
# include "AmgXSolver.hpp"


# define CHK CHKERRQ(ierr)

# define c1 2.0*1.0*M_PI
# define c2 -3.0*c1*c1


PetscErrorCode getArgs(StructArgs & args);

PetscErrorCode generateGrid(const DM &grid, 
        const PetscInt &Nx, const PetscInt &Ny, const PetscInt &Nz,
        const PetscReal &Lx, const PetscReal &Ly, const PetscReal &Lz,
        PetscReal &dx, PetscReal &dy, PetscReal &dz,
        Vec &x, Vec &y, Vec &z);

PetscErrorCode generateRHS(const DM &grid, 
        const Vec &x, const Vec &y, const Vec &z, Vec &rhs);

PetscErrorCode generateExt(const DM &grid, 
        const Vec &x, const Vec &y, const Vec &z, Vec &exact);

PetscErrorCode generateA(const DM &grid, 
        const PetscReal &dx, const PetscReal &dy, const PetscReal &dz, Mat &A);

PetscErrorCode createKSP(KSP &ksp, Mat &A, char *FN);

