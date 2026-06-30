cmake_minimum_required(VERSION 3.20)

project(neutrino
    VERSION 0.1.0
    LANGUAGES CXX C
)

# ============================================================================
# Neutrino CMake Integration
# ============================================================================

include(FetchContent)
include(GenerateExportHeader)

if(NOT DEFINED NEUTRINO_CMAKE_DIR)
    FetchContent_Declare(neutrino_cmake
        GIT_REPOSITORY https://github.com/devbrain/neutrino-cmake.git
        GIT_TAG master
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(neutrino_cmake)
    set(NEUTRINO_CMAKE_DIR "${neutrino_cmake_SOURCE_DIR}/cmake")
endif()

list(APPEND CMAKE_MODULE_PATH "${NEUTRINO_CMAKE_DIR}" "${CMAKE_SOURCE_DIR}/cmake")
include(NeutrinoInit)

# ============================================================================
# Options
# ============================================================================

neutrino_define_library_options(neutrino)

# ============================================================================
include(${NEUTRINO_CMAKE_DIR}/deps/sdlpp.cmake)
neutrino_fetch_sdlpp()

include(${NEUTRINO_CMAKE_DIR}/deps/euler.cmake)
neutrino_fetch_euler()
include(${NEUTRINO_CMAKE_DIR}/deps/onyx_image.cmake)
neutrino_fetch_onyx_image()
include(${NEUTRINO_CMAKE_DIR}/deps/onyx_font.cmake)
neutrino_fetch_onyx_font()
include(${NEUTRINO_CMAKE_DIR}/deps/onyx_anim.cmake)
neutrino_fetch_onyx_anim()

# ============================================================================
# Output directories
# ============================================================================

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ============================================================================
# Subdirectories
# ============================================================================

add_subdirectory(src)

if(NEUTRINO_NEUTRINO_BUILD_TESTS)
    include(${NEUTRINO_CMAKE_DIR}/deps/doctest.cmake)
    neutrino_fetch_doctest()

    enable_testing()
    add_subdirectory(test)
endif()

# ============================================================================
# Summary
# ============================================================================

neutrino_is_top_level(_is_top_level)
if(_is_top_level)
    neutrino_print_options(neutrino)
endif()
