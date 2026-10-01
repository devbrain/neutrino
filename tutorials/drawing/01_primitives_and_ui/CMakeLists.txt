add_executable(neutrino_tutorial_drawing_01_primitives
    main.cc
)

target_link_libraries(neutrino_tutorial_drawing_01_primitives
    PRIVATE
    neutrino::neutrino
)

target_compile_features(neutrino_tutorial_drawing_01_primitives PRIVATE cxx_std_20)
