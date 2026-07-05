add_executable(neutrino_sprite_demo
        sprite_demo.cc
)

target_link_libraries(neutrino_sprite_demo
        PRIVATE
        neutrino::neutrino
)

target_compile_features(neutrino_sprite_demo PRIVATE cxx_std_20)
target_compile_definitions(neutrino_sprite_demo
        PRIVATE
        NEUTRINO_SPRITE_DEMO_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets"
)
