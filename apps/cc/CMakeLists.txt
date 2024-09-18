fetchcontent_declare(
        CXXOPTS_EXT
        GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git
        GIT_PROGRESS TRUE
        GIT_SHALLOW TRUE
        GIT_TAG v3.2.1
        TIMEOUT 10
        UPDATE_COMMAND ""
)
fetchcontent_makeavailable(CXXOPTS_EXT)

add_executable(cc
        main.cc
        tile_names.cc
        tile_names.hh
        data_loader/crystal_caves/crystal_caves.hh
        data_loader/crystal_caves/crystal_caves.cc
        data_loader/map_tile.hh
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
        level/maps_registry.cc
        level/maps_registry.hh
        level/cc_world_mapper.cc
        level/cc_world_mapper.hh
        scenes/main_level_scene.cc
        scenes/main_level_scene.hh
        level/components/component_player.hh
        level/ecs_registry.cc
        level/ecs_registry.hh
        level/hud.cc
        level/hud.hh
        cc_application.cc
        cc_application.hh
        # =======================================================
        # Editor
        # =======================================================
        props_editor/props_editor_app.cc
        props_editor/props_editor_app.hh
        props_editor/props_editor_ui.cc
        props_editor/props_editor_ui.hh
        props_editor/editor_context.cc
        props_editor/editor_context.hh
        props_editor/tiles_selector_widget.cc
        props_editor/tiles_selector_widget.hh
        props_editor/main_menu.cc
        props_editor/main_menu.hh

        # ==========================================================

        #data_loader/crystal_caves/level.cc

        data_loader/crystal_caves/cc_level_loader.cc
        data_loader/crystal_caves/cc_level_loader.hh
        tmx_export.cc
        tmx_export.hh
        data_loader/raw_level_map.cc
        data_loader/raw_level_map.hh
        data_loader/crystal_caves/cc_map_cell.hh
        level/crystal_caves/crystal_caves_map.cc
        level/crystal_caves/crystal_caves_map.hh
        level/crystal_caves/crystal_caves_hud.cc
        level/crystal_caves/crystal_caves_hud.hh
        factory.cc
        factory.hh
        data_loader/crystal_caves/cc1_text_resources.hh
        data_loader/text_resource_loader.cc
        data_loader/text_resource_loader.hh
        data_loader/text_resource.cc
        data_loader/text_resource.hh
)
target_include_directories(cc PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(cc PRIVATE
        $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
        -Wall>
        $<$<CXX_COMPILER_ID:MSVC>:
        /W4>)

target_link_libraries(cc PRIVATE neutrino thirdparty::sdl2-main cxxopts)