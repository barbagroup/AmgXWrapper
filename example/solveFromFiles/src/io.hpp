/**
 * \file io.hpp
 * \brief prototypes of functions in io.cpp
 * \author Pi-Yueh Chuang (pychuang@gwu.edu)
 * \date 2017-05-31
 */


# pragma once


// PETSc
# include <petscmat.h>
# include <petscvec.h>


/**
 * \brief read a matrix from a PETSc binary file.
 *
 * \param mat [out] the resulting PETSc Mat.
 * \param FN [in] the file name of the input file.
 * \param name [in] the name going to be used for the matrix.
 *
 * \return PetscErrorCode.
 */
PetscErrorCode readMat(Mat &mat, char *FN, const char *name);


/**
 * \brief read a vector from a PETSc binary file.
 *
 * \param vec [out] the resulting PETSc Vec.
 * \param FN [in] the file name of the input file.
 * \param name [in] the name going to be used for the vector.
 *
 * \return PetscErrorCode.
 */
PetscErrorCode readVec(Vec &vec, char *FN, const char *name);
