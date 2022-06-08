my_add_executable(apogee
        SOURCES
        main.cc
        FOLDER Demos
        )

target_link_libraries(apogee neutrino::utils neutrino::hal)