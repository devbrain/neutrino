my_add_library_static(tiled
        SOURCES

        ${PROJECT_INCLUDE}/neutrino/tiled/tile_sheet.hh
        tile_sheet.cc
        tile_sheet_impl.hh

        ${PROJECT_INCLUDE}/neutrino/tiled/tile_sheet_manager.hh
        tile_sheet_manager.cc

        ${PROJECT_INCLUDE}/neutrino/tiled/tile_description.hh

        ${PROJECT_INCLUDE}/neutrino/tiled/world.hh
        world.cc

        ${PROJECT_INCLUDE}/neutrino/tiled/world_renderer.hh
        world_renderer.cc

        ${PROJECT_INCLUDE}/neutrino/tiled/camera.hh
        camera.cc

        ${PROJECT_INCLUDE}/neutrino/tiled/tmx.hh
        tmx.cc

        DEPENDS_PUBLIC
            neutrino::engine
        )
