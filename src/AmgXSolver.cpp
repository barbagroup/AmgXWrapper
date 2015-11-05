/**
 * @file AmgXSolver.cpp
 * @brief Definition of member functions of the class AmgXSolver
 * @author Pi-Yueh Chuang (pychuang@gwu.edu)
 * @version alpha
 * @date 2015-09-01
 */
# include <cstdlib>
# include <cuda_runtime.h>
# include "AmgXSolver.hpp"
# include "cudaCHECK.hpp"


/**
 * @brief Initialization of the static member -- count
 *
 * count is used to count the number of instances. The fisrt instance is 
 * responsable for initializing AmgX library and the resource instance.
 */
int AmgXSolver::count = 0;


/**
 * @brief Initialization of the static member -- rsrc
 *
 * Due to the design of AmgX library, using more than one resource instance may
 * cause some problems. So make the resource instance as a static member to keep
 * only one instance there.
 */
AMGX_resources_handle AmgXSolver::rsrc = nullptr;


/**
 * @brief Initialization of AmgXSolver instance
 *
 * This function initializes the current instance. Based on the count, only the 
 * instance initialized first is in charge of initializing AmgX and the 
 * resource instance.
 *
 * @param comm MPI communicator
 * @param _Npart The number of processes involved in this solver
 * @param _myRank The rank of current process
 * @param _mode The mode this solver will run in. Please refer to AmgX manual.
 * @param cfg_file A file containing the configurations of this solver
 *
 * @return Currently meaningless. May be error codes in the future.
 */
int AmgXSolver::initialize(MPI_Comm comm, int _Npart, int _myRank,
        const std::string &_mode, const std::string &cfg_file)
{
    count += 1;

    AmgXComm = comm;
    Npart = _Npart;
    myRank = _myRank;

    // get the number of total cuda devices
    CHECK(cudaGetDeviceCount(&Ndevs));

    // use round-robin to assign devices to processes
    //devs = new int(myRank % Ndevs);
    
    // another way to assign devices
    if (Npart == 1)
    {
        devs = new int(0);
    }
    else
    {
        int         lclSize,
                    lclRank,
                    nPerDev,
                    remain;

        lclSize = std::atoi(std::getenv("OMPI_COMM_WORLD_LOCAL_SIZE"));
        lclRank = std::atoi(std::getenv("OMPI_COMM_WORLD_LOCAL_RANK"));

        nPerDev = lclSize / Ndevs;
        remain = lclSize % Ndevs;
        
        if (lclRank < (nPerDev+1)*remain)
            devs = new int(lclRank / (nPerDev + 1));
        else
            devs = new int((lclRank - (nPerDev + 1) * remain) / nPerDev + remain);
    }


    // only the first instance is in charge of initializing AmgX
    if (count == 1)
    {
        // initialize AmgX
        AMGX_SAFE_CALL(AMGX_initialize());

        // intialize AmgX plugings
        AMGX_SAFE_CALL(AMGX_initialize_plugins());

        // use user defined output mechanism. only the master process can output
        // something on the screen
        if (myRank == 0) 
        { 
            AMGX_SAFE_CALL(
                AMGX_register_print_callback(&(AmgXSolver::print_callback))); 
        }
        else 
        { 
            AMGX_SAFE_CALL(
                AMGX_register_print_callback(&(AmgXSolver::print_none))); 
        }

        // let AmgX to handle errors returned
        AMGX_SAFE_CALL(AMGX_install_signal_handler());
    }

    // create an AmgX configure object
    AMGX_SAFE_CALL(AMGX_config_create_from_file(&cfg, cfg_file.c_str()));

    // let AmgX handle returned error codes internally
    AMGX_SAFE_CALL(AMGX_config_add_parameters(&cfg, "exception_handling=1"));

    // create an AmgX resource object, only the first instance is in charge
    if (count == 1) AMGX_resources_create(&rsrc, cfg, &AmgXComm, 1, devs);

    // set mode
    setMode(_mode);

    // create AmgX vector object for unknowns and RHS
    AMGX_vector_create(&AmgXP, rsrc, mode);
    AMGX_vector_create(&AmgXRHS, rsrc, mode);

    // create AmgX matrix object for unknowns and RHS
    AMGX_matrix_create(&AmgXA, rsrc, mode);

    // create an AmgX solver object
    AMGX_solver_create(&solver, rsrc, mode, cfg);

    // obtain the default number of rings based on current configuration
    AMGX_config_get_default_number_of_rings(cfg, &ring);

    isInitialized = true;
    
    return 0;
}


/**
 * @brief Finalizing the instance.
 *
 * This function destroys AmgX data. The instance last destroyed also needs to 
 * destroy shared resource instance and finalizing AmgX.
 *
 * @return Currently meaningless. May be error codes in the future.
 */
int AmgXSolver::finalize()
{

    // destroy solver instance
    AMGX_solver_destroy(solver);

    // destroy matrix instance
    AMGX_matrix_destroy(AmgXA);

    // destroy RHS and unknown vectors
    AMGX_vector_destroy(AmgXP);
    AMGX_vector_destroy(AmgXRHS);

    // only the last instance need to destroy resource and finalizing AmgX
    if (count == 1)
    {
        AMGX_resources_destroy(rsrc);
        AMGX_SAFE_CALL(AMGX_config_destroy(cfg));

        AMGX_SAFE_CALL(AMGX_finalize_plugins());
        AMGX_SAFE_CALL(AMGX_finalize());
    }
    else
    {
        AMGX_config_destroy(cfg);
    }

    // change status
    isInitialized = false;
    isUploaded_A = false;

    // deallocate device list
    delete [] devs;

    count -= 1;

    return 0;
}


/**
 * @brief A function convert PETSc Mat into AmgX matrix and bind it to solver
 *
 * This function will first extract the raw data from PETSc Mat and convert the
 * column index into 64bit integers. It also create a partition vector that is 
 * required by AmgX. Then, it upload the raw data to AmgX. Finally, it binds
 * the AmgX matrix to the AmgX solver.
 *
 * Be cautious! It lacks mechanism to check whether the PETSc Mat is AIJ format
 * and whether the PETSc Mat is using the same MPI communicator as the 
 * AmgXSolver instance.
 *
 * @param A A PETSc Mat. The coefficient matrix of a system of linear equations.
 * The matrix must be AIJ format and using the same MPI communicator as AmgX.
 *
 * @return Currently meaningless. May be error codes in the future.
 */
int AmgXSolver::setA(Mat &A)
{
    MatType         type;
    PetscInt        nLclRows,
                    nGlbRows;
    const PetscInt  *row, *col;
    Petsc64bitInt   *col64;
    PetscScalar     *data;
    PetscBool       done;
    Mat             lclA;
    PetscErrorCode  ierr;

    int            *partVec;

    // Get number of rows in global matrix
    ierr = MatGetSize(A, &nGlbRows, nullptr);                     CHKERRQ(ierr);

    // Get and check whether the Mat type is supported
    ierr = MatGetType(A, &type);                                  CHKERRQ(ierr);
    if (std::strcmp(type, MATSEQAIJ) == 0)
    {
        // make lclA point to the same memory as A does
        lclA = A;
    }
    else if (std::strcmp(type, MATMPIAIJ) == 0)
    {
        // Get local matrix and its raw data
        ierr = MatMPIAIJGetLocalMat(A, MAT_INITIAL_MATRIX, &lclA);CHKERRQ(ierr);
    }
    else
    {
        std::cerr << "Mat type " << type 
                  << " is not supported now!" << std::endl;
        exit(0);
    }

    ierr = MatGetRowIJ(lclA, 0, PETSC_FALSE, PETSC_FALSE, 
            &nLclRows, &row, &col, &done);                        CHKERRQ(ierr);
    ierr = MatSeqAIJGetArray(lclA, &data);                        CHKERRQ(ierr);

    // check whether MatGetRowIJ worked
    if (! done)
    {
        std::cerr << "MatGetRowIJ did not work!" << std::endl;
        exit(0);
    }

    // obtain a partition vector required for AmgX
    getPartVec(A, partVec);

    // cast 32bit integer array "col" to 64bit integer array "col64"
    col64 = new Petsc64bitInt[row[nLclRows]];
    for(size_t i=0; i<row[nLclRows]; ++i)
        col64[i] = static_cast<Petsc64bitInt>(col[i]);

    // upload matrix A to AmgX
    MPI_Barrier(AmgXComm);
    AMGX_matrix_upload_all_global(AmgXA, nGlbRows, nLclRows, row[nLclRows], 
            1, 1, row, col64, data, nullptr, ring, ring, partVec);

    // Return raw data to the local matrix (required by PETSc)
    ierr = MatRestoreRowIJ(lclA, 0, PETSC_FALSE, PETSC_FALSE, 
            &nLclRows, &row, &col, &done);                        CHKERRQ(ierr);
    ierr = MatSeqAIJRestoreArray(lclA, &data);                    CHKERRQ(ierr);

    // check whether MatRestoreRowIJ worked
    if (! done)
    {
        std::cerr << "MatRestoreRowIJ did not work!" << std::endl;
        exit(0);
    }

    // deallocate col64 and destroy local matrix
    delete [] col64;

    // unlink or destroy lclA
    if (std::strcmp(type, MATSEQAIJ) == 0)
    {
        // unlink lclA
        lclA = nullptr;
    }
    else if (std::strcmp(type, MATMPIAIJ) == 0)
    {
        // destroy local matrix
        ierr = MatDestroy(&lclA);                                 CHKERRQ(ierr);
    }

    // bind the matrix A to the solver
    MPI_Barrier(AmgXComm);
    AMGX_solver_setup(solver, AmgXA);

    // connect (bind) vectors to the matrix
    AMGX_vector_bind(AmgXP, AmgXA);
    AMGX_vector_bind(AmgXRHS, AmgXA);

    // change the state
    isUploaded_A = true;    

    MPI_Barrier(AmgXComm);

    return 0;
}


/**
 * @brief Solve the linear problem based on given RHS and initial guess
 *
 * This function solve the linear problem. Users need to set matrix A before
 * calling this function. The result will be saved back to the PETSc Vec 
 * containing initial guess.
 *
 * It lacks mechanisms to check whether necessary initialization and setting
 * matrix A are done first.
 *
 * @param p Unknowns vector. The values passed in will be an initial guess.
 * @param b Right-hand-side vector.
 *
 * @return Currently meaningless. May be error codes in the future.
 */
int AmgXSolver::solve(Vec &p, Vec &b)
{
    PetscErrorCode      ierr;
    double             *unks,
                       *rhs;
    int                 size;
    AMGX_SOLVE_STATUS   status;

    // get size of local vector (p and b should have the same local size)
    ierr = VecGetLocalSize(p, &size);                             CHKERRQ(ierr);

    // get pointers to the raw data of local vectors
    ierr = VecGetArray(p, &unks);                                 CHKERRQ(ierr);
    ierr = VecGetArray(b, &rhs);                                  CHKERRQ(ierr);

    // upload vectors to AmgX
    AMGX_vector_upload(AmgXP, size, 1, unks);
    AMGX_vector_upload(AmgXRHS, size, 1, rhs);

    // solve
    MPI_Barrier(AmgXComm);
    AMGX_solver_solve(solver, AmgXRHS, AmgXP);

    // get the status of the solver
    AMGX_solver_get_status(solver, &status);

    // check whether the solver successfully solve the problem
    if (status != AMGX_SOLVE_SUCCESS)
        std::cout << "AmgX solver failed to solve the problem! "
                  << "Solver status: " << status << std::endl;

    // download data from device
    AMGX_vector_download(AmgXP, unks);

    // restore PETSc vectors
    ierr = VecRestoreArray(p, &unks);                             CHKERRQ(ierr);
    ierr = VecRestoreArray(b, &rhs);                              CHKERRQ(ierr);

    return 0;
}


/**
 * @brief Setting up AmgX mode.
 *
 * Convert a STL string to AmgX mode and then store this mode. For the usage of 
 * AmgX modes, please refer to AmgX manual.
 *
 * @param _mode A STL string describing the mode.
 *
 * @return Currently meaningless. May be error codes in the future.
 */
int AmgXSolver::setMode(const std::string &_mode)
{
    if (_mode == "hDDI")
        mode = AMGX_mode_hDDI;
    else if (_mode == "hDFI")
        mode = AMGX_mode_hDFI;
    else if (_mode == "hFFI")
        mode = AMGX_mode_hFFI;
    else if (_mode == "dDDI")
        mode = AMGX_mode_dDDI;
    else if (_mode == "dDFI")
        mode = AMGX_mode_dDFI;
    else if (_mode == "dFFI")
        mode = AMGX_mode_dFFI;
    else
    {
        std::cerr << "error: " 
                  << _mode << " is not an available mode." << std::endl;
        exit(0);
    }
    return 0;
}


/**
 * @brief Obtaining a partition vector required by AmgX.
 *
 * This function generate a vector describing which processor an entry of the 
 * solution vector is located on. So the size of this partition vector will be 
 * the same as the number of unknowns.
 *
 * This function is currently very naive. The code is simple, but the efficiency
 * is not very good.
 *
 * @param A A PETSc Mat. The coefficient matrix.
 * @param partVec A raw pointer pointing to int.
 *
 * @return Currently meaningless. May be error codes in the future.
 */
int AmgXSolver::getPartVec(const Mat &A, int *& partVec)
{
    PetscErrorCode      ierr;
    PetscInt            size,
                        bg,
                        ed;
    VecScatter          scatter;
    Vec                 tempMPI,
                        tempSEQ;

    PetscScalar        *tempPartVec;

    ierr = MatGetOwnershipRange(A, &bg, &ed);                     CHKERRQ(ierr);
    ierr = MatGetSize(A, &size, nullptr);                         CHKERRQ(ierr);


    ierr = VecCreate(AmgXComm, &tempMPI);                         CHKERRQ(ierr);
    ierr = VecSetSizes(tempMPI, PETSC_DECIDE, size);              CHKERRQ(ierr);
    ierr = VecSetType(tempMPI, VECMPI);                           CHKERRQ(ierr);

    for(PetscInt i=bg; i<ed; ++i)
    {
        ierr = VecSetValue(tempMPI, i, myRank, INSERT_VALUES);    CHKERRQ(ierr);
    }
    ierr = VecAssemblyBegin(tempMPI);                             CHKERRQ(ierr);
    ierr = VecAssemblyEnd(tempMPI);                               CHKERRQ(ierr);

    ierr = VecScatterCreateToAll(tempMPI, &scatter, &tempSEQ);    CHKERRQ(ierr);
    ierr = VecScatterBegin(scatter, tempMPI, tempSEQ, 
            INSERT_VALUES, SCATTER_FORWARD);                      CHKERRQ(ierr);
    ierr = VecScatterEnd(scatter, tempMPI, tempSEQ, 
            INSERT_VALUES, SCATTER_FORWARD);                      CHKERRQ(ierr);
    ierr = VecScatterDestroy(&scatter);                           CHKERRQ(ierr);
    ierr = VecDestroy(&tempMPI);                                  CHKERRQ(ierr);

    ierr = VecGetArray(tempSEQ, &tempPartVec);                    CHKERRQ(ierr);

    partVec = new int[size];
    for(size_t i=0; i<size; ++i)
        partVec[i] = static_cast<int>(tempPartVec[i]);

    ierr = VecRestoreArray(tempSEQ, &tempPartVec);                CHKERRQ(ierr);

    ierr = VecDestroy(&tempSEQ);                                  CHKERRQ(ierr);
    ierr = VecDestroy(&tempMPI);                                  CHKERRQ(ierr);

    return 0;
}


/**
 * @brief Get the number of iterations of last solve phase
 *
 * @return number of iterations
 */
int AmgXSolver::getIters()
{
    int         iter;

    AMGX_solver_get_iterations_number(solver, &iter);

    return iter;
}


/**
 * @brief Get the residual at a specific iteration in last solve phase
 *
 * @param iter a specific iteration during the last solve phase
 *
 * @return residual
 */
double AmgXSolver::getResidual(const int &iter)
{
    double      res;

    AMGX_solver_get_iteration_residual(solver, iter, 0, &res);

    return res;
}


/**
 * @brief A printing function, using stdout, needed by AmgX.
 *
 * @param msg C-style string
 * @param length The length of the string
 */
void AmgXSolver::print_callback(const char *msg, int length)
{
    std::cout << msg;
}


/**
 * @brief A printing function, print nothing, needed by AmgX.
 *
 * @param msg C-style string
 * @param length The length of the string
 */
void AmgXSolver::print_none(const char *msg, int length) { }
