//
// Created by igor on 20/07/2021.
//

#ifndef NEUTRINO_IMAGE_LAYER_HH
#define NEUTRINO_IMAGE_LAYER_HH

#include "layer.hh"
#include "image.hh"
#include "xml.hh"
#include <memory>

namespace neutrino::tiled::tmx {
    /**
   * @brief An image layer is a layer with a single image.
   */
  class image_layer : public layer {
  public:

      static image_layer parse(const xml_node& elt);

    /**
     * @brief ImageLayer constructor
     */
    image_layer(const std::string& name, double opacity, bool visible)
      : layer(name, opacity, visible), m_image(nullptr)
    {
    }

    /**
     * @brief Assign the image for the layer.
     *
     * @param image the image
     */
    void set_image(std::unique_ptr<image> aimage) {
      m_image = std::move(aimage);
    }

    /**
     * @brief Get the image assigned to the layer
     *
     * @returns the image
     */
    [[nodiscard]] const image *get_image() const noexcept {
      return m_image.get();
    }

  private:
    std::unique_ptr<image> m_image;
  };
}

#endif
