//
// Created by igor on 07/07/2021.
//

#ifndef NEUTRINO_TILED_WORLD_WORLD_RENDERER_HH
#define NEUTRINO_TILED_WORLD_WORLD_RENDERER_HH

#include <neutrino/math/rect.hh>


namespace neutrino {
    namespace hal {
        class renderer;
    }

    namespace tiled {
        class tile_sheet_manager;
        class world;
        class camera;
        class tiles_layer;
        class image_layer;

        class world_renderer {
        public:
            world_renderer(hal::renderer& renderer, tile_sheet_manager& manager);
            void draw(const world& w, const camera& c) const;
        private:
            void draw(const tiles_layer& layer, const math::rect& view_port) const;
            void draw(const image_layer& layer, const math::rect& view_port) const;
            void draw_tile(const tiles_layer& layer, int tile_x, int tile_y, const math::rect& src_rect, const math::rect& dst_rect) const;
        private:
            hal::renderer& m_renderer;
            tile_sheet_manager& m_tiles_manager;
            math::rect m_screen;
        };
    }
}

#endif

