# include <iostream>
# include <string>


# include <petscmat.h>
# include <petscvec.h>

# define CHK CHKERRQ(ierr)


PetscErrorCode PetscOptionsGetString(
        PetscOptions options, const char pre[], const char name[],
        std::string &string, size_t len, PetscBool *set);


int main(int argc, char **argv)
{
    PetscErrorCode      ierr;
    PetscViewer         viewer;

    Mat                 A;

    std::string         matrixFileName;

    PetscBool           matrixFileBool;


    ierr = PetscInitialize(&argc, &argv, nullptr, nullptr);                  CHK;


    ierr = PetscOptionsGetString(nullptr, nullptr, "-matrixFileName", 
            matrixFileName, PETSC_MAX_PATH_LEN, &matrixFileBool);            CHK;


    if (matrixFileBool == PETSC_FALSE)
    {
        std::cerr << "No matrix file input!" << std::endl;
        return 1;
    }


    // read matrix A
    {
        ierr = MatCreate(PETSC_COMM_WORLD, &A);                              CHK;
        ierr = MatSetType(A, MATAIJ);                                        CHK;

        ierr = PetscViewerBinaryOpen(PETSC_COMM_WORLD, 
                matrixFileName.c_str(), FILE_MODE_READ, &viewer);            CHK;

        ierr = MatLoad(A, viewer);                                           CHK;

        ierr = PetscViewerDestroy(&viewer);                                  CHK;
    }


    // output plot
    ierr = MatView(A, PETSC_VIEWER_DRAW_WORLD);                              CHK;

    ierr = MatDestroy(&A);                                                   CHK;

    ierr = PetscFinalize();                                                  CHK;

    return 0;
}


PetscErrorCode PetscOptionsGetString(
        PetscOptions options, const char pre[], const char name[],
        std::string &string, size_t len, PetscBool *set)
{
    PetscErrorCode      ierr;
    char                c_string[PETSC_MAX_PATH_LEN];

    ierr = PetscOptionsGetString(options, pre, name, c_string, len, set); 

    string.assign(c_string);

    return ierr;
}
