//
// Test-only read access to an image_atlas's pages and placements (they are private: an
// atlas is addressed by picture, and pages are storage). Header-only, so every test TU
// sees the same definition.
//
#pragma once

#include <cstddef>

#include <failsafe/enforce.hh>

#include <neutrino/video/atlas/image_atlas.hh>

namespace neutrino::details {
    struct image_atlas_access {
        /// @brief Where a picture landed: its page and its rect on that page.
        struct placement {
            unsigned page;
            rect dest;
        };

        static std::size_t page_count(const image_atlas& a) {
            return a.m_pages.size();
        }

        static const image& page(const image_atlas& a, std::size_t p) {
            ENFORCE(p < a.m_pages.size(), "Page", p, "is out of range");
            return a.m_pages[p];
        }

        static placement place(const image_atlas& a, std::size_t i) {
            ENFORCE(i < a.m_rects.size(), "Picture", i, "is out of range");
            return {a.m_rects[i].page, a.m_rects[i].picture};
        }
    };
}

namespace atlas_test {
    inline std::size_t page_count(const neutrino::image_atlas& a) {
        return neutrino::details::image_atlas_access::page_count(a);
    }

    inline const neutrino::image& page(const neutrino::image_atlas& a, std::size_t p) {
        return neutrino::details::image_atlas_access::page(a, p);
    }

    inline neutrino::details::image_atlas_access::placement place(const neutrino::image_atlas& a, std::size_t i) {
        return neutrino::details::image_atlas_access::place(a, i);
    }
}
