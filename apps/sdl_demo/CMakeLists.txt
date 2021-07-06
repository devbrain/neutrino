include(bin2c)

MACRO(add_demo name)
    my_add_executable(${name}
            SOURCES
            ${name}.cc
            FOLDER Demos
            )
    target_link_libraries(${name} neutrino::demoscene thirdparty::sdl2-main)
ENDMACRO()

MACRO(add_demo_gen name gen)
    BIN2C(${gen} GENERATED)

    my_add_executable(${name}
            SOURCES
            ${name}.cc
            ${GENERATED}
            FOLDER Demos
            )
    target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    target_link_libraries(${name} neutrino::demoscene thirdparty::sdl2-main)
ENDMACRO()

add_demo(fire)
add_demo(plasma)
add_demo(blob)
#add_demo_gen(bobs data/eye.png)
