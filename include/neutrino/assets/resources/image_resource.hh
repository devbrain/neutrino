//
// Created by igor on 31/05/2022.
//

#ifndef INCLUDE_NEUTRINO_ASSETS_RESOURCES_IMAGE_RESOURCE_HH
#define INCLUDE_NEUTRINO_ASSETS_RESOURCES_IMAGE_RESOURCE_HH

#include <istream>
#include <neutrino/hal/video/surface.hh>
#include <neutrino/assets/resources/resource.hh>

namespace neutrino::assets {
  class image_resource : public resource<hal::surface, std::istream &> {
    public:
      image_resource(const resource_id& id, resource_processing_context* ctx);
    protected:
      std::unique_ptr<hal::surface> do_load(std::istream& is) override;
      void do_save(std::ostream& os, const hal::surface& data) override;
  };
}


#endif //INCLUDE_NEUTRINO_ASSETS_RESOURCES_IMAGE_RESOURCE_HH
