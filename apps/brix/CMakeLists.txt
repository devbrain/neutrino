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

        main.cc
        FOLDER Demos
        )

target_link_libraries(brix neutrino::kernel thirdparty::sdl2-main)
