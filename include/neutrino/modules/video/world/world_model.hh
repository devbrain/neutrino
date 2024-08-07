//
// Created by igor on 7/5/24.
//

#ifndef  NEUTRINO_MODULES_VIDEO_WORLD_WORLD_MODEL_HH
#define  NEUTRINO_MODULES_VIDEO_WORLD_WORLD_MODEL_HH

#include <map>
#include <memory>
#include <variant>
#include <list>

#include <neutrino/modules/video/world/color_layer.hh>
#include <neutrino/modules/video/world/image_layer.hh>
#include <neutrino/modules/video/world/tiles_layer.hh>
#include <neutrino/modules/video/world/objects_layer.hh>
#include <neutrino/modules/video/world/animation_sequence.hh>
#include <neutrino/neutrino_export.hh>

namespace neutrino::tiled {
    class NEUTRINO_EXPORT world_model {
        public:
            world_model();

            template<typename T>
            world_model& append(T&& layer) {
                m_layers.emplace_back(std::forward <T>(layer));
                return *this;
            }

            template<typename T>
            world_model& prepend(T&& layer) {
                m_layers.emplace_front(std::forward <T>(layer));
                return *this;
            }

            template<typename T>
            world_model& insert(std::size_t pos, T&& layer) {
                auto itr = m_layers.begin();
                for (auto i = 0u; i <= pos && itr != m_layers.end(); i++, itr++);
                m_layers.insert(itr, std::forward <T>(layer));
                return *this;
            }

            void add_animation(const tile& base_tile, const tile& tile_, const std::chrono::milliseconds& duration);
            void add_animation(const animation_sequence& seq);
            [[nodiscard]] const std::map <tile, animation_sequence>& get_animations() const;

            using layer_t = std::variant <color_layer, image_layer, tiles_layer, objects_layer>;
            using container_t = std::list <layer_t>;

            [[nodiscard]] container_t::const_iterator begin() const;
            [[nodiscard]] container_t::const_iterator end() const;

            void set_geometry(unsigned tile_width, unsigned tile_height, unsigned num_tiles_x, unsigned num_tiles_y);

            [[nodiscard]] pixel_coord_t get_world_width() const;
            [[nodiscard]] pixel_coord_t get_world_height() const;
            [[nodiscard]] pixel_coord_t get_tile_width() const;
            [[nodiscard]] pixel_coord_t get_tile_height() const;

        private:
            std::list <layer_t> m_layers;
            std::map <tile, animation_sequence> m_animations;

            pixel_coord_t m_world_width;
            pixel_coord_t m_world_height;
            pixel_coord_t m_tile_width;
            pixel_coord_t m_tile_height;
    };
}

#endif
