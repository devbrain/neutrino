add_executable(neutrino_tutorial_drawing_02_batches
    main.cc
)

target_link_libraries(neutrino_tutorial_drawing_02_batches
    PRIVATE
    neutrino::neutrino
)

target_compile_features(neutrino_tutorial_drawing_02_batches PRIVATE cxx_std_20)

target_compile_definitions(neutrino_tutorial_drawing_02_batches
    PRIVATE
    NEUTRINO_TUTORIAL_ASSET_DIR="${NEUTRINO_TUTORIALS_ASSET_DIR}"
)
