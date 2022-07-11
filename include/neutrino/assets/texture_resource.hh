//
// Created by igor on 31/05/2022.
//

#ifndef INCLUDE_NEUTRINO_ASSETS_RESOURCES_TEXTURE_RESOURCE_HH
#define INCLUDE_NEUTRINO_ASSETS_RESOURCES_TEXTURE_RESOURCE_HH

#include <neutrino/hal/video/renderer.hh>
#include <neutrino/hal/video/texture.hh>
#include <neutrino/assets/resources/resource_io.hh>

namespace neutrino::assets {
  class texture_resource_reader : public resource_reader<hal::texture, hal::renderer&, const resource_id&>
  {
    private:
      std::unique_ptr<hal::texture> do_load(hal::renderer& r, const resource_id& image_id) override;
  };

  using texture_resource = resource<hal::texture>;
}


#endif //INCLUDE_NEUTRINO_ASSETS_RESOURCES_TEXTURE_RESOURCE_HH
