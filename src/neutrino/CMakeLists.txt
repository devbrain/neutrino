# ============================================================================
# Neutrino Library Target
# ============================================================================
#
# The library is described as a set of subsystems, then built from all of them at once.
#
#   neutrino_subsystem(<name> [DIR <dir>] [PUBLIC <headers>...] [PRIVATE <files>...])
#
#   DIR      the subsystem's folder; defaults to <name>. "." is the library root.
#   PUBLIC   installed headers, relative to include/neutrino/<dir>.
#   PRIVATE  sources and internal headers, relative to src/neutrino/<dir>.
#
# Each call appends to NEUTRINO_SOURCES; add_library() below takes the whole list.

include(GenerateExportHeader)

set(NEUTRINO_SOURCES)

function(neutrino_subsystem name)
    cmake_parse_arguments(PARSE_ARGV 1 arg "" "DIR" "PUBLIC;PRIVATE")
    if(arg_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "neutrino_subsystem(${name}): unexpected arguments: ${arg_UNPARSED_ARGUMENTS}")
    endif()
    if(NOT DEFINED arg_DIR)
        set(arg_DIR ${name})
    endif()
    set(files)
    foreach(f IN LISTS arg_PUBLIC)
        list(APPEND files ${PROJECT_SOURCE_DIR}/include/neutrino/${arg_DIR}/${f})
    endforeach()
    foreach(f IN LISTS arg_PRIVATE)
        list(APPEND files ${CMAKE_CURRENT_SOURCE_DIR}/${arg_DIR}/${f})
    endforeach()
    set(NEUTRINO_SOURCES ${NEUTRINO_SOURCES} ${files} PARENT_SCOPE)
endfunction()

# ----------------------------------------------------------------------------
# Subsystems
# ----------------------------------------------------------------------------

neutrino_subsystem(core DIR .
    PUBLIC
        application.hh
        coordinates.hh
        entry_point.hh
        neutrino.hh
    PRIVATE
        application.cc
)

neutrino_subsystem(detail
    PUBLIC
        enum.hh
        hash.hh
        id_strong_type.hh
    PRIVATE
        hash.cc
)

neutrino_subsystem(utils
    PRIVATE
        json.cc
        json.hh
        lru.hh
)

neutrino_subsystem(services
    PRIVATE
        service_access.hh
        service_locator.cc
        service_locator.hh
)

neutrino_subsystem(input
    PUBLIC
        gamepad_button.hh
        hotkey.hh
        input.hh
        input_snapshot.hh
        keys.hh
        mouse_click.hh
    PRIVATE
        gamepad_button.cc
        gamepads.cc
        gamepads.hh
        hotkey.cc
        input_snapshot.cc
        modifier_match.hh
        mouse_click.cc
)

neutrino_subsystem(physics
    PUBLIC
        physics.hh
        geometry/clip.hh
        geometry/distance.hh
        geometry/enclose.hh
        geometry/overlap.hh
        geometry/shapes.hh
        geometry/sweep.hh
        geometry/translate.hh
        geometry/triangle.hh
        geometry/types.hh
        geometry/units.hh
        collide/aabb_tree_node.hh
        collide/aabb_tree_storage.hh
        collide/dynamic_aabb_tree.hh
        collide/grid.hh
        collide/sorted_array.hh
        collide/world.hh
        collide/world_internal.hh
        collide/world_types.hh
)

neutrino_subsystem(scene
    PUBLIC
        base_scene.hh
        scene_transitions.hh
    PRIVATE
        base_scene.cc
        scenes_manager.cc
        scenes_manager.hh
)

neutrino_subsystem(audio
    PUBLIC
        audio.hh
        music_stream.hh
        pc_speaker.hh
        sound_effect.hh
    PRIVATE
        callback_relay.hh
        memory_io.cc
        memory_io.hh
        music_stream.cc
        pc_speaker.cc
        sdl_io_stream.cc
        sdl_io_stream.hh
        sound_effect.cc
        sound_system.cc
        sound_system.hh
)

neutrino_subsystem(video
    PUBLIC
        color.hh
        draw.hh
        geometry_types.hh
        globals.hh
        palette.hh
        sprites.hh
        texture.hh
        world.hh
    PRIVATE
        draw.cc
        globals.cc
        palette.cc
        sprites.cc
        texture.cc
)

neutrino_subsystem(video_image DIR video/image
    PUBLIC
        image.hh
        image_bank.hh
        image_bank_builder.hh
        image_builder.hh
        image_loader.hh
    PRIVATE
        image_bank_builder.cc
        image_builder.cc
        image_loader.cc
)

neutrino_subsystem(video_atlas DIR video/atlas
    PUBLIC
        atlas_id.hh
        bitmask.hh
        image_atlas.hh
    PRIVATE
        atlas_id_gen.cc
        atlas_id_gen.hh
        bitmask.cc
        image_atlas.cc
        shelf_packer.hh
)

neutrino_subsystem(video_sprite DIR video/sprite
    PUBLIC
        atlas_loader.hh
        image_identity.hh
        render_bundle.hh
        sprite_animation.hh
        sprite_appearance.hh
        sprite_cache.hh
        sprite_def.hh
        sprite_def_builder.hh
        sprite_set.hh
        sprite_sheet.hh
        sprite_state.hh
        texture_atlas.hh
    PRIVATE
        atlas_loader.cc
        image_decode.hh
        image_identity.cc
        render_bundle.cc
        resource_cache_core.hh
        resource_registry.hh
        sprite_animation.cc
        sprite_cache.cc
        sprite_def.cc
        sprite_def_builder.cc
        sprite_set.cc
        sprite_sheet.cc
        sprite_state.cc
        sprites_manager.cc
        sprites_manager.hh
        surface_lock.hh
        texture_atlas.cc
        texture_registry.cc
        texture_registry.hh
)

neutrino_subsystem(video_world DIR video/world
    PUBLIC
        camera.hh
        render_layer.hh
        resource_cache.hh
        sprite_batch.hh
        texture_layer.hh
        tileset_bundle.hh
        world_compositor.hh
        world_renderer.hh
    PRIVATE
        camera.cc
        resource_cache.cc
        sprite_batch.cc
        tileset_bundle.cc
        world_compositor.cc
        world_renderer.cc
)

neutrino_subsystem(world
    PUBLIC
        content_key.hh
        tmx_loader.hh
        world.hh
        world_common.hh
        world_layers.hh
        world_tileset.hh
    PRIVATE
        tmx_loader.cc
        world.cc
        tmx/component.cc
        tmx/component.hh
        tmx/decompressor.cc
        tmx/decompressor.hh
        tmx/encoding.cc
        tmx/encoding.hh
        tmx/image.cc
        tmx/image.hh
        tmx/layers.cc
        tmx/layers.hh
        tmx/node.cc
        tmx/node.hh
        tmx/objects.cc
        tmx/objects.hh
        tmx/parser.cc
        tmx/parser.hh
        tmx/sinfl.h
        tmx/tilesets.cc
        tmx/tilesets.hh
        tmx/world_parser.cc
        tmx/world_parser.hh
)

neutrino_subsystem(simplex
    PUBLIC
        application.hh
        entry_point.hh
    PRIVATE
        application.cc
        detail/application_adapter.cc
        detail/application_adapter.hh
        detail/simplex_scene.cc
        detail/simplex_scene.hh
)

neutrino_subsystem(text DIR video/text
        PUBLIC
            glyph_atlas.hh
            text.hh
        PRIVATE
            glyph_atlas.cc
            glyph_atlas_access.hh
)
# ----------------------------------------------------------------------------
# Target
# ----------------------------------------------------------------------------

neutrino_library_type(neutrino _lib_type)

# The engine's sources are compiled once, into neutrino_objects, which carries all of the
# engine's build settings and dependencies. The library (static or shared) is made from those
# objects. The test binary links neutrino_objects directly, so it can reach internals that a
# shared, hidden-visibility libneutrino does not export.

add_library(neutrino_objects OBJECT ${NEUTRINO_SOURCES})
target_include_directories(neutrino_objects
        PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
target_compile_features(neutrino_objects PUBLIC cxx_std_20)
target_link_libraries(neutrino_objects
        PUBLIC
        neutrino::sdlpp
        neutrino::musac
        musac::backend_sdl3
        musac::codecs
        neutrino::onyx_image
        neutrino::onyx_font
        PRIVATE
        neutrino::bstream
        nlohmann_json::nlohmann_json
        pugixml::pugixml)

if(NEUTRINO_NEUTRINO_TMX_ENABLE_ZSTD)
        target_compile_definitions(neutrino_objects
                PRIVATE
                NEUTRINO_TMX_ENABLE_ZSTD)
        target_link_libraries(neutrino_objects
                PRIVATE
                libzstd_static)
endif()

add_library(neutrino ${_lib_type})
add_library(neutrino::neutrino ALIAS neutrino)
target_link_libraries(neutrino PUBLIC neutrino_objects)

if(_lib_type STREQUAL "SHARED")
        # Export only what is marked NEUTRINO_EXPORT. CMake defines neutrino_EXPORTS for the
        # sources of a shared target; the objects are compiled in neutrino_objects, so define it there.
        set_target_properties(neutrino_objects PROPERTIES
                POSITION_INDEPENDENT_CODE ON
                CXX_VISIBILITY_PRESET hidden
                C_VISIBILITY_PRESET hidden
                VISIBILITY_INLINES_HIDDEN YES)
        target_compile_definitions(neutrino_objects PRIVATE neutrino_EXPORTS)
        set_target_properties(neutrino PROPERTIES
                CXX_VISIBILITY_PRESET hidden
                C_VISIBILITY_PRESET hidden
                VISIBILITY_INLINES_HIDDEN YES)
        # Static private dependencies (pugixml, zstd) are linked in; keep their symbols out of
        # the .so's exports.
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang" AND NOT APPLE)
                target_link_options(neutrino PRIVATE "LINKER:--exclude-libs,ALL")
        endif()
endif()

generate_export_header(neutrino
        BASE_NAME NEUTRINO
        EXPORT_FILE_NAME ${PROJECT_BINARY_DIR}/include/neutrino/neutrino_export.h
)
