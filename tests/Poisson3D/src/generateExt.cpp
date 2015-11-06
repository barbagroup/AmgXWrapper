# include "headers.hpp"


PetscErrorCode generateExt(const DM &grid, 
        const Vec &x, const Vec &y, const Vec &z, Vec &exact)
{
    PetscErrorCode      ierr;   // error codes returned by PETSc routines

    PetscInt            xbg, xed,
                        ybg, yed,
                        zbg, zed;

    PetscReal           ***x_arry,
                        ***y_arry,
                        ***z_arry,
                        ***exact_arry;


    // get indices for left-bottom and right-top corner
    ierr = DMDAGetCorners(grid, &xbg, &ybg, &zbg, &xed, &yed, &zed); CHKERRQ(ierr);

    xed += xbg;
    yed += ybg;
    zed += zbg;

    // generate rhs
    ierr = DMDAVecGetArray(grid, x, &x_arry);                      CHKERRQ(ierr);
    ierr = DMDAVecGetArray(grid, y, &y_arry);                      CHKERRQ(ierr);
    ierr = DMDAVecGetArray(grid, z, &z_arry);                      CHKERRQ(ierr);
    ierr = DMDAVecGetArray(grid, exact, &exact_arry);              CHKERRQ(ierr);
    for(int k=zbg; k<zed; ++k)
    {
        for(int j=ybg; j<yed; ++j)
        {
            for(int i=xbg; i<xed; ++i)
                exact_arry[k][j][i] = std::cos(c1 * x_arry[k][j][i]) * 
                    std::cos(c1 * y_arry[k][j][i]) * std::cos(c1 * z_arry[k][j][i]);
        }
    }
    ierr = DMDAVecRestoreArray(grid, x, &x_arry);                  CHKERRQ(ierr);
    ierr = DMDAVecRestoreArray(grid, y, &y_arry);                  CHKERRQ(ierr);
    ierr = DMDAVecRestoreArray(grid, z, &z_arry);                  CHKERRQ(ierr);
    ierr = DMDAVecRestoreArray(grid, exact, &exact_arry);          CHKERRQ(ierr);

    return 0;
}

