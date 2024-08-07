set (src
        ${PROJECT_INCLUDE}/neutrino/neutrino_export.hh
        # =============================================================
        # EVENTS
        # =============================================================
        ${PROJECT_INCLUDE}/neutrino/events/events.hh
        ${PROJECT_INCLUDE}/neutrino/events/events_reactor.hh
        ${PROJECT_INCLUDE}/neutrino/events/detail/event_producer.hh
        events/events_reactor.cc
        events/neutrino_events.cc
        events/neutrino_events.hh

        # =============================================================
        # SCENE
        # =============================================================
        ${PROJECT_INCLUDE}/neutrino/scene/scene.hh
        ${PROJECT_INCLUDE}/neutrino/scene/title_scene.hh
        ${PROJECT_INCLUDE}/neutrino/scene/scene_manager.hh
        scene/scene.cc
        scene/scene_manager.cc
        scene/title_scene.cc

        # =============================================================
        # APPLICATION
        # =============================================================
        ${PROJECT_INCLUDE}/neutrino/application.hh
        application.cc

        # =============================================================
        # CONFIG
        # =============================================================
        ${PROJECT_INCLUDE}/neutrino/ini/config.hh
        ${PROJECT_INCLUDE}/neutrino/ini/config_parser.hh
        ini/ini_parser.cc
        ini/inih.h
        ini/inih.c

        # =============================================================
        # modules/PHYSICS
        # =============================================================
        ${PROJECT_INCLUDE}/neutrino/modules/physics/components/body.hh
        # =============================================================
        # modules/VIDEO
        # =============================================================
        ${PROJECT_INCLUDE}/neutrino/modules/video/types.hh
        ${PROJECT_INCLUDE}/neutrino/modules/video/texture_atlas.hh
        ${PROJECT_INCLUDE}/neutrino/modules/video/tile.hh
        ${PROJECT_INCLUDE}/neutrino/modules/video/tile_font_spec.hh
        ${PROJECT_INCLUDE}/neutrino/modules/video/fonts_registry.hh
        ${PROJECT_INCLUDE}/neutrino/modules/video/fonts_renderer.hh
        ${PROJECT_INCLUDE}/neutrino/modules/video/renderer_state.hh
        ${PROJECT_INCLUDE}/neutrino/modules/video/world_renderer.hh
        ${PROJECT_INCLUDE}/neutrino/modules/video/world/tiles_layer.hh
        ${PROJECT_INCLUDE}/neutrino/modules/video/world/objects_layer.hh
        ${PROJECT_INCLUDE}/neutrino/modules/video/world/color_layer.hh
        ${PROJECT_INCLUDE}/neutrino/modules/video/world/image_layer.hh
        ${PROJECT_INCLUDE}/neutrino/modules/video/world/world_model.hh
        ${PROJECT_INCLUDE}/neutrino/modules/video/world/animation_sequence.hh
        ${PROJECT_INCLUDE}/neutrino/modules/video/components/sprite_component.hh
        ${PROJECT_INCLUDE}/neutrino/modules/video/systems/sprite_system.hh
        ${PROJECT_INCLUDE}/neutrino/modules/video/systems/world_rendering_system.hh
        modules/video/texture_atlas.cc
        modules/video/tile.cc
        modules/video/tile_font_spec.cc
        modules/video/fonts_registry.cc
        modules/video/fonts_renderer.cc
        modules/video/world_renderer.cc
        modules/video/world/tiles_layer.cc
        modules/video/world/objects_layer.cc
        modules/video/world/color_layer.cc
        modules/video/world/image_layer.cc
        modules/video/world/world_model.cc
        modules/video/world/animation_sequence.cc
        modules/video/components/sprite_component.cc
        modules/video/systems/sprite_system.cc
        modules/video/systems/world_rendering_system.cc
        # =============================================================
        # ECS
        # =============================================================
        ${PROJECT_INCLUDE}/neutrino/ecs/types.hh
        ${PROJECT_INCLUDE}/neutrino/ecs/registry.hh
        ${PROJECT_INCLUDE}/neutrino/ecs/systems_holder.hh
        ${PROJECT_INCLUDE}/neutrino/ecs/abstract_system.hh
        ${PROJECT_INCLUDE}/neutrino/ecs/iterators/registry_iterator.hh
        ${PROJECT_INCLUDE}/neutrino/ecs/iterators/registry_iterator_impl.hh
        ${PROJECT_INCLUDE}/neutrino/ecs/detail/sorted_array.hh
        ${PROJECT_INCLUDE}/neutrino/ecs/detail/bi_map.hh
        ${PROJECT_INCLUDE}/neutrino/ecs/detail/component_bucket.hh
        ecs/detail/component_bucket.cc
        ecs/registry.cc
        ecs/abstract_system.cc
        ecs/systems_holder.cc
        # =============================================================
        # UTILS
        # =============================================================
        ${PROJECT_INCLUDE}/neutrino/utils/line.hh
        ${PROJECT_INCLUDE}/neutrino/utils/random.hh
)

if (NOT DEFINED CMAKE_CXX_VISIBILITY_PRESET AND
        NOT DEFINED CMAKE_VISIBILITY_INLINES_HIDDEN)
    set(CMAKE_CXX_VISIBILITY_PRESET hidden)
    set(CMAKE_VISIBILITY_INLINES_HIDDEN YES)
endif ()

if (DEFINED neutrino_SHARED_LIBS)
    set(BUILD_SHARED_LIBS "${neutrino_SHARED_LIBS}")
endif ()

add_library(neutrino ${src})

add_library(neutrino::neutrino ALIAS neutrino)

set_target_properties(neutrino PROPERTIES
        VERSION ${neutrino_VERSION}
        SOVERSION ${neutrino_VERSION_MAJOR})

target_include_directories(neutrino
        PUBLIC ${PROJECT_INCLUDE} ${PROJECT_CONFIG_INCLUDE}
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${test_include}
)

target_compile_options(neutrino PRIVATE
        $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
        -Wall>
        $<$<CXX_COMPILER_ID:MSVC>:
        /W4>)

target_compile_features(neutrino PUBLIC cxx_std_17)

include(GenerateExportHeader)
generate_export_header(neutrino EXPORT_FILE_NAME ${PROJECT_CONFIG_INCLUDE}/neutrino/neutrino_export_def.h)
if (neutrino_ENABLE_TESTING)
    target_compile_definitions(neutrino PRIVATE neutrino_BUILD_FOR_TESTING)
endif()

target_link_libraries(neutrino
        PUBLIC neutrino::assets neutrino::vfs bsw::bsw boost_callable_traits JGuegant::dense_hash_map
        PRIVATE ryml::ryml thirdparty::json box2d
)