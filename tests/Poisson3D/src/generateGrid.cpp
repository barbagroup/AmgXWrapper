# include "headers.hpp"


/**
 * @brief Generate 3D Cartesian grid
 *
 * @param grid
 * @param Nx
 * @param Ny
 * @param Nz
 * @param Lx
 * @param Ly
 * @param Lz
 * @param dx
 * @param dy
 * @param dz
 * @param x
 * @param y
 * @param z
 *
 * @return 
 */
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


/**
 * @brief Generate 2D Cartesian grid
 *
 * @param grid
 * @param Nx
 * @param Ny
 * @param Lx
 * @param Ly
 * @param dx
 * @param dy
 * @param x
 * @param y
 *
 * @return 
 */
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


