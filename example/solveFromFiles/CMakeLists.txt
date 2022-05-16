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
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    RESOURCE "${CFGFILES}"
    INSTALL_RPATH "${CMAKE_INSTALL_FULL_LIBDIR};${PETSC_LIBRARY_DIRS}"
)

target_include_directories(solveFromFiles PRIVATE ${PROJECT_SOURCE_DIR}/src)

target_link_libraries(solveFromFiles 
    PRIVATE MPI::MPI_CXX
    PRIVATE PkgConfig::PETSC 
    PRIVATE AmgXWrapper
)

install(
    TARGETS solveFromFiles
    RUNTIME DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${CMAKE_PROJECT_NAME}/example/solveFromFiles
    RESOURCE DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${CMAKE_PROJECT_NAME}/example/solveFromFiles/configs
)
