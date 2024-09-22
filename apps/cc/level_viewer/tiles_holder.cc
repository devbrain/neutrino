//
// Created by igor on 9/18/24.
//

#include "tiles_holder.hh"
#include <bsw/exception.hh>

tiles_holder::tiles_holder(neutrino::sdl::texture&& tex,
                           std::vector <neutrino::sdl::rect>&& rects,
                           unsigned tile_w, unsigned tile_h)
    : m_tiles_texture(std::move(tex)),
      m_rects(std::move(rects)) {
    m_img_area = m_tiles_texture.get_dimensions();

    for (const auto& r : m_rects) {
        m_tiles_uv.emplace_back(static_cast <float>(r.x) / static_cast <float>(m_img_area.w),
                                static_cast <float>(r.y) / static_cast <float>(m_img_area.h));
    }
    m_tile_size_uv = ImVec2(static_cast <float>(tile_w) / static_cast <float>(m_img_area.w),
                            static_cast <float>(tile_h) / static_cast <float>(m_img_area.h));
}

const neutrino::sdl::texture& tiles_holder::get_texture() const {
    return m_tiles_texture;
}

neutrino::sdl::rect tiles_holder::get_texture_rect(int tile_id) const {
    ENFORCE(tile_id < m_rects.size());
    return m_rects[tile_id];
}

ImVec2 tiles_holder::get_uv(int tile_id) const {
    ENFORCE(tile_id < m_tiles_uv.size());
    return m_tiles_uv[tile_id];
}

std::size_t tiles_holder::size() const {
    return m_rects.size();
}

ImVec2 tiles_holder::get_uv_dims() const {
    return m_tile_size_uv;
}
