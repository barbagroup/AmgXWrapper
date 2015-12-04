# include "headers.hpp"


PetscErrorCode generateGrid(const DM &grid, 
        const PetscInt &Nx, const PetscInt &Ny,
        const PetscReal &Lx, const PetscReal &Ly,
        PetscReal &dx, PetscReal &dy, Vec &x, Vec &y)
{
    PetscErrorCode      ierr;   // error codes returned by PETSc routines

    PetscInt            xbg, xed,
                        ybg, yed;

    PetscReal           **x_arry,
                        **y_arry;

    // calculate spacing
    dx = Lx / (PetscReal) Nx;
    dy = Ly / (PetscReal) Ny;


    // get indices for left-bottom and right-top corner
    ierr = DMDAGetCorners(grid, &xbg, &ybg, nullptr, &xed, &yed, nullptr); CHKERRQ(ierr);

    xed += xbg;
    yed += ybg;

    // generate x coordinates
    ierr = DMDAVecGetArray(grid, x, &x_arry);                      CHKERRQ(ierr);
    for(int j=ybg; j<yed; ++j)
    {
        for(int i=xbg; i<xed; ++i)
            x_arry[j][i] = (0.5 + (PetscReal)i) * dx;
    }
    ierr = DMDAVecRestoreArray(grid, x, &x_arry);                  CHKERRQ(ierr);

    // generate y coordinates
    ierr = DMDAVecGetArray(grid, y, &y_arry);                      CHKERRQ(ierr);
    for(int j=ybg; j<yed; ++j)
    {
        for(int i=xbg; i<xed; ++i)
            y_arry[j][i] = (0.5 + (PetscReal)j) * dy;
    }
    ierr = DMDAVecRestoreArray(grid, y, &y_arry);                  CHKERRQ(ierr);

    return 0;
}

