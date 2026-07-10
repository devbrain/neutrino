add_executable(neutrino_map_viewer
        map_viewer.cc
)

target_link_libraries(neutrino_map_viewer
        PRIVATE
        neutrino::neutrino
)

target_compile_features(neutrino_map_viewer PRIVATE cxx_std_20)
target_compile_definitions(neutrino_map_viewer
        PRIVATE
        NEUTRINO_MAP_VIEWER_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets"
)
