/**
 * @file structArgs.cpp
 * @brief Definition of the structure StructArgs
 * @author Pi-Yueh Chuang (pychuang@gwu.edu)
 * @version alpha
 * @date 2015-11-06
 */
# include <petscsys.h>

# define MAX_LEN PETSC_MAX_PATH_LEN

struct StructArgs
{
    PetscInt            Nx,     // number of elements in x-direction
                        Ny;     // number of elements in y-direction

    PetscBool           optFileBool,
                        VTKFileBool;

    char                mode[MAX_LEN],        // either GPU, CPU, or PETSc
                        cfgFileName[MAX_LEN], // config file
                        optFileName[MAX_LEN], // output file
                        caseName[MAX_LEN],    // case name
                        VTKFileName[MAX_LEN];
};
