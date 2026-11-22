//
// Created by igor on 22/11/2026.
//

#include <neutrino/video/screenshot.hh>

#include <utility>

#include <failsafe/enforce.hh>

#include <neutrino/video/globals.hh>
#include <neutrino/video/texture.hh>

#include "scene/scene_frame.hh"
#include "scene/scenes_manager.hh"
#include "services/service_access.hh"
#include "services/service_locator.hh"
#include "video/target_geometry.hh"

namespace neutrino {
    image screenshot() {
        ENFORCE(maybe_application() != nullptr, "screenshot() needs a running application");
        ENFORCE(!detail::scene_frame::rendering(),
                "screenshot() inside a scene's render() or compositor: frames do not nest");
        ENFORCE(!get_renderer().has_texture_target(),
                "screenshot() while a texture is bound: call it with the window as the target");

        // The window's size and its mapping of render space: the scenes land on the same
        // pixels as on the screen (R42).
        const detail::target_geometry window = detail::bound_target_geometry();
        auto shot = texture::create(window.size, texture_access::target);
        ENFORCE(shot.has_value(), "screenshot(): a texture of the window's size could not be made");
        {
            auto into_shot = shot->bind(colors::black);
            detail::apply_presentation(window);
            service_locator::instance().get_scenes_manager().render();
            // Off for the read: under a presentation a readback sees only the render area.
            detail::apply_presentation(detail::target_geometry{window.size});
        }
        image img = shot->to_image();

        // A window has no alpha: where a scene copied with none left a transparent pixel, the
        // screen shows black, so does the shot. Through a format without alpha and back: two
        // conversions, not a loop over the pixels.
        const auto format = img.format();
        if (auto opaque = img.convert(sdlpp::pixel_format_enum::RGB888)) { // sdlpp-ok: internal
            if (auto back = opaque->convert(format)) {
                img = std::move(*back);
            }
        }
        return img;
    }
}
