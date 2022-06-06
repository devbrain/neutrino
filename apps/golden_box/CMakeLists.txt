my_add_executable(gb
        SOURCES
        main.cc
        FOLDER Demos
        )

target_link_libraries(gb neutrino::utils neutrino::hal thirdparty::sdl2-main)