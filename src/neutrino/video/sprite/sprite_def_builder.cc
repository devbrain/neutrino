//
// Created by igor on 29/09/2026.
//

#include <neutrino/video/sprite/sprite_def_builder.hh>

#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>

#include <failsafe/enforce.hh>
#include <sdlpp/video/surface.hh>

#include "utils/json.hh"

namespace neutrino {
    // ============================================================================
    // sprite_clip_builder
    // ============================================================================

    sprite_clip_builder::sprite_clip_builder(sprite_def_builder* parent, std::string name, bool loop)
        : m_parent(parent) {
        m_clip.name = std::move(name);
        m_clip.loop = loop;
    }

    sprite_clip_builder& sprite_clip_builder::frame(
        std::string visual,
        sprite_animation_duration duration,
        sprite_flip flip) {
        m_clip.frames.push_back(sprite_frame_def{std::move(visual), duration, flip});
        return *this;
    }

    sprite_clip_builder& sprite_clip_builder::frame(
        std::size_t visual_index,
        sprite_animation_duration duration,
        sprite_flip flip) {
        return frame(std::to_string(visual_index), duration, flip);
    }

    sprite_clip_builder& sprite_clip_builder::loop(bool enable) noexcept {
        m_clip.loop = enable;
        return *this;
    }

    sprite_def_builder& sprite_clip_builder::end_clip() {
        ENFORCE(m_parent != nullptr)("sprite_clip_builder has null parent");
        m_parent->commit_clip(std::move(m_clip));
        return *m_parent;
    }

    sprite_def sprite_clip_builder::build() {
        end_clip();
        return std::move(*m_parent).build();
    }

    // ============================================================================
    // sprite_def_builder
    // ============================================================================

    sprite_def_builder& sprite_def_builder::from_file(std::filesystem::path path, unsigned width, unsigned height) {
        m_def.image.source = image_from_disk{std::move(path)};
        if (width > 0)  m_def.image.width = width;
        if (height > 0) m_def.image.height = height;
        return *this;
    }

    sprite_def_builder& sprite_def_builder::from_memory(std::vector <std::uint8_t> bytes, unsigned width, unsigned height) {
        m_def.image.source = image_from_memory{std::move(bytes)};
        if (width > 0)  m_def.image.width = width;
        if (height > 0) m_def.image.height = height;
        return *this;
    }

    sprite_def_builder& sprite_def_builder::from_memory(std::span <const std::uint8_t> bytes, unsigned width, unsigned height) {
        m_def.image.source = image_from_memory{std::vector <std::uint8_t>(bytes.begin(), bytes.end())};
        if (width > 0)  m_def.image.width = width;
        if (height > 0) m_def.image.height = height;
        return *this;
    }

    sprite_def_builder& sprite_def_builder::from_memory(std::string_view bytes, unsigned width, unsigned height) {
        return from_memory(
            std::span <const std::uint8_t>(reinterpret_cast <const std::uint8_t*>(bytes.data()), bytes.size()),
            width, height);
    }

    sprite_def_builder& sprite_def_builder::from_stream(std::istream& is, unsigned width, unsigned height) {
        std::vector <std::uint8_t> buffer;
        if (is.good()) {
            const auto cur = is.tellg();
            if (cur != -1) {
                is.seekg(0, std::ios::end);
                const auto end = is.tellg();
                if (end >= cur) {
                    buffer.reserve(static_cast <std::size_t>(end - cur));
                }
                is.seekg(cur, std::ios::beg);
            }
            char chunk[4096];
            while (is.read(chunk, sizeof(chunk))) {
                const auto count = is.gcount();
                buffer.insert(buffer.end(),
                              reinterpret_cast <const std::uint8_t*>(chunk),
                              reinterpret_cast <const std::uint8_t*>(chunk) + count);
            }
            const auto remaining = is.gcount();
            if (remaining > 0) {
                buffer.insert(buffer.end(),
                              reinterpret_cast <const std::uint8_t*>(chunk),
                              reinterpret_cast <const std::uint8_t*>(chunk) + remaining);
            }
        }
        return from_memory(std::move(buffer), width, height);
    }

    sprite_def_builder& sprite_def_builder::from_surface(
        std::shared_ptr <const sdlpp::surface> surface,
        std::optional <std::uint64_t> identity) {
        if (surface && surface->get()) {
            if (m_def.image.width == 0) {
                m_def.image.width = static_cast <unsigned>(surface->get()->w);
            }
            if (m_def.image.height == 0) {
                m_def.image.height = static_cast <unsigned>(surface->get()->h);
            }
        }
        m_def.image.source = image_from_surface{std::move(surface), identity};
        return *this;
    }

    sprite_def_builder& sprite_def_builder::from_surface(
        sdlpp::surface surface,
        std::optional <std::uint64_t> identity) {
        return from_surface(std::make_shared <const sdlpp::surface>(std::move(surface)), identity);
    }

    sprite_def_builder& sprite_def_builder::image_source(world_image_source source, unsigned width, unsigned height) {
        m_def.image.source = std::move(source);
        if (width > 0)  m_def.image.width = width;
        if (height > 0) m_def.image.height = height;
        return *this;
    }

    sprite_def_builder& sprite_def_builder::image(world_image img) {
        m_def.image = std::move(img);
        return *this;
    }

    sprite_def_builder& sprite_def_builder::image_size(unsigned width, unsigned height) noexcept {
        m_def.image.width = width;
        m_def.image.height = height;
        return *this;
    }

    sprite_def_builder& sprite_def_builder::image_size(dim size) noexcept {
        m_def.image.width = static_cast <unsigned>(std::max(0, size.width));
        m_def.image.height = static_cast <unsigned>(std::max(0, size.height));
        return *this;
    }

    sprite_def_builder& sprite_def_builder::transparent_color(sdlpp::color color) noexcept {
        m_def.image.transparent_color = color;
        return *this;
    }

    sprite_def_builder& sprite_def_builder::with_grid(sprite_grid grid) {
        m_def.grid = grid;
        return *this;
    }

    sprite_def_builder& sprite_def_builder::with_grid(
        unsigned cell_w,
        unsigned cell_h,
        sprite_origin_rule origin,
        unsigned margin,
        unsigned spacing,
        unsigned columns,
        unsigned count) {
        m_def.grid = sprite_grid{
            .cell_w = cell_w,
            .cell_h = cell_h,
            .columns = columns,
            .count = count,
            .margin = margin,
            .spacing = spacing,
            .origin = origin
        };
        return *this;
    }

    sprite_def_builder& sprite_def_builder::clear_grid() noexcept {
        m_def.grid.reset();
        return *this;
    }

    sprite_def_builder& sprite_def_builder::add_visual(std::string name, rect src, point origin) {
        m_def.visuals.push_back(sprite_visual_def{
            .name = std::move(name),
            .src = src,
            .origin = origin,
            .source_size = std::nullopt,
            .trim_offset = std::nullopt
        });
        return *this;
    }

    sprite_def_builder& sprite_def_builder::add_visual(std::string name, rect src, sprite_origin_rule origin_rule) {
        return add_visual(std::move(name), src, origin_for(origin_rule, dim{src.w, src.h}));
    }

    sprite_def_builder& sprite_def_builder::add_trimmed_visual(
        std::string name,
        rect packed_src,
        point pivot_in_untrimmed,
        dim source_size,
        point trim_offset) {
        m_def.visuals.push_back(sprite_visual_def{
            .name = std::move(name),
            .src = packed_src,
            .origin = pivot_in_untrimmed,
            .source_size = source_size,
            .trim_offset = trim_offset
        });
        return *this;
    }

    sprite_def_builder& sprite_def_builder::add_visual(sprite_visual_def visual) {
        m_def.visuals.push_back(std::move(visual));
        return *this;
    }

    sprite_def_builder& sprite_def_builder::add_visuals(std::vector <sprite_visual_def> visuals) {
        m_def.visuals.reserve(m_def.visuals.size() + visuals.size());
        for (auto& v : visuals) {
            m_def.visuals.push_back(std::move(v));
        }
        return *this;
    }

    sprite_clip_builder sprite_def_builder::clip(std::string name, bool loop) {
        return sprite_clip_builder(this, std::move(name), loop);
    }

    void sprite_def_builder::commit_clip(sprite_clip_def clip) {
        m_def.clips.push_back(std::move(clip));
    }

    sprite_def_builder& sprite_def_builder::add_clip(sprite_clip_def clip) {
        commit_clip(std::move(clip));
        return *this;
    }

    sprite_def_builder& sprite_def_builder::add_clip(
        std::string name,
        std::initializer_list <std::string_view> visuals,
        sprite_animation_duration frame_duration,
        bool loop,
        sprite_flip flip) {
        sprite_clip_def c;
        c.name = std::move(name);
        c.loop = loop;
        c.frames.reserve(visuals.size());
        for (std::string_view v : visuals) {
            c.frames.push_back(sprite_frame_def{std::string(v), frame_duration, flip});
        }
        return add_clip(std::move(c));
    }

    sprite_def_builder& sprite_def_builder::add_clip(
        std::string name,
        std::initializer_list <std::size_t> visual_indices,
        sprite_animation_duration frame_duration,
        bool loop,
        sprite_flip flip) {
        sprite_clip_def c;
        c.name = std::move(name);
        c.loop = loop;
        c.frames.reserve(visual_indices.size());
        for (std::size_t idx : visual_indices) {
            c.frames.push_back(sprite_frame_def{std::to_string(idx), frame_duration, flip});
        }
        return add_clip(std::move(c));
    }

    sprite_def_builder& sprite_def_builder::add_clip_range(
        std::string name,
        std::size_t first_index,
        std::size_t count,
        sprite_animation_duration frame_duration,
        bool loop,
        sprite_flip flip) {
        sprite_clip_def c;
        c.name = std::move(name);
        c.loop = loop;
        c.frames.reserve(count);
        for (std::size_t i = 0; i < count; ++i) {
            c.frames.push_back(sprite_frame_def{std::to_string(first_index + i), frame_duration, flip});
        }
        return add_clip(std::move(c));
    }

    sprite_def_builder& sprite_def_builder::load_aseprite_metadata(std::string_view text) {
        using utils::json;
        const json doc = json::parse(text);

        const json& meta = doc.at("meta");
        if (m_def.image.empty() && meta.contains("image")) {
            m_def.image.source = image_from_disk{meta.value("image", std::string{})};
        }
        if (meta.contains("size")) {
            m_def.image.width = meta.at("size").value("w", 0u);
            m_def.image.height = meta.at("size").value("h", 0u);
        }

        const json& frames = doc.at("frames");
        m_def.visuals.reserve(m_def.visuals.size() + frames.size());
        for (std::size_t i = 0; i < frames.size(); ++i) {
            const json& f = frames[i];
            sprite_visual_def v;
            v.name = std::to_string(i);
            const auto& fr = f.at("frame");
            v.src = rect{fr.at("x").get <int>(), fr.at("y").get <int>(),
                         fr.at("w").get <int>(), fr.at("h").get <int>()};
            v.origin = point{0, 0};
            if (f.value("trimmed", false)) {
                const auto& sss = f.at("spriteSourceSize");
                v.trim_offset = point{sss.at("x").get <int>(), sss.at("y").get <int>()};
                const auto& sz = f.at("sourceSize");
                v.source_size = dim{sz.at("w").get <int>(), sz.at("h").get <int>()};
            }
            m_def.visuals.push_back(std::move(v));
        }

        if (meta.contains("frameTags")) {
            for (const json& tag : meta.at("frameTags")) {
                sprite_clip_def clip_def;
                clip_def.name = tag.value("name", std::string{});
                clip_def.loop = true;
                const int from = tag.value("from", 0);
                const int to = tag.value("to", 0);
                const std::string dir = tag.value("direction", std::string{"forward"});

                const auto add_frame = [&] (int idx) {
                    if (idx < 0 || static_cast <std::size_t>(idx) >= frames.size()) {
                        return;
                    }
                    const int dur = frames[static_cast <std::size_t>(idx)].value("duration", 100);
                    clip_def.frames.push_back(sprite_frame_def{
                        std::to_string(idx),
                        sprite_animation_duration{static_cast <float>(dur)},
                        sprite_flip::none});
                };

                if (dir == "reverse") {
                    for (int i = to; i >= from; --i) {
                        add_frame(i);
                    }
                } else {
                    for (int i = from; i <= to; ++i) {
                        add_frame(i);
                    }
                }
                m_def.clips.push_back(std::move(clip_def));
            }
        }

        return *this;
    }

    sprite_def_builder sprite_def_builder::from_aseprite_json(std::string_view json_text) {
        sprite_def_builder b;
        b.load_aseprite_metadata(json_text);
        return b;
    }

    sprite_def_builder sprite_def_builder::from_aseprite_json(
        std::string_view json_text,
        std::vector <std::uint8_t> image_bytes) {
        sprite_def_builder b;
        b.from_memory(std::move(image_bytes));
        b.load_aseprite_metadata(json_text);
        return b;
    }

    sprite_def_builder sprite_def_builder::from_aseprite_json(
        std::string_view json_text,
        std::span <const std::uint8_t> image_bytes) {
        sprite_def_builder b;
        b.from_memory(image_bytes);
        b.load_aseprite_metadata(json_text);
        return b;
    }

    sprite_def_builder sprite_def_builder::from_aseprite_json(
        std::string_view json_text,
        const sprite_resource_resolver& resolver) {
        sprite_def_builder b;
        b.load_aseprite_metadata(json_text);

        if (resolver) {
            using utils::json;
            const json doc = json::parse(json_text);
            if (doc.contains("meta") && doc["meta"].contains("image")) {
                const std::string img_name = doc["meta"]["image"].get <std::string>();
                std::vector <std::uint8_t> bytes = resolver(img_name);
                ENFORCE(!bytes.empty())("sprite_def_builder: resolver returned empty data for image '", img_name, "'");
                b.from_memory(std::move(bytes));
            }
        }

        return b;
    }

    sprite_def_builder& sprite_def_builder::reset() noexcept {
        m_def = sprite_def{};
        return *this;
    }

    void sprite_def_builder::validate() const {
        ENFORCE(!m_def.image.empty())("sprite_def_builder: image source must be specified before build()");
        for (const auto& c : m_def.clips) {
            ENFORCE(!c.name.empty())("sprite_def_builder: clip name cannot be empty");
            ENFORCE(!c.frames.empty())("sprite_def_builder: clip '", c.name, "' has no frames");
            for (const auto& f : c.frames) {
                ENFORCE(!f.visual.empty())("sprite_def_builder: clip '", c.name, "' contains a frame with empty visual name");
                ENFORCE(f.duration > sprite_animation_duration::zero())
                    ("sprite_def_builder: clip '", c.name, "' frame duration must be positive");
            }
        }
    }

    sprite_def sprite_def_builder::build() const & {
        validate();
        return m_def;
    }

    sprite_def sprite_def_builder::build() && {
        validate();
        return std::move(m_def);
    }
}
