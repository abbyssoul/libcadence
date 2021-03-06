cmake_minimum_required(VERSION 3.3)
project(cadence)
set(PROJECT_DESCRIPTION "IPC library for modern C++")


# Custom build target to generate code coverage:
# Don't want this to run on every build.
option(CADENCE_GTEST_SUPPORT "Build without GTEST" ON)
option(COVERALLS "Generate coveralls data" OFF)
option(SANITIZE "Enable 'sanitize' compiler flag" OFF)

# Include common compile flag
include(cmake/compile_flags.cmake)
include(GNUInstallDirs)
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

# Configure the project:
configure_file(lib${PROJECT_NAME}.pc.in lib${PROJECT_NAME}.pc @ONLY)

# ---------------------------------
# Build project dependencies
# ---------------------------------
set(CADENCE_EXTERNAL_DEP_GTEST_DIR "external/gtest/googletest"
    CACHE PATH "The path to the Google Test framework.")

set(CADENCE_EXTERNAL_DEP_ASIO_DIR "external/asio/asio/include"
    CACHE PATH "The path to the ASIO async IO library.")

include_directories(include)

add_subdirectory(src)
add_subdirectory(test EXCLUDE_FROM_ALL)
add_subdirectory(examples EXCLUDE_FROM_ALL)

# Install include headers
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Install pkgconfig descriptor
install(FILES ${CMAKE_BINARY_DIR}/lib${PROJECT_NAME}.pc
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)

# ---------------------------------
# Show build configuration status:
# ---------------------------------
message(STATUS, "BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
message(STATUS, "CXXFLAGS: ${CMAKE_CXX_FLAGS}")
message(STATUS, "SANITIZE: ${SANITIZE}")
message(STATUS, "COVERALLS: ${COVERALLS}")
