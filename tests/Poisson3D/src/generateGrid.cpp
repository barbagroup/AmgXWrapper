# include "headers.hpp"


PetscErrorCode generateGrid(const DM &grid, 
        const PetscInt &Nx, const PetscInt &Ny, const PetscInt &Nz,
        const PetscReal &Lx, const PetscReal &Ly, const PetscReal &Lz,
        PetscReal &dx, PetscReal &dy, PetscReal &dz,
        Vec &x, Vec &y, Vec &z)
{
    PetscErrorCode      ierr;   // error codes returned by PETSc routines

    PetscInt            xbg, xed,
                        ybg, yed,
                        zbg, zed;

    PetscReal           ***x_arry,
                        ***y_arry,
                        ***z_arry;

    // calculate spacing
    dx = Lx / (PetscReal) Nx;
    dy = Ly / (PetscReal) Ny;
    dz = Lz / (PetscReal) Nz;


    // get indices for left-bottom and right-top corner
    ierr = DMDAGetCorners(grid, &xbg, &ybg, &zbg, &xed, &yed, &zed); CHKERRQ(ierr);

    xed += xbg;
    yed += ybg;
    zed += zbg;

    // generate x coordinates
    ierr = DMDAVecGetArray(grid, x, &x_arry);                      CHKERRQ(ierr);
    for(int k=zbg; k<zed; ++k)
    {
        for(int j=ybg; j<yed; ++j)
        {
            for(int i=xbg; i<xed; ++i)
                x_arry[k][j][i] = (0.5 + (PetscReal)i) * dx;
        }
    }
    ierr = DMDAVecRestoreArray(grid, x, &x_arry);                  CHKERRQ(ierr);

    // generate y coordinates
    ierr = DMDAVecGetArray(grid, y, &y_arry);                      CHKERRQ(ierr);
    for(int k=zbg; k<zed; ++k)
    {
        for(int j=ybg; j<yed; ++j)
        {
            for(int i=xbg; i<xed; ++i)
                y_arry[k][j][i] = (0.5 + (PetscReal)j) * dy;
        }
    }
    ierr = DMDAVecRestoreArray(grid, y, &y_arry);                  CHKERRQ(ierr);

    // generate z coordinates
    ierr = DMDAVecGetArray(grid, z, &z_arry);                      CHKERRQ(ierr);
    for(int k=zbg; k<zed; ++k)
    {
        for(int j=ybg; j<yed; ++j)
        {
            for(int i=xbg; i<xed; ++i)
                z_arry[k][j][i] = (0.5 + (PetscReal)k) * dz;
        }
    }
    ierr = DMDAVecRestoreArray(grid, z, &z_arry);                  CHKERRQ(ierr);

    return 0;
}

