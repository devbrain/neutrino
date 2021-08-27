include(bin2c)

set(MAPS
        map/map_city.tmx
        map/city.png
        )

set(GENERATED_HEADERS)
foreach (FILE ${MAPS})
    BIN2C(${FILE} generated)
    LIST(APPEND GENERATED_HEADERS ${generated})
endforeach ()


my_add_executable(load_tmx
        SOURCES
        load_tmx.cc
        ${GENERATED_HEADERS}
        FOLDER Demos
        )

target_link_libraries(load_tmx neutrino::tiled thirdparty::sdl2-main)