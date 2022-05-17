my_add_library_static(kernel
        SOURCES

        ${PROJECT_INCLUDE}/neutrino/kernel/application.hh
        application.cc

        ${PROJECT_INCLUDE}/neutrino/kernel/application_description.hh
        application_description.cc

        main_window.hh
        main_window.cc

        ${PROJECT_INCLUDE}/neutrino/kernel/hdi/events.hh
        hdi/events.cc
        hdi/kbd_actions.cc
        hdi/kbd_actions.hh
        hdi/kbd_mapper.cc
        hdi/kbd_mapper.hh
        ${PROJECT_INCLUDE}/neutrino/kernel/hdi/input_config.hh
        hdi/input_config.cc
        ${PROJECT_INCLUDE}/neutrino/kernel/hdi/pointer_config.hh
        hdi/pointer_config.cc
        hdi/pointer_mapper.cc
        hdi/pointer_mapper.hh

        ${PROJECT_INCLUDE}/neutrino/kernel/ecs/sparse_set.hh
        ${PROJECT_INCLUDE}/neutrino/kernel/ecs/sparse_map.hh
        ${PROJECT_INCLUDE}/neutrino/kernel/ecs/registry.hh
        ${PROJECT_INCLUDE}/neutrino/kernel/ecs/id_factory.hh
        ecs/registry.cc
        ecs/id_factory.cc

        gfx/texture.cc
        gfx/texture.hh
        ${PROJECT_INCLUDE}/neutrino/kernel/gfx/texture_atlas.hh
        gfx/texture_atlas.cc
        ${PROJECT_INCLUDE}/neutrino/kernel/gfx/world_renderer.hh
        gfx/world_renderer.cc
        ${PROJECT_INCLUDE}/neutrino/kernel/gfx/grid.hh
        gfx/grid.cc
        ${PROJECT_INCLUDE}/neutrino/kernel/gfx/gfx_assets.hh
        gfx/gfx_assets.cc
        ${PROJECT_INCLUDE}/neutrino/kernel/gfx/animation_state.hh
        gfx/animation_state.cc

        ${PROJECT_INCLUDE}/neutrino/kernel/rc/types.hh
        ${PROJECT_INCLUDE}/neutrino/kernel/rc/image.hh
        rc/image.cc
        ${PROJECT_INCLUDE}/neutrino/kernel/rc/tilesheet_description.hh
        rc/tilesheet_description.cc
        ${PROJECT_INCLUDE}/neutrino/kernel/rc/world/object.hh
        rc/world/object.cc
        ${PROJECT_INCLUDE}/neutrino/kernel/rc/world/image_layer.hh
        rc/world/image_layer.cc
        ${PROJECT_INCLUDE}/neutrino/kernel/rc/world/tiles_layer.hh
        rc/world/tiles_layer.cc
        ${PROJECT_INCLUDE}/neutrino/kernel/rc/world/world.hh
        rc/world/world.cc
        ${PROJECT_INCLUDE}/neutrino/kernel/rc/tile_handle.hh
        ${PROJECT_INCLUDE}/neutrino/kernel/rc/animation_description.hh
        rc/animation_description.cc
        ${PROJECT_INCLUDE}/neutrino/kernel/rc/path_resolver.hh

        #rc/tmx/tmx.cc
        rc/tmx/world_props.hh
        rc/tmx/cell.hh
        rc/tmx/cell.cc
        rc/tmx/animation.hh
        rc/tmx/animation.cc
        rc/tmx/color.hh
        rc/tmx/component.cc
        rc/tmx/component.hh
        rc/tmx/image.hh
        rc/tmx/image.cc
        rc/tmx/layer.hh
        rc/tmx/layer.cc
        rc/tmx/image_layer.hh
        rc/tmx/image_layer.cc
        rc/tmx/object.hh
        rc/tmx/object.cc
        rc/tmx/object_layer.hh
        rc/tmx/object_layer.cc
        rc/tmx/terrain.hh
        rc/tmx/terrain.cc
        rc/tmx/tile.hh
        rc/tmx/tile.cc
        rc/tmx/tile_layer.hh
        rc/tmx/tile_layer.cc
        rc/tmx/tile_set.hh
        rc/tmx/tile_set.cc
        rc/tmx/wang_set.cc
        rc/tmx/wang_set.hh
        rc/tmx/group.cc
        rc/tmx/group.hh
        rc/tmx/map.hh
        rc/tmx/map.cc
        rc/tmx/xml_reader.cc
        rc/tmx/xml_reader.hh
        rc/tmx/json_reader.cc
        rc/tmx/json_reader.hh
        rc/tmx/reader.hh
        rc/tmx/reader.cc

        rc/tmx/parse_data.cc
        rc/tmx/parse_data.hh
        rc/tmx/csv/parser.hpp

        DEPENDS_PUBLIC
            neutrino::hal
            neutrino::math
            neutrino::utils
        DEPENDS_PRIVATE
            thirdparty::pugixml
            thirdparty::json
        )
include(bin2c)

set(TEST_DATA
        test1/example.tmx
        test1/js_example.json
        test1/test_base64_level.tmx

        test1/test_csv_level.tmx
        test1/test_xml_level.tmx
        test-rs/tiled_flipped.tmx
        test-rs/tilesheet.tsx
        test-rs/tiled_base64_zlib.tmx
        test-rs/tiled_base64_gzip.tmx
        test-rs/js_tiled_base64_gzip.json
        test-rs/tiled_base64_zstandard.tmx
        test3/b64zlib.tmx
        test-rs/tiled_base64_zlib_infinite.tmx
        test-rs/js_tiled_base64_zlib_infinite.json
        test-rs/tilesheet.tsx
        test-rs/tiled_image_layers.tmx
        test-rs/js_tiled_image_layers.json
        test4/example.tmx
        test4/js_example.json
        json/simple_map.json
        wang/isometric_staggered_grass_and_water.tmx
        wang/js_isometric_staggered_grass_and_water.json
        )

set(GENERATED_HEADERS)
foreach (FILE ${TEST_DATA})
    set(PATH "rc/tmx/test/data/${FILE}")
    bin2c(${PATH} generated)
    list(APPEND GENERATED_HEADERS ${generated})
endforeach ()

add_library(neutrino_tiled_with_tests OBJECT
        rc/tmx/test/test_color.cc
        rc/tmx/test/test1_example_tmx.cc
        rc/tmx/test/test_encodings.cc
        rc/tmx/test/test_flipped.cc
        rc/tmx/test/test_utils.cc
        rc/tmx/test/test_utils.hh
        rc/tmx/test/test_compression.cc
        rc/tmx/test/test_text.cc
        rc/tmx/test/test_infinite.cc
        rc/tmx/test/test_properties.cc
        rc/tmx/test/test_json_simple.cc
        rc/tmx/test/test_json_complex.cc
        rc/tmx/test/test_wang.cc
        rc/tmx/test/test_image_layers.cc
        ${GENERATED_HEADERS}
        )

target_link_libraries(neutrino_tiled_with_tests
        neutrino::kernel
        thirdparty::doctest
        )
target_include_directories(neutrino_tiled_with_tests
        PUBLIC ${PROJECT_INCLUDE} ${PROJECT_CONFIG_INCLUDE}
        PRIVATE ${PROJECT_INCLUDE_PRIVATE} ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR}/rc/tmx/test/data/
        )