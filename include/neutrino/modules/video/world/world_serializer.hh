//
// Created by igor on 8/27/24.
//

#ifndef WORLD_SERIALIZER_HH
#define WORLD_SERIALIZER_HH

#include <filesystem>
#include <neutrino/neutrino_export.hh>
#include <sdlpp/video/render.hh>

namespace neutrino {
    namespace tiled {
        class NEUTRINO_EXPORT world_model;
    }
    class NEUTRINO_EXPORT texture_atlas;

    NEUTRINO_EXPORT void serialize_tmx(const std::filesystem::path& out_dir,
                       const tiled::world_model& model,
                       const texture_atlas& atlas,
                       const sdl::renderer& renderer);
}

#endif
