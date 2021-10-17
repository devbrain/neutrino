include(bin2c)

macro(add_demo name)
    my_add_executable(${name}
            SOURCES
            ${name}.cc
            demoscene.hh
            FOLDER Demos
            )
    target_link_libraries(${name} neutrino::demoscene thirdparty::sdl2-main)
endmacro()

macro(add_demo_kernel name)
    my_add_executable(${name}
            SOURCES
            ${name}.cc
            demoscene.hh
            FOLDER Demos
            )
    target_link_libraries(${name} neutrino::kernel thirdparty::sdl2-main)
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

add_demo_kernel(fire)
add_demo_kernel(stars)
add_demo_kernel(plasma)
add_demo_kernel(blob)