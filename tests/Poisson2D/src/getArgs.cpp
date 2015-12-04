/**
 * @file getArgs.cpp
 * @brief Get parameters from command-line arguments
 * @author Pi-Yueh Chuang (pychuang@gwu.edu)
 * @version alpha
 * @date 2015-11-06
 */
# include "StructArgs.hpp"

# define CHKMSG(flag, message)                  \
    if (!flag)                                  \
    {                                           \
        PetscPrintf(PETSC_COMM_WORLD, message); \
        PetscPrintf(PETSC_COMM_WORLD, "\n");    \
        exit(EXIT_FAILURE);                     \
    }

PetscErrorCode getArgs(StructArgs & args)
{
    PetscErrorCode      ierr;   // error codes returned by PETSc routines
    PetscBool           set;    // temporary booling variable

    ierr = PetscOptionsGetString(nullptr, nullptr, "-caseName", 
            args.caseName, MAX_LEN, &set);                         CHKERRQ(ierr);
    CHKMSG(set, "caseName not yet set!");

    ierr = PetscOptionsGetString(nullptr, nullptr, "-mode", 
            args.mode, MAX_LEN, &set);                             CHKERRQ(ierr);
    CHKMSG(set, "mode not yet set!");

    ierr = PetscOptionsGetString(nullptr, nullptr, "-cfgFileName", 
            args.cfgFileName, MAX_LEN, &set);                      CHKERRQ(ierr);
    CHKMSG(set, "cfgFileName (configuration file) not yet set!");

    ierr = PetscOptionsGetString(nullptr, nullptr, "-optFileName", 
            args.optFileName, MAX_LEN, &(args.optFileBool));       CHKERRQ(ierr);

    ierr = PetscOptionsGetString(nullptr, nullptr, "-VTKFileName", 
            args.VTKFileName, MAX_LEN, &(args.VTKFileBool));       CHKERRQ(ierr);

    ierr = PetscOptionsGetInt(nullptr, nullptr, 
            "-Nx", &args.Nx, &set);                                CHKERRQ(ierr);
    CHKMSG(set, "Nx not yet set!");

    ierr = PetscOptionsGetInt(nullptr, nullptr, 
            "-Ny", &args.Ny, &set);                                CHKERRQ(ierr);
    CHKMSG(set, "Ny not yet set!");

    return 0;
}
