//
// Created by igor on 20/11/2026.
//

#include <failsafe/enforce.hh>

#include <neutrino/video/globals.hh>
#include <neutrino/video/painter_state.hh>

#include "scene_frame.hh"
#include "scene_context_access.hh"
#include "video/canvas.hh"
#include "video/composite_draw.hh"
#include "video/layer_stack.hh"
#include "video/painter_context.hh"
#include "video/target_geometry.hh"

namespace neutrino::detail {
    render_shortcuts& shortcuts() noexcept {
        static render_shortcuts s;
        return s;
    }

    bool is_default_compositor(const compositor& c) noexcept {
        const auto* f = c.target <void (*)(composition&)>();
        return f && *f == &default_compositor;
    }

    scene_frame::scene_frame(base_scene* scene, std::span <const texture* const> below, frame_target onto)
        : m_layers(scene_context_access::layer_stack_of(scene)),
          m_canvas(scene_context_access::canvas_of(scene)),
          m_compositor(scene_context_access::compositor_of(scene)),
          m_below(below) {
        ENFORCE(!rendering(), "a scene is already being rendered");
        // The scene's painter state for render() and its compositor (R48), before anything is
        // bound: the canvas or the base layer takes its clip.
        if (scene != nullptr) {
            m_painter.emplace(*scene_context_access::painter_state_stack_of(scene));
        }
        // The target bound now is the one the scene manager draws into: the canvas and the
        // layers mirror it (R42).
        const target_geometry geometry = bound_target_geometry();
        if (onto == frame_target::bound_target) {
            ENFORCE(!m_layers, "a scene with layers is drawn onto its canvas");
            if (m_canvas) {
                // Not used this frame: an every-frame canvas gives its texture back, and shows
                // nothing (has_content() is false), so it is neither copied nor below().
                m_canvas->release_every_frame();
                m_canvas->begin_frame(geometry);
                m_canvas = nullptr;
            }
        } else if (m_canvas) {
            m_canvas->begin_frame(geometry);
        }
        if (m_layers) {
            m_layers->begin_frame(geometry);   // resize, reset the flags, push the base layer
        } else if (m_canvas) {
            m_canvas_binding = m_canvas->bind(); // render() draws onto the canvas
            if (!m_canvas_binding && onto != frame_target::canvas_or_bound_target) {
                // Its drawing would land under the scenes below it, copied into the target
                // later, or be overwritten by them: it does nothing instead (painter.md §3.6).
                m_discard.emplace();
            }
        }
        // With neither, render() draws into the target bound now.
        // Only now: if anything above throws, the destructor does not run, and nothing is set.
        painter_context::instance().frame = this;
    }

    void scene_frame::finish() {
        // First: if end_frame() fails, it has already ended every push.
        m_finished = true;
        if (m_layers) {
            m_layers->end_frame();             // fails if a layer is left pushed
            combine();
        } else if (m_canvas) {
            // Nothing tells the canvas a draw landed on it, so a render() that returned
            // counts as having drawn.
            m_canvas->note_drawn();
        }
        if (m_canvas) {
            m_canvas->end_frame();
        }
        m_canvas_binding.reset();              // the target from before the frame again
        m_discard.reset();
        if (m_painter) {
            m_painter->finish("the frame");    // a push left is undone, then fails
        }
    }

    void scene_frame::combine() {
        // Pushing and declaring fail until it ends, however the compositor ends.
        struct combining {
            layer_stack& layers;
            explicit combining(layer_stack& l) : layers(l) { layers.begin_combine(); }
            ~combining() { layers.end_combine(); }
            combining(const combining&) = delete;
            combining& operator=(const combining&) = delete;
        } const guard{*m_layers};

        // The base layer's texture as the canvas (R41): drawing it onto a cleared canvas would
        // copy it exactly, so the declared layers go straight onto it instead.
        texture* base = m_layers->base_texture();
        const bool share = shortcuts().base_layer_as_canvas && base && m_canvas
                           && m_canvas->redraw() == redraw_mode::every_frame && m_compositor
                           && is_default_compositor(*m_compositor)
                           && is_identity_style(m_layers->style(m_layers->base_layer()));
        if (share) {
            if (auto binding = m_layers->bind_base()) {
                m_layers->draw_declared();     // the default compositor, the base layer aside
                m_canvas->share(*base);
            }
        } else if (m_canvas && m_compositor && *m_compositor) {
            // Cleared unless redrawn on demand. Without a texture the compositor is skipped:
            // it would draw into the target below, before the canvases under this one.
            if (auto binding = m_canvas->bind()) {
                // The default painter state, not what render() left (R43). It ends before the
                // binding, while the canvas is still bound.
                const auto state = scoped_state(painter_state{});
                painter_state_stack& painter = painter_state_stack::instance();
                const std::size_t inside = painter.depth();
                if (is_default_compositor(*m_compositor)) {
                    m_layers->draw_all();      // what it does, with the presentation off once
                } else {
                    composition c(*m_layers, m_below);
                    (*m_compositor)(c);
                }
                painter.end_call(inside, "the compositor");
                m_canvas->note_drawn();
            }
        }
        // After the compositor: needs_redraw() keeps one answer for the whole frame.
        m_layers->mark_drawn();
    }

    scene_frame::~scene_frame() {
        if (!m_finished && m_layers) {
            m_layers->abort_frame();           // render() threw: end every push
        }
        m_canvas_binding.reset();              // never throws
        m_discard.reset();
        if (painter_context* c = painter_context::maybe()) {
            c->frame = nullptr;
        }
        // Then m_painter ends: the pushes render() left if it threw are undone, and the
        // painter state current before the frame is back. Never throws.
    }

    void scene_frame::clear_current() {
        ENFORCE(rendering(), "clear() outside a scene's render()");
        scene_frame& f = *painter_context::instance().frame;
        if (f.m_layers) {
            f.m_layers->clear_current();
        } else if (f.m_canvas_binding) {
            f.m_canvas->clear();
        } else if (f.m_discard) {
            // The canvas could not be made: nothing to clear.
        } else {
            // Drawing straight into the target (R44), or the canvas could not be made: the
            // scene's picture is the target, cleared as the window is, to black.
            auto& r = get_renderer();
            const sdlpp::renderer::draw_color_guard black(r, colors::black); // sdlpp-ok: internal
            (void) r.clear();
        }
    }

    bool scene_frame::draws_into_target() const {
        if (m_layers) {
            return false;                      // into its layers; a failed base layer discards
        }
        return !m_canvas_binding && !m_discard;
    }

    bool scene_frame::rendering() noexcept {
        const painter_context* c = painter_context::maybe();
        return c != nullptr && c->frame != nullptr;
    }

    layer_stack* scene_frame::get_current_layers_stack() noexcept {
        const painter_context* c = painter_context::maybe();
        return c != nullptr && c->frame != nullptr ? c->frame->m_layers : nullptr;
    }
}
