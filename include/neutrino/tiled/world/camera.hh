//
// Created by igor on 07/07/2021.
//

#ifndef NEUTRINO_TILED_WORLD_CAMERA_HH
#define NEUTRINO_TILED_WORLD_CAMERA_HH

#include <vector>
#include <neutrino/math/rect.hh>

namespace neutrino::tiled {
    class world;
    class world_renderer;

    class camera {
        friend class world_renderer;
    public:
        explicit camera(const world& w);

        void bind_camera_to_layer(std::size_t layer_id, int x, int y, int w, int h);

        void move_x(std::size_t layer_id, int dx);
        void move_y(std::size_t layer_id, int dy);

        void move_x(int dx);
        void move_y(int dy);

        void clear();
    private:
        std::vector<math::rect> m_cameras;
    };
}

#endif

