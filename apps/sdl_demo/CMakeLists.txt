include(bin2c)

macro(add_demo name)
    my_add_executable(${name}
            SOURCES
            ${name}.cc
            FOLDER Demos
            )
    target_link_libraries(${name} neutrino::demoscene thirdparty::sdl2-main)
endmacro()

macro(add_demo_gen name gen)
    bin2c(${gen} GENERATED)

    my_add_executable(${name}
            SOURCES
            ${name}.cc
            ${GENERATED}
            FOLDER Demos
            )
    target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    target_link_libraries(${name} neutrino::demoscene thirdparty::sdl2-main)
endmacro()

add_demo(fire)
add_demo(plasma)
add_demo(blob)
#add_demo_gen(bobs data/eye.png)
