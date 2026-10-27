add_executable(neutrino_tutorial_drawing_05_colors
    main.cc
)

target_link_libraries(neutrino_tutorial_drawing_05_colors
    PRIVATE
    neutrino::neutrino
)

target_compile_features(neutrino_tutorial_drawing_05_colors PRIVATE cxx_std_20)
