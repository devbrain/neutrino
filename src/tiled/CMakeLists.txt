my_add_library_static(tiled
        SOURCES

        ${PROJECT_INCLUDE}/neutrino/tiled/world/tile_types.hh
        ${PROJECT_INCLUDE}/neutrino/tiled/world/tile_sheet.hh
        world/tile_sheet.cc
        world/tile_sheet_impl.hh

        ${PROJECT_INCLUDE}/neutrino/tiled/world/tile_sheet_manager.hh
        world/tile_sheet_manager.cc

        ${PROJECT_INCLUDE}/neutrino/tiled/world/tile_description.hh

        ${PROJECT_INCLUDE}/neutrino/tiled/world/world.hh
        world/world.cc

        #${PROJECT_INCLUDE}/neutrino/tiled/world/world_renderer.hh
        #world/world_renderer.cc

        ${PROJECT_INCLUDE}/neutrino/tiled/world/camera.hh
        world/camera.cc

        ${PROJECT_INCLUDE}/neutrino/tiled/loader/tmx.hh
        loader/tmx/tmx.cc
        loader/tmx/cell.hh
        loader/tmx/color.hh
        loader/tmx/component.cc
        loader/tmx/component.hh
        loader/tmx/image.hh
        loader/tmx/layer.hh
        loader/tmx/image_layer.hh
        loader/tmx/object.hh
        loader/tmx/object_layer.hh
        loader/tmx/terrain.hh
        loader/tmx/tile.hh
        loader/tmx/tile_layer.hh
        loader/tmx/tile_set.hh
        loader/tmx/tile_set.cc
        loader/tmx/map.hh
        loader/tmx/map.cc
        loader/tmx/xml.cc
        loader/tmx/xml.hh

        ${PROJECT_INCLUDE}/neutrino/tiled/window.hh
        window.cc

        ${PROJECT_INCLUDE}/neutrino/tiled/video.hh
        video.cc

        DEPENDS_PUBLIC
            neutrino::engine
            thirdparty::pugixml
        )
include(bin2c)

set (TEST_DATA
        test1/example.tmx
        test1/test_base64_level.tmx
        test1/test_csv_level.tmx
        test1/test_xml_level.tmx
        )
set (GENERATED_HEADERS )
foreach(FILE ${TEST_DATA})
    set (PATH "loader/tmx/test/data/${FILE}")
    BIN2C(${PATH} generated)
    LIST(APPEND GENERATED_HEADERS ${generated})
endforeach()

add_library(neutrino_tiled_with_tests OBJECT
        loader/tmx/test/test_color.cc
        loader/tmx/test/test1_example_tmx.cc
        loader/tmx/test/test1_base64_level.cc
        loader/tmx/test/test1_csv_level.cc
        loader/tmx/test/test1_xml_level.cc
        loader/tmx/test/test_utils.cc
        loader/tmx/test/test_utils.hh

        ${GENERATED_HEADERS}
        )

target_link_libraries(neutrino_tiled_with_tests
        neutrino::tiled
        thirdparty::doctest
        )
target_include_directories(neutrino_tiled_with_tests
        PUBLIC  ${PROJECT_INCLUDE} ${PROJECT_CONFIG_INCLUDE}
        PRIVATE ${PROJECT_INCLUDE_PRIVATE} ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR}/loader/tmx/test/data/
        )
