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

        DEPENDS_PUBLIC
            neutrino::engine
        )
