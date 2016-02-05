/**
 * @file structArgs.cpp
 * @brief Definition of the structure StructArgs
 * @author Pi-Yueh Chuang (pychuang@gwu.edu)
 * @version alpha
 * @date 2015-11-06
 */
# include <petscsys.h>
# include <iostream>

# define MAX_LEN PETSC_MAX_PATH_LEN

class StructArgs
{
    public: 
        PetscInt            Nx,     // number of elements in x-direction
                            Ny,     // number of elements in y-direction
                            Nz;     // number of elements in z-direction

        PetscInt            Nruns = 10; // number of runs of solving

        PetscBool           optFileBool,
                            VTKFileBool;

        char                mode[MAX_LEN],        // either GPU, CPU, or PETSc
                            cfgFileName[MAX_LEN], // config file
                            optFileName[MAX_LEN], // output file
                            caseName[MAX_LEN],    // case name
                            VTKFileName[MAX_LEN]; // VTK output file name

        void print()
        {
            std::cout << "Case Name: " << caseName << std::endl;
            std::cout << "Nx: " << Nx << std::endl;
            std::cout << "Ny: " << Ny << std::endl;
            std::cout << "Nz: " << Nz << std::endl;
            std::cout << "Mode: " << mode << std::endl;
            std::cout << "Config File: " << cfgFileName << std::endl;
            std::cout << "Number of Solves: " << Nruns << std::endl;

            std::cout << "Output PETSc Log File ? " << optFileBool << std::endl;
            if (optFileBool == PETSC_TRUE)
                std::cout << "Output PETSc Log File Name: " 
                          << optFileName << std::endl;

            std::cout << "Output VTK file ? " << VTKFileBool << std::endl;
            if (VTKFileBool == PETSC_TRUE)
                std::cout << "Output VTK File Name: " 
                          << VTKFileName << std::endl;
        }
};
