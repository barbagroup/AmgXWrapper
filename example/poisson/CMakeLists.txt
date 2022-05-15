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

add_executable(poisson ${SOURCE})

set_target_properties(poisson PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    RESOURCE "${CFGFILES}"
    INSTALL_RPATH "${CMAKE_INSTALL_FULL_LIBDIR};${PETSC_LIBRARY_DIRS}"
)

target_include_directories(poisson PRIVATE ${PROJECT_SOURCE_DIR}/src)

target_link_libraries(poisson 
    PRIVATE MPI::MPI_CXX
    PRIVATE PkgConfig::PETSC 
    PRIVATE AmgXWrapper
)

install(
    TARGETS poisson
    RUNTIME DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${CMAKE_PROJECT_NAME}/example/poisson
    RESOURCE DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${CMAKE_PROJECT_NAME}/example/poisson/configs
)
