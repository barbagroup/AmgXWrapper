/**
 * @file applyNeumannBC.cpp
 * @brief Definition of function applyNeumannBC(...)
 * @author Pi-Yueh Chuang (pychuang@gwu.edu)
 * @date 2016-02-13
 */


// header
# include "applyNeumannBC.hpp"


// definition of applyNeumannBC
PetscErrorCode applyNeumannBC(Mat &A, Vec &RHS, const Vec &exact)
{
    PetscFunctionBeginUser;

    PetscErrorCode      ierr;

    PetscInt            row[1] = {0};

    PetscReal           scale;

    ierr = MatGetValues(A, 1, row, 1, row, &scale); CHKERRQ(ierr);

    ierr = MatZeroRowsColumns(A, 1, row, scale, exact, RHS); CHKERRQ(ierr);

    PetscFunctionReturn(0);
}
