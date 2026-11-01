//
// Created by igor on 29/10/2026.
//

#include "neutrino/video/atlas/image_atlas.hh"

#include <neutrino/video/atlas/image_atlas.hh>

#include <algorithm>
#include <cstring>
#include <utility>

#include <failsafe/enforce.hh>
#include <failsafe/exception.hh>
#include <neutrino/video/color.hh>

#include "shelf_packer.hh"
#include "services/service_locator.hh"

namespace neutrino {
    namespace details {
        template<>
        struct dimensions_access <image_bank::entry> {
            static dim load_dim(const image_bank::entry& e) {
                return e.pixels.dimensions <dim>();
            }
        };
    }

    namespace {
        // Default page cap: the renderer's max texture size, or 2048 before a renderer exists.
        [[nodiscard]] int default_page_cap() {
            if (const auto s = service_locator::instance().get_max_texture_size()) {
                return s;
            }
            return 2048;
        }

        [[nodiscard]] bool is_empty(const image& img) {
            return !img || img.width() == 0 || img.height() == 0;
        }

        // Same palette: the same (refcounted) palette, or equal colours.
        [[nodiscard]] bool same_palette(const image& a, const image& b) {
            const auto pa = a.get_palette();
            const auto pb = b.get_palette();
            if (pa.get() == pb.get()) {
                return true;
            }
            if (!pa || !pb) {
                return false;
            }
            return pa.to_vector() == pb.to_vector();
        }

        // The bank can be packed as indexed pages: every non-empty picture is INDEX8 with
        // the same palette and the same colour key (or none). Returns the first such picture.
        [[nodiscard]] const image* indexed_source(const image_bank& bank) {
            const image* first = nullptr;
            for (const auto& e : bank.entries) {
                if (is_empty(e.pixels)) {
                    continue;
                }
                if (e.pixels.format() != pixel_format::INDEX8) {
                    return nullptr;
                }
                if (!first) {
                    first = &e.pixels;
                    continue;
                }
                if (!same_palette(*first, e.pixels) ||
                    first->color_key() != e.pixels.color_key()) {
                    return nullptr;
                }
            }
            return first;
        }

        // Blit a sub-rect and fail loudly: a silently dropped blit would leave wrong pixels
        // in an otherwise "successful" atlas.
        void enforced_blit(const image& src, image& dst, const rect& src_rect, point at) {
            ENFORCE(src.blit_to <rect>(dst, src_rect, at).has_value());
        }

        // Extrude the picture's border outward into its gutter, so sampling past an edge
        // reads the picture's own colour rather than a neighbour or the page background.
        void extrude_border(const image& src, image& page, const rect& at, int gutter) {
            const int w = at.w, h = at.h, x = at.x, y = at.y;
            for (int m = 1; m <= gutter; ++m) {
                enforced_blit(src, page, rect{0, 0, 1, h}, point{x - m, y});
                enforced_blit(src, page, rect{w - 1, 0, 1, h}, point{x + w + m - 1, y});
                enforced_blit(src, page, rect{0, 0, w, 1}, point{x, y - m});
                enforced_blit(src, page, rect{0, h - 1, w, 1}, point{x, y + h + m - 1});
            }
            for (int my = 1; my <= gutter; ++my) {
                for (int mx = 1; mx <= gutter; ++mx) {
                    enforced_blit(src, page, rect{0, 0, 1, 1}, point{x - mx, y - my});
                    enforced_blit(src, page, rect{w - 1, 0, 1, 1}, point{x + w + mx - 1, y - my});
                    enforced_blit(src, page, rect{0, h - 1, 1, 1}, point{x - mx, y + h + my - 1});
                    enforced_blit(src, page, rect{w - 1, h - 1, 1, 1}, point{x + w + mx - 1, y + h + my - 1});
                }
            }
        }

        // An empty page in the output format, filled with "transparent": the colour key
        // (or index 0) for indexed pages, alpha 0 otherwise.
        [[nodiscard]] image make_page(dim size, pixel_format format, const image* indexed_like) {
            // NB: ENFORCE the has_value(), not the expected itself -- ENFORCE moves the value out.
            auto page = image::create_rgb(size.width, size.height, format);
            ENFORCE(page.has_value(), "Can not create an atlas page", size.width, "x", size.height);
            if (indexed_like) {
                if (const auto pal = page->set_palette(indexed_like->get_palette()); !pal) {
                    THROW_RUNTIME("Can not set the atlas page palette:", pal.error());
                }
                const auto key = indexed_like->color_key();
                if (key) {
                    ENFORCE(page->set_color_key(*key).has_value());
                    // A new page is zero-filled; write the key index (fill() would map a
                    // colour to the nearest palette entry, which need not be the key).
                    auto* pixels = static_cast <std::uint8_t*>(page->get_pixels());
                    const auto pitch = static_cast <std::size_t>(page->get_pitch());
                    for (int y = 0; y < size.height; ++y) {
                        std::memset(pixels + static_cast <std::size_t>(y) * pitch, static_cast <int>(*key),
                                    static_cast <std::size_t>(size.width));
                    }
                }
            } else {
                ENFORCE(page->fill(color{0, 0, 0, 0}).has_value());
            }
            return std::move(*page);
        }
    }

    image_atlas::image_atlas(std::vector <image>&& pages, std::vector <atlas_region>&& regions)
        : pages(std::move(pages)),
          regions(std::move(regions)) {
        // Regions keep bank order (position is the contract), so ids get their own sorted
        // index; ties sort by position, so a repeated id finds its first one.
        by_id.reserve(this->regions.size());
        for (std::size_t i = 0; i < this->regions.size(); ++i) {
            by_id.emplace_back(this->regions[i].id, i);
        }
        std::sort(by_id.begin(), by_id.end());
    }

    image_atlas::image_atlas(image_atlas&&) noexcept = default;
    image_atlas& image_atlas::operator =(image_atlas&&) noexcept = default;

    std::size_t image_atlas::page_count() const noexcept {
        return pages.size();
    }

    const image& image_atlas::page(std::size_t i) const {
        ENFORCE(i < pages.size(), "Page", i, "is out of range; the atlas has", pages.size(), "pages");
        return pages[i];
    }

    std::size_t image_atlas::region_count() const noexcept {
        return regions.size();
    }

    const atlas_region& image_atlas::region(size_t i) const {
        ENFORCE(i < regions.size(), "Region", i, "is out of range; the atlas has", regions.size(), "regions");
        return regions[i];
    }

    std::optional <size_t> image_atlas::find_by_id(uint32_t id) const {
        const auto it = std::lower_bound(by_id.begin(), by_id.end(), std::pair <uint32_t, std::size_t>{id, 0});
        if (it != by_id.end() && it->first == id) {
            return it->second;
        }
        return std::nullopt;
    }



    image_atlas build_atlas(const image_bank& bank, std::optional <pixel_format> atlas_texture_format,
                            std::optional <dim> max_page) {
        const int cap = default_page_cap();
        const dim max_dims = max_page ? *max_page : dim{cap, cap};
        constexpr int gutter = 1;

        // Output format: indexed when asked for (the bank must qualify) or, by default,
        // when the bank qualifies; otherwise the requested format, RGBA32 by default.
        const image* indexed_like = indexed_source(bank);
        pixel_format format = pixel_format::RGBA32;
        if (atlas_texture_format) {
            format = *atlas_texture_format;
            if (format == pixel_format::INDEX8) {
                ENFORCE(indexed_like != nullptr,
                    "An indexed atlas needs INDEX8 pictures sharing one palette and colour key");
            } else {
                indexed_like = nullptr;
            }
        } else if (indexed_like) {
            format = pixel_format::INDEX8;
        }

        const auto layout = details::shelf_pack(std::span(bank.entries), max_dims, gutter);

        std::vector <image> pages;
        pages.reserve(layout.pages.size());
        for (const auto& size : layout.pages) {
            pages.push_back(make_page(size, format, indexed_like));
        }

        std::vector <atlas_region> regions(bank.entries.size());

        static uint32_t atlas_id_counter = 0;
        auto current_atlas_id = details::id_access::make <atlas_id>(atlas_id_counter++);

        for (std::size_t i = 0; i < bank.entries.size(); ++i) {
            const auto& entry = bank.entries[i];
            const auto& placement = layout.placements[i];

            regions[i] = atlas_region{
                .atlas = current_atlas_id,
                .page = placement.page,
                .dest = placement.dest,
                .id   = entry.id
            };
            if (is_empty(entry.pixels)) {
                continue;
            }
            auto& page = pages[placement.page];
            if (indexed_like) {
                // Same palette: a straight index copy; colour-keyed pixels are skipped and
                // keep the page's key fill.
                enforced_blit(entry.pixels, page, rect{0, 0, placement.dest.w, placement.dest.h},
                              point{placement.dest.x, placement.dest.y});
                extrude_border(entry.pixels, page, placement.dest, gutter);
            } else {
                // Convert first (a colour key becomes alpha 0), then copy with blending off
                // so partial alpha is stored as-is instead of blended onto the clear page.
                auto converted = entry.pixels.convert(format);
                ENFORCE(converted.has_value(), "Can not convert entry", i, "to the atlas format");
                ENFORCE(converted->set_blend_mode(sdlpp::blend_mode::none).has_value());
                enforced_blit(*converted, page, rect{0, 0, placement.dest.w, placement.dest.h},
                              point{placement.dest.x, placement.dest.y});
                extrude_border(*converted, page, placement.dest, gutter);
            }
        }

        return image_atlas{std::move(pages), std::move(regions)};
    }
}
