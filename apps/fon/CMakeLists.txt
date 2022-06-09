my_add_executable(fon
        SOURCES
        fon.cc
        FOLDER Demos
        )

target_link_libraries(fon neutrino::assets neutrino::hal)