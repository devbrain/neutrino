//
// Created by igor on 31/05/2022.
//

#include <neutrino/assets/resources/texture_resource.hh>
#include <neutrino/assets/resources/image_resource.hh>
#include <neutrino/utils/exception.hh>

namespace neutrino::assets {
  texture_resource::texture_resource(const resource_id& id, resource_processing_context* ctx)
  : resource<hal::texture, hal::renderer&, const resource_id&> (id, ctx) {

  }

  std::unique_ptr<hal::texture> texture_resource::do_load(hal::renderer& r, const resource_id& image_id) {
    auto image = image_resource::get (image_id);
    if (!image) {
      RAISE_EX("Can not find image resource ", image_id);
    }
    return std::make_unique<hal::texture>(r, *image);
  }
}