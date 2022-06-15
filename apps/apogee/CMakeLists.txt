my_add_executable(apogee
        SOURCES
        main.cc
        keen3/egahead.hh
        keen3/egahead.cc
        FOLDER Demos
        )

target_link_libraries(apogee neutrino::utils neutrino::assets neutrino::hal)