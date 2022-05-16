# =====================================================================
# \file CMakeLists.txt
# \brief for cmake
# \author Pi-Yueh Chuang (pychuang@gwu.edu)
# \date 2017-05-24
# =====================================================================

cmake_minimum_required(VERSION 3.18.0)

# =====================================================================
# Project settings
# =====================================================================
message("\n====================================")
message("Begin configuration")
message("====================================")

# start generator
project(AmgXWrapper
    VERSION 1.6
    HOMEPAGE_URL https://github.com/barbagroup/AmgXWrapper
    LANGUAGES CUDA CXX
)

# use GNU standard installation folder heirarchy
include(GNUInstallDirs)

# to help generate config file for downstream projects
include(CMakePackageConfigHelpers)

# set default build type
if (NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY VALUE "Release")
endif()

# add options
option(BUILD_SHARED_LIBS "Whether to build shared library." ON)

# set default searching paths for dependencies
set(PETSC_DIR "$ENV{PETSC_DIR}" CACHE PATH "The path to PETSc.")
set(PETSC_ARCH "$ENV{PETSC_ARCH}" CACHE STRING "The specific build of PETSc under PETSC_DIR.")
set(CUDA_DIR "$ENV{CUDA_DIR}" CACHE PATH "The path to CUDA.")
set(AMGX_DIR "$ENV{AMGX_DIR}" CACHE PATH "The path to AmgX.")

# hardcoded configuration
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# combine PETSC_DIR and PETSC_ARCH into a single variable
set(PETSC_DIR "${PETSC_DIR}/${PETSC_ARCH}")

# configure extra paths to find cmake/pkg-config modules/configs
set(CMAKE_PREFIX_PATH ${PETSC_DIR} ${CMAKE_PREFIX_PATH})
set(CMAKE_PREFIX_PATH ${AMGX_DIR} ${CMAKE_PREFIX_PATH})

# force FindCUDAToolkit to use CUDA_DIR
set(CUDA_TOOLKIT_ROOT_DIR ${CUDA_DIR} CACHE PATH "" FORCE)

# =====================================================================
# Find pkg-config, doxygen, MPI, and CUDA Toolkit
# =====================================================================
find_package(PkgConfig REQUIRED)
find_package(Doxygen)
find_package(MPI REQUIRED)
find_package(CUDAToolkit REQUIRED)

# =====================================================================
# Find PETSc
# =====================================================================
pkg_search_module(PETSC REQUIRED IMPORTED_TARGET petsc)
message(STATUS "Found PETSc version ${PETSC_VERSION} at ${PETSC_PREFIX}")

# =====================================================================
# Find AmgX
# =====================================================================
find_path(AMGX_INCLUDE_DIRS NAMES amgx_c.h REQUIRED)
message(STATUS "Found amgx_c.h at ${AMGX_INCLUDE_DIRS}")

find_file(AMGX_CONFIG_FOUND NAMES amgx_config.h REQUIRED)
message(STATUS "Found amgx_config.h: ${AMGX_CONFIG_FOUND}")

find_library(AMGX_LIBRARIES NAMES amgxsh REQUIRED)
message(STATUS "Found libamgxsh.so: ${AMGX_LIBRARIES}")

get_filename_component(AMGX_LIBRARY_DIR "${AMGX_LIBRARIES}" PATH)
message(STATUS "Found AmgX lib dir: ${AMGX_LIBRARY_DIR}")

# make it an imported target
add_library(amgxwrapper::_amgx UNKNOWN IMPORTED)

set_target_properties(amgxwrapper::_amgx PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES ${AMGX_INCLUDE_DIRS}
    IMPORTED_LOCATION ${AMGX_LIBRARIES}
)

# =====================================================================
# Target libAmgxWrapper.so
# =====================================================================
file(GLOB_RECURSE SOURCE CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.hpp"
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/*.cu"
)

add_library(AmgXWrapper ${SOURCE})

set_target_properties(AmgXWrapper PROPERTIES
    CUDA_RUNTIME_LIBRARY Shared
    PUBLIC_HEADER ${CMAKE_SOURCE_DIR}/src/AmgXSolver.hpp
    POSITION_INDEPENDENT_CODE ${BUILD_SHARED_LIBS}
    INSTALL_RPATH "${PETSC_LIBRARY_DIRS};${AMGX_LIBRARY_DIR}"
)

target_include_directories(AmgXWrapper
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_options(AmgXWrapper
    PRIVATE -Wno-deprecated-declarations
)

target_link_libraries(AmgXWrapper
    PRIVATE MPI::MPI_CXX
    PRIVATE PkgConfig::PETSC
    PUBLIC amgxwrapper::_amgx
)

target_link_options(AmgXWrapper
    PRIVATE "-Wl,--as-needed"
    PRIVATE "-Wl,--no-allow-shlib-undefined"
    PRIVATE "-Wl,--no-undefined"
)

# generate AmgXWrapperConfig.cmake for cmake's `find_package`
configure_package_config_file(
    ${PROJECT_NAME}Config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
    PATH_VARS
        CMAKE_INSTALL_LIBDIR
        CMAKE_INSTALL_INCLUDEDIR
        AMGX_LIBRARIES
        AMGX_INCLUDE_DIRS
)

# generate a AmgXWrapperVersion.cmake for cmake's `find_package`
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    COMPATIBILITY AnyNewerVersion
)

install(
    TARGETS AmgXWrapper
    EXPORT AmgXWrapperTarget
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
    EXPORT AmgXWrapperTarget NAMESPACE AmgXWrapper::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

install(
    FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
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
    install(DIRECTORY ${PROJECT_BINARY_DIR}/doc TYPE DOC)
else()
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/doc TYPE DOC)
endif()

# =====================================================================
# Examples
# =====================================================================
add_subdirectory(example/poisson)
add_subdirectory(example/solveFromFiles)
