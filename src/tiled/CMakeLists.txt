my_add_library_static(tiled
        SOURCES
        ${PROJECT_INCLUDE}/neutrino/tiled/world_renderer.hh
        world_renderer.cc

        ${PROJECT_INCLUDE}/neutrino/tiled/world/types.hh
        ${PROJECT_INCLUDE}/neutrino/tiled/world/tile_description.hh

        ${PROJECT_INCLUDE}/neutrino/tiled/texture_atlas.hh
        texture_atlas.cc
        ${PROJECT_INCLUDE}/neutrino/tiled/world/builder/texture_atlas_builder.hh
        world/builder/texture_atlas_builder.cc

        ${PROJECT_INCLUDE}/neutrino/tiled/world/world_props.hh
        ${PROJECT_INCLUDE}/neutrino/tiled/world/world.hh
        world/world.cc
        ${PROJECT_INCLUDE}/neutrino/tiled/world/builder/world_builder.hh
        world/builder/world_builder.cc

        ${PROJECT_INCLUDE}/neutrino/tiled/world/layer.hh
        world/layer.cc
        ${PROJECT_INCLUDE}/neutrino/tiled/world/builder/layers_builder.hh
        world/builder/layers_builder.cc

        # TMX Loader
        ${PROJECT_INCLUDE}/neutrino/tiled/loader/tmx.hh
        ${PROJECT_INCLUDE}/neutrino/tiled/loader/path_resolver.hh
        loader/tmx/tmx.cc
        loader/tmx/cell.hh
        loader/tmx/cell.cc
        loader/tmx/animation.hh
        loader/tmx/animation.cc
        loader/tmx/color.hh
        loader/tmx/component.cc
        loader/tmx/component.hh
        loader/tmx/image.hh
        loader/tmx/image.cc
        loader/tmx/layer.hh
        loader/tmx/layer.cc
        loader/tmx/image_layer.hh
        loader/tmx/image_layer.cc
        loader/tmx/object.hh
        loader/tmx/object.cc
        loader/tmx/object_layer.hh
        loader/tmx/object_layer.cc
        loader/tmx/terrain.hh
        loader/tmx/terrain.cc
        loader/tmx/tile.hh
        loader/tmx/tile.cc
        loader/tmx/tile_layer.hh
        loader/tmx/tile_layer.cc
        loader/tmx/tile_set.hh
        loader/tmx/tile_set.cc
        loader/tmx/wang_set.cc
        loader/tmx/wang_set.hh
        loader/tmx/group.cc
        loader/tmx/group.hh
        loader/tmx/map.hh
        loader/tmx/map.cc
        loader/tmx/xml_reader.cc
        loader/tmx/xml_reader.hh
        loader/tmx/json_reader.cc
        loader/tmx/json_reader.hh
        loader/tmx/reader.hh
        loader/tmx/reader.cc

        loader/tmx/parse_data.cc
        loader/tmx/parse_data.hh
        loader/tmx/csv/parser.hpp

        DEPENDS_PUBLIC
            neutrino::kernel
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
    set(PATH "loader/tmx/test/data/${FILE}")
    bin2c(${PATH} generated)
    list(APPEND GENERATED_HEADERS ${generated})
endforeach ()

add_library(neutrino_tiled_with_tests OBJECT
        loader/tmx/test/test_color.cc
        loader/tmx/test/test1_example_tmx.cc
        loader/tmx/test/test_encodings.cc
        loader/tmx/test/test_flipped.cc
        loader/tmx/test/test_utils.cc
        loader/tmx/test/test_utils.hh
        loader/tmx/test/test_compression.cc
        loader/tmx/test/test_text.cc
        loader/tmx/test/test_infinite.cc
        loader/tmx/test/test_properties.cc
        loader/tmx/test/test_json_simple.cc
        loader/tmx/test/test_json_complex.cc
        loader/tmx/test/test_wang.cc
        loader/tmx/test/test_image_layers.cc
        ${GENERATED_HEADERS}
        )

target_link_libraries(neutrino_tiled_with_tests
        neutrino::tiled
        thirdparty::doctest
        )
target_include_directories(neutrino_tiled_with_tests
        PUBLIC ${PROJECT_INCLUDE} ${PROJECT_CONFIG_INCLUDE}
        PRIVATE ${PROJECT_INCLUDE_PRIVATE} ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR}/loader/tmx/test/data/
)
