//
// Created by igor on 30/06/2022.
//

#ifndef SRC_ASSETS_TILES_WORLD_TMX_IMAGE_LOADER_HH
#define SRC_ASSETS_TILES_WORLD_TMX_IMAGE_LOADER_HH

#include <string>
#include "neutrino/hal/video/surface.hh"
#include "neutrino/assets/resources/resource_io.hh"
#include "neutrino/assets/tiles/path_resolver.hh"
#include "assets/tiles/tmx/color.hh"

namespace neutrino::assets {
  class tmx_image_loader : public resource_reader<hal::surface,
                                                  const path_resolver_t&,
                                                  const std::string&,
                                                  const tmx::colori&,
                                                  int,
                                                  int
                                                  > {
    private:
      std::unique_ptr<hal::surface> do_load(const path_resolver_t& resolver, const std::string& name, const tmx::colori& tint, int offset_x, int offset_y) override;

  };
}

#endif //SRC_ASSETS_TILES_WORLD_TMX_IMAGE_LOADER_HH
