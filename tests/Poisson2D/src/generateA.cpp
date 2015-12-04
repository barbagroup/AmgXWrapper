# include "headers.hpp"

PetscErrorCode generateA(const DM &grid, 
        const PetscReal &dx, const PetscReal &dy, Mat &A)
{
    PetscErrorCode      ierr;   // error codes returned by PETSc routines

    PetscInt            xbg, xed,
                        ybg, yed;

    PetscInt            Nx, Ny;

    MatStencil          row;

    PetscScalar         Cx,
                        Cy,
                        Cd;



    // get indices for left-bottom and right-top corner
    ierr = DMDAGetCorners(grid, &xbg, &ybg, nullptr, &xed, &yed, nullptr); CHKERRQ(ierr);

    ierr = DMDAGetInfo(grid, nullptr, &Nx, &Ny, nullptr, 
            nullptr, nullptr, nullptr, nullptr, nullptr,
            nullptr, nullptr, nullptr, nullptr);                   CHKERRQ(ierr);

    xed += xbg;
    yed += ybg;

    Cx = 1.0 / dx / dx;
    Cy = 1.0 / dy / dy;
    Cd = -2.0 * (Cx + Cy);

    for(int j=ybg; j<yed; ++j)
    {
        for(int i=xbg; i<xed; ++i)
        {
            row.i = i; row.j = j; row.k = 0; row.c = 0;

            // Corners
            if (j == 0 && i == 0)
            {
                MatStencil      col[4];
                PetscScalar     values[4];

                for (int stcl=0; stcl<3; stcl++)  col[stcl] = row;

                               col[1].j += 1;
                               col[2].i += 1;

                values[0] = Cd + Cy + Cx + 1;
                                values[1] = Cy;
                                values[2] = Cx;
           
                ierr = MatSetValuesStencil(
                    A, 1, &row, 3, col, values, INSERT_VALUES); CHKERRQ(ierr);
            }
            else if (j == 0 && i == Nx-1)
            {
                MatStencil      col[3];
                PetscScalar     values[3];

                for (int stcl=0; stcl<3; stcl++)  col[stcl] = row;

                               col[1].j += 1;
                col[2].i -= 1;               

                values[0] = Cd +  Cy + Cx;
                                values[1] = Cy;
                values[2] = Cx;                
           
                ierr = MatSetValuesStencil(
                    A, 1, &row, 3, col, values, INSERT_VALUES); CHKERRQ(ierr);
            }
            else if (j == Ny-1 && i == Nx-1)
            {
                MatStencil      col[3];
                PetscScalar     values[3];

                for (int stcl=0; stcl<3; stcl++)  col[stcl] = row;

                col[1].j -= 1;               
                col[2].i -= 1;               

                values[0] = Cd +  Cy + Cx;
                values[1] = Cy;                
                values[2] = Cx;                
           
                ierr = MatSetValuesStencil(
                    A, 1, &row, 3, col, values, INSERT_VALUES); CHKERRQ(ierr);
            }
            else if (j == Ny-1 && i == 0)
            {
                MatStencil      col[3];
                PetscScalar     values[3];

                for (int stcl=0; stcl<3; stcl++)  col[stcl] = row;

                col[1].j -= 1;               
                               col[2].i += 1;

                values[0] = Cd +  Cy + Cx;
                values[1] = Cy;                
                                values[2] = Cx;
           
                ierr = MatSetValuesStencil(
                    A, 1, &row, 3, col, values, INSERT_VALUES); CHKERRQ(ierr);
            }


            // Edges
            else if (j == 0)
            {
                MatStencil      col[4];
                PetscScalar     values[4];

                for (int stcl=0; stcl<4; stcl++)  col[stcl] = row;

                               col[1].j += 1;
                col[2].i -= 1; col[3].i += 1;

                values[0] = Cd +  Cy;
                                values[1] = Cy;
                values[2] = Cx; values[3] = Cx;
           
                ierr = MatSetValuesStencil(
                    A, 1, &row, 4, col, values, INSERT_VALUES); CHKERRQ(ierr);
            }
            else if (j == Ny-1)
            {
                MatStencil      col[4];
                PetscScalar     values[4];

                for (int stcl=0; stcl<4; stcl++)  col[stcl] = row;

                col[1].j -= 1;               
                col[2].i -= 1; col[3].i += 1;

                values[0] = Cd +  Cy;
                values[1] = Cy;                
                values[2] = Cx; values[3] = Cx;
           
                ierr = MatSetValuesStencil(
                    A, 1, &row, 4, col, values, INSERT_VALUES); CHKERRQ(ierr);
            }
            else if (i == 0)
            {
                MatStencil      col[4];
                PetscScalar     values[4];

                for (int stcl=0; stcl<4; stcl++)  col[stcl] = row;

                col[1].j -= 1; col[2].j += 1;
                               col[3].i += 1;

                values[0] = Cd +  Cx;
                values[1] = Cy; values[2] = Cy;
                                values[3] = Cx;
           
                ierr = MatSetValuesStencil(
                    A, 1, &row, 4, col, values, INSERT_VALUES); CHKERRQ(ierr);
            }
            else if (i == Nx-1)
            {
                MatStencil      col[4];
                PetscScalar     values[4];

                for (int stcl=0; stcl<4; stcl++)  col[stcl] = row;

                col[1].j -= 1; col[2].j += 1;
                col[3].i -= 1;               

                values[0] = Cd +  Cx;
                values[1] = Cy; values[2] = Cy;
                values[3] = Cx;                
           
                ierr = MatSetValuesStencil(
                    A, 1, &row, 4, col, values, INSERT_VALUES); CHKERRQ(ierr);
            }


            // Interior
            else
            {
                MatStencil      col[5];
                PetscScalar     values[5];

                for (int stcl=0; stcl<5; stcl++)  col[stcl] = row;

                col[1].j -= 1; col[2].j += 1;
                col[3].i -= 1; col[4].i += 1;

                values[0] = Cd;
                values[1] = Cy; values[2] = Cy;
                values[3] = Cx; values[4] = Cx;
           
                ierr = MatSetValuesStencil(
                    A, 1, &row, 5, col, values, INSERT_VALUES); CHKERRQ(ierr);
            }
        }
    }

    ierr = MatAssemblyBegin(A, MAT_FINAL_ASSEMBLY);                CHKERRQ(ierr);
    ierr = MatAssemblyEnd(A, MAT_FINAL_ASSEMBLY);                  CHKERRQ(ierr);

    return 0;
}
