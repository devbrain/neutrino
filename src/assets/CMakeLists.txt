my_add_library_static(assets
        SOURCES
            # FS ------------------------------------------------------------
            ${PROJECT_INCLUDE}/neutrino/assets/fs/abstract_fs.hh
            fs/abstract_fs.cc
            ${PROJECT_INCLUDE}/neutrino/assets/fs/phys_fs.hh
            fs/phys_fs.cc

            # Image ------------------------------------------------------------
            ${PROJECT_INCLUDE}/neutrino/assets/image/color.hh
            ${PROJECT_INCLUDE}/neutrino/assets/image/image.hh
            ${PROJECT_INCLUDE}/neutrino/assets/image/image_io.hh
            image/image_io.cc
            image/image.cc
            image/image_loader.cc
            image/image_loader.hh
            image/thirdparty/SDL_stbimage.h
            image/thirdparty/stb_image.h
            image/thirdparty/stb_image.c
            image/thirdparty/stb_image_write.h
            image/thirdparty/stb_image_write.c
            image/thirdparty/SDL_lbm.h
            image/thirdparty/SDL_pcx.h
            image/stb_image_loader.cc
            image/stb_image_loader.hh
            image/image_writer.cc
            image/image_writer.hh

            image/thirdparty/pngpp/color.hpp
            image/thirdparty/pngpp/config.hpp
            image/thirdparty/pngpp/consumer.hpp
            image/thirdparty/pngpp/convert_color_space.hpp
            image/thirdparty/pngpp/end_info.hpp
            image/thirdparty/pngpp/error.hpp
            image/thirdparty/pngpp/ga_pixel.hpp
            image/thirdparty/pngpp/generator.hpp
            image/thirdparty/pngpp/gray_pixel.hpp
            image/thirdparty/pngpp/image.hpp
            image/thirdparty/pngpp/image_info.hpp
            image/thirdparty/pngpp/index_pixel.hpp
            image/thirdparty/pngpp/info_base.hpp
            image/thirdparty/pngpp/info.hpp
            image/thirdparty/pngpp/io_base.hpp
            image/thirdparty/pngpp/packed_pixel.hpp
            image/thirdparty/pngpp/palette.hpp
            image/thirdparty/pngpp/pixel_buffer.hpp
            image/thirdparty/pngpp/pixel_traits.hpp
            image/thirdparty/pngpp/png.hpp
            image/thirdparty/pngpp/reader.hpp
            image/thirdparty/pngpp/require_color_space.hpp
            image/thirdparty/pngpp/rgba_pixel.hpp
            image/thirdparty/pngpp/rgb_pixel.hpp
            image/thirdparty/pngpp/solid_pixel_buffer.hpp
            image/thirdparty/pngpp/streaming_base.hpp
            image/thirdparty/pngpp/tRNS.hpp
            image/thirdparty/pngpp/types.hpp
            image/thirdparty/pngpp/writer.hpp

            image/thirdparty/libamivideo/amivideotypes.h
            image/thirdparty/libamivideo/palette.c
            image/thirdparty/libamivideo/palette.h
            image/thirdparty/libamivideo/screen.c
            image/thirdparty/libamivideo/screen.h
            image/thirdparty/libamivideo/viewportmode.c
            image/thirdparty/libamivideo/viewportmode.h
            image/amiga/iff/fourcc.cc
            image/amiga/iff/fourcc.hh
            image/amiga/iff/iff_events.cc
            image/amiga/iff/iff_events.hh
            image/amiga/iff/iff_parser.hh
            image/amiga/iff/ea/ea.hh
            image/amiga/iff/ea/ea_events.cc
            image/amiga/iff/ea/ea_events.hh
            image/amiga/amiga_chunks.cc
            image/amiga/amiga_chunks.hh
            image/amiga/amiga_image.cc
            image/amiga/amiga_image.hh
            image/amiga/lbm.cc
            image/amiga/lbm.hh

            ios_rwops.hh
            # Explode ------------------------------------------------------------
            ${PROJECT_INCLUDE}/neutrino/assets/explode/explode_exe_file.hh
            explode/explode_exe_file.cc
            explode/mz/exe_file.cc
            explode/mz/exe_file.hh
            explode/mz/io.cc
            explode/mz/io.hh
            explode/mz/knowledge_dynamics.cc
            explode/mz/knowledge_dynamics.hh
            explode/mz/struct_reader.hh
            explode/mz/unexepack.cc
            explode/mz/unexepack.hh
            explode/mz/unlzexe.hh
            explode/mz/unlzexe.cc
            explode/mz/unpklite.cc
            explode/mz/unpklite.hh

            # TILES -----------------------------------------------------------------
            ${PROJECT_INCLUDE}/neutrino/assets/tiles/types.hh
            ${PROJECT_INCLUDE}/neutrino/assets/tiles/tilesheet_description.hh
            tiles/tilesheet_description.cc
            ${PROJECT_INCLUDE}/neutrino/assets/tiles/world/object.hh
            tiles/world/object.cc
            ${PROJECT_INCLUDE}/neutrino/assets/tiles/world/image_layer.hh
            tiles/world/image_layer.cc
            ${PROJECT_INCLUDE}/neutrino/assets/tiles/world/color_layer.hh
            tiles/world/color_layer.cc
            ${PROJECT_INCLUDE}/neutrino/assets/tiles/world/tiles_layer.hh
            tiles/world/tiles_layer.cc
            ${PROJECT_INCLUDE}/neutrino/assets/tiles/world/world.hh
            tiles/world/world.cc
            ${PROJECT_INCLUDE}/neutrino/assets/tiles/tile_handle.hh
            ${PROJECT_INCLUDE}/neutrino/assets/tiles/animation_description.hh
            tiles/animation_description.cc
            ${PROJECT_INCLUDE}/neutrino/assets/tiles/path_resolver.hh
            ${PROJECT_INCLUDE}/neutrino/assets/tiles/image_atlas.hh
            tiles/image_atlas.cc
            ${PROJECT_INCLUDE}/neutrino/assets/tiles/world/world_assets.hh
            #rc/tmx/tmx.cc
            tiles/tmx/world_props.hh
            tiles/tmx/cell.hh
            tiles/tmx/cell.cc
            tiles/tmx/animation.hh
            tiles/tmx/animation.cc
            tiles/tmx/color.hh
            tiles/tmx/component.cc
            tiles/tmx/component.hh
            tiles/tmx/image.hh
            tiles/tmx/image.cc
            tiles/tmx/layer.hh
            tiles/tmx/layer.cc
            tiles/tmx/image_layer.hh
            tiles/tmx/image_layer.cc
            tiles/tmx/object.hh
            tiles/tmx/object.cc
            tiles/tmx/object_layer.hh
            tiles/tmx/object_layer.cc
            tiles/tmx/terrain.hh
            tiles/tmx/terrain.cc
            tiles/tmx/tile.hh
            tiles/tmx/tile.cc
            tiles/tmx/tile_layer.hh
            tiles/tmx/tile_layer.cc
            tiles/tmx/tile_set.hh
            tiles/tmx/tile_set.cc
            tiles/tmx/wang_set.cc
            tiles/tmx/wang_set.hh
            tiles/tmx/group.cc
            tiles/tmx/group.hh
            tiles/tmx/map.hh
            tiles/tmx/map.cc
            tiles/tmx/xml_reader.cc
            tiles/tmx/xml_reader.hh
            tiles/tmx/json_reader.cc
            tiles/tmx/json_reader.hh
            tiles/tmx/reader.hh
            tiles/tmx/reader.cc

            tiles/tmx/parse_data.cc
            tiles/tmx/parse_data.hh
            tiles/tmx/csv/parser.hpp


            pe/abstract_reporter.cpp
            pe/abstract_reporter.hpp
            pe/clr.cpp
            pe/clr.hpp
            pe/clr_res.cpp
            pe/clr_res.hpp
            pe/clr_tables_helper.hpp
            pe/clr_tables.hpp
            pe/exefile.cpp
            pe/exefile.hpp
            pe/imports_parser.cpp
            pe/imports_parser.hpp
            pe/istream_wrapper.cpp
            pe/istream_wrapper.hpp
            pe/macros.h
            pe/pe_drv.cpp
            pe/pefile.cpp
            pe/pefile.hpp
            pe/predef.h
            pe/resource_builder.cpp
            pe/resource_builder.hpp
            pe/resource_directory.cpp
            pe/resource_directory.hpp
            pe/rs_dialog.cpp
            pe/rs_dialog.hpp
            pe/rs_icon_group.cpp
            pe/rs_icon_group.hpp
            pe/rs_manifest.cpp
            pe/rs_manifest.hpp
            pe/rs_message_table.cpp
            pe/rs_message_table.hpp
            pe/rs_string_table.cpp
            pe/rs_string_table.hpp
            pe/rs_version.cpp
            pe/rs_version.hpp

        DEPENDS_PUBLIC
            neutrino::hal
            neutrino::math
            neutrino::utils
        DEPENDS_PRIVATE
            thirdparty::pugixml
            thirdparty::json
            thirdparty::png
            thirdparty::mio

)
add_library(assets_mz_with_tests OBJECT
            explode/mz/test/explode_test.cc
            explode/mz/test/md5.c
            explode/mz/test/md5.h
            explode/mz/test/dot.h
            explode/mz/test/lex.h
            explode/mz/test/pklite_100.h
            explode/mz/test/pklite_112.h
            explode/mz/test/pklite_115.h
            explode/mz/test/pklite_120.h
            explode/mz/test/pklite_150.h
            explode/mz/test/pklite_201.h
            explode/mz/test/pklite_E_112.h
            explode/mz/test/pklite_E_115.h
            explode/mz/test/pklite_E_120.h
            explode/mz/test/tntega.h
            explode/mz/test/z90.h
            explode/mz/test/z91e.h
            explode/mz/test/z91.h
        )
target_link_libraries(assets_mz_with_tests
        PUBLIC
            neutrino::assets
            neutrino::utils
        PRIVATE
            thirdparty::doctest
        )
target_include_directories(assets_mz_with_tests
        PUBLIC ${PROJECT_INCLUDE} ${PROJECT_CONFIG_INCLUDE}
        PRIVATE ${PROJECT_INCLUDE_PRIVATE} ${CMAKE_CURRENT_BINARY_DIR}
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
        flip/test.tmx
        )

set(GENERATED_HEADERS)
foreach (FILE ${TEST_DATA})
    set(PATH "tiles/tmx/test/data/${FILE}")
    bin2c(${PATH} generated)
    list(APPEND GENERATED_HEADERS ${generated})
endforeach ()

add_library(neutrino_tiled_with_tests OBJECT
        tiles/tmx/test/test_color.cc
        tiles/tmx/test/test1_example_tmx.cc
        tiles/tmx/test/test_encodings.cc
        tiles/tmx/test/test_flipped.cc
        tiles/tmx/test/test_utils.cc
        tiles/tmx/test/test_utils.hh
        tiles/tmx/test/test_compression.cc
        tiles/tmx/test/test_text.cc
        tiles/tmx/test/test_infinite.cc
        tiles/tmx/test/test_properties.cc
        tiles/tmx/test/test_json_simple.cc
        tiles/tmx/test/test_json_complex.cc
        tiles/tmx/test/test_wang.cc
        tiles/tmx/test/test_image_layers.cc
        ${GENERATED_HEADERS}
        )

target_link_libraries(neutrino_tiled_with_tests
        neutrino::kernel
        thirdparty::doctest
        )
target_include_directories(neutrino_tiled_with_tests
        PUBLIC ${PROJECT_INCLUDE} ${PROJECT_CONFIG_INCLUDE}
        PRIVATE ${PROJECT_INCLUDE_PRIVATE} ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR}/tiles/tmx/test/data/
        )