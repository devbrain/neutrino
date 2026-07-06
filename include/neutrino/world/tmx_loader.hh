//
// Created by igor on 05/07/2026.
//

#pragma once

#include <filesystem>
#include <functional>
#include <string>
#include <string_view>

#include <neutrino/neutrino_export.h>
#include <neutrino/world/world.hh>

namespace neutrino {
    using tmx_path_resolver = std::function <std::string(std::string_view source)>;

    /**
     * @brief Load a TMX/XML or Tiled JSON document from memory.
     *
     * The optional resolver is used for external tilesets such as TSX files.
     * The returned value is the public Neutrino world model, not a TMX AST.
     */
    NEUTRINO_EXPORT world load_tmx_world(
        std::string_view document,
        tmx_path_resolver resolver = {});

    /**
     * @brief Load a TMX/XML or Tiled JSON document from a file.
     *
     * External tilesets are resolved relative to the loaded file by default.
     */
    NEUTRINO_EXPORT world load_tmx_world_file(const std::filesystem::path& path);
}
