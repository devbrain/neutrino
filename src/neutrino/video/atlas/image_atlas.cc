//
// Created by igor on 29/10/2026.
//

#include <algorithm>
#include <cstring>
#include <utility>

#include "neutrino/video/atlas/image_atlas.hh"

#include <failsafe/enforce.hh>
#include <failsafe/exception.hh>
#include <neutrino/video/color.hh>

#include "shelf_packer.hh"
#include "services/service_locator.hh"
#include "atlas_id_gen.hh"

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

        // The bank can be packed as indexed pages: every non-empty picture is INDEX8 with
        // the same palette and the same colour key (or none). Returns the first such picture.
        [[nodiscard]] const image* indexed_source(const image& pixels) {
            if (is_empty(pixels)) {
                return nullptr;
            }
            if (pixels.format() != pixel_format::INDEX8) {
                return nullptr;
            }
            return &pixels;
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

    image_atlas::image_atlas(atlas_id self_id, std::vector <atlas_placement>&& rects, std::vector <image>&& pages,
                             std::vector <atlas_region>&& regions)
        : m_atlas(self_id),
          m_pages(std::move(pages)),
          m_regions(std::move(regions)),
          m_rects(std::move(rects)) {
        // Regions keep bank order and the region is the picture's address; ids are the
        // game's, so they get their own sorted index. Ties sort by position, so a repeated
        // id finds its first one.
        m_by_id.reserve(this->m_regions.size());
        for (std::size_t i = 0; i < this->m_regions.size(); ++i) {
            m_by_id.emplace_back(this->m_regions[i].id, i);
        }
        std::sort(m_by_id.begin(), m_by_id.end());
    }

    image_atlas::image_atlas(image_atlas&&) noexcept = default;
    image_atlas& image_atlas::operator =(image_atlas&&) noexcept = default;


    std::size_t image_atlas::picture_count() const noexcept {
        return m_regions.size();
    }

    const atlas_region& image_atlas::region(size_t picture_idx) const {
        ENFORCE(picture_idx < m_regions.size(), "Picture", picture_idx, "is out of range; the atlas has", m_regions.size(), "pictures");
        return m_regions[picture_idx];
    }

    dim image_atlas::picture_size(size_t picture_idx) const {
        ENFORCE(picture_idx < m_rects.size(), "Picture", picture_idx, "is out of range; the atlas has", m_rects.size(), "pictures");
        const auto& reg = m_rects[picture_idx];
        return reg.picture.dimensions();
    }

    std::optional <atlas_region> image_atlas::find_by_id(uint32_t id) const {
        const auto it = std::lower_bound(m_by_id.begin(), m_by_id.end(), std::pair <uint32_t, std::size_t>{id, 0});
        if (it != m_by_id.end() && it->first == id) {
            return region(it->second);
        }
        return std::nullopt;
    }

    atlas_id image_atlas::id() const noexcept {
        return m_atlas;
    }

    atlas_address::atlas_address(const atlas_id& atlas, std::size_t picture_idx)
        : atlas(atlas),
          picture_idx(picture_idx) {
    }

    atlas_region::atlas_region(const atlas_id& atlas, std::size_t picture_idx, uint32_t id)
        : atlas_address(atlas, picture_idx),
          id(id) {
    }

    namespace {
        // has_pictures: false for a bank of only empty entries, which makes no pages, so any
        // requested format is fine for it.
        pixel_format deduce_image_format(std::optional <pixel_format> atlas_texture_format,
                                         const image*& indexed_like, bool has_pictures = true) {
            pixel_format format = pixel_format::RGBA32;
            if (atlas_texture_format) {
                format = *atlas_texture_format;
                if (format == pixel_format::INDEX8) {
                    ENFORCE(indexed_like != nullptr || !has_pictures,
                            "An indexed atlas needs INDEX8 pictures sharing one palette and colour key");
                } else {
                    indexed_like = nullptr;
                }
            } else if (indexed_like) {
                format = pixel_format::INDEX8;
            }
            return format;
        }
    }

    image_atlas build_atlas(const image_bank& bank, std::optional <pixel_format> atlas_texture_format,
                            std::optional <dim> max_page) {
        const int cap = default_page_cap();
        const dim max_dims = max_page ? *max_page : dim{cap, cap};
        constexpr int gutter = 1;

        // Output format: indexed when asked for (the bank must qualify) or, by default,
        // when the bank qualifies; otherwise the requested format, RGBA32 by default.
        const image* indexed_like = indexed_source(bank);
        const bool has_pictures = std::any_of(bank.entries.begin(), bank.entries.end(),
                                              [](const auto& e) { return !is_empty(e.pixels); });
        pixel_format format = deduce_image_format(atlas_texture_format, indexed_like, has_pictures);

        const auto layout = details::shelf_pack(std::span(bank.entries), max_dims, gutter);

        std::vector <image> pages;
        pages.reserve(layout.pages.size());

        std::vector <image_atlas::atlas_placement> placements;
        placements.reserve(layout.placements.size());
        for (const auto& size : layout.pages) {
            pages.push_back(make_page(size, format, indexed_like));
        }
        for (const auto& p : layout.placements) {
            placements.emplace_back(p.page, p.dest);
        }

        std::vector <atlas_region> regions;
        regions.reserve(bank.entries.size());

        auto current_atlas_id = details::make_atlas_id();

        for (std::size_t i = 0; i < bank.entries.size(); ++i) {
            const auto& entry = bank.entries[i];
            const auto& placement = layout.placements[i];
            regions.emplace_back(current_atlas_id, i, entry.id);
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

        return image_atlas{current_atlas_id, std::move(placements), std::move(pages), std::move(regions)};
    }

    image_atlas build_atlas(const image& img, std::optional <pixel_format> atlas_texture_format) {

        const int cap = default_page_cap();
        auto size = img.dimensions <dim>();
        ENFORCE(size.width > 0 && size.height > 0, "Can not build atlas from the empty image");
        ENFORCE(size.width <= cap && size.height <= cap, "Image size is larger than the maximal texture size");

        auto current_atlas_id = details::make_atlas_id();

        const image* indexed_like = indexed_source(img);
        pixel_format format = deduce_image_format(atlas_texture_format, indexed_like);
        std::vector <image> pages;
        pages.reserve(1);
        pages.push_back(make_page(size, format, indexed_like));

        std::vector <atlas_region> regions;
        regions.reserve(1);

        std::vector <image_atlas::atlas_placement> picture_rects;

        const auto placement = details::shelf_placement{
            .page = 0,
            .dest = rect{point{0, 0}, img.dimensions <dim>()}
        };

        picture_rects.emplace_back(0, placement.dest);
        regions.emplace_back(current_atlas_id, 0, 0);

        if (indexed_like) {
            // Same palette: a straight index copy; colour-keyed pixels are skipped and
            // keep the page's key fill.
            enforced_blit(img, pages[0], rect{0, 0, placement.dest.w, placement.dest.h},
                          point{placement.dest.x, placement.dest.y});
        } else {
            // Convert first (a colour key becomes alpha 0), then copy with blending off
            // so partial alpha is stored as-is instead of blended onto the clear page.
            auto converted = img.convert(format);
            ENFORCE(converted.has_value(), "Can not convert image to the atlas format");
            ENFORCE(converted->set_blend_mode(sdlpp::blend_mode::none).has_value());
            enforced_blit(*converted, pages[0], rect{0, 0, placement.dest.w, placement.dest.h},
                          point{placement.dest.x, placement.dest.y});
        }
        return image_atlas{current_atlas_id, std::move(picture_rects), std::move(pages), std::move(regions)};
    }
}
