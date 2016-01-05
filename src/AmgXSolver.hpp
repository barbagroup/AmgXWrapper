/**
 * @file AmgXSolver.hpp
 * @brief Declaration of the class AmgXSolver
 * @author Pi-Yueh Chuang (pychuang@gwu.edu)
 * @version alpha
 * @date 2015-09-01
 */

# pragma once

# include <iostream>
# include <string>
# include <cstring>
# include <petscmat.h>
# include <petscvec.h>
# include <amgx_c.h>

/**
 * @brief A wrapper class for an interface between PETSc and AmgX
 *
 * This class is a wrapper of AmgX library. PETSc user only need to pass 
 * PETSc matrix and vectors into the AmgXSolver instance to solve their problem.
 *
 */
class AmgXSolver
{
    public:

        /// default constructor
        AmgXSolver() = default;

        /// initialization of instance
        int initialize(MPI_Comm comm, 
                const std::string &_mode, const std::string &cfg_file);

        /// finalization
        int finalize();

        /// convert PETSc matrix into AmgX matrix and pass it to solver
        int setA(Mat &A);

        /// solve the problem, soultion vector will be updated in the end
        int solve(Vec &p, Vec &b);

        /// Get the number of iterations of last solve phase
        int getIters();

        /// Get the residual at a specific iteration in last solve phase
        double getResidual(const int &iter);

        /// get the memory usage on device
        int getMemUsage();


    private:

        static int              count;      /*!< only one instance allowed*/
        static AMGX_resources_handle   rsrc;       /*< AmgX resource object*/

        bool                    isInitialized = false,  /*< as its name*/
                                isUploaded_A = false,   /*< as its name*/
                                isUploaded_P = false,   /*< as its name*/
                                isUploaded_B = false;   /*< as its name*/






        int                     nDevs,      /*< # of cuda devices*/
                                devID;

        int                     gpuProc = MPI_UNDEFINED;

        MPI_Comm                globalCpuWorld,
                                localCpuWorld,
                                gpuWorld,
                                devWorld;

        int                     globalSize,
                                localSize,
                                gpuWorldSize,
                                devWorldSize;

        int                     myGlobalRank,
                                myLocalRank,
                                myGpuWorldRank,
                                myDevWorldRank;

        int                     nodeNameLen;
        char                    nodeName[MPI_MAX_PROCESSOR_NAME];



        int                     ring;       /*< a parameter used by AmgX*/

        AMGX_Mode               mode;       /*< AmgX mode*/
        AMGX_config_handle      cfg;        /*< AmgX config object*/
        AMGX_matrix_handle      AmgXA;      /*< AmgX coeff mat*/
        AMGX_vector_handle      AmgXP,      /*< AmgX unknowns vec*/
                                AmgXRHS;    /*< AmgX RHS vec*/
        AMGX_solver_handle      solver;     /*< AmgX solver object*/


        /// set up the mode of AmgX solver
        int setMode(const std::string &_mode);

        /// generate a partition vector required by AmgX
        int getPartVec(const Mat &A, int *& partVec);

        /// a printing function using stdout
        static void print_callback(const char *msg, int length);

        /// a printing function that prints nothing, used by AmgX
        static void print_none(const char *msg, int length);


        int initMPIcomms(MPI_Comm &comm);

};
