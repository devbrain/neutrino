//
// Created by igor on 31/05/2022.
//

#include <neutrino/assets/resources/image_resource.hh>
#include <neutrino/assets/image/image_io.hh>

namespace neutrino::assets {
  image_resource::image_resource(const resource_id& id, resource_processing_context* ctx)
  : resource<hal::surface, std::istream&> (id, ctx) {
  }

  std::unique_ptr<hal::surface> image_resource::do_load(std::istream& is) {
    return std::make_unique<hal::surface>(load_image (is));
  }

  void image_resource::do_save(std::ostream& os, const hal::surface& data) {
    save_png (os, data);
  }
}