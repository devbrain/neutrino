//
// Created by igor on 07/07/2021.
//

#include <neutrino/tiled/camera.hh>
#include <neutrino/tiled/world.hh>

namespace neutrino::tiled {
    camera::camera(const world& w) : m_cameras (w.m_layers.size()) {
        for (std::size_t layer_id=0; layer_id < w.m_layers.size(); layer_id++) {
            auto dims = w.dims_in_pixels(layer_id);
            m_cameras[layer_id] = {0,0, dims[0], dims[1]};
        }
    }

    void camera::bind_camera_to_layer(std::size_t layer_id, int x, int y, int w, int h) {
        m_cameras[layer_id] = {x,y, w,h};
    }

    void camera::move_x(std::size_t layer_id, int dx) {
        m_cameras[layer_id].point[0] += dx;
    }
    void camera::move_y(std::size_t layer_id, int dy) {
        m_cameras[layer_id].point[1] += dy;
    }

    void camera::move_x(int dx) {
        for (auto& r : m_cameras) {
            r.point[0] += dx;
        }
    }
    void camera::move_y(int dy) {
        for (auto& r : m_cameras) {
            r.point[1] += dy;
        }
    }

    void camera::clear() {
        m_cameras.clear();
    }
}