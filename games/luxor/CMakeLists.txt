include(FetchContent)

# -----------------------------------------------------------------------------
# Grammar & Parser Tools (Lemon parser & re2c scanner)
# -----------------------------------------------------------------------------
include(NeutrinoGrammar)

set(CFG_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/gen")

neutrino_add_grammar(
    NAME cfg
    PARSER "${CMAKE_CURRENT_SOURCE_DIR}/resources/parser/cfg_parser.y"
    SCANNER "${CMAKE_CURRENT_SOURCE_DIR}/resources/parser/cfg_scanner.re"
    OUTPUT_DIR "${CFG_GEN_DIR}"
    OUTPUT_SOURCES CFG_GRAMMAR_SOURCES
    OUTPUT_HEADERS CFG_GRAMMAR_HEADERS
)

# -----------------------------------------------------------------------------
# 1. 3rd-Party Dependencies (Fetched without separate library targets)
# -----------------------------------------------------------------------------

# sinfl.h: Small Deflate / Zlib single-header library
FetchContent_Declare(
    sinfl
    URL https://raw.githubusercontent.com/vurtun/lib/master/sinfl.h
    DOWNLOAD_NO_EXTRACT TRUE
)
FetchContent_MakeAvailable(sinfl)

# 7-Zip LZMA C SDK
FetchContent_Declare(
    7zip
    GIT_REPOSITORY https://github.com/ip7z/7zip.git
    GIT_TAG        main
    GIT_SHALLOW    TRUE
    SOURCE_SUBDIR  none
)
FetchContent_MakeAvailable(7zip)

# tinyobjloader
FetchContent_Declare(
    tinyobjloader
    GIT_REPOSITORY https://github.com/tinyobjloader/tinyobjloader.git
    GIT_TAG        release
    GIT_SHALLOW    TRUE
    SOURCE_SUBDIR  none
)
FetchContent_MakeAvailable(tinyobjloader)

# 3rd-party source lists and include directories
set(LUXOR_3RD_PARTY_SOURCES
    ${7zip_SOURCE_DIR}/C/LzmaDec.c
    ${7zip_SOURCE_DIR}/C/7zAlloc.c
    ${tinyobjloader_SOURCE_DIR}/tiny_obj_loader.cc
)

set(LUXOR_3RD_PARTY_INCLUDES
    ${sinfl_SOURCE_DIR}
    ${7zip_SOURCE_DIR}/C
    ${tinyobjloader_SOURCE_DIR}
)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set_source_files_properties(
        ${7zip_SOURCE_DIR}/C/LzmaDec.c
        ${7zip_SOURCE_DIR}/C/7zAlloc.c
        ${tinyobjloader_SOURCE_DIR}/tiny_obj_loader.cc
        PROPERTIES COMPILE_OPTIONS "-w"
    )
endif()

# -----------------------------------------------------------------------------
# 2. Targets
# -----------------------------------------------------------------------------

set(LUXOR_COMMON_SOURCES
        resources/storage/archive.hh
        resources/storage/archive.cc
        resources/storage/vfs.hh
        resources/storage/vfs.cc
        resources/storage/game.hh
        resources/storage/game.cc
        resources/parser/cfg_parser.hh
        resources/parser/cfg_parser.cc
        resources/parser/spr_parser.hh
        resources/parser/spr_parser.cc
        resources/parser/font_parser.hh
        resources/parser/font_parser.cc
        resources/parser/depth_parser.hh
        resources/parser/depth_parser.cc
        resources/parser/sl3_parser.hh
        resources/parser/sl3_parser.cc
        resources/loader.cc
        resources/loader.hh
        resources/assets.cc
        resources/assets.hh
        util/expected.hh
        ${CFG_GRAMMAR_SOURCES}
        ${LUXOR_3RD_PARTY_SOURCES}
)

add_library(luxor_common OBJECT
        ${LUXOR_COMMON_SOURCES}
)

set_target_properties(luxor_common PROPERTIES
        POSITION_INDEPENDENT_CODE ON
)

target_compile_features(luxor_common PUBLIC cxx_std_20)
target_compile_definitions(luxor_common PRIVATE "LOGGER_DEFAULT_CATEGORY=Luxor")
target_include_directories(luxor_common PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CFG_GEN_DIR}
        ${LUXOR_3RD_PARTY_INCLUDES}
)

target_link_libraries(luxor_common PUBLIC
        failsafe
        nlohmann_json::nlohmann_json
        neutrino::neutrino
)

add_executable(luxor_tool
        tools/luxor_tool.cc
)
target_link_libraries(luxor_tool PRIVATE luxor_common)


add_executable(luxor
        luxor.cc
        scenes/play_scene.cc
        scenes/play_scene.hh
)
target_compile_definitions(luxor PRIVATE "LOGGER_DEFAULT_CATEGORY=Luxor")
target_link_libraries(luxor PRIVATE luxor_common neutrino::neutrino)

# -----------------------------------------------------------------------------
# 3. Unit Tests (doctest)
# -----------------------------------------------------------------------------
include(${NEUTRINO_CMAKE_DIR}/deps/doctest.cmake)
neutrino_fetch_doctest()

add_executable(luxor_unittest
        tests/test_main.cc
        tests/test_archive.cc
        tests/test_vfs.cc
        tests/test_game.cc
        tests/test_config_parser.cc
        tests/test_spr_parser.cc
        tests/test_font_parser.cc
        tests/test_obj_loader.cc
        tests/test_depth_parser.cc
        tests/test_assets.cc
        tests/test_sl3_parser.cc
)

target_link_libraries(luxor_unittest PRIVATE
        luxor_common
        doctest::doctest
)

add_test(NAME luxor_unittest COMMAND luxor_unittest)