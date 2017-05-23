/**
 * @file AmgXSolver.hpp
 * @brief Definition of class AmgXSolver
 * @author Pi-Yueh Chuang (pychuang@gwu.edu)
 * @date 2015-09-01
 */

# pragma once

// STL
# include <iostream>
# include <string>
# include <cstring>
# include <functional>
# include <vector>
# include <cstdlib>

// CUDA
# include <cuda_runtime.h>

// AmgX
# include <amgx_c.h>

// PETSc
# include <petscmat.h>
# include <petscvec.h>
# include <petscis.h>

// others
# include "check.hpp"

/**
 * @brief A wrapper class for coupling PETSc and AmgX.
 *
 * This class is a wrapper of AmgX library for PETSc. PETSc users only need to
 * pass a PETSc matrix and vectors into the AmgXSolver instance to solve their
 * problems.
 *
 */
class AmgXSolver
{
    public:

        /** \brief default constructor. */
        AmgXSolver() = default;


        /** \brief destructor. */
        ~AmgXSolver() = default;


        /**
         * \brief initialize a AmgXSolver instance.
         *
         * \param comm [in] MPI communicator.
         * \param mode [in] a string, target mode of AmgX (e.g., dDDI).
         * \param cfgFile [in] a string indicate the path to AmgX configuration file.
         *
         * \return PetscErrorCode.
         */
        PetscErrorCode initialize(MPI_Comm comm,
                const std::string &mode, const std::string &cfgFile);


        /**
         * \brief finalization.
         *
         * \return PetscErrorCode.
         */
        PetscErrorCode finalize();


        /**
         * \brief set up the matrix used by AmgX.
         *
         * \param A [in] a PETSc Mat.
         *
         * This function will automatically convert PETSc matrix to AmgX matrix.
         * If the AmgX solver is set to be the GPU one, we also redestribute the
         * matrix in this function and upload it to GPUs.
         *
         * \return PetscErrorCode.
         */
        PetscErrorCode setA(const Mat &A);


        /**
         * \brief solve the linear system.
         *
         * \param p [in, out] a PETSc Vec object representing unknowns.
         * \param b [in] a PETSc Vec representing right hand side.
         *
         * p vector will be used as initial guess and will be updated to the 
         * solution in the end of solving.
         *
         * For cases that use more MPI processes than the number of GPUs, this 
         * function will do data gathering before solving and data scattering 
         * after the solving.
         *
         * \return PetscErrorCode.
         */
        PetscErrorCode solve(Vec &p, Vec &b);


        /**
         * \brief get the number of iterations of the last solving.
         *
         * \return PetscErrorCode.
         */
        PetscErrorCode getIters();


        /**
         * \brief get the residual at a specific iteration during the last solving.
         *
         * \param iter [in] the target iteration.
         * \param res [out] the returned residual.
         *
         * \return 
         */
        PetscErrorCode getResidual(const int &iter, double &res);


        /**
         * \brief get the memory usage on devices.
         *
         * \return PetscErrorCode.
         */
        PetscErrorCode getMemUsage();


    private:



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



        static int              count;      /*!< only one instance allowed*/
        int                     ring;       /*< a parameter used by AmgX*/

        AMGX_Mode               mode;               /*< AmgX mode*/
        AMGX_config_handle      cfg = nullptr;      /*< AmgX config object*/
        AMGX_matrix_handle      AmgXA = nullptr;    /*< AmgX coeff mat*/
        AMGX_vector_handle      AmgXP = nullptr,    /*< AmgX unknowns vec*/
                                AmgXRHS = nullptr;  /*< AmgX RHS vec*/
        AMGX_solver_handle      solver = nullptr;   /*< AmgX solver object*/
        static AMGX_resources_handle   rsrc;        /*< AmgX resource object*/



        bool                    isInitialized = false;  /*< as its name*/



        /// set up the mode of AmgX solver
        int setMode(const std::string &_mode);

        /// a printing function using stdout
        static void print_callback(const char *msg, int length);

        /// a printing function that prints nothing, used by AmgX
        static void print_none(const char *msg, int length);


        int initMPIcomms(MPI_Comm &comm);
        int initAmgX(const std::string &_mode, const std::string &_cfg);


        VecScatter      redistScatter = nullptr;
        Vec             redistLhs = nullptr,
                        redistRhs = nullptr;

        int getLocalMatRawData(Mat &localA, PetscInt &localN,
                std::vector<PetscInt> &row, std::vector<Petsc64bitInt> &col,
                std::vector<PetscScalar> &data);

        int getDevIS(const Mat &A, IS &devIS);
        int getLocalA(const Mat &A, const IS &devIS, Mat &localA);
        int redistMat(const Mat &A, const IS &devIS, Mat &newA);
        int getPartVec(
                const IS &devIS, const PetscInt &N, std::vector<PetscInt> &partVec);
        int destroyLocalA(const Mat &A, Mat &localA);
        int getVecScatter(const Mat &A1, const Mat &A2, const IS &devIS);

        int solve_real(Vec &p, Vec &b);

};
