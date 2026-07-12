//
// Created by igor on 12/07/2026.
//

#include <neutrino/video/sprite/sprite_def.hh>

#include <bit>
#include <cstddef>
#include <span>
#include <string>
#include <string_view>

#include <neutrino/detail/hash.hh>

namespace neutrino {
    point origin_for(sprite_origin_rule rule, dim size) noexcept {
        const int w = size.width;
        const int h = size.height;
        switch (rule) {
        case sprite_origin_rule::top_left:      return point{0,     0};
        case sprite_origin_rule::top_center:    return point{w / 2, 0};
        case sprite_origin_rule::top_right:     return point{w,     0};
        case sprite_origin_rule::center_left:   return point{0,     h / 2};
        case sprite_origin_rule::center:        return point{w / 2, h / 2};
        case sprite_origin_rule::center_right:  return point{w,     h / 2};
        case sprite_origin_rule::bottom_left:   return point{0,     h};
        case sprite_origin_rule::bottom_center: return point{w / 2, h};
        case sprite_origin_rule::bottom_right:  return point{w,     h};
        }
        return point{0, 0};
    }

    std::vector <sprite_visual_def> expand_grid(const sprite_grid& grid, dim image_size) {
        std::vector <sprite_visual_def> out;
        if (grid.cell_w == 0 || grid.cell_h == 0) {
            return out;
        }

        // Derive columns/count from the image the way a uniform tileset does: net of
        // margin, one stride per cell (+spacing).
        const unsigned iw = static_cast <unsigned>(image_size.width);
        const unsigned ih = static_cast <unsigned>(image_size.height);
        const unsigned two_margin = grid.margin * 2;

        unsigned columns = grid.columns;
        if (columns == 0 && iw > two_margin) {
            columns = (iw - two_margin + grid.spacing) / (grid.cell_w + grid.spacing);
        }
        unsigned count = grid.count;
        if (count == 0 && columns > 0 && ih > two_margin) {
            const unsigned rows = (ih - two_margin + grid.spacing) / (grid.cell_h + grid.spacing);
            count = columns * rows;
        }
        if (columns == 0 || count == 0) {
            return out;
        }

        const point origin = origin_for(grid.origin,
                                        dim{static_cast <int>(grid.cell_w), static_cast <int>(grid.cell_h)});
        out.reserve(count);
        for (unsigned i = 0; i < count; ++i) {
            const unsigned col = i % columns;
            const unsigned row = i / columns;
            sprite_visual_def v;
            v.name = std::to_string(i);
            v.src = rect{
                static_cast <int>(grid.margin + col * (grid.cell_w + grid.spacing)),
                static_cast <int>(grid.margin + row * (grid.cell_h + grid.spacing)),
                static_cast <int>(grid.cell_w),
                static_cast <int>(grid.cell_h)
            };
            v.origin = origin; // untrimmed cells: pivot from the rule, no trim
            out.push_back(std::move(v));
        }
        return out;
    }

    point baked_visual_origin(const sprite_visual_def& v) noexcept {
        const point t = v.trim_offset.value_or(point{0, 0});
        return point{v.origin.x - t.x, v.origin.y - t.y};
    }

    content_key key_for(const sprite_def& def, image_identifier& ident) {
        std::uint64_t digest = 0;
        std::uint64_t length = 0;

        const auto fold_u = [&] (std::uint64_t v) { details::hash_combine64(digest, v); };
        const auto fold_f = [&] (float v) {
            details::hash_combine64(digest, std::bit_cast <std::uint32_t>(v));
        };
        const auto fold_s = [&] (std::string_view s) {
            const content_key h = content_hash(std::as_bytes(std::span <const char>(s.data(), s.size())));
            details::hash_combine64(digest, h.hash);
            details::hash_combine64(digest, h.length);
        };

        fold_image_identity(digest, length, ident, def.image);

        fold_u(def.grid.has_value() ? 1u : 0u);
        if (def.grid) {
            const sprite_grid& g = *def.grid;
            fold_u(g.cell_w);
            fold_u(g.cell_h);
            fold_u(g.columns);
            fold_u(g.count);
            fold_u(g.margin);
            fold_u(g.spacing);
            fold_u(static_cast <std::uint64_t>(g.origin));
        }

        // Visuals and clips fold in DECLARED order (sequences, not the resolved name map),
        // so the key never depends on map iteration order.
        for (const sprite_visual_def& v : def.visuals) {
            fold_s(v.name);
            fold_u(static_cast <std::uint32_t>(v.src.x));
            fold_u(static_cast <std::uint32_t>(v.src.y));
            fold_u(static_cast <std::uint32_t>(v.src.w));
            fold_u(static_cast <std::uint32_t>(v.src.h));
            fold_u(static_cast <std::uint32_t>(v.origin.x));
            fold_u(static_cast <std::uint32_t>(v.origin.y));
            fold_u(v.source_size.has_value() ? 1u : 0u);
            if (v.source_size) {
                fold_u(static_cast <std::uint32_t>(v.source_size->width));
                fold_u(static_cast <std::uint32_t>(v.source_size->height));
            }
            fold_u(v.trim_offset.has_value() ? 1u : 0u);
            if (v.trim_offset) {
                fold_u(static_cast <std::uint32_t>(v.trim_offset->x));
                fold_u(static_cast <std::uint32_t>(v.trim_offset->y));
            }
        }

        for (const sprite_clip_def& c : def.clips) {
            fold_s(c.name);
            fold_u(c.loop ? 1u : 0u);
            fold_u(c.frames.size());
            for (const sprite_frame_def& f : c.frames) {
                fold_s(f.visual);
                fold_f(f.duration.count());
                fold_u(static_cast <std::uint64_t>(f.flip));
            }
        }

        return content_key{digest, length};
    }
}
