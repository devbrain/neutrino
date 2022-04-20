include(bin2c)

set(ASSETS
        assets/foo.png
)

set(GENERATED_HEADERS)
foreach (FILE ${ASSETS})
    bin2c(${FILE} generated)
    list(APPEND GENERATED_HEADERS ${generated})
endforeach ()

my_add_executable(hal-demo
        SOURCES
        hal-demo.cc
        ${GENERATED_HEADERS}
        FOLDER Demos
        )

target_link_libraries(hal-demo neutrino::kernel thirdparty::sdl2-main)