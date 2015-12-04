/**
 * @file Poisson3D.cpp
 * @brief An example and benchmark of AmgX and PETSc
 * @author Pi-Yueh Chuang (pychuang@gwu.edu)
 * @version alpha
 * @date 2015-09-01
 */
# include "headers.hpp"

static std::string help = "Test PETSc and AmgX solvers.";

int main(int argc, char **argv)
{
    using namespace boost;

    StructArgs          args;   // a structure containing CMD arguments

    PetscReal           Lx = 1.0, // Lx
                        Ly = 1.0; // Ly

    PetscReal           dx,     // dx, calculated using Lx=1.0
                        dy;     // dy, calculated using Ly=1.0

    DM                  grid;   // DM object

    Vec                 x,      // x-coordinates
                        y;      // y-coordinates

    Vec                 u,      // unknowns
                        rhs,    // RHS
                        bc,     // boundary conditions
                        u_exact,// exact solution
                        err;    // errors

    Mat                 A;      // coefficient matrix

    KSP                 ksp;    // PETSc KSP solver instance

    AmgXSolver          amgx;   // AmgX wrapper instance

    PetscReal           norm2,  // L2 norm of solution errors
                        normM;  // infinity norm of solution errors

    PetscInt            Niters; // iterations used to converge

    PetscErrorCode      ierr;   // error codes returned by PETSc routines

    PetscMPIInt         size,   // MPI size
                        myRank; // rank of current process


    KSPConvergedReason  reason; // to store the KSP convergence reason

    std::string         solveTime;

    PetscLogStage       stageSolving;

    timer::cpu_timer    timer;


    // initialize PETSc and MPI
    ierr = PetscInitialize(&argc, &argv, nullptr, help.c_str());             CHK;
    ierr = PetscLogDefaultBegin();                                           CHK;
    ierr = PetscLogStageRegister("solving", &stageSolving);                  CHK;


    // obtain the rank and size of MPI
    ierr = MPI_Comm_size(PETSC_COMM_WORLD, &size);                           CHK;
    ierr = MPI_Comm_rank(PETSC_COMM_WORLD, &myRank);                         CHK;


    // get parameters from command-line arguments
    ierr = getArgs(args);                                                    CHK;


    // create DMDA object
    ierr = DMDACreate2d(PETSC_COMM_WORLD, 
            DM_BOUNDARY_NONE, DM_BOUNDARY_NONE, DMDA_STENCIL_STAR, 
            args.Nx, args.Ny, PETSC_DECIDE, PETSC_DECIDE,
            1, 1, nullptr, nullptr, &grid);                                  CHK;

    ierr = PetscObjectSetName((PetscObject) grid, "DMDA grid");              CHK;

    ierr = DMDASetUniformCoordinates(grid, 0., 1., 0., 1., 0., 1.);          CHK;
            

    // create vectors (x, y, p, b, u)
    ierr = DMCreateGlobalVector(grid, &x);                                   CHK;
    ierr = DMCreateGlobalVector(grid, &y);                                   CHK;
    ierr = DMCreateGlobalVector(grid, &u);                                   CHK;
    ierr = DMCreateGlobalVector(grid, &rhs);                                 CHK;
    ierr = DMCreateGlobalVector(grid, &bc);                                  CHK;
    ierr = DMCreateGlobalVector(grid, &u_exact);                             CHK;
    ierr = DMCreateGlobalVector(grid, &err);                                 CHK;

    ierr = PetscObjectSetName((PetscObject) x, "x coordinates");             CHK;
    ierr = PetscObjectSetName((PetscObject) y, "y coordinates");             CHK;
    ierr = PetscObjectSetName((PetscObject) u, "vec for unknowns");          CHK;
    ierr = PetscObjectSetName((PetscObject) rhs, "RHS");                     CHK;
    ierr = PetscObjectSetName((PetscObject) bc, "boundary conditions");      CHK;
    ierr = PetscObjectSetName((PetscObject) u_exact, "exact solution");      CHK;
    ierr = PetscObjectSetName((PetscObject) err, "errors");                  CHK;


    // set values of dx, dy, dx, x, y, and z
    ierr = MPI_Barrier(PETSC_COMM_WORLD);                                    CHK;
    ierr = generateGrid(grid, args.Nx, args.Ny, Lx, Ly, dx, dy, x, y);       CHK;


    // set values of RHS -- the vector rhs
    ierr = MPI_Barrier(PETSC_COMM_WORLD);                                    CHK;
    ierr = generateRHS(grid, x, y, rhs);                                  CHK;


    // generate exact solution
    ierr = MPI_Barrier(PETSC_COMM_WORLD);                                    CHK;
    ierr = generateExt(grid, x, y, u_exact);                              CHK;


    // set all entries as zeros in the vector of unknows
    ierr = MPI_Barrier(PETSC_COMM_WORLD);                                    CHK;
    ierr = VecSet(u, 0.0);                                                   CHK;


    // initialize and set up the coefficient matrix A
    ierr = MPI_Barrier(PETSC_COMM_WORLD);                                    CHK;
    ierr = DMSetMatType(grid, MATAIJ);                                       CHK;
    ierr = DMCreateMatrix(grid, &A);                                         CHK;
    ierr = MatSetOption(A, MAT_NEW_NONZERO_LOCATION_ERR, PETSC_TRUE);        CHK;
    ierr = MatSetOption(A, MAT_NEW_NONZERO_ALLOCATION_ERR, PETSC_TRUE);      CHK;

    ierr = generateA(grid, dx, dy, A);                                   CHK;


    // create a solver and solve based whether it is AmgX or PETSc
    if (std::strcmp(args.mode, "PETSc") == 0) // PETSc mode
    {
        ierr = createKSP(ksp, A, args.cfgFileName);                          CHK;

        ierr = PetscLogStagePush(stageSolving);                              CHK;

        timer.start();
        ierr = KSPSolve(ksp, rhs, u);                                        CHK;
        solveTime = timer.format();

        ierr = PetscLogStagePop();                                           CHK;

        ierr = KSPGetConvergedReason(ksp, &reason);                          CHK;
        if (reason < 0)
        {
            ierr = PetscPrintf(PETSC_COMM_WORLD, "\nDiverged: %d\n", reason);CHK;
            exit(EXIT_FAILURE);
        }

        ierr = KSPGetIterationNumber(ksp, &Niters);                          CHK;
    }
    else // CPU and GPU modes using AmgX library
    {
        if (std::strcmp(args.mode, "GPU") == 0) // AmgX GPU mode
            amgx.initialize(PETSC_COMM_WORLD, "dDDI", args.cfgFileName);
        else // AmgX CPU mode (not yet implemented in the wrapper) and other mode
        {   
            std::cerr << "Invalid mode." << std::endl;
            exit(EXIT_FAILURE);
        }


        ierr = MPI_Barrier(PETSC_COMM_WORLD);                                CHK;
        amgx.setA(A);

        ierr = MPI_Barrier(PETSC_COMM_WORLD);                                CHK;
        ierr = PetscLogStagePush(stageSolving);                              CHK;

        amgx.getMemUsage();

        timer.start();
        amgx.solve(u, rhs);
        solveTime = timer.format();

        ierr = PetscLogStagePop();

        Niters = amgx.getIters();
    }

    // calculate norms of errors
    ierr = VecCopy(u, err);                                                  CHK;
    ierr = VecAXPY(err, -1.0, u_exact);                                      CHK;
    ierr = VecNorm(err, NORM_2, &norm2);                                     CHK;
    ierr = VecNorm(err, NORM_INFINITY, &normM);                              CHK;


    ierr = PetscPrintf(PETSC_COMM_WORLD, "\nCase Name: %s\n", args.caseName);CHK;
    ierr = PetscPrintf(PETSC_COMM_WORLD, 
            "\tNx: %D Ny: %D\n", args.Nx, args.Ny);          CHK;
    ierr = PetscPrintf(PETSC_COMM_WORLD, 
            "\tSolve Time: %s", solveTime.c_str());                          CHK;
    ierr = PetscPrintf(PETSC_COMM_WORLD, "\tL2-Norm: %g\n", (double)norm2);  CHK;
    ierr = PetscPrintf(PETSC_COMM_WORLD, "\tMax-Norm: %g\n", (double)normM); CHK;
    ierr = PetscPrintf(PETSC_COMM_WORLD, "\tIterations %D\n", Niters);       CHK; 


    // output a file for petsc performance
    if (args.optFileBool == PETSC_TRUE)
    {
        PetscViewer         viewer; // PETSc viewer

        std::strcat(args.optFileName ,".log");

        ierr = PetscViewerASCIIOpen(
                PETSC_COMM_WORLD, args.optFileName, &viewer);                CHK;
        ierr = PetscLogView(viewer);                                         CHK;
        ierr = PetscViewerDestroy(&viewer);                                  CHK;
    }
    
    // output VTK file for post-processing
    if (args.VTKFileBool == PETSC_TRUE)
    {
        PetscViewer         viewerVTK;

        std::strcat(args.VTKFileName ,".vts");

        ierr = PetscViewerCreate(PETSC_COMM_WORLD, &viewerVTK);              CHK;
        ierr = PetscViewerSetType(viewerVTK, PETSCVIEWERVTK);                CHK;
        ierr = PetscViewerSetFormat(viewerVTK, PETSC_VIEWER_VTK_VTS);        CHK;
        ierr = PetscViewerFileSetMode(viewerVTK, FILE_MODE_WRITE);           CHK;
        ierr = PetscViewerFileSetName(viewerVTK, args.VTKFileName);          CHK;

        ierr = VecView(u_exact, viewerVTK);                                  CHK;

        ierr = PetscViewerFileSetMode(viewerVTK, FILE_MODE_APPEND);          CHK;

        ierr = VecView(u, viewerVTK);                                        CHK;
        ierr = VecView(err, viewerVTK);                                      CHK;

        ierr = PetscViewerDestroy(&viewerVTK);                               CHK;
    }
    

    // destroy solver
    if (std::strcmp(args.mode, "PETSc") == 0)
        ierr = KSPDestroy(&ksp);
    else
        amgx.finalize();


    ierr = PetscFinalize();                                                  CHK;

    return 0;
}

