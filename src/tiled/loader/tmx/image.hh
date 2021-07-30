//
// Created by igor on 19/07/2021.
//

#ifndef NEUTRINO_IMAGE_HH
#define NEUTRINO_IMAGE_HH

#include <string>
#include <optional>
#include <vector>
#include <neutrino/math/point.hh>
#include "xml.hh"
#include <memory>

namespace neutrino::tiled::tmx {
   /**
   * @brief An image is an image file on the system.
   */
    class image {
    public:

        static std::unique_ptr<image> parse(const xml_node& elt);

        /**
         * @brief Image constructor.
         */
        image(std::string format, std::string source, std::string trans,
              int width, int height)
              : m_format(std::move(format)),
                m_source(std::move(source)),
                m_trans(std::move(trans)),
                m_width(width),
                m_height(height)
        {
        }

        /**
         * @brief Get the format of the file (if provided).
         *
         * @returns the format of the file or empty string.
         */
        [[nodiscard]] std::optional<std::string> format() const noexcept {
            if (m_format.empty()) {
                return std::nullopt;
            }
            return m_format;
        }

        /**
         * @brief Get the path to the image file.
         *
         * @returns the path to the image file
         */
        [[nodiscard]] const std::string& source() const noexcept {
            return m_source;
        }

        /**
         * @brief Get the transparent color (if provided)
         *
         * @returns the transparent color
         */
        [[nodiscard]] std::optional<std::string> transparent() const noexcept {
            if (m_trans.empty()) {
                return std::nullopt;
            }
            return m_trans;
        }

        /**
         * @brief Get the width of the image (if provided)
         *
         * @returns the width of the image
         */
        [[nodiscard]] std::optional<int> width() const noexcept {
            if (m_width > 0)
            {
                return m_width;
            }
            return std::nullopt;
        }

        /**
         * @brief Get the height of the image (if provided)
         *
         * @returns the height of the image
         */
        [[nodiscard]] std::optional<int> height() const noexcept {
            if (m_height > 0)
            {
                return m_height;
            }
            return std::nullopt;
        }

        /**
         * @brief Get the declared size of the image.
         *
         * @returns the size of the image.
         */
        [[nodiscard]] std::optional<math::point2d> size() const noexcept {
            if (m_width > 0 && m_height > 0)
            {
                return math::point2d{m_width, m_height};
            }
            return std::nullopt;
        }

        [[nodiscard]] const std::vector<char>& data() const noexcept {
            return m_data;
        }
    private:
        const std::string m_format;
        const std::string m_source;
        const std::string m_trans;

        const int m_width;
        const int m_height;
        std::vector<char> m_data;
    };

}

#endif //NEUTRINO_IMAGE_HH
