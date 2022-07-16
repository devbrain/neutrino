//
// Created by igor on 13/07/2022.
//

#include <neutrino/assets/tiles/world/tilesheet_resource.hh>
#include <neutrino/utils/override.hh>
#include <neutrino/utils/exception.hh>
#include <utility>

namespace neutrino::assets {

  tilesheet_resource::tilesheet_resource (hal::surface s)
  : surface(std::move(s)) {

  }

  tilesheet_resource::tilesheet_resource (hal::surface s, tilesheet_rects rects)
  : surface(std::move (s)), rectangles(std::move (rects)) {

  }

  static tilesheet_rects eval_dimension_properties (const assets::lazy_tilesheet& lazy_ts) {
    const auto& td = std::get<1>(lazy_ts);
    if (const auto* ti = std::get_if<assets::lazy_tilesheet_info> (&td)) {
      return get_tilesheet_coords (ti->image_width(), ti->image_height(), ti->info());
    }
    else {
      return *std::get_if<assets::tilesheet_rects> (&td);
    }
  }

  static tilesheet_rects eval_dimension_properties (const assets::tilesheet& ts) {
    const auto& [s, td] = ts;
    if (const auto* ti = std::get_if<assets::tilesheet_info> (&td)) {
      return get_tilesheet_coords (s, *ti);
    }
    else {
      return *std::get_if<assets::tilesheet_rects> (&td);
    }
  }

  static hal::surface load_surface(const lazy_image_loader& loader) {
    auto img = loader.load();
    auto expected = loader.dimensions();
    if (expected) {
      auto [w, h] = img.dimensions ();
      math::dimension2di_t actual{(int) w, (int) h};
      ENFORCE(*expected == actual);
    }
    return img;
  }

  std::unique_ptr<tilesheet_resource> tilesheet_resource_reader::do_load (tilesheet_loader_input input) {
    return std::visit(neutrino::utils::overload(
          [](hal::surface s) {
            return std::make_unique<tilesheet_resource>(std::move(s));
          },
          [](tilesheet tl) {
            return std::make_unique<tilesheet_resource>(std::move(std::get<0>(tl)),
                                                        eval_dimension_properties (tl));
          },
          [](const lazy_tilesheet& ldr) {
            return std::make_unique<tilesheet_resource>(load_surface (std::get<0>(ldr)),
                                                        eval_dimension_properties (ldr));
          },
          [](const lazy_image_loader& ldr) {
            return std::make_unique<tilesheet_resource>(load_surface (ldr));
          }
        ), std::move(input));
  }

}