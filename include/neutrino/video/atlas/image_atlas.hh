//
// Created by igor on 29/10/2026.
//

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

#include <neutrino/neutrino_export.h>
#include <neutrino/detail/id_strong_type.hh>
#include <neutrino/video/image/image_bank.hh>
#include <neutrino/video/geometry_types.hh>

namespace neutrino {
    namespace details {
        struct atlas_id_tag;
    }

    class NEUTRINO_EXPORT atlas_id
        : public details::id_strong_type <details::atlas_id_tag> {
        friend struct details::id_access;

        public:
            atlas_id() = default;

        private:
            explicit atlas_id(std::uint32_t value)
                : id_strong_type(value) {
            }
    };

    struct atlas_region {
        atlas_id atlas;
        unsigned page = 0; // index into image_atlas::pages
        rect dest; // where on that page
        uint32_t id = 0; // carried from the bank entry
    };

    class image_atlas;

    [[nodiscard]] NEUTRINO_EXPORT image_atlas build_atlas(const image_bank& bank,
                                                          std::optional <pixel_format> atlas_texture_format = {},
                                                          std::optional <dim> max_page = {});

    class NEUTRINO_EXPORT image_atlas {
        friend image_atlas build_atlas(const image_bank&, std::optional <pixel_format>, std::optional <dim>);

        public:
            image_atlas(const image_atlas&) = delete;
            image_atlas& operator =(const image_atlas&) = delete;

            image_atlas(image_atlas&&) noexcept;
            image_atlas& operator =(image_atlas&&) noexcept;

            /// @brief Number of pages (images); each becomes one texture on upload.
            [[nodiscard]] std::size_t page_count() const noexcept;
            /// @brief Page @p i. @throws std::runtime_error when @p i is out of range.
            [[nodiscard]] const image& page(std::size_t i) const;

            /// @brief Number of regions; equals the size of the bank the atlas was built from.
            [[nodiscard]] std::size_t region_count() const noexcept;
            /// @brief Region @p i, i.e. where bank entry @p i landed. @throws std::runtime_error when out of range.
            [[nodiscard]] const atlas_region& region(size_t i) const;
            /// @brief Position of the region whose @c id is @p id, or nullopt. Ids are the game's
            /// and need not be unique: when several regions share @p id, the first one is returned.
            [[nodiscard]] std::optional <size_t> find_by_id(uint32_t id) const;

        private:
            image_atlas(std::vector <image>&& pages, std::vector <atlas_region>&& regions);

        private:
            std::vector <image> pages;
            std::vector <atlas_region> regions; // regions[i] == bank entry i
            std::vector <std::pair <uint32_t, std::size_t>> by_id; // (id, position), sorted, for find_by_id
    };
}

template<>
struct std::hash <neutrino::atlas_id>
    : std::hash <neutrino::details::id_strong_type <neutrino::details::atlas_id_tag>> {
};
