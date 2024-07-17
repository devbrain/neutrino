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
        # SYSTEMS/VIDEO
        # =============================================================
        ${PROJECT_INCLUDE}/neutrino/systems/video/types.hh
        ${PROJECT_INCLUDE}/neutrino/systems/video/texture_atlas.hh
        systems/video/texture_atlas.cc
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
        PUBLIC neutrino::assets neutrino::vfs bsw::bsw boost_callable_traits
        PRIVATE ryml::ryml thirdparty::json
)