//
// Created by igor on 30/06/2022.
//

#include <neutrino/assets/image/image_io.hh>
#include <neutrino/utils/io/memory_stream_buf.hh>

#include "tmx_image_loader.hh"

namespace neutrino::assets {
  std::unique_ptr<hal::surface> tmx_image_loader::do_load(const path_resolver_t& resolver,
                                                          const std::string& name,
                                                          const tmx::colori& tint, int offset_x, int offset_y) {
    std::string input = resolver(name);
    utils::io::memory_input_stream is(input.c_str(), static_cast<std::streamsize>(input.size()));
    auto s = assets::load_image (is);
    if (offset_x > 0 || offset_y > 0) {
      auto [w, h] = s.dimensions();
      ENFORCE(offset_x < (int)w && offset_y < (int)h);
      hal::surface s2(w-offset_x, h-offset_y, s.get_pixel_format());
      math::rect src_rect(offset_x, offset_y, (int)(w-offset_x), (int)(h-offset_y));
      math::rect dst_rect(0, 0, (int)(w-offset_x), (int)(h-offset_y));
      if (tint != tmx::colori(255, 255, 255, 255)) {
        s.color_mod (tint.r, tint.b, tint.b);
      }
      s.blit (src_rect, s2, dst_rect);
      return std::make_unique<hal::surface>(std::move(s2));
    }
    if (tint != tmx::colori(255, 255, 255, 255)) {
      s.color_mod (tint.r, tint.b, tint.b);
      auto [w, h] = s.dimensions();
      hal::surface s2(w, h, s.get_pixel_format());
      s.blit (s2);
      return std::make_unique<hal::surface>(std::move(s2));
    }
    return std::make_unique<hal::surface>(std::move(s));
  }
}