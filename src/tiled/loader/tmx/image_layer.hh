//
// Created by igor on 20/07/2021.
//

#ifndef NEUTRINO_IMAGE_LAYER_HH
#define NEUTRINO_IMAGE_LAYER_HH

#include "layer.hh"
#include "image.hh"
#include "reader.hh"
#include "group.hh"
#include <memory>

namespace neutrino::tiled::tmx
{
    /**
   * @brief An image layer is a layer with a single image.
   */
    class image_layer : public layer
    {
    public:

        static image_layer parse(const reader& elt, const group* parent = nullptr);

        image_layer(const std::string& name, double opacity, bool visible, int id,
                    int offsetx, int offsety, colori tint)
                : layer(name, opacity, visible, id),
                m_offsetx(offsetx),
                m_offsety(offsety),
                m_tint(tint),
                m_image(nullptr)
        {
        }

        void set_image(std::unique_ptr<image> aimage)
        {
            m_image = std::move(aimage);
        }

        [[nodiscard]] const image* get_image() const noexcept
        {
            return m_image.get();
        }

        [[nodiscard]] int offset_x() const noexcept {
            return m_offsetx;
        }

        [[nodiscard]] int offset_y() const noexcept {
            return m_offsety;
        }

        [[nodiscard]] colori tint() const noexcept {
            return m_tint;
        }
    private:
        int m_offsetx;
        int m_offsety;
        colori m_tint;
        std::unique_ptr<image> m_image;
    };
}

#endif
