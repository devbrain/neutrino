//
// Created by igor on 06/07/2021.
//

#ifndef NEUTRINO_TILE_SHEET_IMPL_HH
#define NEUTRINO_TILE_SHEET_IMPL_HH

#include <vector>
#include <neutrino/hal/video/surface.hh>
#include <neutrino/math/rect.hh>


namespace neutrino::tiled::detail {
    struct tile_sheet_impl {
        tile_sheet_impl(hal::surface tex, const std::vector<math::rect>* coords)
        : surface(std::move(tex)), is_image(coords != nullptr) {
            if (coords) {
                rects = *coords;
            } else {
                int w, h;
                std::tie(std::ignore, std::ignore, w, h) = surface.pixels_data();
                rects.emplace_back(0,0, w, h);
            }
        }
        hal::surface surface;
        std::vector<math::rect> rects;
        bool is_image;
    };
}

#endif
