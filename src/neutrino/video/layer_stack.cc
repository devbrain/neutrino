//
// Created by igor on 16/11/2026.
//

#include <atomic>
#include <exception>

#include <failsafe/enforce.hh>
#include <failsafe/logger.hh>

#include "video/layer_stack.hh"

#include "neutrino/video/globals.hh"
#include "services/service_access.hh"
#include "video/composite_draw.hh"
#include "video/texture_allocator.hh"

namespace neutrino::detail {
    namespace {
        // What a layer is cleared to (painter.md §2.8): a mod layer multiplies what is below,
        // so it starts opaque white, which leaves it unchanged; every other layer starts
        // transparent.
        [[nodiscard]] color clear_color_of(const layer_settings& s) noexcept {
            return s.style.blend == blend_mode::mod ? colors::white : colors::transparent;
        }
    }

    std::uint32_t layer_stack::next_serial() noexcept {
        // Scenes, and so their painters, may be built on any thread: a loading thread, say.
        static std::atomic <std::uint32_t> s_counter{0};
        for (;;) {
            const std::uint32_t serial = s_counter.fetch_add(1, std::memory_order_relaxed);
            if (serial != layer_id::INVALID) {
                return serial;
            }
        }
    }

    layer_stack::layer_stack(target_geometry geometry)
        : m_geometry(geometry), m_serial(next_serial()) {
    }

    layer_stack::~layer_stack() {
        // A vector destroys its elements oldest first; the bindings must end newest first, or
        // the target bound before the first push is not the one left bound.
        while (!m_stack.empty()) {
            m_stack.pop_back();
        }
    }

    layer_id layer_stack::declare(const layer_settings& settings) {
        ENFORCE(!m_combining, "declare_layer inside a compositor");
        if (m_layers.empty()) {
            // The base layer (R31): the stack's geometry, every frame, default composite
            // settings.
            m_layers.emplace_back(layer_settings{}, texture_handle{}, false);
            m_ids.push_back({m_serial, 0});
        }
        const auto idx = static_cast <uint32_t>(m_layers.size());
        m_layers.emplace_back(settings, texture_handle{}, false);
        m_ids.push_back({m_serial, idx});
        return {m_serial, idx};
    }

    layer_id layer_stack::base_layer() const {
        ENFORCE(!m_layers.empty(), "base_layer() in a scene that declared no layers");
        return {m_serial, 0};
    }

    void layer_stack::push(layer_id lid) {
        ENFORCE(!m_combining, "push_layer inside a compositor: the layers are finished");
        ENFORCE(is_valid(lid), "layer", lid, "is not one of this stack's");
        push_index(lid.m_index, false);
    }

    void layer_stack::push_index(std::size_t idx, bool frame) {
        auto& lr = m_layers[idx];
        const color clear = clear_color_of(lr.settings);
        // A layer without a size of its own mirrors the stack's geometry, or the bound
        // target's until the stack has one (R42).
        const target_geometry mirrored = m_geometry.size.empty() ? bound_target_geometry() : m_geometry;
        const bool first = !lr.pushed;
        lr.pushed = true;
        // Taken if the layer has none: never pushed, or given back. A layer with a size of its
        // own keeps it, degenerate or not. Its clear colour follows the live blend (mod or
        // not). An every-frame layer is cleared at its first push in the frame. A layer with a
        // size of its own is drawn in its own pixels, without a presentation.
        const dim size = lr.settings.size.value_or(mirrored.size);
        auto binding = bind_pooled(lr.target, {size, clear}, first && lr.settings.redraw == redraw_mode::every_frame,
                                   lr.settings.size ? target_geometry{} : mirrored);
        m_stack.push_back({idx, std::move(binding), frame});
    }

    void layer_stack::pop() {
        ENFORCE(!m_stack.empty() && !m_stack.back().frame, "pop_layer with nothing pushed");
        const auto& top = m_stack.back();
        if (top.target) {
            // A binding of the game's that outlived the layer would be ended by ours and leave
            // the wrong target bound; catch it here instead.
            const auto bound = get_renderer().get_target();
            const auto& own = m_layers[top.layer].target;
            ENFORCE(bound.has_value() && own && *bound == own.get().native().get(),
                    "pop_layer while the bound target is not the layer's: a texture::bind() "
                    "scope outlived it");
        }
        end_newest();
    }

    void layer_stack::end_newest() {
        pushed_entry e = std::move(m_stack.back());
        m_stack.pop_back();
        e.target.reset();                      // the previous target is bound again, with the painter's clip
    }

    void layer_stack::begin_frame(const target_geometry& geometry) {
        ENFORCE(m_stack.empty(), "begin_frame() with a frame still open");
        const bool resized = geometry.size != m_geometry.size;
        const bool presentation_changed = geometry != m_geometry;
        m_geometry = geometry;                 // a new presentation applies at the next bind
        if (m_layers.empty()) {
            return;                            // no layers: drawing goes onto the canvas
        }
        for (auto& lr : m_layers) {
            // The blend is live: a resize, a reset or clear() refills with the colour of the
            // blend the layer has now (painter.md §2.8).
            lr.target.set_clear_color(clear_color_of(lr.settings));
        }
        if (resized) {
            auto& allocator = require_texture_allocator();
            for (auto& lr : m_layers) {
                if (!lr.settings.size) {
                    allocator.resize(lr.target, geometry.size);   // an empty handle is left alone
                }
            }
        } else if (presentation_changed) {
            // Same pixels, another presentation: what an on-demand layer holds was drawn under
            // another mapping of render space. A layer with a size of its own has none.
            for (auto& lr : m_layers) {
                if (!lr.settings.size && lr.settings.redraw == redraw_mode::on_demand) {
                    lr.target.invalidate();
                }
            }
        }
        for (auto& lr : m_layers) {
            lr.pushed = false;
        }
        push_index(0, true);                   // the base layer, cleared: its first push this frame
    }

    void layer_stack::end_frame() {
        if (m_layers.empty()) {
            return;
        }
        ENFORCE(!m_stack.empty() && m_stack.front().frame, "end_frame() without begin_frame()");
        if (m_stack.size() > 1) {
            const layer_id left{m_serial, static_cast <std::uint32_t>(m_stack.back().layer)};
            abort_frame();                     // leave the target from before the frame bound
            ENFORCE(false, "the frame ended with layer", left, "still pushed");
        }
        end_newest();                          // the base layer
        for (auto& lr : m_layers) {
            if (!lr.pushed && lr.settings.redraw == redraw_mode::every_frame && lr.target) {
                lr.target = texture_handle{};  // back to the pool until its next push
            }
        }
    }

    void layer_stack::abort_frame() noexcept {
        while (!m_stack.empty()) {
            try {
                end_newest();
            } catch (const std::exception& e) {
                LOG_ERROR("could not end a layer push while aborting the frame:", e.what());
            } catch (...) {
                LOG_ERROR("could not end a layer push while aborting the frame");
            }
        }
    }

    bool layer_stack::needs_redraw(layer_id lid) const {
        ENFORCE(is_valid(lid), "layer", lid, "is not one of this stack's");
        const auto& lr = m_layers[lid.m_index];
        return lr.settings.redraw == redraw_mode::on_demand && lr.target.needs_redraw();
    }

    void layer_stack::clear_current() {
        ENFORCE(!m_stack.empty(), "clear() with no layer frame open");
        auto& lr = m_layers[m_stack.back().layer];
        lr.target.set_clear_color(clear_color_of(lr.settings));
        lr.target.clear();                     // nothing for a failed texture
    }

    void layer_stack::release_every_frame() {
        ENFORCE(m_stack.empty(), "release_every_frame() with a frame open");
        for (auto& lr : m_layers) {
            if (lr.settings.redraw == redraw_mode::every_frame && lr.target) {
                lr.target = texture_handle{};  // back to the pool until its next push
            }
        }
    }

    const texture* layer_stack::target(layer_id lid) const {
        ENFORCE(is_valid(lid), "layer", lid, "is not one of this stack's");
        const auto& lr = m_layers[lid.m_index];
        return lr.target ? &lr.target.get() : nullptr;
    }

    std::span <const layer_id> layer_stack::layers() const noexcept {
        return m_ids;
    }

    layer_style& layer_stack::style(layer_id lid) {
        ENFORCE(is_valid(lid), "layer", lid, "is not one of this stack's");
        return m_layers[lid.m_index].settings.style;
    }

    const layer_style& layer_stack::style(layer_id lid) const {
        ENFORCE(is_valid(lid), "layer", lid, "is not one of this stack's");
        return m_layers[lid.m_index].settings.style;
    }

    bool layer_stack::has_content(layer_id lid) const {
        ENFORCE(is_valid(lid), "layer", lid, "is not one of this stack's");
        return has_content_index(lid.m_index);
    }

    bool layer_stack::has_content_index(std::size_t idx) const {
        const auto& lr = m_layers[idx];
        if (!lr.target) {
            return false;                      // no texture, or one that could not be made
        }
        return lr.pushed || (lr.settings.redraw == redraw_mode::on_demand && !lr.target.needs_redraw());
    }

    void layer_stack::draw_layer(layer_id lid, const layer_style& settings) {
        ENFORCE(is_valid(lid), "layer", lid, "is not one of this stack's");
        ENFORCE(m_combining, "draw_layer outside a compositor");
        draw_index(lid.m_index, settings);
    }

    void layer_stack::draw_layers(layer_id first, layer_id last) {
        ENFORCE(is_valid(first), "layer", first, "is not one of this stack's");
        ENFORCE(is_valid(last), "layer", last, "is not one of this stack's");
        ENFORCE(first.m_index <= last.m_index, "draw_layers: layer", first, "comes after", last);
        ENFORCE(m_combining, "draw_layers outside a compositor");
        for (std::size_t idx = first.m_index; idx <= last.m_index; ++idx) {
            draw_index(idx, m_layers[idx].settings.style);
        }
    }

    void layer_stack::draw_index(std::size_t idx, const layer_style& settings) {
        if (!has_content_index(idx)) {
            return;
        }
        auto& lr = m_layers[idx];
        const pixel_space px(bound_mapping()); // the canvas's own pixels (R43)
        draw_composited(lr.target.get(), settings,
                        lr.settings.size ? in_place::render_area : in_place::mirrored, px);
    }

    texture* layer_stack::base_texture() {
        if (m_layers.empty() || !m_layers.front().target) {
            return nullptr;
        }
        return &m_layers.front().target.get();
    }

    std::optional <renderer_target_scope> layer_stack::bind_base() {
        ENFORCE(m_combining, "bind_base outside combining");
        texture* t = base_texture();
        if (!t) {
            return std::nullopt;
        }
        auto scope = t->bind();
        apply_presentation(m_geometry);        // the base layer mirrors the target (R42)
        return scope;
    }

    void layer_stack::draw_declared() {
        ENFORCE(m_combining, "draw_declared outside combining");
        draw_from(1);
    }

    void layer_stack::draw_all() {
        ENFORCE(m_combining, "draw_all outside combining");
        draw_from(0);
    }

    void layer_stack::draw_from(std::size_t first) {
        // Nothing else draws between these layers, so one pixel_space serves them all: the
        // presentation and clip are turned off and back once, not once per layer.
        std::optional <pixel_space> px;
        for (std::size_t idx = first; idx < m_layers.size(); ++idx) {
            if (!has_content_index(idx)) {
                continue;
            }
            if (!px) {
                px.emplace(bound_mapping());
            }
            auto& lr = m_layers[idx];
            draw_composited(lr.target.get(), lr.settings.style,
                            lr.settings.size ? in_place::render_area : in_place::mirrored, *px);
        }
    }

    const pixel_mapping& layer_stack::bound_mapping() {
        // Read once per target: every layer is drawn onto the same canvas, unless a compositor
        // binds a texture of its own meanwhile.
        const auto bound = get_renderer().get_target();
        const void* now = bound ? static_cast <const void*>(*bound) : nullptr;
        if (!m_mapping || m_mapping_target != now) {
            m_mapping = pixel_mapping::of_bound_target();
            m_mapping_target = now;
        }
        return *m_mapping;
    }

    void layer_stack::begin_combine() {
        ENFORCE(m_stack.empty(), "combining with a layer frame still open");
        ENFORCE(!m_combining, "combining while already combining");
        m_combining = true;
        m_mapping.reset();
    }

    void layer_stack::end_combine() noexcept {
        m_combining = false;
        m_mapping.reset();
    }

    bool layer_stack::combining() const noexcept {
        return m_combining;
    }

    void layer_stack::mark_drawn() {
        for (auto& lr : m_layers) {
            if (lr.pushed) {
                lr.target.mark_drawn();        // does nothing without a texture
            }
        }
    }

    bool layer_stack::is_valid(const layer_id& lid) const noexcept {
        return lid && lid.m_stack == m_serial && lid.m_index < m_layers.size();
    }
}
