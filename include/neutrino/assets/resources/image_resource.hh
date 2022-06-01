//
// Created by igor on 31/05/2022.
//

#ifndef INCLUDE_NEUTRINO_ASSETS_RESOURCES_IMAGE_RESOURCE_HH
#define INCLUDE_NEUTRINO_ASSETS_RESOURCES_IMAGE_RESOURCE_HH

#include <istream>
#include <neutrino/hal/video/surface.hh>
#include <neutrino/assets/resources/resource_io.hh>

namespace neutrino::assets {
  class image_resource_reader : public resource_reader<hal::surface, std::istream &> {
    private:
      std::unique_ptr<hal::surface> do_load(std::istream& is) override;
  };

  class image_resource_writer : public resource_writer<hal::surface> {
    private:
      void do_save(std::ostream& os, const hal::surface& v) override;
  };
}


#endif //INCLUDE_NEUTRINO_ASSETS_RESOURCES_IMAGE_RESOURCE_HH
