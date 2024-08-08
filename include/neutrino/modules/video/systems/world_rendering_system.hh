//
// Created by igor on 8/8/24.
//

#ifndef NEUTRINO_MODULES_VIDEO_SYSTEMS_WORLD_RENDERING_SYSTEM_HH
#define NEUTRINO_MODULES_VIDEO_SYSTEMS_WORLD_RENDERING_SYSTEM_HH

#include <neutrino/modules/video/world_renderer.hh>
#include <neutrino/modules/video/tile.hh>
#include <neutrino/modules/video/world/animation_sequence.hh>
#include <neutrino/ecs/abstract_system.hh>


namespace neutrino::ecs {
    class NEUTRINO_EXPORT world_rendering_system : public abstract_system {
        public:
            explicit world_rendering_system(const world_renderer& world_renderer_);
        protected:
            [[nodiscard]] const texture_atlas& get_atlas() const;
            [[nodiscard]] sdl::renderer& get_renderer() const;
            [[nodiscard]] sdl::rect get_world_viewport() const;
            [[nodiscard]] sdl::rect get_screen_viewport() const;

            void draw_tile(const tile& tid, int x, int y) const;
            void draw_tile(const tile& tid, const sdl::point& p) const;

            void draw_tile(const tiled::animation_sequence& seq, unsigned frame, int x, int y) const;
            void draw_tile(const tiled::animation_sequence& seq, unsigned frame, const sdl::point& p) const;
        private:
            const world_renderer& m_world_renderer;
    };
}

#endif
