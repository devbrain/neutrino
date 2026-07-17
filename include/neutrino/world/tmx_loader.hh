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
    /**
     * @brief Resolves an external tileset reference to its document text.
     *
     * The parser calls this when a map references an external tileset (e.g. a TSX
     * file). @c source is the reference string exactly as written in the document
     * (typically a relative path); the callback returns the referenced document's
     * *contents* (not its path) for the parser to parse in place. It may throw to
     * reject an unresolvable or unsafe reference.
     */
    using tmx_path_resolver = std::function <std::string(std::string_view source)>;

    /**
     * @brief Load a TMX/XML or Tiled JSON document from memory.
     *
     * The returned value is the public Neutrino world model, not a TMX AST.
     *
     * @param document The full XML (TMX/TSX) or JSON map document.
     * @param resolver Resolver for external tilesets such as TSX files; may be empty
     *                 when the document embeds all tilesets. Encountering an external
     *                 tileset without a resolver is an error.
     * @return The parsed world.
     * @throws std::runtime_error if the document is empty, is not recognizable XML or
     *         JSON, is malformed, or references an external tileset that cannot be
     *         resolved (including anything the resolver itself throws).
     */
    NEUTRINO_EXPORT world load_tmx_world(
        std::string_view document,
        tmx_path_resolver resolver = {});

    /**
     * @brief Load a TMX/XML or Tiled JSON document from a file.
     *
     * Installs a default resolver that reads external tilesets from paths relative to
     * @p path's directory, rejecting absolute paths and paths that escape that directory.
     *
     * @param path Filesystem path to the map document.
     * @return The parsed world.
     * @throws std::runtime_error if the file cannot be opened, the document is malformed,
     *         or an external tileset path is absolute, escapes the map's directory, or
     *         cannot be read.
     */
    NEUTRINO_EXPORT world load_tmx_world_file(const std::filesystem::path& path);
}
