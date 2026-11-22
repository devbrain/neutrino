//
// Created by igor on 21/11/2026.
//

#include "video/canvas.hh"

#include <exception>

#include <failsafe/logger.hh>

#include <neutrino/video/globals.hh>

#include "services/service_access.hh"
#include "video/composite_draw.hh"
#include "video/draw_failures.hh"
#include "video/texture_allocator.hh"

namespace neutrino::detail {
    void canvas::set_blend(blend_mode mode) noexcept {
        m_blend = mode;
    }

    blend_mode canvas::blend() const noexcept {
        return m_blend;
    }

    void canvas::set_redraw(redraw_mode mode) noexcept {
        m_redraw = mode;
    }

    redraw_mode canvas::redraw() const noexcept {
        return m_redraw;
    }

    void canvas::begin_frame(const target_geometry& geometry) {
        if (!m_target.empty() && m_target.size() != geometry.size) {
            // A failed texture reports the size it asked for, so it is retried only at
            // another size, as a layer's is.
            require_texture_allocator().resize(m_target, geometry.size);
        } else if (m_redraw == redraw_mode::on_demand && m_begun && geometry != m_geometry) {
            // Same pixels, another presentation: what it holds was drawn under another
            // mapping of render space. An every-frame canvas is cleared anyway.
            m_target.invalidate();
        }
        m_geometry = geometry;                 // a new presentation applies at the next bind
        m_begun = true;
        m_bound = false;
        m_drawn = false;
        m_shared = nullptr;                    // shared for one frame at a time
    }

    std::optional <renderer_target_scope> canvas::bind() {
        if (!m_begun) {
            return std::nullopt;               // no geometry to take a texture at
        }
        // Taken at the first bind, or the first since release(). A canvas is cleared to
        // transparent, whatever its blend (painter.md §2.8); an every-frame one at its first
        // bind in the frame.
        auto scope = bind_pooled(m_target, {m_geometry.size, colors::transparent},
                                 !m_bound && m_redraw == redraw_mode::every_frame, m_geometry);
        if (scope) {
            m_bound = true;
        }
        return scope;
    }

    void canvas::note_drawn() noexcept {
        m_drawn = true;
    }

    void canvas::share(texture& t) noexcept {
        release();                             // its own texture is not needed this frame
        m_shared = &t;
        m_drawn = true;
    }

    void canvas::end_frame() {
        if (m_drawn && !m_shared) {
            m_target.mark_drawn();             // does nothing without a texture
        }
    }

    void canvas::clear() {
        m_target.clear();                      // nothing without a texture
    }

    void canvas::release() noexcept {
        m_target = texture_handle{};
        m_shared = nullptr;
    }

    void canvas::release_every_frame() noexcept {
        if (m_redraw == redraw_mode::every_frame) {
            release();
        }
    }

    bool canvas::drawn() const noexcept {
        return m_drawn;
    }

    bool canvas::needs_redraw() const {
        return m_redraw == redraw_mode::on_demand && m_target.needs_redraw();
    }

    const texture* canvas::get() const {
        if (m_shared) {
            return m_shared;
        }
        return m_target ? &m_target.get() : nullptr;
    }

    dim canvas::size() const {
        return m_shared ? m_shared->size() : m_target.size();
    }

    bool canvas::has_content() const noexcept {
        if (m_shared) {
            return true;
        }
        return m_target && (m_redraw == redraw_mode::on_demand || m_drawn);
    }

    bool canvas::shared() const noexcept {
        return m_shared != nullptr;
    }

    void canvas::draw_into_target(const pixel_space& px) {
        if (!has_content()) {
            return;
        }
        // The canvas mirrors the target: in place it lands 1:1 (R42, R43).
        draw_composited(m_shared ? *m_shared : m_target.get(), layer_style{.blend = m_blend},
                        in_place::mirrored, px);
    }

    void draw_canvases(std::span <canvas* const> canvases) {
        // Each canvas mirrors the target, so it lands 1:1 on its pixels.
        const pixel_space px;
        for (canvas* c : canvases) {
            if (!c) {
                continue;
            }
            // A copy that fails is skipped and counted, so it does not fail the frame for every
            // scene.
            try {
                c->draw_into_target(px);
            } catch (const std::exception& e) {
                LOG_ERROR("a canvas cannot be drawn:", e.what());
                count_draw_failure();
            }
        }
    }
}
