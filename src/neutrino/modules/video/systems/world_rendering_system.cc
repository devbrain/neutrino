//
// Created by igor on 8/8/24.
//

#include <neutrino/modules/video/systems/world_rendering_system.hh>

namespace neutrino::ecs {
    world_rendering_system::world_rendering_system(const world_renderer& world_renderer_)
        : m_world_renderer(world_renderer_) {
    }

    const texture_atlas& world_rendering_system::get_atlas() const {
        return m_world_renderer.get_atlas();
    }

    sdl::renderer& world_rendering_system::get_renderer() const {
        return m_world_renderer.get_renderer();
    }

    sdl::rect world_rendering_system::get_world_viewport() const {
        return {m_world_renderer.get_camera(), m_world_renderer.get_dimension()};
    }

    sdl::rect world_rendering_system::get_screen_viewport() const {
        return {{0, 0}, m_world_renderer.get_dimension()};
    }

    void world_rendering_system::draw_tile(const tile& tid, int x, int y) const {
        auto [text_ptr, rect] = get_atlas().get(tid);
        if (text_ptr) {
            const sdl::rect dst_rect(x, y, rect.w, rect.h);
            get_renderer().copy(*text_ptr, rect, dst_rect);
        }
    }

    void world_rendering_system::draw_tile(const tile& tid, const sdl::point& p) const {
        draw_tile(tid, p.x, p.y);
    }

    void world_rendering_system::draw_tile(const tiled::animation_sequence& seq, unsigned frame, int x, int y) const {
        ENFORCE(frame < seq.get_frames().size());
        draw_tile(seq.get_frames()[frame].m_tile, x, y);
    }

    void world_rendering_system::draw_tile(const tiled::animation_sequence& seq, unsigned frame,
                                           const sdl::point& p) const {
        draw_tile(seq, frame, p.x, p.y);
    }
}
