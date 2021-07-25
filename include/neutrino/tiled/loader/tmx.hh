//
// Created by igor on 08/07/2021.
//

#ifndef NEUTRINO_TILED_LOADER_TMX_HH
#define NEUTRINO_TILED_LOADER_TMX_HH

#include <filesystem>
#include <iosfwd>
#include <memory>

namespace neutrino::tiled {
    class tile_sheet_manager;
    class world;

    namespace tmx
    {
        std::unique_ptr<world> load(std::istream& is, tile_sheet_manager& tsm);
        std::unique_ptr<world> load(std::filesystem::path& path, tile_sheet_manager& tsm);
    }
}

#endif
