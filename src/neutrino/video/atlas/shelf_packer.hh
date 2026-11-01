//
// Created by igor on 29/10/2026.
//

#pragma once

#include <span>
#include <vector>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <failsafe/enforce.hh>
#include <neutrino/video/geometry_types.hh>

namespace neutrino::details {
    struct shelf_placement {
        unsigned page = 0;
        rect dest; // picture rect, inside its gutter
    };

    struct shelf_layout {
        std::vector <dim> pages; // trimmed size of each page
        std::vector <shelf_placement> placements; // placements[i] = sizes[i]
    };

    template<typename T>
    struct dimensions_access;

    struct placeable {
        placeable(const dim& rect, std::size_t index, int gutter)
            : rect(rect.width + 2 * gutter, rect.height + 2 * gutter),
              index(index) {
            //  Each picture occupies a cell of cw = w + 2g by ch = h + 2g.
        }

        [[nodiscard]] std::int64_t area() const noexcept {
            return static_cast <std::int64_t>(rect.width) * static_cast <std::int64_t>(rect.height);
        }

        dim rect;
        std::size_t index;
    };

    // throws when a cell (size + 2*gutter) exceeds max_page
    template<typename WithDimensions>
    shelf_layout shelf_pack(std::span <const WithDimensions> entries, dim max_page, int gutter) {
        ENFORCE(gutter >= 0, "Gutter must not be negative, got", gutter);
        ENFORCE(max_page.width > 0 && max_page.height > 0,
            "Page size must be positive, got", max_page.width, "x", max_page.height);

        shelf_layout rc;
        rc.placements.resize(entries.size());

        // 1. Validate. If any cell is wider or taller than M, throw, naming the entry. An empty bank gives an atlas with no pages.
        std::vector <placeable> placeables;
        int widest = 0;
        std::int64_t area = 0;
        for (std::size_t i = 0; i < entries.size(); i++) {
            const dim d = dimensions_access <WithDimensions>::load_dim(entries[i]);
            ENFORCE(d.width >= 0 && d.height >= 0, "Entry", i, "has a negative size", d.width, "x", d.height);
            ENFORCE(d.width + 2 * gutter <= max_page.width && d.height + 2 * gutter <= max_page.height,
                "Entry", i, "(", d.width, "x", d.height, "+ gutter", gutter, ") does not fit page",
                 max_page.width, "x", max_page.height);
            if (d.width > 0 && d.height > 0) {
                const auto& p = placeables.emplace_back(d, i, gutter);
                widest = std::max(widest, p.rect.width);
                area = area + p.area();
            }
        }

        if (placeables.empty()) {
            return rc;
        }

        const int area_sq = static_cast <int>(std::ceil(std::sqrt(static_cast <double>(area))));
        auto w = std::clamp(area_sq, widest, max_page.width);

        std::stable_sort(placeables.begin(), placeables.end(), [](const placeable& a, const placeable& b) {
            return b.rect.height < a.rect.height;
        });
        int page = 0, y = 0, x = 0, shelf_h = 0, page_w = 0;

        for (const auto& p : placeables) {
            if (x + p.rect.width > w) {
                y += shelf_h;
                x = 0;
                shelf_h = 0;
            }
            if (y + p.rect.height > max_page.height) {
                // page full -> next page; a break only happens at the start of a shelf,
                // so y already includes the page's last shelf
                rc.pages.emplace_back(page_w, y);
                page += 1;
                y = 0;
                x = 0;
                shelf_h = 0;
                page_w = 0;
            }
            rc.placements[p.index].page = static_cast <unsigned>(page);
            rc.placements[p.index].dest = {
                x + gutter, y + gutter, p.rect.width - 2 * gutter, p.rect.height - 2 * gutter
            };
            x += p.rect.width;
            page_w = std::max(page_w, x);
            shelf_h = std::max(shelf_h, p.rect.height);
        }
        rc.pages.emplace_back(page_w, y + shelf_h);
        return rc;
    }
}
