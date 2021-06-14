my_add_executable(sdl_demo
        SOURCES
            main.cc
        FOLDER Demos
)
target_link_libraries(sdl_demo neutrino::engine thirdparty::sdl2-main)