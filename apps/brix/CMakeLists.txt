my_add_executable(brix
        SOURCES
        rc/bi_loader.cc
        rc/bi_loader.hh
        rc/game_assets.cc
        rc/game_assets.hh
        rc/map.hh
        rc/native_bi_data.inc
        rc/resource_loader.hh
        rc/resource_type.hh
        rc/rc.cc
        rc/rc.hh

        main.cc
        FOLDER Demos
        )

target_link_libraries(brix neutrino::kernel neutrino::assets thirdparty::sdl2-main)
