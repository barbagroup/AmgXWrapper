# include "headers.hpp"


PetscErrorCode generateExt(const DM &grid, 
        const Vec &x, const Vec &y, Vec &exact)
{
    PetscErrorCode      ierr;   // error codes returned by PETSc routines

    PetscInt            xbg, xed,
                        ybg, yed;

    PetscReal           **x_arry,
                        **y_arry,
                        **exact_arry;


    // get indices for left-bottom and right-top corner
    ierr = DMDAGetCorners(grid, &xbg, &ybg, nullptr, &xed, &yed, nullptr); CHKERRQ(ierr);

    xed += xbg;
    yed += ybg;

    // generate rhs
    ierr = DMDAVecGetArray(grid, x, &x_arry);                      CHKERRQ(ierr);
    ierr = DMDAVecGetArray(grid, y, &y_arry);                      CHKERRQ(ierr);
    ierr = DMDAVecGetArray(grid, exact, &exact_arry);              CHKERRQ(ierr);
    for(int j=ybg; j<yed; ++j)
    {
        for(int i=xbg; i<xed; ++i)
            exact_arry[j][i] = 
                std::cos(c1 * x_arry[j][i]) * std::cos(c1 * y_arry[j][i]);
    }
    ierr = DMDAVecRestoreArray(grid, x, &x_arry);                  CHKERRQ(ierr);
    ierr = DMDAVecRestoreArray(grid, y, &y_arry);                  CHKERRQ(ierr);
    ierr = DMDAVecRestoreArray(grid, exact, &exact_arry);          CHKERRQ(ierr);

    return 0;
}

