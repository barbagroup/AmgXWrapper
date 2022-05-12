# =====================================================================
# @file CMakeLists.txt
# @brief for cmake
# @author Pi-Yueh Chuang (pychuang@gwu.edu)
# @version alpha
# @date 2015-09-01
# =====================================================================

file(GLOB_RECURSE SOURCE CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

file(GLOB_RECURSE CFGFILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/configs/*.info"
)

add_executable(solveFromFiles ${SOURCE})

set_target_properties(solveFromFiles PROPERTIES
    CXX_STANDARD 14
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    RESOURCE "${CFGFILES}"
    INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}
)

target_include_directories(solveFromFiles
    PRIVATE ${PETSC_INCLUDE_DIRS} 
    PRIVATE ${PROJECT_SOURCE_DIR}/src
)

target_link_directories(solveFromFiles
    PRIVATE ${AMGX_LIBRARY_DIRS}
    PRIVATE ${PETSC_LIBRARY_DIRS}
)

target_link_libraries(solveFromFiles 
    PRIVATE ${AMGX_LIBRARIES} 
    PRIVATE ${MPI_CXX_LIBRARIES}
    PRIVATE ${PETSC_LIBRARIES} 
    PRIVATE AmgXWrapper
)

install(
    TARGETS solveFromFiles
    RUNTIME DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${CMAKE_PROJECT_NAME}/example/solveFromFiles
    RESOURCE DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${CMAKE_PROJECT_NAME}/example/solveFromFiles/configs
)
