# =====================================================================
# \file CMakeLists.txt
# \brief for cmake
# \author Pi-Yueh Chuang (pychuang@gwu.edu)
# \date 2017-05-24
# =====================================================================


CMAKE_MINIMUM_REQUIRED(VERSION 3.18.0)


# =====================================================================
# Cache variables
# =====================================================================
# set default flags for release and debug mode
SET(CMAKE_CXX_FLAGS_DEBUG "-g" CACHE STRING "")
SET(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG" CACHE STRING "")
SET(CMAKE_C_FLAGS_DEBUG "-g" CACHE STRING "")
SET(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG" CACHE STRING "")
SET(CMAKE_SHARED_LINKER_FLAGS "-Wl,--no-undefined" CACHE STRING "")

# set default build type
SET(CMAKE_BUILD_TYPE RELEASE CACHE STRING "")

# set default searching paths for dependencies
SET(PETSC_DIR "$ENV{PETSC_DIR}" CACHE PATH "The path to PETSc.")
SET(PETSC_ARCH "$ENV{PETSC_ARCH}" CACHE STRING "The specific build of PETSc under PETSC_DIR.")
SET(CUDA_DIR "$ENV{CUDA_DIR}" CACHE PATH "The path to CUDA.")
SET(AMGX_DIR "$ENV{AMGX_DIR}" CACHE PATH "The path to AmgX.")


# =====================================================================
# Project settings
# =====================================================================
MESSAGE("\n====================================")
MESSAGE("Begin configuration")
MESSAGE("====================================")

# start generator
PROJECT(AmgXWrapper CUDA CXX)

# use GNU standard installation folder heirarchy
INCLUDE(GNUInstallDirs)

# set the sub folders in build folder
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})

# other cmake setting
SET(CMAKE_VERBOSE_MAKEFILE ON)
SET(CMAKE_COLOR_MAKEFILE ON)

# add options
OPTION(BUILD_SHARED_LIBS "Whether to build shared library." ON)


# =====================================================================
# pkg-config
# =====================================================================
MESSAGE("-- Finding pkg-config")
FIND_PACKAGE(PkgConfig REQUIRED)

# =====================================================================
# MPI
# =====================================================================
MESSAGE("-- Finding MPI")
FIND_PACKAGE(MPI REQUIRED)

# =====================================================================
# PETSc
# =====================================================================
MESSAGE("-- Finding PETSc")
SET(ENV{PKG_CONFIG_PATH} "${PETSC_DIR}/${PETSC_ARCH}/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
PKG_SEARCH_MODULE(PETSC REQUIRED NO_CMAKE_PATH NO_CMAKE_ENVIRONMENT_PATH IMPORTED_TARGET petsc)


# =====================================================================
# CUDA
# =====================================================================
MESSAGE("-- Finding CUDA Toolkit")

SET(CUDA_TOOLKIT_ROOT_DIR ${CUDA_DIR} CACHE PATH "" FORCE)
FIND_PACKAGE(CUDAToolkit REQUIRED)

# =====================================================================
# AmgX
# =====================================================================
MESSAGE("-- Finding AmgX")

MESSAGE("-- Finding amgx_c.h")
FIND_PATH(
    AMGX_INCLUDE_DIRS NAMES amgx_c.h
    PATHS ${AMGX_DIR} $ENV{AMGX_DIR} PATH_SUFFIXES "include" "base/include" "examples"
    REQUIRED
)

MESSAGE("-- Finding amgx_config.h")
FIND_FILE(
    AMGX_CONFIG_FOUND NAMES amgx_config.h
    PATHS ${AMGX_INCLUDE_DIRS}
    REQUIRED NO_DEFAULT_PATH
)

MESSAGE("-- Finding libamgxsh.so")
FIND_LIBRARY(
    AMGX_LIBRARIES NAMES amgxsh
    PATHS ${AMGX_DIR} $ENV{AMGX_DIR} PATH_SUFFIXES "lib" "lib64"
    REQUIRED
)
GET_FILENAME_COMPONENT(AMGX_LIBRARY_DIR "${AMGX_LIBRARIES}" PATH)


# =====================================================================
# Doxygen
# =====================================================================
MESSAGE("-- Finding Doxygen")
FIND_PACKAGE(Doxygen)

# =====================================================================
# Target libAmgxWrapper.so
# =====================================================================
SET(SRC ${CMAKE_SOURCE_DIR}/src)

SET(SOURCE
    ${SRC}/AmgXSolver.cpp
    ${SRC}/init.cpp
    ${SRC}/misc.cpp
    ${SRC}/setA.cpp
    ${SRC}/solve.cpp
    ${SRC}/consolidate.cu
)

# target AmgXWrapper
ADD_LIBRARY(AmgXWrapper ${SOURCE})
SET_TARGET_PROPERTIES(AmgXWrapper PROPERTIES CUDA_ARCHITECTURES "all")
SET_TARGET_PROPERTIES(AmgXWrapper PROPERTIES CXX_STANDARD 14)

TARGET_INCLUDE_DIRECTORIES(AmgXWrapper
    PRIVATE ${AMGX_INCLUDE_DIRS}
    PRIVATE ${MPI_CXX_INCLUDE_DIRS}
    PRIVATE ${PETSC_INCLUDE_DIRS}
    PRIVATE ${SRC}
)

TARGET_LINK_OPTIONS(AmgXWrapper
    PRIVATE "-L${CUDAToolkit_LIBRARY_DIR}"
    PRIVATE "-L${AMGX_LIBRARY_DIR}"
    PRIVATE ${MPI_CXX_LINK_FLAGS}
    PRIVATE "-L${PETSC_LIBARAY_DIR}"
)

TARGET_LINK_LIBRARIES(AmgXWrapper
    PRIVATE ${AMGX_LIBRARIES}
    PRIVATE ${MPI_CXX_LIBRARIES}
    PRIVATE ${PETSC_LIBRARIES}
)

ADD_CUSTOM_COMMAND(
    TARGET AmgXWrapper
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/include
    COMMAND ${CMAKE_COMMAND} -E copy ${SRC}/AmgXSolver.hpp ${PROJECT_BINARY_DIR}/include
)

# installation
INSTALL(
    TARGETS AmgXWrapper
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

INSTALL(FILES ${SRC}/AmgXSolver.hpp DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# =====================================================================
# Target doc
# =====================================================================
IF (DOXYGEN_FOUND)
    SET(DOXYGEN_INPUT ${CMAKE_SOURCE_DIR}/doc/Doxyfile)
    SET(DOXYGEN_OUTPUT ${PROJECT_BINARY_DIR}/doc)
    FILE(MAKE_DIRECTORY ${DOXYGEN_OUTPUT})
    ADD_CUSTOM_TARGET(doc
        COMMAND ${CMAKE_COMMAND} -E echo "Building API Documentation..."
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_INPUT}
        COMMAND ${CMAKE_COMMAND} -E echo "Done."
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    )
    INSTALL(DIRECTORY ${PROJECT_BINARY_DIR}/doc DESTINATION ${CMAKE_INSTALL_PREFIX})
ELSE()
    INSTALL(DIRECTORY ${PROJECT_SOURCE_DIR}/doc DESTINATION ${CMAKE_INSTALL_PREFIX})
ENDIF()

# =====================================================================
# Print information
# =====================================================================
MESSAGE("\n====================================")
MESSAGE("Config Information:")
MESSAGE("====================================\n")

MESSAGE("Build type: ${CMAKE_BUILD_TYPE}")
MESSAGE("Build shared library: ${BUILD_SHARED_LIBS}")
MESSAGE("Doxygen available: ${DOXYGEN_FOUND}")
MESSAGE("Installation path: ${CMAKE_INSTALL_PREFIX}\n")

MESSAGE("Compilers:")
MESSAGE("\tCMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}")
MESSAGE("\tCMAKE_C_COMPILER: ${CMAKE_C_COMPILER}\n")

MESSAGE("Flags:")
MESSAGE("\tCMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}: ${CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}}\n")

MESSAGE("MPI")
MESSAGE("\tMPI_CXX_VERSION: ${MPI_CXX_VERSION}")
MESSAGE("\tMPI_CXX_INCLUDE_DIRS: ${MPI_CXX_INCLUDE_DIRS}")
MESSAGE("\tMPI_CXX_LIBRARIES: ${MPI_CXX_LIBRARIES}")
MESSAGE("\tMPI_CXX_LINK_FLAGS: ${MPI_CXX_LINK_FLAGS}\n")

MESSAGE("PETSc:")
MESSAGE("\tPETSC_DIR: ${PETSC_DIR}")
MESSAGE("\tPETSC_VERSION: ${PETSC_VERSION}")
MESSAGE("\tPETSC_INCLUDE_DIRS: ${PETSC_INCLUDE_DIRS}")
MESSAGE("\tPETSC_LIBRARY_DIRS: ${PETSC_LIBRARY_DIRS}")
MESSAGE("\tPETSC_LIBRARIES: ${PETSC_LIBRARIES}\n")

MESSAGE("CUDA:")
MESSAGE("\tCUDA_TOOLKIT_ROOT_DIR: ${CUDA_TOOLKIT_ROOT_DIR}")
MESSAGE("\tCUDAToolkit_LIBRARY_DIR: ${CUDAToolkit_LIBRARY_DIR}\n")

MESSAGE("AmgX:")
MESSAGE("\tAMGX_DIR: ${AMGX_DIR}")
MESSAGE("\tAMGX_INCLUDE_DIRS: ${AMGX_INCLUDE_DIRS}")
MESSAGE("\tAMGX_LIBRARY_DIR: ${AMGX_LIBRARY_DIR}")
MESSAGE("\tAMGX_LIBRARIES: ${AMGX_LIBRARIES}\n")
