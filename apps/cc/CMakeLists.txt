add_executable(cc
        main.cc
        tile_names.cc
        tile_names.hh
        data_loader/crystal_caves/cc_decode.cc
        data_loader/crystal_caves/cc_decode.hh
        data_loader/crystal_caves/cc_tiles_mapping.hh
        data_loader/crystal_caves/crystal_caves.hh
        data_loader/crystal_caves/crystal_caves.cc
        data_loader/map_tile.hh
        data_loader/raw_map.hh
        data_loader/raw_map.cc
        data_loader/data_manager.cc
        data_loader/data_manager.hh
        data_loader/load_picture.cc
        data_loader/load_picture.hh
        data_loader/data_directory.hh
        data_loader/data_directory.cc

        scenes/title_scene.cc
        scenes/title_scene.hh
        scenes/main_screen_scene.cc
        scenes/main_screen_scene.hh
        scenes/scenes_registry.cc
        scenes/scenes_registry.hh
        scenes/dialog_box.cc
        scenes/dialog_box.hh
        scenes/dialogs.cc
        scenes/dialogs.hh

)
target_include_directories(cc PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(cc PRIVATE
        $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
        -Wall>
        $<$<CXX_COMPILER_ID:MSVC>:
        /W4>)

target_link_libraries(cc PRIVATE neutrino thirdparty::sdl2-main)