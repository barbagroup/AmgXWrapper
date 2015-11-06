# =====================================================================
# @file CMakeLists.txt
# @brief for cmake
# @author Pi-Yueh Chuang (pychuang@gwu.edu)
# @version alpha
# @date 2015-09-01
# =====================================================================


CMAKE_MINIMUM_REQUIRED(VERSION 2.8)


MESSAGE("===================================================================")
MESSAGE("Specifying CXX compiler and flags...\n")
MESSAGE("NOTE: clang++ is default C++ compiler. To use other compiler, ")
MESSAGE("please define the environment variable CXX\n")

IF (NOT DEFINED ENV{CXX})
    SET(CMAKE_CXX_COMPILER "clang++")
    MESSAGE("CXX compiler: clang++")
ELSE()
    SET(CMAKE_CXX_COMPILER $ENV{CXX})
    MESSAGE("CXX compiler: " ${CMAKE_CXX_COMPILER})
ENDIF()


SET(CMAKE_CXX_FLAGS "-std=c++11" CACHE STRING "")
SET(CMAKE_CXX_FLAGS_DEBUG "-g" CACHE STRING "")
SET(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG" CACHE STRING "")



MESSAGE("\n===================================================================")
MESSAGE("Specifying C compiler and flags...\n")
MESSAGE("NOTE: clang is default C compiler. To use other compiler, ")
MESSAGE("please define the environment variable CXX\n")

IF (NOT DEFINED ENV{CC})
    SET(CMAKE_C_COMPILER "clang")
    MESSAGE("C compiler: clang")
ELSE()
    SET(CMAKE_C_COMPILER $ENV{CC})
    MESSAGE("C compiler: " ${CMAKE_C_COMPILER})
ENDIF()

SET(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG" CACHE STRING "")
SET(CMAKE_C_FLAGS_DEBUG "-g" CACHE STRING "")



MESSAGE("\n===================================================================")
MESSAGE("Specifying other variables...")
SET(CMAKE_VERBOSE_MAKEFILE ON)
SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)

IF(NOT DEFINED CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE RELEASE CACHE STRING "")
ENDIF()

MESSAGE("-- Build type: " ${CMAKE_BUILD_TYPE})

SET(CMAKE_SHARED_LINKER_FLAGS "-Wl,--no-undefined")

MESSAGE("\n===================================================================")
MESSAGE("Starting generator...")
PROJECT(Poisson3D)


MESSAGE("\n===================================================================")
MESSAGE("Trying to find PETSc...")
MESSAGE("\nNOTE: If it fails to find PETSc, please set the environment variable")
MESSAGE("PETSC_DIR in order to notify CMake the location of PETSc\n")


MESSAGE("-- Finding petscksp.h")
FIND_PATH(PETSC_INCLUDE_DIRS 
          NAMES petscksp.h  
          PATHS ${PETSC_DIR} $ENV{PETSC_DIR} 
          PATH_SUFFIXES "include")

IF(${PETSC_INCLUDE_DIRS} STREQUAL "PETSC_INCLUDE_DIRS-NOTFOUND")
    MESSAGE(FATAL_ERROR "Could not find petscksp.h!!!")
ELSE()
    MESSAGE("-- Finding petscksp.h - found in " ${PETSC_INCLUDE_DIRS})
ENDIF()


MESSAGE("-- Finding libpetsc.so or libpetsc.a")
FIND_LIBRARY(PETSC_LIBRARIES
             NAMES petsc
             PATHS ${PETSC_DIR} $ENV{PETSC_DIR}
             PATH_SUFFIXES "lib")

IF(${PETSC_LIBRARIES} STREQUAL "PETSC_LIBRARIES-NOTFOUND")
    MESSAGE(FATAL_ERROR "libpetsc.so or libpetsc.a was not found")
ELSE()
    MESSAGE("-- Finding libpetsc.so or libpetsc.a - found: " ${PETSC_LIBRARIES})
ENDIF()



MESSAGE("\n===================================================================")
MESSAGE("Trying to find AmgX...")
MESSAGE("\nNOTE: If it fails to find AmgX, please set the environment variable")
MESSAGE("AMGX_DIR in order to notify CMake the location of AmgX\n")


MESSAGE("-- Finding amgx_c.h")
FIND_PATH(AMGX_INCLUDE_DIRS 
          NAMES amgx_c.h  
          PATHS ${AMGX_DIR} $ENV{AMGX_DIR} 
          PATH_SUFFIXES "include")

IF(${AMGX_INCLUDE_DIRS} STREQUAL "AMGX_INCLUDE_DIRS-NOTFOUND")
    MESSAGE(FATAL_ERROR "Could not find amgx_c.h!!!")
ELSE()
    MESSAGE("-- Finding amgx_c.h - found in " ${AMGX_INCLUDE_DIRS})
ENDIF()


MESSAGE("-- Finding amgx_capi.h")
FIND_FILE(AMGX_CAPI_FOUND 
          NAMES amgx_capi.h 
          PATHS ${AMGX_INCLUDE_DIRS} 
          NO_DEFAULT_PATH)

IF(${AMGX_CAPI_FOUND} STREQUAL "AMGX_CAPI_FOUND-NOTFOUND")
    MESSAGE(FATAL_ERROR "amgx_capi.h was not found in: " ${AMGX_INCLUDE_DIRS})
ELSE()
    MESSAGE("-- Finding amgx_capi.h - found")
ENDIF()


MESSAGE("-- Finding amgx_config.h")
FIND_FILE(AMGX_CONFIG_FOUND 
          NAMES amgx_config.h 
          PATHS ${AMGX_INCLUDE_DIRS} 
          NO_DEFAULT_PATH)

IF(${AMGX_CONFIG_FOUND} STREQUAL "AMGX_CONFIG_FOUND-NOTFOUND")
    MESSAGE(FATAL_ERROR "amgx_config.h was not found in: " ${AMGX_INCLUDE_DIRS})
ELSE()
    MESSAGE("-- Finding amgx_config.h - found")
ENDIF()


MESSAGE("-- Finding libamgxsh.so")
FIND_LIBRARY(AMGX_LIBRARIES
             NAMES amgxsh
             PATHS ${AMGX_DIR} $ENV{AMGX_DIR}
             PATH_SUFFIXES "lib")

IF(${AMGX_LIBRARIES} STREQUAL "AMGX_LIBRARIES-NOTFOUND")
    MESSAGE(FATAL_ERROR "libamgxsh.so was not found")
ELSE()
    MESSAGE("-- Finding libamgxsh.so - found: " ${AMGX_LIBRARIES})
ENDIF()



MESSAGE("\n===================================================================")
MESSAGE("Trying to find Boost...\n")
FIND_PACKAGE(Boost REQUIRED COMPONENTS timer system)

IF(${Boost_FOUND})
    MESSAGE("\tBoost_INCLUDE_DIRS: " ${Boost_INCLUDE_DIRS})
    MESSAGE("\tBoost_LIBRARY_DIRS: " ${Boost_LIBRARY_DIRS})
    MESSAGE("\tBoost_LIBRARIES: " ${Boost_LIBRARIES})
ENDIF()



MESSAGE("\n===================================================================")
MESSAGE("Trying to find CUDA 6.5...")
MESSAGE("\nNOTE: If you have several different versions of CUDA, and if CMake ")
MESSAGE("can not find CUDA 6.5, please define CUDA_TOOLKIT_ROOT_DIR manually ")
MESSAGE("when calling CMake. For example:") 
MESSAGE("\n\t$ cmake . -DCUDA_TOOLKIT_ROOT_DIR=/opt/cuda65\n")

FIND_PACKAGE(CUDA 6.5 EXACT REQUIRED)

IF(${CUDA_FOUND})
    SET(CUDA_LIBRARY_DIRS ${CUDA_TOOLKIT_ROOT_DIR}/lib64)
    MESSAGE("\tCUDA_TOOLKIT_ROOT_DIR: " ${CUDA_TOOLKIT_ROOT_DIR})
    MESSAGE("\tCUDA_VERSION: " ${CUDA_VERSION_STRING})
    MESSAGE("\tCUDA_INCLUDE_DIRS: " ${CUDA_INCLUDE_DIRS})
    MESSAGE("\tCUDA_LIBRARY_DIRS: " ${CUDA_LIBRARY_DIRS})
    MESSAGE("\tCUDA_LIBRARIES: " ${CUDA_LIBRARIES})
ENDIF()



MESSAGE("\n===================================================================")
MESSAGE("Trying to find MPI")

FIND_PACKAGE(MPI REQUIRED)

IF(${MPI_CXX_FOUND})
    MESSAGE("\tFound MPI: " ${MPI_CXX_FOUND})
    MESSAGE("\tMPI_CXX_COMPILER: " ${MPI_CXX_COMPILER})
    MESSAGE("\tMPI_CXX_COMPILE_FLAGS: " ${MPI_CXX_COMPILE_FLAGS})
    MESSAGE("\tMPI_CXX_INCLUDE_PATH: " ${MPI_CXX_INCLUDE_PATH})
    MESSAGE("\tMPI_CXX_LINK_FLAGS: " ${MPI_CXX_LINK_FLAGS})
    MESSAGE("\tMPI_CXX_LIBRARIES: " ${MPI_CXX_LIBRARIES})
ENDIF()



MESSAGE("\n===================================================================")
MESSAGE("Trying to find libblas.a or libblas.so")

FIND_LIBRARY(BLAS_LIBRARIES 
             NAMES blas
             PATHS ${BLAS_DIR} $ENV{BLAS_DIR} $ENV{BLASDIR}
             PATH_SUFFIXES "lib" "lib64")

IF(${BLAS_LIBRARIES} STREQUAL "BLAS_LIBRARIES-NOTFOUND")
    MESSAGE(FATAL_ERROR "libblas.a or libblas.so was not found")
ELSE()
    MESSAGE("-- Finding libblas.a or libblas.so - found: " ${BLAS_LIBRARIES})
ENDIF()



MESSAGE("\n===================================================================")
MESSAGE("Trying to find libfblas.a or libfblas.so")

FIND_LIBRARY(FBLAS_LIBRARIES 
             NAMES fblas
             PATHS ${BLAS_DIR} $ENV{BLAS_DIR} $ENV{BLASDIR}
             PATH_SUFFIXES "lib" "lib64")

IF(${FBLAS_LIBRARIES} STREQUAL "FBLAS_LIBRARIES-NOTFOUND")
    MESSAGE("-- Finding libfblas.a or libfblas.so - not found")
    SET(FBLAS_LIBRARIES "")
ELSE()
    MESSAGE("-- Finding libfblas.a or libfblas.so - found: " ${FBLAS_LIBRARIES})
ENDIF()



MESSAGE("\n===================================================================")
MESSAGE("Trying to find liblapack.a or liblapack.so")

FIND_LIBRARY(LAPACK_LIBRARIES 
             NAMES lapack
             PATHS ${LAPACK_DIR} $ENV{LAPACK_DIR} $ENV{LAPACKDIR}
             PATH_SUFFIXES "lib" "lib64")

IF(${LAPACK_LIBRARIES} STREQUAL "LAPACK_LIBRARIES-NOTFOUND")
    MESSAGE(FATAL_ERROR "liblapack.a or liblapack.so was not found")
ELSE()
    MESSAGE("-- Finding liblapack.a or liblapack.so - found: " ${LAPACK_LIBRARIES})
ENDIF()



MESSAGE("\n===================================================================")
MESSAGE("Trying to find libflapack.a or libflapack.so")

FIND_LIBRARY(FLAPACK_LIBRARIES 
             NAMES flapack
             PATHS ${LAPACK_DIR} $ENV{LAPACK_DIR} $ENV{LAPACKDIR}
             PATH_SUFFIXES "lib" "lib64")

IF(${FLAPACK_LIBRARIES} STREQUAL "FLAPACK_LIBRARIES-NOTFOUND")
    MESSAGE("-- Finding libflapack.a or libflapack.so - not found")
    SET(FLAPACK_LIBRARIES "")
ELSE()
    MESSAGE("-- Finding libflapack.a or libflapack.so - found: " ${FLAPACK_LIBRARIES})
ENDIF()



MESSAGE("\n===================================================================")
MESSAGE("Trying to find libssl.a or libssl.so")

FIND_LIBRARY(SSL_LIBRARIES 
             NAMES ssl
             PATHS ${SSL_DIR} $ENV{SSL_DIR} $ENV{PATH} $ENV{LD_LIBRARY_PATH}
             PATH_SUFFIXES "lib" "lib64")

IF(${SSL_LIBRARIES} STREQUAL "SSL_LIBRARIES-NOTFOUND")
    MESSAGE(FATAL_ERROR "libssl.a or libssl.so was not found")
ELSE()
    MESSAGE("-- Finding libssl.a or libssl.so - found: " ${SSL_LIBRARIES})
ENDIF()



MESSAGE("\n===================================================================")
MESSAGE("Trying to find libcrypto.a or libcrypto.so")

FIND_LIBRARY(CRYPTO_LIBRARIES 
             NAMES crypto
             PATHS ${CRYPTO_DIR} $ENV{CRYPTO_DIR} $ENV{PATH} $ENV{LD_LIBRARY_PATH}
             PATH_SUFFIXES "lib" "lib64")

IF(${CRYPTO_LIBRARIES} STREQUAL "CRYPTO_LIBRARIES-NOTFOUND")
    MESSAGE(FATAL_ERROR "libcrypto.a or libcrypto.so was not found")
ELSE()
    MESSAGE("-- Finding libcrypto.a or libcrypto.so - found: " ${CRYPTO_LIBRARIES})
ENDIF()



MESSAGE("\n===================================================================")
MESSAGE("Trying to find libgfortran.a or libgfortran.so")

FIND_LIBRARY(GFORTRAN_LIBRARIES 
             NAMES gfortran
             PATHS ${GFORTRAN_DIR} $ENV{GFORTRAN_DIR} $ENV{PATH} $ENV{LD_LIBRARY_PATH}
             PATH_SUFFIXES "lib" "lib64")

IF(${GFORTRAN_LIBRARIES} STREQUAL "GFORTRAN_LIBRARIES-NOTFOUND")
    MESSAGE(FATAL_ERROR "libgfortran.a or libgfortran.so was not found")
ELSE()
    MESSAGE("-- Finding libgfortran.a or libgfortran.so - found: " ${GFORTRAN_LIBRARIES})
ENDIF()



MESSAGE("\n===================================================================")
MESSAGE("Setting the information for target and source files...")


SET(SRC ${CMAKE_SOURCE_DIR}/src)
SET(AmgXWrapperSRC ${CMAKE_SOURCE_DIR}/../../src)

SET(SOURCE
    ${SRC}/Poisson3D.cpp 
    ${SRC}/generateA.cpp
    ${SRC}/generateGrid.cpp
    ${SRC}/generateRHS.cpp
    ${SRC}/generateExt.cpp
    ${SRC}/createKSP.cpp
    ${SRC}/getArgs.cpp
    ${AmgXWrapperSRC}/AmgXSolver.cpp)

INCLUDE_DIRECTORIES(
    ${Boost_INCLUDE_DIRS}
    ${PETSC_INCLUDE_DIRS} 
    ${AMGX_INCLUDE_DIRS} 
    ${CUDA_INCLUDE_DIRS} 
    ${MPI_CXX_INCLUDE_PATH}
    ${AmgXWrapperSRC})

LINK_DIRECTORIES(
    ${Boost_LIBRARY_DIRS}
    ${CUDA_LIBRARY_DIRS} 
    ${PETSC_LIBRARY_DIRS} 
    ${AMGX_LIBRARY_DIRS}
    ${PETSC_LIBRARY_DIRS})

ADD_EXECUTABLE(Poisson3D ${SOURCE})

TARGET_LINK_LIBRARIES(Poisson3D 
    ${Boost_LIBRARIES}
    ${PETSC_LIBRARIES} 
    ${BLAS_LIBRARIES} 
    ${LAPACK_LIBRARIES} 
    ${FBLAS_LIBRARIES} 
    ${FLAPACK_LIBRARIES} 
    ${AMGX_LIBRARIES} 
    ${CUDA_LIBRARIES} 
    ${MPI_CXX_LIBRARIES}
    ${SSL_LIBRARIES}
    ${CRYPTO_LIBRARIES}
    ${GFORTRAN_LIBRARIES})
