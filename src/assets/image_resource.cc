//
// Created by igor on 31/05/2022.
//

#include "neutrino/assets/image_resource.hh"
#include "neutrino/assets/image/image_io.hh"

namespace neutrino::assets {
  std::unique_ptr<hal::surface> image_resource_reader::do_load(std::istream& is) {
    return std::make_unique<hal::surface>(load_image (is));
  }

  void image_resource_writer::do_save(std::ostream& os, const hal::surface& data) {
    save_png (os, data);
  }
}