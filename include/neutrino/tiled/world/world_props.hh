//
// Created by igor on 21/09/2021.
//

#ifndef INCLUDE_NEUTRINO_TILED_WORLD_WORLD_PROPS_HH
#define INCLUDE_NEUTRINO_TILED_WORLD_WORLD_PROPS_HH

namespace neutrino::tiled {
  /**
   * @brief the orientation of the map.
   */
  enum class orientation_t {
    UNKNOWN,    /**< Unknown orientation */
    ORTHOGONAL, /**< Orthogonal orientation */
    ISOMETRIC,  /**< Isometric orientation */
    STAGGERED,  /**< Staggered orientation */
    HEXAGONAL,  /**< Hexagonal orientation */
  };

  /**
   * @brief Stagger index of the hexagonal map.
   */
  enum class stagger_index_t {
    ODD,
    EVEN,
  };

  /**
   * @brief Stagger axis of the hexagonal map.
   */
  enum class stagger_axis_t {
    X,
    Y,
  };

  /**
   * @brief the render order of the tiles.
   */
  enum class render_order_t {
    RIGHT_DOWN, /**< Right down order */
    RIGHT_UP,   /**< Right up order */
    LEFT_DOWN,  /**< Left down order */
    LEFT_UP,    /**< Left up order */
  };
}

#endif //INCLUDE_NEUTRINO_TILED_WORLD_WORLD_PROPS_HH
