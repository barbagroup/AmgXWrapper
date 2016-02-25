# include <fstream>
# include <iostream>
# include <string>
# include <regex>

# include <petsc.h>


PetscErrorCode PetscOptionsGetString(
        PetscOptions options, const char pre[], const char name[],
        std::string &string, size_t len, PetscBool *set);

int main(int argc, char **argv)
{
    PetscErrorCode          ierr;

    std::string             inFileName,
                            outFileName,
                            type,
                            object,
                            format,
                            field,
                            symmetry;

    std::ifstream           fin;

    PetscViewer             fout;

    PetscInt                rank,
                            size;

    ierr = PetscInitialize(&argc, &argv, nullptr, nullptr);        CHKERRQ(ierr);

    ierr = MPI_Comm_size(PETSC_COMM_WORLD, &size);                 CHKERRQ(ierr);
    ierr = MPI_Comm_rank(PETSC_COMM_WORLD, &rank);                 CHKERRQ(ierr);

    if ((size > 1) && (rank == 0))
        std::cout << "Too many processes involved. "
                  << "Only the rank 0 will be used." << std::endl;

    if (rank != 0)
    {
        ierr = PetscFinalize();                                    CHKERRQ(ierr);
        return 0;
    }

    ierr = PetscOptionsGetString(nullptr, nullptr, "-fin", 
            inFileName, PETSC_MAX_PATH_LEN, nullptr);              CHKERRQ(ierr);

    ierr = PetscOptionsGetString(nullptr, nullptr, "-fout", 
            outFileName, PETSC_MAX_PATH_LEN, nullptr);             CHKERRQ(ierr);

    ierr = PetscOptionsGetString(nullptr, nullptr, "-type", 
            type, PETSC_MAX_PATH_LEN, nullptr);                    CHKERRQ(ierr);


    std::cout << "Opening the input MatrixMarket file ..." << std::endl;
    fin.open(inFileName, std::ifstream::in);

    if (! fin.is_open())
    {
        std::cerr << "File not opened!" << std::endl;
        return 1;
    }


    std::cout << "Read format of the data ..." << std::endl;
    while ((fin.peek() == '%') && (fin.good()))
    {
        std::string         line;
        std::regex          info("MatrixMarket ([[:alpha:]]+) "
                                              "([[:alpha:]]+) "
                                              "([[:alpha:]]+) "
                                              "([[:alpha:]]+)");
        std::smatch         match;
        
        
        std::getline(fin, line);
        if (std::regex_search(line, match, info))
        {
            object = match.str(1);
            format = match.str(2);
            field = match.str(3);
            symmetry = match.str(4);

            std::cout << "\tThe format is: "
                      << object << " " << format << " " 
                      << field << " " << symmetry << std::endl;
        }
    }

    if ((object.length() == 0) || (format.length() == 0) ||
            (field.length() == 0) || (symmetry.length() == 0)) 
    {
        std::cerr << "Headline is not correct!" << std::endl;
        std::cout << "\tobject: " << object 
                  << "\tformat: " << format 
                  << "\tfield: " << field 
                  << "\tsymmetry: " << symmetry << std::endl;
        return 1;
    }


    if ((type == "matrix") && (object == "matrix") && (format == "coordinate"))
    {

        PetscInt    id1,
                    id2,
                    nNZ;

        Mat         A;

        std::cout << "Reading the size of the matrix ..." << std::endl;
        fin >> id1 >> id2 >> nNZ;

        std::cout << "\tThe size of the matrix is: " << id1 << " x " << id2
                  << " with " << nNZ << " non-zeros." << std::endl;


        std::cout << "Creating PETSc SEQAIJ matrix ..." << std::endl;
        ierr = MatCreate(PETSC_COMM_SELF, &A);                     CHKERRQ(ierr);
        ierr = MatSetType(A, MATSEQAIJ);                           CHKERRQ(ierr);
        ierr = MatSetSizes(A, PETSC_DECIDE, PETSC_DECIDE, id1, id2); CHKERRQ(ierr);



        std::vector<PetscInt>                   nNZCount(id1, 0);
        std::vector<std::vector<PetscInt>>      col(id1);
        std::vector<std::vector<PetscScalar>>   values(id1);


        std::cout << "Reading data ..." << std::endl;
        for(int i=0; i<nNZ; ++i)
        {
            PetscScalar     value;

            fin >> id1 >> id2 >> value;
            nNZCount[id1-1] += 1;
            col[id1-1].push_back(id2-1);
            values[id1-1].push_back(value);
        }
        fin.close();
        std::cout << "Reading data finished ..." << std::endl;



        std::cout << 
            "Setting preallocation for the PETSc matrix ..." << std::endl;
        ierr = MatSeqAIJSetPreallocation(A, 0, nNZCount.data()); CHKERRQ(ierr);

        std::cout << "Setting values in the PETSc matrix ..." << std::endl;
        for(int i=0; i<nNZCount.size(); ++i)
        {
            ierr = MatSetValues(A, 1, &i, col[i].size(), col[i].data(), 
                    values[i].data(), INSERT_VALUES);              CHKERRQ(ierr); 
        }

        std::cout << "Assemblying the PETSc matrix ..." << std::endl;
        ierr = MatAssemblyBegin(A, MAT_FINAL_ASSEMBLY);            CHKERRQ(ierr);
        ierr = MatAssemblyEnd(A, MAT_FINAL_ASSEMBLY);              CHKERRQ(ierr);
        std::cout << "Assemblying the PETSc matrix finished ..." << std::endl;


        std::cout << "Writing matrix to PETSc binary file ..." << std::endl;
        ierr = PetscViewerBinaryOpen(PETSC_COMM_SELF, 
                outFileName.c_str(), FILE_MODE_WRITE, &fout);      CHKERRQ(ierr);
        ierr = MatView(A, fout);                                   CHKERRQ(ierr);
        ierr = PetscViewerDestroy(&fout);                          CHKERRQ(ierr);
        ierr = MatDestroy(&A);                                     CHKERRQ(ierr);
        std::cout << 
            "Writing matrix to PETSc binary file finished..." << std::endl;

        ierr = PetscFinalize();                                    CHKERRQ(ierr);

        return 0;
    }


    if ((type == "vector") && (object == "matrix") && (format == "array"))
    {
        PetscInt    n,
                    id;

        Vec         v;

        std::cout << "Reading the size of the vector ..." << std::endl;
        fin >> n >> id;

        if (id != 1)
        {
            std::cerr << "The number column of the data is not 1 !!" << std::endl;
            std::cerr << "This may not be data for a vector!!" << std::endl;
            return 1;
        }

        std::cout << "\tThe size of the vector is: " << n << std::endl;


        std::cout << "Creating PETSc SEQ vector ..." << std::endl;
        ierr = VecCreate(PETSC_COMM_SELF, &v);                     CHKERRQ(ierr);
        ierr = VecSetType(v, VECSEQ);                              CHKERRQ(ierr);
        ierr = VecSetSizes(v, PETSC_DECIDE, n);                    CHKERRQ(ierr);


        std::cout << "Reading data and setting PETSc vector..." << std::endl;
        for(int i=0; i<n; ++i)
        {
            PetscScalar     value;

            fin >> value;
            ierr = VecSetValue(v, i, value, INSERT_VALUES);        CHKERRQ(ierr);
        }
        fin.close();

        std::cout << 
            "Reading data and setting PETSc vector finished ..." << std::endl;

        std::cout << "Assemblying the PETSc vector ..." << std::endl;
        ierr = VecAssemblyBegin(v);                                CHKERRQ(ierr);
        ierr = VecAssemblyEnd(v);                                  CHKERRQ(ierr);
        std::cout << "Assemblying the PETSc vector finished ..." << std::endl;


        std::cout << "Writing vector to PETSc binary file ..." << std::endl;
        ierr = PetscViewerBinaryOpen(PETSC_COMM_SELF, 
                outFileName.c_str(), FILE_MODE_WRITE, &fout);      CHKERRQ(ierr);
        ierr = VecView(v, fout);                                   CHKERRQ(ierr);
        ierr = PetscViewerDestroy(&fout);                          CHKERRQ(ierr);
        ierr = VecDestroy(&v);                                     CHKERRQ(ierr);
        std::cout << 
            "Writing vector to PETSc binary file finished..." << std::endl;

        ierr = PetscFinalize();                                    CHKERRQ(ierr);

        return 0;
    }


    std::cerr << 
        "It seems this kind of format has not been supported yet!!" << std::endl;

    ierr = PetscFinalize();                                    CHKERRQ(ierr);


    return 1;
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
