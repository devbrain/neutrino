//
// Shared camera / plane fixtures for the world-video tests.
//
#pragma once

#include <neutrino/video/world/camera.hh>
#include <neutrino/world/world.hh>

namespace neutrino::test {
    /// @brief A look-at camera at @p target with @p zoom.
    inline camera cam_at(world_point target, float zoom) {
        camera c;
        c.target = target;
        c.zoom = zoom;
        return c;
    }

    /// @brief A layer header with a uniform parallax factor and optional offset.
    inline world_layer_header plane_at(float parallax = 1.0f, world_point offset = {0.0f, 0.0f}) {
        world_layer_header h;
        h.parallax_x = parallax;
        h.parallax_y = parallax;
        h.offset = offset;
        return h;
    }
}
