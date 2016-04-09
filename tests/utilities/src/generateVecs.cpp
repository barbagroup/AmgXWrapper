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

    Vec                 rhs,
                        exact;

    std::string         matrixFileName,
                        exactFileName,
                        rhsFileName;

    PetscBool           matrixFileBool,
                        exactFileBool,
                        rhsFileBool;


    ierr = PetscInitialize(&argc, &argv, nullptr, nullptr);                  CHK;


    ierr = PetscOptionsGetString(nullptr, nullptr, "-matrixFileName", 
            matrixFileName, PETSC_MAX_PATH_LEN, &matrixFileBool);            CHK;

    ierr = PetscOptionsGetString(nullptr, nullptr, "-exactFileName", 
            exactFileName, PETSC_MAX_PATH_LEN, &exactFileBool);              CHK;


    if (matrixFileBool == PETSC_FALSE)
    {
        std::cerr << "No matrix file input!" << std::endl;
        return 1;
    }

    {
        std::size_t     pos = matrixFileName.find_last_of('.');

        rhsFileName = matrixFileName;
        rhsFileName.insert(pos, "_rhs");
    }


    {
        ierr = MatCreate(PETSC_COMM_WORLD, &A);                              CHK;
        ierr = MatSetType(A, MATAIJ);                                        CHK;

        ierr = PetscViewerBinaryOpen(PETSC_COMM_WORLD, 
                matrixFileName.c_str(), FILE_MODE_READ, &viewer);            CHK;

        ierr = MatLoad(A, viewer);                                           CHK;

        ierr = PetscViewerDestroy(&viewer);                                  CHK;
    }

    if (exactFileBool == PETSC_TRUE)
    {
        ierr = VecCreate(PETSC_COMM_WORLD, &exact);                          CHK;
        ierr = VecSetType(exact, VECSTANDARD);                               CHK;

        ierr = PetscViewerBinaryOpen(PETSC_COMM_WORLD, 
                exactFileName.c_str(), FILE_MODE_READ, &viewer);             CHK;

        ierr = VecLoad(exact, viewer);                                       CHK;

        ierr = PetscViewerDestroy(&viewer);                                  CHK;

        ierr = VecDuplicate(exact, &rhs);                                    CHK;
        ierr = MatMult(A, exact, rhs);                                       CHK;

        ierr = PetscViewerBinaryOpen(PETSC_COMM_WORLD, 
                rhsFileName.c_str(), FILE_MODE_WRITE, &viewer);              CHK;

        ierr = VecView(rhs, viewer);                                         CHK;
        ierr = PetscViewerDestroy(&viewer);                                  CHK;
    }
    else
    {
        {
            std::size_t     pos = matrixFileName.find_last_of('.');

            exactFileName = matrixFileName;
            exactFileName.insert(pos, "_exact");
        }

        PetscInt        nx,
                        ny;

        ierr = MatGetSize(A, &nx, &ny);                                      CHK;

        ierr = VecCreate(PETSC_COMM_WORLD, &exact);                          CHK;
        ierr = VecSetType(exact, VECSTANDARD);                               CHK;
        ierr = VecSetSizes(exact, PETSC_DECIDE, ny);                         CHK;
        ierr = VecSetRandom(exact, nullptr);                                 CHK;

        ierr = PetscViewerBinaryOpen(PETSC_COMM_WORLD, 
                exactFileName.c_str(), FILE_MODE_WRITE, &viewer);            CHK;
        ierr = VecView(exact, viewer);                                       CHK;
        ierr = PetscViewerDestroy(&viewer);                                  CHK;

        ierr = VecDuplicate(exact, &rhs);                                    CHK;
        ierr = MatMult(A, exact, rhs);                                       CHK;

        ierr = PetscViewerBinaryOpen(PETSC_COMM_WORLD, 
                rhsFileName.c_str(), FILE_MODE_WRITE, &viewer);              CHK;
        ierr = VecView(rhs, viewer);                                         CHK;
        ierr = PetscViewerDestroy(&viewer);                                  CHK;
    }


    ierr = MatDestroy(&A);                                                   CHK;
    ierr = VecDestroy(&exact);                                               CHK;
    ierr = VecDestroy(&rhs);                                                 CHK;

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
