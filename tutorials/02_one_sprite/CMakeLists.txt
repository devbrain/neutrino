add_executable(neutrino_tutorial_02_one_sprite
    main.cc
)

target_link_libraries(neutrino_tutorial_02_one_sprite
    PRIVATE
    neutrino::neutrino
)

target_compile_features(neutrino_tutorial_02_one_sprite PRIVATE cxx_std_20)

target_compile_definitions(neutrino_tutorial_02_one_sprite
    PRIVATE
    NEUTRINO_TUTORIAL_ASSET_DIR="${NEUTRINO_TUTORIALS_ASSET_DIR}"
)
