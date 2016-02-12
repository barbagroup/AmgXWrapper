# include "headers.hpp"

PetscErrorCode applyNeumannBC(Mat &A, Vec &RHS, const Vec &exact)
{
    PetscErrorCode      ierr;

    PetscInt            row[1] = {0};

    ierr = MatZeroRowsColumns(A, 1, row, 1.0, exact, RHS);         CHKERRQ(ierr);
    return 0;
}
