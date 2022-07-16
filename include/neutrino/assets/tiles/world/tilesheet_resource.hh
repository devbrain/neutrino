//
// Created by igor on 13/07/2022.
//

#ifndef INCLUDE_NEUTRINO_ASSETS_TILES_WORLD_TILESHEET_RESOURCE_HH
#define INCLUDE_NEUTRINO_ASSETS_TILES_WORLD_TILESHEET_RESOURCE_HH

#include <variant>
#include <optional>

#include <neutrino/assets/tiles/tilesheet_description.hh>
#include <neutrino/assets/tiles/types.hh>
#include <neutrino/assets/tiles/tile_handle.hh>
#include <neutrino/assets/resources/resource_io.hh>

namespace neutrino::assets {

  struct tilesheet_resource {
    explicit tilesheet_resource (hal::surface s);
    tilesheet_resource (hal::surface s, tilesheet_rects rects);

    hal::surface surface;
    std::optional<tilesheet_rects> rectangles;
  };



  using tilesheet_loader_input = std::variant<lazy_tilesheet, lazy_image_loader, hal::surface, tilesheet>;

  class tilesheet_resource_reader : public resource_reader<tilesheet_resource, tilesheet_loader_input> {
    private:
      std::unique_ptr<tilesheet_resource> do_load(tilesheet_loader_input input) override;
    private:

  };
}


#endif //INCLUDE_NEUTRINO_ASSETS_TILES_WORLD_TILESHEET_RESOURCE_HH
