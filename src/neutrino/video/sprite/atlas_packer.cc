//
// Created by igor on 07/07/2026.
//

#include <algorithm>
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <failsafe/enforce.hh>
#include <neutrino/video/sprite/atlas_packer.hh>
#include "services/service_locator.hh"

#define STBRP_STATIC
#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"

namespace neutrino {
    namespace {
        // Per-page pixel cap when the caller doesn't specify one: the active
        // renderer's max texture size, or a safe default with no renderer.
        int default_page_cap() {
            if (const auto s = service_locator::instance().get_max_texture_size()) {
                return s;
            }
            return 2048;
        }
    }

    pack_result pack_atlas(std::span <const rect> rects, int max_width, int max_height, int margin) {
        ENFORCE(max_width > 0 && max_height > 0 && margin >= 0);
        std::vector <stbrp_rect> work;
        work.reserve(rects.size());
        int id = 0;
        for (const auto& r : rects) {
            ENFORCE(r.w > 0 && r.h > 0);
            // Inflate by the gutter belt in 64-bit so an oversize tile or a large
            // margin is rejected here rather than overflowing the int stbrp_rect
            // fields. iw/ih <= max_* (positive ints) makes the narrowing safe.
            const std::int64_t iw = static_cast <std::int64_t>(r.w) + 2LL * margin;
            const std::int64_t ih = static_cast <std::int64_t>(r.h) + 2LL * margin;
            ENFORCE(iw <= max_width && ih <= max_height);
            work.emplace_back(id++, static_cast <int>(iw), static_cast <int>(ih), 0, 0, 0);
        }

        std::vector <stbrp_node> nodes(max_width);
        stbrp_context ctx;
        pack_result out;

        for (unsigned page = 0; !work.empty(); ++page) {
            stbrp_init_target(&ctx, max_width, max_height, nodes.data(), static_cast <int>(nodes.size()));
            stbrp_pack_rects(&ctx, work.data(), static_cast <int>(work.size()));

            std::vector <stbrp_rect> failed;
            int used_w = 0, used_h = 0;
            std::size_t start = out.placements.size();
            for (const auto& r : work) {
                if (r.was_packed) {
                    out.placements.push_back({
                        page, static_cast <uint32_t>(r.id),
                        rect{r.x + margin, r.y + margin, r.w - 2 * margin, r.h - 2 * margin}
                    });
                    used_w = std::max(used_w, r.x + r.w); // +r.w includes the gutter belt
                    used_h = std::max(used_h, r.y + r.h);
                } else {
                    failed.push_back(r);
                }
            }
            std::sort(out.placements.begin() + start, out.placements.end(),
                      [](const auto& a, const auto& b) { return a.input_index < b.input_index; });
            out.pages.emplace_back(used_w, used_h); // trimmed extent, not the cap
            work = std::move(failed);
        }
        return out;
    }

    pack_result pack_atlas(std::span <const rect> rects, int margin) {
        const int cap = default_page_cap();
        return pack_atlas(rects, cap, cap, margin);
    }

    namespace {
        // Blit a sub-rect and fail loudly: a silently-dropped blit would leave
        // wrong pixels in an otherwise "successful" atlas.
        void enforced_blit(const sdlpp::surface& src, sdlpp::surface& dst,
                           const rect& src_rect, point at) {
            ENFORCE(src.blit_to <rect>(dst, src_rect, at).has_value());
        }

        // Extrude the border of the tile — sub-rect @p s of @p src — outward into
        // its gutter belt around destination origin @p at, via strip blits (no
        // pixel locking, so this sidesteps the known sdlpp::lock_guard bug), so
        // linear sampling past a tile edge reads the tile's own colour rather than
        // a neighbour or transparent black.
        void extrude_border(const sdlpp::surface& src, const rect& s,
                            sdlpp::surface& page, const rect& at, int margin) {
            const int sx = s.x, sy = s.y, w = s.w, h = s.h;
            const int x = at.x, y = at.y;
            for (int m = 1; m <= margin; ++m) {
                enforced_blit(src, page, rect{sx, sy, 1, h}, point{x - m, y});
                enforced_blit(src, page, rect{sx + w - 1, sy, 1, h}, point{x + w + m - 1, y});
                enforced_blit(src, page, rect{sx, sy, w, 1}, point{x, y - m});
                enforced_blit(src, page, rect{sx, sy + h - 1, w, 1}, point{x, y + h + m - 1});
            }
            for (int my = 1; my <= margin; ++my) {
                for (int mx = 1; mx <= margin; ++mx) {
                    enforced_blit(src, page, rect{sx, sy, 1, 1}, point{x - mx, y - my});
                    enforced_blit(src, page, rect{sx + w - 1, sy, 1, 1}, point{x + w + mx - 1, y - my});
                    enforced_blit(src, page, rect{sx, sy + h - 1, 1, 1}, point{x - mx, y + h + my - 1});
                    enforced_blit(src, page, rect{sx + w - 1, sy + h - 1, 1, 1},
                                  point{x + w + mx - 1, y + h + my - 1});
                }
            }
        }
    }

    surface_atlases pack_regions(std::span <const pack_region> regions,
                                 sdlpp::pixel_format_enum format,
                                 int max_width,
                                 int max_height,
                                 int margin,
                                 std::optional <cpu_texture_atlas_mask_options> generate) {
        std::vector <rect> dims;
        dims.reserve(regions.size());
        for (const auto& r : regions) {
            dims.push_back(rect{0, 0, r.src.w, r.src.h});
        }

        auto rc = pack_atlas(dims, max_width, max_height, margin);

        std::vector <sdlpp::surface> pages;
        std::vector <std::vector <cpu_texture_atlas_frame>> frames(rc.pages.size());
        pages.reserve(rc.pages.size());
        for (const auto& sz : rc.pages) {
            // NB: ENFORCE the has_value(), not the expected itself — ENFORCE
            // unwraps and MOVES the value out of an expected, which would leave
            // the surface null while has_value() stays true.
            auto page = sdlpp::surface::create_rgb(sz.width, sz.height, format);
            ENFORCE(page.has_value());
            ENFORCE(page->fill(sdlpp::color{0, 0, 0, 0}).has_value()); // transparent gaps
            pages.push_back(std::move(*page));
        }

        // Convert each distinct source surface to the target format once, with
        // blend none for a straight copy. Many regions may share one source
        // (e.g. every frame of a tilesheet), so caching avoids re-converting it.
        std::unordered_map <const sdlpp::surface*, sdlpp::surface> converted;
        auto converted_source = [&](const sdlpp::surface* s) -> sdlpp::surface& {
            auto it = converted.find(s);
            if (it == converted.end()) {
                auto c = s->convert(format);
                ENFORCE(c.has_value());
                ENFORCE(c->set_blend_mode(sdlpp::blend_mode::none).has_value());
                it = converted.emplace(s, std::move(*c)).first;
            }
            return it->second;
        };

        for (const auto& p : rc.placements) {
            const auto& region = regions[p.input_index];
            auto& src = converted_source(region.surface);
            enforced_blit(src, pages[p.page], region.src, point{p.bounds.x, p.bounds.y});
            extrude_border(src, region.src, pages[p.page], p.bounds, margin);
            // Preserve an input mask; else generate one from the source pixels if a
            // policy was given — evaluated against the ORIGINAL (pre-conversion)
            // surface so colour-key / INDEX8 transparency semantics still hold.
            if (region.mask) {
                frames[p.page].emplace_back(p.bounds, *region.mask);
            } else if (generate) {
                auto m = cpu_texture_atlas_frame::evaluate_bitmask(
                    *region.surface, region.src, *generate);
                if (m) {
                    frames[p.page].emplace_back(p.bounds, *m);
                } else {
                    frames[p.page].emplace_back(p.bounds);
                }
            } else {
                frames[p.page].emplace_back(p.bounds);
            }
        }

        surface_atlases out;
        out.pages.reserve(pages.size());
        for (std::size_t i = 0; i < pages.size(); ++i) {
            out.pages.emplace_back(std::move(pages[i]), std::move(frames[i]));
        }
        out.placements = std::move(rc.placements);
        return out;
    }

    surface_atlases pack_surfaces(std::span <const sdlpp::surface> images,
                                  sdlpp::pixel_format_enum format,
                                  int max_width,
                                  int max_height,
                                  int margin,
                                  std::optional <cpu_texture_atlas_mask_options> generate) {
        std::vector <pack_region> regions;
        regions.reserve(images.size());
        for (const auto& s : images) {
            regions.push_back(pack_region{
                &s, rect{0, 0, static_cast <int>(s.width()), static_cast <int>(s.height())}, nullptr});
        }
        return pack_regions(regions, format, max_width, max_height, margin, generate);
    }

    surface_atlases pack_atlases(std::span <const cpu_texture_atlas> atlases,
                                 sdlpp::pixel_format_enum format,
                                 int max_width,
                                 int max_height,
                                 int margin,
                                 std::optional <cpu_texture_atlas_mask_options> generate) {
        std::vector <pack_region> regions;
        for (const auto& a : atlases) {
            for (std::size_t f = 0; f < a.frame_count(); ++f) {
                const auto& fr = a.frame(f);
                regions.push_back(pack_region{
                    &a.surface(), fr.atlas_rect, fr.mask ? &*fr.mask : nullptr});
            }
        }
        return pack_regions(regions, format, max_width, max_height, margin, generate);
    }

    // ------------------------------------------------------------------
    // Convenience overloads: derive the pixel format from the first input
    // and the per-page cap from the renderer (default_page_cap()).
    // ------------------------------------------------------------------

    surface_atlases pack_regions(std::span <const pack_region> regions, int margin,
                                 std::optional <cpu_texture_atlas_mask_options> generate) {
        if (regions.empty()) {
            return {};
        }
        const int cap = default_page_cap();
        return pack_regions(regions, regions.front().surface->format(), cap, cap, margin, generate);
    }

    surface_atlases pack_regions(std::span <const pack_region> regions,
                                 sdlpp::pixel_format_enum format, int margin,
                                 std::optional <cpu_texture_atlas_mask_options> generate) {
        if (regions.empty()) {
            return {};
        }
        const int cap = default_page_cap();
        return pack_regions(regions, format, cap, cap, margin, generate);
    }

    surface_atlases pack_surfaces(std::span <const sdlpp::surface> images, int margin,
                                  std::optional <cpu_texture_atlas_mask_options> generate) {
        if (images.empty()) {
            return {};
        }
        const int cap = default_page_cap();
        return pack_surfaces(images, images.front().format(), cap, cap, margin, generate);
    }

    surface_atlases pack_surfaces(std::span<const sdlpp::surface> images, sdlpp::pixel_format_enum format, int margin,
        std::optional<cpu_texture_atlas_mask_options> generate) {
        if (images.empty()) {
            return {};
        }
        const int cap = default_page_cap();
        return pack_surfaces(images, format, cap, cap, margin, generate);
    }

    surface_atlases pack_atlases(std::span <const cpu_texture_atlas> atlases, int margin,
                                 std::optional <cpu_texture_atlas_mask_options> generate) {
        if (atlases.empty()) {
            return {};
        }
        const int cap = default_page_cap();
        return pack_atlases(atlases, atlases.front().surface().format(), cap, cap, margin, generate);
    }

    surface_atlases pack_atlases(std::span<const cpu_texture_atlas> atlases, sdlpp::pixel_format_enum format,
        int margin, std::optional<cpu_texture_atlas_mask_options> generate) {
        if (atlases.empty()) {
            return {};
        }
        const int cap = default_page_cap();
        return pack_atlases(atlases, format, cap, cap, margin, generate);
    }
}
