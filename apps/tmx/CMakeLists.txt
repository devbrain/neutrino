include(bin2c)

set(MAPS
        map/map_city.tmx
        map/city.png
        map/demo.tmx
        map/tileset.png
        map/formosa.tmx
        map/moon_overlay.png
        map/overworld.png
        map/tiles.png
        )

set(GENERATED_HEADERS)
foreach (FILE ${MAPS})
    bin2c(${FILE} generated)
    list(APPEND GENERATED_HEADERS ${generated})
endforeach ()


my_add_executable(load_tmx
        SOURCES
        load_tmx.cc
        ${GENERATED_HEADERS}
        FOLDER Demos
        )

target_link_libraries(load_tmx PRIVATE neutrino::kernel thirdparty::sdl2-main)