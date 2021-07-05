cmake_minimum_required(VERSION 3.18)
set_property(GLOBAL PROPERTY CTEST_TARGETS_ADDED 1) # hack to prevent CTest added targets

set (CMAKE_MODULE_PATH "${CMAKE_BINARY_DIR};${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ===========================================================
project(neutrino)
# ===========================================================
option(NEUTRINO_BUILD_TEST "Build Neutrino tests" OFF)
# ===========================================================
include(GenerateExportHeader)
include(build)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads)
find_package(Filesystem)
# ===========================================================

# Output binary to predictable location.
set(BINARY_OUT_DIR ${CMAKE_BINARY_DIR}/bin)
set(LIB_OUT_DIR ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BINARY_OUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BINARY_OUT_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${LIB_OUT_DIR})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${BINARY_OUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${BINARY_OUT_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${LIB_OUT_DIR})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${BINARY_OUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${BINARY_OUT_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${LIB_OUT_DIR})
# ===========================================================
set (PROJECT_ROOT ${CMAKE_SOURCE_DIR})
set (PROJECT_INCLUDE_PRIVATE ${CMAKE_SOURCE_DIR}/src)
set (PROJECT_INCLUDE         ${CMAKE_SOURCE_DIR}/include)
set (PROJECT_CONFIG_INCLUDE  ${CMAKE_BINARY_DIR}/include)
# ===========================================================
# CONFIGURATION
# ===========================================================
include (TestBigEndian)
TEST_BIG_ENDIAN(IS_BIG_ENDIAN)
if(NOT IS_BIG_ENDIAN)
    set (NEUTRINO_IS_LITTLE_ENDIAN 1)
endif()
file(MAKE_DIRECTORY ${PROJECT_CONFIG_INCLUDE}/neutrino)
configure_file(${PROJECT_INCLUDE}/configure.h.in ${PROJECT_CONFIG_INCLUDE}/neutrino/neutrino_config.h @ONLY)
# ===========================================================
# IDE Support
# ===========================================================
set (CMAKE_EXPORT_COMPILE_COMMANDS ON)
string(COMPARE EQUAL "${CMAKE_CFG_INTDIR}" "." ONYX_IS_SINGLE_CONFIGURATION)

# Organize unrelated targets to clean IDE hierarchy.
set(COMMANDS_FOLDER "COMMANDS")

# This will also clean up the CMake ALL_BUILD, INSTALL, RUN_TESTS and ZERO_CHECK projects.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER ${COMMANDS_FOLDER})

# ========================================================================
add_subdirectory(ext)
add_subdirectory(src)

if (NEUTRINO_BUILD_TEST)
    add_subdirectory(test)
endif()

add_subdirectory(apps)




