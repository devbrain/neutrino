my_add_executable(pong
        SOURCES
            pong.cc
        FOLDER Demos
        )

target_link_libraries(pong neutrino::kernel thirdparty::sdl2-main)