//
// Created by igor on 06/07/2021.
//

#ifndef NEUTRINO_TILED_WORLD_HH
#define NEUTRINO_TILED_WORLD_HH

#include <neutrino/tiled/tile_description.hh>
#include <neutrino/math/rect.hh>
#include <map>

namespace neutrino::tiled {

    class camera;
    class world_renderer;

    class world {
        friend class camera;
        friend class world_renderer;
    public:
        void clear();

        [[nodiscard]] std::size_t add(tiles_layer world_layer);
        [[nodiscard]] std::size_t add(image_layer world_layer);

        [[nodiscard]] std::size_t add(sprite sprite_frames);
        [[nodiscard]] std::size_t bind_sprite_to_layer(std::size_t sprite_id, std::size_t layer_id, std::size_t current_frame, bool active);
        [[nodiscard]] std::size_t bind_sprite_to_layer(std::size_t sprite_id, std::size_t layer_id, std::size_t current_frame);
        [[nodiscard]] std::size_t bind_sprite_to_layer(std::size_t sprite_id, std::size_t layer_id);

        void sprite_state(std::size_t bound_sprite_id, std::size_t current_frame, bool active);
        // frame, num of frames, is active
        [[nodiscard]] std::tuple<std::size_t, std::size_t, bool> sprite_state(std::size_t bound_sprite_id) const;

        [[nodiscard]] math::dimension_t dims_in_pixels(std::size_t layer_id) const;
        [[nodiscard]] math::dimension_t dims_in_tiles(std::size_t layer_id) const;
        [[nodiscard]] math::dimension_t tile_dims(std::size_t layer_id) const;
    private:
        std::vector<layer> m_layers;

        struct sprite_info {
            std::size_t sprite_id;
            std::size_t current;
            bool active;
        };

        std::vector<sprite> m_sprites;
        std::vector<sprite_info> m_bound_sprites;
        std::map<std::size_t, std::vector<std::size_t>> m_sprites_to_layers;
    };


}

#endif
