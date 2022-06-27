my_add_executable(fon
        SOURCES
        fon.cc
        bm.hh
        traits.hh
        test_utils.hh
        test_sr.cc
        test_getter_setter.cc
        FOLDER Demos
        )

target_link_libraries(fon neutrino::assets neutrino::hal thirdparty::doctest)