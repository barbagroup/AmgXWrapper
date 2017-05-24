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
        printf("Error: %s:%d, ", __FILE__, __LINE__);                       \
        printf("code:%d, reason: %s\n", error, cudaGetErrorString(error));  \
        exit(1);                                                            \
    }                                                                       \
}


// abbreviation for check PETSc returned error code
# define CHK CHKERRQ(ierr)
