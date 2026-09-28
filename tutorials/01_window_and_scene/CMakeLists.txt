add_executable(neutrino_tutorial_01_window_and_scene
    main.cc
)

target_link_libraries(neutrino_tutorial_01_window_and_scene
    PRIVATE
    neutrino::neutrino
)

target_compile_features(neutrino_tutorial_01_window_and_scene PRIVATE cxx_std_20)
