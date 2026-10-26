add_executable(simplex_starfield
        starfield.cc
)

target_link_libraries(simplex_starfield
        PRIVATE
        neutrino::neutrino

)

target_compile_features(simplex_starfield PRIVATE cxx_std_20)
