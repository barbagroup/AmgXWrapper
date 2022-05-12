# =====================================================================
# \file CMakeLists.txt
# \brief for cmake
# \author Pi-Yueh Chuang (pychuang@gwu.edu)
# \date 2017-05-24
# =====================================================================

cmake_minimum_required(VERSION 3.18.0)

# =====================================================================
# Cache variables
# =====================================================================
# set default flags for release and debug mode
set(CMAKE_CXX_FLAGS_DEBUG "-g" CACHE STRING "")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG" CACHE STRING "")
set(CMAKE_C_FLAGS_DEBUG "-g" CACHE STRING "")
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG" CACHE STRING "")
set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--no-undefined" CACHE STRING "")

# set default build type
set(CMAKE_BUILD_TYPE RELEASE CACHE STRING "")

# set default searching paths for dependencies
set(PETSC_DIR "$ENV{PETSC_DIR}" CACHE PATH "The path to PETSc.")
set(PETSC_ARCH "$ENV{PETSC_ARCH}" CACHE STRING "The specific build of PETSc under PETSC_DIR.")
set(CUDA_DIR "$ENV{CUDA_DIR}" CACHE PATH "The path to CUDA.")
set(AMGX_DIR "$ENV{AMGX_DIR}" CACHE PATH "The path to AmgX.")

# =====================================================================
# Project settings
# =====================================================================
message("\n====================================")
message("Begin configuration")
message("====================================")

# start generator
project(AmgXWrapper CUDA CXX)

# use GNU standard installation folder heirarchy
include(GNUInstallDirs)

# set the sub folders in build folder
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})

# other cmake setting
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_COLOR_MAKEFILE ON)

# add options
option(BUILD_SHARED_LIBS "Whether to build shared library." ON)

# =====================================================================
# pkg-config
# =====================================================================
message("-- Finding pkg-config")
find_package(PkgConfig REQUIRED)

# =====================================================================
# MPI
# =====================================================================
message("-- Finding MPI")
find_package(MPI REQUIRED)

# =====================================================================
# PETSc
# =====================================================================
message("-- Finding PETSc")
set(ENV{PKG_CONFIG_PATH} "${PETSC_DIR}/${PETSC_ARCH}/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
PKG_SEARCH_MODULE(PETSC REQUIRED NO_CMAKE_PATH NO_CMAKE_ENVIRONMENT_PATH IMPORTED_TARGET petsc)


# =====================================================================
# CUDA
# =====================================================================
message("-- Finding CUDA Toolkit")

set(CUDA_TOOLKIT_ROOT_DIR ${CUDA_DIR} CACHE PATH "" FORCE)
find_package(CUDAToolkit REQUIRED)

# =====================================================================
# AmgX
# =====================================================================
message("-- Finding AmgX")

message("-- Finding amgx_c.h")
find_path(
    AMGX_INCLUDE_DIRS NAMES amgx_c.h
    PATHS ${AMGX_DIR} $ENV{AMGX_DIR} PATH_SUFFIXES "include" "base/include" "examples"
    REQUIRED
)

message("-- Finding amgx_config.h")
find_file(
    AMGX_CONFIG_FOUND NAMES amgx_config.h
    PATHS ${AMGX_INCLUDE_DIRS}
    REQUIRED NO_DEFAULT_PATH
)

message("-- Finding libamgxsh.so")
find_library(
    AMGX_LIBRARIES NAMES amgxsh
    PATHS ${AMGX_DIR} $ENV{AMGX_DIR} PATH_SUFFIXES "lib" "lib64"
    REQUIRED
)
get_filename_component(AMGX_LIBRARY_DIR "${AMGX_LIBRARIES}" PATH)


# =====================================================================
# Doxygen
# =====================================================================
message("-- Finding Doxygen")
find_package(Doxygen)

# =====================================================================
# Target libAmgxWrapper.so
# =====================================================================
set(SRC ${CMAKE_SOURCE_DIR}/src)

set(SOURCE
    ${SRC}/AmgXSolver.cpp
    ${SRC}/init.cpp
    ${SRC}/misc.cpp
    ${SRC}/setA.cpp
    ${SRC}/solve.cpp
    ${SRC}/consolidate.cu
)

add_library(AmgXWrapper ${SOURCE})

set_target_properties(AmgXWrapper PROPERTIES
    CUDA_ARCHITECTURES "all"
    CXX_STANDARD 14
    PUBLIC_HEADER ${SRC}/AmgXSolver.hpp
)

target_include_directories(AmgXWrapper
    PRIVATE ${AMGX_INCLUDE_DIRS}
    PRIVATE ${MPI_CXX_INCLUDE_DIRS}
    PRIVATE ${PETSC_INCLUDE_DIRS}
    PRIVATE ${SRC}
)

target_link_options(AmgXWrapper
    PRIVATE "-L${CUDAToolkit_LIBRARY_DIR}"
    PRIVATE "-L${AMGX_LIBRARY_DIR}"
    PRIVATE ${MPI_CXX_LINK_FLAGS}
    PRIVATE "-L${PETSC_LIBARAY_DIR}"
)

target_link_libraries(AmgXWrapper
    PRIVATE ${AMGX_LIBRARIES}
    PRIVATE ${MPI_CXX_LIBRARIES}
    PRIVATE ${PETSC_LIBRARIES}
)

install(
    TARGETS AmgXWrapper
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# =====================================================================
# Target doc
# =====================================================================
if (DOXYGEN_FOUND)
    set(DOXYGEN_INPUT ${CMAKE_SOURCE_DIR}/doc/Doxyfile)
    set(DOXYGEN_OUTPUT ${PROJECT_BINARY_DIR}/doc)
    file(MAKE_DIRECTORY ${DOXYGEN_OUTPUT})
    add_custom_target(doc
        COMMAND ${CMAKE_COMMAND} -E echo "Building API Documentation..."
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_INPUT}
        COMMAND ${CMAKE_COMMAND} -E echo "Done."
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    )
    install(DIRECTORY ${PROJECT_BINARY_DIR}/doc DESTINATION ${CMAKE_INSTALL_PREFIX})
else()
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/doc DESTINATION ${CMAKE_INSTALL_PREFIX})
endif()

# =====================================================================
# Target poisson
# =====================================================================
add_subdirectory(example/poisson)

# =====================================================================
# Print information
# =====================================================================
message("\n====================================")
message("Config Information:")
message("====================================\n")

message("Build type: ${CMAKE_BUILD_TYPE}")
message("Build shared library: ${BUILD_SHARED_LIBS}")
message("Doxygen available: ${DOXYGEN_FOUND}")
message("Installation path: ${CMAKE_INSTALL_PREFIX}\n")

message("Compilers:")
message("\tCMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}")
message("\tCMAKE_C_COMPILER: ${CMAKE_C_COMPILER}\n")

message("Flags:")
message("\tCMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}: ${CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}}\n")

message("MPI")
message("\tMPI_CXX_VERSION: ${MPI_CXX_VERSION}")
message("\tMPI_CXX_INCLUDE_DIRS: ${MPI_CXX_INCLUDE_DIRS}")
message("\tMPI_CXX_LIBRARIES: ${MPI_CXX_LIBRARIES}")
message("\tMPI_CXX_LINK_FLAGS: ${MPI_CXX_LINK_FLAGS}\n")

message("PETSc:")
message("\tPETSC_DIR: ${PETSC_DIR}")
message("\tPETSC_VERSION: ${PETSC_VERSION}")
message("\tPETSC_INCLUDE_DIRS: ${PETSC_INCLUDE_DIRS}")
message("\tPETSC_LIBRARY_DIRS: ${PETSC_LIBRARY_DIRS}")
message("\tPETSC_LIBRARIES: ${PETSC_LIBRARIES}\n")

message("CUDA:")
message("\tCUDA_TOOLKIT_ROOT_DIR: ${CUDA_TOOLKIT_ROOT_DIR}")
message("\tCUDAToolkit_LIBRARY_DIR: ${CUDAToolkit_LIBRARY_DIR}\n")

message("AmgX:")
message("\tAMGX_DIR: ${AMGX_DIR}")
message("\tAMGX_INCLUDE_DIRS: ${AMGX_INCLUDE_DIRS}")
message("\tAMGX_LIBRARY_DIR: ${AMGX_LIBRARY_DIR}")
message("\tAMGX_LIBRARIES: ${AMGX_LIBRARIES}\n")
