//
// Created by igor on 04/07/2026.
//

#pragma once
#include <sdlpp/utility/geometry.hh>
#include <sdlpp/utility/dimension.hh>

namespace neutrino {
    // Coordinate type aliases
    using point = sdlpp::point<int>;
    using line = sdlpp::line<int>;
    using rect = sdlpp::rect<int>;
    using circle = sdlpp::circle<int>;
    using dim = sdlpp::size<int>;
}