# ============================================================================
# Neutrino Library Target
# ============================================================================

include(GenerateExportHeader)

neutrino_library_type(neutrino _lib_type)

add_library(neutrino ${_lib_type}
        ${PROJECT_SOURCE_DIR}/include/neutrino/neutrino.hh
        ${PROJECT_SOURCE_DIR}/include/neutrino/input/hotkey.hh
        ${PROJECT_SOURCE_DIR}/include/neutrino/input/mouse_click.hh
        neutrino.cc
        application.cc
        input/hotkey.cc
        input/mouse_click.cc
)
add_library(neutrino::neutrino ALIAS neutrino)

target_include_directories(neutrino
        PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
)

target_compile_features(neutrino PUBLIC cxx_std_20)
target_link_libraries(neutrino
        PUBLIC
        neutrino::sdlpp
        neutrino::musac
        musac::backend_sdl3
        musac::codecs
        neutrino::onyx_image
        neutrino::onyx_font)

generate_export_header(neutrino
        BASE_NAME NEUTRINO
        EXPORT_FILE_NAME ${PROJECT_BINARY_DIR}/include/neutrino/neutrino_export.h
)


