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
SET(EXECUTABLE_OUTPUT_PATH ./bin)

IF(NOT DEFINED CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE RELEASE CACHE STRING "")
ENDIF()

MESSAGE("-- Build type: " ${CMAKE_BUILD_TYPE})

SET(CMAKE_SHARED_LINKER_FLAGS "-Wl,--no-undefined")

MESSAGE("\n===================================================================")
MESSAGE("Starting generator...")
PROJECT(utilities)


MESSAGE("\n===================================================================")
MESSAGE("Trying to find PETSc...")
MESSAGE("\nNOTE: If it fails to find PETSc, please set the environment variable")
MESSAGE("PETSC_DIR in order to notify CMake the location of PETSc\n")

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/../extra/cmake-modules")

FIND_PACKAGE(PETSc REQUIRED)

IF(${PETSc_FOUND})
    MESSAGE("\tPETSC_DIR: " ${PETSC_DIR})
    MESSAGE("\tPETSC_INCLUDES: " ${PETSC_INCLUDES})
    MESSAGE("\tPETSC_LIBRARIES: " ${PETSC_LIBRARIES})
ENDIF()



MESSAGE("\n===================================================================")
MESSAGE("Setting the information for target and source files...")


SET(SRC ${CMAKE_SOURCE_DIR}/src)

INCLUDE_DIRECTORIES(${PETSC_INCLUDES}) 

LINK_DIRECTORIES(${PETSC_LIBRARY_DIRS})

ADD_EXECUTABLE(MMtoPETScBin ${SRC}/MMtoPETScBin.cpp)

TARGET_LINK_LIBRARIES(MMtoPETScBin ${PETSC_LIBRARIES}) 
