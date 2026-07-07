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
neutrino_option(
    NEUTRINO_NEUTRINO_TMX_ENABLE_ZSTD
    "Enable zstd-compressed TMX data support"
    ON)

# ============================================================================
include(${NEUTRINO_CMAKE_DIR}/deps/sdlpp.cmake)
neutrino_fetch_sdlpp()

include(${NEUTRINO_CMAKE_DIR}/deps/euler.cmake)
neutrino_fetch_euler()

# stb single-header libraries. Declared BEFORE onyx_image (which also pulls stb):
# FetchContent honours the first declaration, so pinning to a fixed commit here
# makes the stb version reproducible for both neutrino and onyx_image instead of
# tracking a moving master. The neutrino library uses stb_rect_pack.h for atlas
# packing (Phase 3); header-only — STB_RECT_PACK_IMPLEMENTATION is defined in
# exactly one TU (atlas_packer.cc).
FetchContent_Declare(stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG 31c1ad37456438565541f4919958214b6e762fb4
    DOWNLOAD_ONLY YES
)
FetchContent_MakeAvailable(stb)

if(NOT TARGET neutrino_stb_rect_pack)
    add_library(neutrino_stb_rect_pack INTERFACE)
    target_include_directories(neutrino_stb_rect_pack
        INTERFACE $<BUILD_INTERFACE:${stb_SOURCE_DIR}>)
endif()

include(${NEUTRINO_CMAKE_DIR}/deps/onyx_image.cmake)
neutrino_fetch_onyx_image()
include(${NEUTRINO_CMAKE_DIR}/deps/onyx_font.cmake)
neutrino_fetch_onyx_font()

include(${NEUTRINO_CMAKE_DIR}/deps/onyx_anim.cmake)
neutrino_fetch_onyx_anim()

include(${NEUTRINO_CMAKE_DIR}/deps/musac.cmake)
neutrino_fetch_musac()

FetchContent_Declare(pugixml
    GIT_REPOSITORY https://github.com/zeux/pugixml.git
    GIT_TAG v1.14
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(pugixml)

FetchContent_Declare(nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

if(NEUTRINO_NEUTRINO_TMX_ENABLE_ZSTD)
    set(ZSTD_BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
    set(ZSTD_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(ZSTD_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)
    set(ZSTD_BUILD_SHARED OFF CACHE BOOL "" FORCE)
    set(ZSTD_BUILD_STATIC ON CACHE BOOL "" FORCE)
    set(ZSTD_BUILD_COMPRESSION OFF CACHE BOOL "" FORCE)
    set(ZSTD_BUILD_DECOMPRESSION ON CACHE BOOL "" FORCE)
    set(ZSTD_BUILD_DICTBUILDER OFF CACHE BOOL "" FORCE)
    set(ZSTD_BUILD_DEPRECATED OFF CACHE BOOL "" FORCE)
    set(ZSTD_LEGACY_SUPPORT OFF CACHE BOOL "" FORCE)
    set(ZSTD_MULTITHREAD_SUPPORT OFF CACHE BOOL "" FORCE)
    set(ZSTDLIB_VISIBLE hidden CACHE STRING "" FORCE)
    set(ZSTDERRORLIB_VISIBLE hidden CACHE STRING "" FORCE)
    set(ZDICTLIB_VISIBLE hidden CACHE STRING "" FORCE)
    set(ZSTDLIB_STATIC_API hidden CACHE STRING "" FORCE)
    set(ZDICTLIB_STATIC_API hidden CACHE STRING "" FORCE)

    FetchContent_Declare(zstd
        GIT_REPOSITORY https://github.com/facebook/zstd.git
        GIT_TAG v1.5.6
        GIT_SHALLOW TRUE
        SOURCE_SUBDIR build/cmake
    )
    FetchContent_MakeAvailable(zstd)

    if(TARGET libzstd_static)
        set_target_properties(libzstd_static
            PROPERTIES
            POSITION_INDEPENDENT_CODE ON
            C_VISIBILITY_PRESET hidden
        )
    endif()
endif()

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

if(NEUTRINO_NEUTRINO_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# ============================================================================
# Summary
# ============================================================================

neutrino_is_top_level(_is_top_level)
if(_is_top_level)
    neutrino_print_options(neutrino)
endif()
