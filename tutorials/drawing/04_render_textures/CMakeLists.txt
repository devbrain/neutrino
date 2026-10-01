add_executable(neutrino_tutorial_drawing_04_render_textures
    main.cc
)

target_link_libraries(neutrino_tutorial_drawing_04_render_textures
    PRIVATE
    neutrino::neutrino
)

target_compile_features(neutrino_tutorial_drawing_04_render_textures PRIVATE cxx_std_20)
