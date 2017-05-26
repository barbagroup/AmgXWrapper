/**
 * @file main.cpp
 * @brief An example and benchmark of AmgX and PETSc
 * @author Pi-Yueh Chuang (pychuang@gwu.edu)
 * @version beta
 * @date 2015-02-01
 */
# include "headers.hpp"


int main(int argc, char **argv)
{
    StructArgs          args;   // a structure containing CMD arguments


    Vec                 u,      // unknowns
                        rhs,    // RHS
                        u_exact,// exact solution
                        err;    // errors

    Mat                 A;      // coefficient matrix

    KSP                 ksp;    // PETSc KSP solver instance

    AmgXSolver          amgx;   // AmgX wrapper instance

    PetscErrorCode      ierr;   // error codes returned by PETSc routines

    PetscMPIInt         size,   // MPI size
                        myRank; // rank of current process

    PetscClassId        solvingID,
                        warmUpID;

    PetscLogEvent       solvingEvent,
                        warmUpEvent;





    // initialize PETSc and MPI
    ierr = PetscInitialize(&argc, &argv, nullptr, nullptr); CHKERRQ(ierr);
    ierr = PetscLogDefaultBegin(); CHKERRQ(ierr);


    // obtain the rank and size of MPI
    ierr = MPI_Comm_size(PETSC_COMM_WORLD, &size); CHKERRQ(ierr);
    ierr = MPI_Comm_rank(PETSC_COMM_WORLD, &myRank); CHKERRQ(ierr);


    // get parameters from command-line arguments
    ierr = args.getArgs(); CHKERRQ(ierr);


    // pring case information
    {
        ierr = PetscPrintf(PETSC_COMM_WORLD, "\n"); CHKERRQ(ierr);
        for(int i=0; i<72; ++i) ierr = PetscPrintf(PETSC_COMM_WORLD, "=");
        ierr = PetscPrintf(PETSC_COMM_WORLD, "\n"); CHKERRQ(ierr);
        for(int i=0; i<72; ++i) ierr = PetscPrintf(PETSC_COMM_WORLD, "-");
        ierr = PetscPrintf(PETSC_COMM_WORLD, "\n"); CHKERRQ(ierr);

        ierr = args.print(); CHKERRQ(ierr);

        for(int i=0; i<72; ++i) ierr = PetscPrintf(PETSC_COMM_WORLD, "-");
        ierr = PetscPrintf(PETSC_COMM_WORLD, "\n"); CHKERRQ(ierr);
        for(int i=0; i<72; ++i) ierr = PetscPrintf(PETSC_COMM_WORLD, "=");
        ierr = PetscPrintf(PETSC_COMM_WORLD, "\n"); CHKERRQ(ierr);

    }


    // create matrix A and load from file
    ierr = readMat(A, args.matrixFileName, "A"); CHKERRQ(ierr);

    // create vector rhs and load from file
    ierr = readVec(rhs, args.rhsFileName, "RHS"); CHKERRQ(ierr);

    // create vector u_exact and load from file
    ierr = readVec(u_exact, args.exactFileName, "exact solution"); CHKERRQ(ierr);


    // create vectors u and set to zeros
    {
        ierr = VecDuplicate(rhs, &u);                                        CHKERRQ(ierr);
        ierr = PetscObjectSetName((PetscObject) u, "unknowns");              CHKERRQ(ierr);
        ierr = VecSet(u, 0.0);                                               CHKERRQ(ierr);
    }

    // create vectors err and set to zeros
    {
        ierr = VecDuplicate(rhs, &err);                                      CHKERRQ(ierr);
        ierr = PetscObjectSetName((PetscObject) err, "errors");              CHKERRQ(ierr);
        ierr = VecSet(err, 0.0);                                             CHKERRQ(ierr);
    }


    // register a PETSc event for warm-up and solving
    ierr = PetscClassIdRegister("SolvingClass", &solvingID);                 CHKERRQ(ierr);
    ierr = PetscClassIdRegister("WarmUpClass", &warmUpID);                   CHKERRQ(ierr);
    ierr = PetscLogEventRegister("Solving", solvingID, &solvingEvent);       CHKERRQ(ierr);
    ierr = PetscLogEventRegister("WarmUp", warmUpID, &warmUpEvent);          CHKERRQ(ierr);

    // create a solver and solve based whether it is AmgX or PETSc
    if (std::strcmp(args.mode, "PETSc") == 0) // PETSc mode
    {
        ierr = createKSP(ksp, A, args.cfgFileName);                          CHKERRQ(ierr);

        ierr = solve(ksp, A, u, rhs, u_exact, err, 
                args, warmUpEvent, solvingEvent);                            CHKERRQ(ierr);

        // destroy KSP
        ierr = KSPDestroy(&ksp);                                             CHKERRQ(ierr);

    }
    else // AmgX mode
    {
        if (std::strcmp(args.mode, "AmgX") == 0) // AmgX GPU mode
            amgx.initialize(PETSC_COMM_WORLD, "dDDI", args.cfgFileName);
        else // AmgX CPU mode (not yet implemented in the wrapper) and other mode
        {   
            std::cerr << "Invalid mode." << std::endl;
            exit(EXIT_FAILURE);
        }


        ierr = MPI_Barrier(PETSC_COMM_WORLD);                                CHKERRQ(ierr);
        amgx.setA(A);

        ierr = solve(amgx, A, u, rhs, u_exact, err, 
                args, warmUpEvent, solvingEvent);                            CHKERRQ(ierr);

        // destroy solver
        ierr = amgx.finalize();                                              CHKERRQ(ierr);

    }


    // output a file for petsc performance
    if (args.optFileBool == PETSC_TRUE)
    {
        PetscViewer         viewer; // PETSc viewer

        std::strcat(args.optFileName ,".log");

        ierr = PetscViewerASCIIOpen(
                PETSC_COMM_WORLD, args.optFileName, &viewer);                CHKERRQ(ierr);
        ierr = PetscLogView(viewer);                                         CHKERRQ(ierr);
        ierr = PetscViewerDestroy(&viewer);                                  CHKERRQ(ierr);
    }
    

    // destroy vectors, matrix, dmda
    {
        ierr = VecDestroy(&u);                                               CHKERRQ(ierr);
        ierr = VecDestroy(&rhs);                                             CHKERRQ(ierr);
        ierr = VecDestroy(&u_exact);                                         CHKERRQ(ierr);
        ierr = VecDestroy(&err);                                             CHKERRQ(ierr);

        ierr = MatDestroy(&A);                                               CHKERRQ(ierr);
    }


    {
        ierr = PetscPrintf(PETSC_COMM_WORLD, "\n");                          CHKERRQ(ierr);
        for(int i=0; i<72; ++i) ierr = PetscPrintf(PETSC_COMM_WORLD, "=");
        ierr = PetscPrintf(PETSC_COMM_WORLD, "\n");                          CHKERRQ(ierr);
        for(int i=0; i<72; ++i) ierr = PetscPrintf(PETSC_COMM_WORLD, "-");
        ierr = PetscPrintf(PETSC_COMM_WORLD, "\n");                          CHKERRQ(ierr);
        ierr = PetscPrintf(PETSC_COMM_WORLD, 
                "End of %s\n", args.caseName);                               CHKERRQ(ierr);
        for(int i=0; i<72; ++i) ierr = PetscPrintf(PETSC_COMM_WORLD, "-");
        ierr = PetscPrintf(PETSC_COMM_WORLD, "\n");                          CHKERRQ(ierr);
        for(int i=0; i<72; ++i) ierr = PetscPrintf(PETSC_COMM_WORLD, "=");
        ierr = PetscPrintf(PETSC_COMM_WORLD, "\n");                          CHKERRQ(ierr);
    }

    // finalize PETSc
    ierr = PetscFinalize();                                                  CHKERRQ(ierr);

    return 0;
}
