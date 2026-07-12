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
        ${PROJECT_SOURCE_DIR}/include/neutrino/video/image_loader.hh
        ${PROJECT_SOURCE_DIR}/include/neutrino/video/geometry_types.hh
        ${PROJECT_SOURCE_DIR}/include/neutrino/video/globals.hh
        video/globals.cc
        ${PROJECT_SOURCE_DIR}/include/neutrino/video/draw.hh
        video/draw.cc
        ${PROJECT_SOURCE_DIR}/include/neutrino/video/sprites.hh
        video/sprites.cc
        ${PROJECT_SOURCE_DIR}/include/neutrino/video/sprite/bitmask.hh
        video/sprite/bitmask.cc
        ${PROJECT_SOURCE_DIR}/include/neutrino/video/sprite/cpu_texture_atlas.hh
        video/sprite/cpu_texture_atlas.cc
        video/sprite/surface_lock.hh
        ${PROJECT_SOURCE_DIR}/include/neutrino/video/sprite/texture_atlas.hh
        video/sprite/texture_atlas.cc
        ${PROJECT_SOURCE_DIR}/include/neutrino/video/sprite/sprite_appearance.hh
        ${PROJECT_SOURCE_DIR}/include/neutrino/video/sprite/sprite_animation.hh
        video/sprite/sprite_animation.cc
        ${PROJECT_SOURCE_DIR}/include/neutrino/video/sprite/sprite_state.hh
        video/sprite/sprite_state.cc
        ${PROJECT_SOURCE_DIR}/include/neutrino/video/sprite/sprite_sheet.hh
        video/sprite/sprite_sheet.cc
        video/sprite/resource_registry.hh
        video/sprite/sprites_manager.cc
        video/sprite/sprites_manager.hh
        video/sprite/texture_registry.cc
        video/sprite/texture_registry.hh
        ${PROJECT_SOURCE_DIR}/include/neutrino/video/sprite/atlas_packer.hh
        video/sprite/atlas_packer.cc
        ${PROJECT_SOURCE_DIR}/include/neutrino/video/sprite/render_bundle.hh
        video/sprite/render_bundle.cc
        ${PROJECT_SOURCE_DIR}/include/neutrino/video/sprite/image_identity.hh
        video/sprite/image_identity.cc
        video/sprite/resource_cache_core.hh
        ${PROJECT_SOURCE_DIR}/include/neutrino/video/sprite/sprite_def.hh
        video/sprite/sprite_def.cc
        ${PROJECT_SOURCE_DIR}/include/neutrino/video/sprite/sprite_set.hh
        video/sprite/sprite_set.cc
        ${PROJECT_SOURCE_DIR}/include/neutrino/video/sprite/sprite_cache.hh
        video/sprite/sprite_cache.cc
        ${PROJECT_SOURCE_DIR}/include/neutrino/video/sprite/atlas_loader.hh
        video/sprite/atlas_loader.cc
        ${PROJECT_SOURCE_DIR}/include/neutrino/video/world/tileset_bundle.hh
        video/world/tileset_bundle.cc
        ${PROJECT_SOURCE_DIR}/include/neutrino/video/world/resource_cache.hh
        video/world/resource_cache.cc
        ${PROJECT_SOURCE_DIR}/include/neutrino/video/world/world_renderer.hh
        video/world/world_renderer.cc
        ${PROJECT_SOURCE_DIR}/include/neutrino/video/world/camera.hh
        video/world/camera.cc
        ${PROJECT_SOURCE_DIR}/include/neutrino/video/world/sprite_batch.hh
        video/world/sprite_batch.cc
        ${PROJECT_SOURCE_DIR}/include/neutrino/video/world/render_layer.hh
        ${PROJECT_SOURCE_DIR}/include/neutrino/video/world/texture_layer.hh
        ${PROJECT_SOURCE_DIR}/include/neutrino/video/world/world_compositor.hh
        video/world/world_compositor.cc
        utils/lru.hh
        # -------------------------------------------------------
        # Utils
        # -------------------------------------------------------
        ${PROJECT_SOURCE_DIR}/include/neutrino/detail/hash.hh
        detail/hash.cc
        utils/json.cc
        utils/json.hh
        # -------------------------------------------------------
        # World
        # -------------------------------------------------------
        services/service_access.hh
        ${PROJECT_SOURCE_DIR}/include/neutrino/world/content_key.hh
        ${PROJECT_SOURCE_DIR}/include/neutrino/world/world_common.hh
        ${PROJECT_SOURCE_DIR}/include/neutrino/world/world_layers.hh
        ${PROJECT_SOURCE_DIR}/include/neutrino/world/world_tileset.hh
        ${PROJECT_SOURCE_DIR}/include/neutrino/world/world.hh
        ${PROJECT_SOURCE_DIR}/include/neutrino/world/tmx_loader.hh
        world/world.cc
        world/tmx_loader.cc
        world/tmx/component.cc
        world/tmx/component.hh
        world/tmx/decompressor.cc
        world/tmx/decompressor.hh
        world/tmx/encoding.cc
        world/tmx/encoding.hh
        world/tmx/image.cc
        world/tmx/image.hh
        world/tmx/layers.cc
        world/tmx/layers.hh
        world/tmx/node.cc
        world/tmx/node.hh
        world/tmx/objects.cc
        world/tmx/objects.hh
        world/tmx/parser.cc
        world/tmx/parser.hh
        world/tmx/sinfl.h
        world/tmx/tilesets.cc
        world/tmx/tilesets.hh
        world/tmx/world_parser.cc
        world/tmx/world_parser.hh
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
        neutrino::onyx_font
        PRIVATE
        nlohmann_json::nlohmann_json
        pugixml::pugixml
        neutrino_stb_rect_pack)

if(NEUTRINO_NEUTRINO_TMX_ENABLE_ZSTD)
        target_compile_definitions(neutrino
                PRIVATE
                NEUTRINO_TMX_ENABLE_ZSTD)
        target_link_libraries(neutrino
                PRIVATE
                libzstd_static)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        set_source_files_properties(world/tmx/decompressor.cc
                PROPERTIES
                COMPILE_OPTIONS "-fvisibility=hidden")
endif()

generate_export_header(neutrino
        BASE_NAME NEUTRINO
        EXPORT_FILE_NAME ${PROJECT_BINARY_DIR}/include/neutrino/neutrino_export.h
)
