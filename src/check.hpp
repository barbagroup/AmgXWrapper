/**
 * \file check.hpp
 * \brief some macro definitions for checking function returns. 
 * \author Pi-Yueh Chuang (pychuang@gwu.edu)
 * \date 2017-05-23
 */


# pragma once


// for checking CUDA function call
# define CHECK(call)                                                        \
{                                                                           \
    const cudaError_t       error = call;                                   \
    if (error != cudaSuccess)                                               \
    {                                                                       \
        SETERRQ4(PETSC_COMM_WORLD, PETSC_ERR_SIG,                           \
            "Error: %s:%d, code:%d, reason: %s\n",                          \
            __FILE__, __LINE__, error, cudaGetErrorString(error));          \
    }                                                                       \
}


// abbreviation for check PETSc returned error code
# define CHK CHKERRQ(ierr)
