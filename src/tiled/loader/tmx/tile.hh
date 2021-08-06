//
// Created by igor on 20/07/2021.
//

#ifndef NEUTRINO_TILE_HH
#define NEUTRINO_TILE_HH

#include "component.hh"
#include "image.hh"
#include "object_layer.hh"
#include "animation.hh"
#include "reader.hh"

#include <memory>
#include <array>


namespace neutrino::tiled::tmx {
    /**
  * @brief A tile is a rectangular part of a tileset.
  */
    class tile : public component {
    public:
        static constexpr auto INVALID = static_cast<unsigned>(-1);
    public:
        static tile parse(const reader& e);
        /**
         * @brief Tile constructor.
         */
        tile(unsigned id, const std::array<unsigned, 4>& terrain, unsigned probability)
                : m_id(id), m_terrain(terrain), m_probability(probability), m_image(nullptr)
        {
        }

        /**
         * @brief Get the local id of the tile.
         *
         * @return the local id of the tile
         */
        [[nodiscard]] unsigned id() const noexcept {
            return m_id;
        }

        /**
         * @brief Get the terrains on the four corners.
         *
         * @returns the terrains
         */
        [[nodiscard]] const std::array<unsigned, 4>& terrain() const noexcept {
            return m_terrain;
        }

        /**
         * @brief Get the terrain in the top left corner.
         *
         * @return the terrain in the top left corner
         */
        [[nodiscard]] unsigned top_left_terrain() const noexcept {
            return m_terrain[0];
        }

        /**
         * @brief Get the terrain in the top right corner.
         *
         * @return the terrain in the top right corner
         */
        [[nodiscard]] unsigned top_right_terrain() const noexcept {
            return m_terrain[1];
        }

        /**
         * @brief Get the terrain in the bottom left corner.
         *
         * @return the terrain in the bottom left corner
         */
        [[nodiscard]] unsigned bottom_left_terrain() const noexcept {
            return m_terrain[2];
        }

        /**
         * @brief Get the terrain in the bottom right corner.
         *
         * @return the terrain in the bottom right corner
         */
        [[nodiscard]] unsigned bottom_right_terrain() const noexcept {
            return m_terrain[3];
        }

        /**
         * @brief Get the probability of this tile.
         *
         * @return the probability of this tile
         */
        [[nodiscard]] unsigned probability() const noexcept {
            return m_probability;
        }

        /**
         * @brief Get the image of this tile.
         *
         * @param image the image of this tile
         */
        void set_image(std::unique_ptr<image> aimage) {
            m_image = std::move(aimage);
        }

        /**
         * @brief Tell whether the tile has an image.
         *
         * @returns true if the tile has an image (generally false)
         */
        [[nodiscard]] bool has_image() const noexcept {
            return m_image != nullptr;
        }

        /**
         * @brief Get the image of this tile.
         *
         * @returns the image of this tile
         */
        [[nodiscard]] const image *get_image() const noexcept {
            return m_image.get();
        }

        [[nodiscard]] const animation& get_animation() const noexcept {
            return m_animation;
        }

        [[nodiscard]] const object_layer* objects() const noexcept {
            if (m_objects) {
                return m_objects.get();
            }
            return nullptr;
        }
    private:
        const unsigned m_id;
        const std::array<unsigned, 4> m_terrain;
        const unsigned m_probability;
        animation m_animation;
        std::unique_ptr<image> m_image;
        std::unique_ptr<object_layer> m_objects;
    };

}

#endif
