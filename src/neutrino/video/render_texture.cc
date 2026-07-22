//
// See render_texture.hh.
//

#include <neutrino/video/render_texture.hh>

#include <optional>
#include <utility>

#include <sdlpp/video/blend_mode.hh>
#include <sdlpp/video/pixels.hh>

namespace neutrino {
    std::optional <render_texture> render_texture::create(dim size) {
        if (size.width <= 0 || size.height <= 0) {
            return std::nullopt;
        }
        auto tex = sdlpp::texture::create(get_renderer(),
                                          sdlpp::pixel_format_enum::RGBA8888,
                                          sdlpp::texture_access::target,
                                          size.width, size.height);
        if (!tex) {
            return std::nullopt;
        }
        // Alpha-blend when blitted, so a compose that left transparent regions shows through.
        (void) tex->set_blend_mode(sdlpp::blend_mode::blend);
        return render_texture(std::move(*tex), size);
    }

    void render_texture::blit(const rect& dst) const {
        (void) get_renderer().copy(m_texture, std::optional <rect>{}, std::optional{dst});
    }

    void render_texture::blit() const {
        blit(rect{0, 0, m_size.width, m_size.height});
    }
}
