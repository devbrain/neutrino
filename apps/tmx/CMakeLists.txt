 my_add_executable(load_tmx
         SOURCES
         load_tmx.cc
         FOLDER Demos
         )
 target_link_libraries(load_tmx neutrino::tiled thirdparty::sdl2-main)