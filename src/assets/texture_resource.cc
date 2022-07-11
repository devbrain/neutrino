//
// Created by igor on 31/05/2022.
//

#include "neutrino/assets/texture_resource.hh"
#include "neutrino/assets/image_resource.hh"
#include "neutrino/utils/exception.hh"

namespace neutrino::assets {
  std::unique_ptr<hal::texture> texture_resource_reader::do_load(hal::renderer& r, const resource_id& image_id) {
    auto image = resource<hal::surface>::get (image_id);
    return std::make_unique<hal::texture>(r, *image.get());
  }
}
