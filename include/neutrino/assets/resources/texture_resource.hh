//
// Created by igor on 31/05/2022.
//

#ifndef INCLUDE_NEUTRINO_ASSETS_RESOURCES_TEXTURE_RESOURCE_HH
#define INCLUDE_NEUTRINO_ASSETS_RESOURCES_TEXTURE_RESOURCE_HH

#include <neutrino/hal/video/renderer.hh>
#include <neutrino/hal/video/texture.hh>
#include <neutrino/assets/resources/resource.hh>

namespace neutrino::assets {
  class texture_resource : public resource<hal::texture, hal::renderer&, const resource_id&>
  {
    public:
      texture_resource(const resource_id& id, resource_processing_context* ctx);
    private:
      std::unique_ptr<hal::texture> do_load(hal::renderer& r, const resource_id& image_id) override;
  };
}


#endif //INCLUDE_NEUTRINO_ASSETS_RESOURCES_TEXTURE_RESOURCE_HH
