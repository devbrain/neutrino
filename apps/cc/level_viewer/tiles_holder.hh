//
// Created by igor on 9/18/24.
//

#ifndef  TILES_HOLDER_HH
#define  TILES_HOLDER_HH

#include <vector>
#include <sdlpp/sdlpp.hh>
#include <neutrino/imgui/imgui.h>

class tiles_holder {
    public:
        tiles_holder() = default;
        explicit tiles_holder(neutrino::sdl::texture&& tex,
                              std::vector <neutrino::sdl::rect>&& rects,
                              unsigned tile_w, unsigned tile_h);
        tiles_holder& operator =(tiles_holder&& other) noexcept = default;

        [[nodiscard]] const neutrino::sdl::texture& get_texture() const;
        [[nodiscard]] neutrino::sdl::rect get_texture_rect(int tile_id) const;
        [[nodiscard]] ImVec2 get_uv(int tile_id) const;
        [[nodiscard]] std::size_t size() const;
        [[nodiscard]] ImVec2 get_uv_dims() const;

    private:
        neutrino::sdl::texture m_tiles_texture;
        neutrino::sdl::area_type m_img_area;
        std::vector <neutrino::sdl::rect> m_rects;
        std::vector <ImVec2> m_tiles_uv;
        ImVec2 m_tile_size_uv;
};

#endif
