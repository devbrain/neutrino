# ============================================================================
# Neutrino Library Target
# ============================================================================

include(GenerateExportHeader)

neutrino_library_type(neutrino _lib_type)

add_library(neutrino ${_lib_type}
        # -------------------------------------------------------
        # Input
        # -------------------------------------------------------
        ${PROJECT_SOURCE_DIR}/include/neutrino/input/hotkey.hh
        ${PROJECT_SOURCE_DIR}/include/neutrino/input/mouse_click.hh
        ${PROJECT_SOURCE_DIR}/include/neutrino/input/gamepad_button.hh

        # -------------------------------------------------------
        # Physics
        # -------------------------------------------------------
        ${PROJECT_SOURCE_DIR}/include/neutrino/physics/geometry/clip.hh
        ${PROJECT_SOURCE_DIR}/include/neutrino/physics/geometry/distance.hh
        ${PROJECT_SOURCE_DIR}/include/neutrino/physics/geometry/enclose.hh
        ${PROJECT_SOURCE_DIR}/include/neutrino/physics/geometry/overlap.hh
        ${PROJECT_SOURCE_DIR}/include/neutrino/physics/geometry/shapes.hh
        ${PROJECT_SOURCE_DIR}/include/neutrino/physics/geometry/translate.hh
        ${PROJECT_SOURCE_DIR}/include/neutrino/physics/geometry/triangle.hh
        ${PROJECT_SOURCE_DIR}/include/neutrino/physics/geometry/types.hh
        ${PROJECT_SOURCE_DIR}/include/neutrino/physics/geometry/units.hh

        ${PROJECT_SOURCE_DIR}/include/neutrino/physics/collide/aabb_tree_node.hh
        ${PROJECT_SOURCE_DIR}/include/neutrino/physics/collide/aabb_tree_storage.hh
        ${PROJECT_SOURCE_DIR}/include/neutrino/physics/collide/dynamic_aabb_tree.hh
        ${PROJECT_SOURCE_DIR}/include/neutrino/physics/collide/grid.hh
        ${PROJECT_SOURCE_DIR}/include/neutrino/physics/collide/sorted_array.hh
        ${PROJECT_SOURCE_DIR}/include/neutrino/physics/collide/world.hh
        ${PROJECT_SOURCE_DIR}/include/neutrino/physics/collide/world_internal.hh
        ${PROJECT_SOURCE_DIR}/include/neutrino/physics/collide/world_types.hh

        # -------------------------------------------------------
        # Scenes
        # -------------------------------------------------------
        ${PROJECT_SOURCE_DIR}/include/neutrino/scene/base_scene.hh
        ${PROJECT_SOURCE_DIR}/include/neutrino/scene/scene_transitions.hh
        scene/base_scene.cc
        scene/scenes_manager.hh
        scene/scenes_manager.cc

        # -------------------------------------------------------
        # Audio
        # -------------------------------------------------------
        ${PROJECT_SOURCE_DIR}/include/neutrino/audio/audio.hh
        ${PROJECT_SOURCE_DIR}/include/neutrino/audio/sound_effect.hh
        ${PROJECT_SOURCE_DIR}/include/neutrino/audio/music_stream.hh
        ${PROJECT_SOURCE_DIR}/include/neutrino/audio/pc_speaker.hh
        audio/sound_system.hh
        audio/sound_system.cc
        audio/sound_effect.cc
        audio/music_stream.cc
        audio/pc_speaker.cc
        audio/sdl_io_stream.hh
        audio/sdl_io_stream.cc
        audio/memory_io.hh
        audio/memory_io.cc
        # -------------------------------------------------------
        # Video
        # -------------------------------------------------------
        ${PROJECT_SOURCE_DIR}/include/neutrino/video/sprite/detail/id_strong_type.hh
        ${PROJECT_SOURCE_DIR}/include/neutrino/video/geometry_types.hh
        ${PROJECT_SOURCE_DIR}/include/neutrino/video/globals.hh
        video/globals.cc
        ${PROJECT_SOURCE_DIR}/include/neutrino/video/draw.hh
        video/draw.cc
        ${PROJECT_SOURCE_DIR}/include/neutrino/video/sprite/bitmask.hh
        video/sprite/bitmask.cc
        ${PROJECT_SOURCE_DIR}/include/neutrino/video/sprite/cpu_texture_atlas.hh
        video/sprite/cpu_texture_atlas.cc
        ${PROJECT_SOURCE_DIR}/include/neutrino/video/sprite/texture_atlas.hh
        video/sprite/texture_atlas.cc
        ${PROJECT_SOURCE_DIR}/include/neutrino/video/sprite/sprite_appearance.hh
        ${PROJECT_SOURCE_DIR}/include/neutrino/video/sprite/sprite_sheet.hh
        video/sprite/sprite_sheet.cc
        video/sprite/resource_registry.hh
        video/sprite/sprites_manager.cc
        video/sprite/sprites_manager.hh
        video/sprite/texture_registry.cc
        video/sprite/texture_registry.hh
        # -------------------------------------------------------
        # Services
        # -------------------------------------------------------
        services/service_locator.cc
        services/service_locator.hh
        input/gamepads.cc
        input/gamepads.hh
        input/modifier_match.hh

        application.cc
        input/hotkey.cc
        input/mouse_click.cc
        input/gamepad_button.cc
)
add_library(neutrino::neutrino ALIAS neutrino)

target_include_directories(neutrino
        PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(neutrino PUBLIC cxx_std_20)
target_link_libraries(neutrino
        PUBLIC
        neutrino::sdlpp
        neutrino::musac
        musac::backend_sdl3
        musac::codecs
        neutrino::onyx_image
        neutrino::onyx_font)

generate_export_header(neutrino
        BASE_NAME NEUTRINO
        EXPORT_FILE_NAME ${PROJECT_BINARY_DIR}/include/neutrino/neutrino_export.h
)
