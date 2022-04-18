my_add_executable(hal-demo
        SOURCES
        hal-demo.cc
        FOLDER Demos
        )

target_link_libraries(hal-demo neutrino::kernel thirdparty::sdl2-main)