//
// Created by igor on 29/10/2026.
//

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

#include <neutrino/video/atlas/atlas_id.hh>
#include <neutrino/neutrino_export.h>
#include <neutrino/detail/id_strong_type.hh>
#include <neutrino/video/image/image_bank.hh>
#include <neutrino/video/geometry_types.hh>

namespace neutrino {


    struct NEUTRINO_EXPORT atlas_address {
        atlas_address(const atlas_id& atlas, std::size_t picture_idx);
        atlas_id atlas;
        std::size_t picture_idx;
    };

    struct NEUTRINO_EXPORT atlas_region : public atlas_address{
        atlas_region(const atlas_id& atlas, std::size_t picture_idx, uint32_t id);
        uint32_t id = 0;
    };

    class image_atlas;
    class gpu_texture_atlas;
    [[nodiscard]] NEUTRINO_EXPORT image_atlas build_atlas(const image_bank& bank,
                                                          std::optional <pixel_format> atlas_texture_format = {},
                                                          std::optional <dim> max_page = {});

    [[nodiscard]] NEUTRINO_EXPORT image_atlas build_atlas(const image& img,
                                                          std::optional <pixel_format> atlas_texture_format = {});

    class NEUTRINO_EXPORT image_atlas {
        friend image_atlas build_atlas(const image_bank&, std::optional <pixel_format>, std::optional <dim>);
        friend image_atlas build_atlas(const image& img, std::optional <pixel_format> atlas_texture_format);
        friend class gpu_texture_atlas;
        public:
            image_atlas(const image_atlas&) = delete;
            image_atlas& operator =(const image_atlas&) = delete;

            image_atlas(image_atlas&&) noexcept;
            image_atlas& operator =(image_atlas&&) noexcept;



            /// @brief Number of pictures; equals the size of the bank the atlas was built from.
            [[nodiscard]] std::size_t picture_count() const noexcept;


            /// @brief Address of picture @p picture_idx, i.e. bank entry @p picture_idx. @throws std::runtime_error when out of range.
            [[nodiscard]] const atlas_region& region(size_t picture_idx) const;

            /// @brief Size of picture @p picture_idx. @throws std::runtime_error when out of range.
            [[nodiscard]] dim picture_size(size_t picture_idx) const;
            /// @brief The region whose @c id is @p id, or nullopt. Ids are the game's and need
            /// not be unique: when several regions share @p id, the first one is returned.
            [[nodiscard]] std::optional <atlas_region> find_by_id(uint32_t id) const;

            /// @brief This atlas's id, assigned by build_atlas; every region's @c atlas equals it.
            [[nodiscard]] atlas_id id() const noexcept;

        private:

            struct atlas_placement {
                atlas_placement(unsigned page, const rect& picture)
                    : page(page),
                      picture(picture) {
                }
                unsigned page;
                rect picture;
            };

            image_atlas(atlas_id self_id, std::vector<atlas_placement>&& rects, std::vector <image>&& pages, std::vector <atlas_region>&& regions);

        private:
            atlas_id m_atlas;
            std::vector <image> m_pages;
            std::vector <atlas_region> m_regions; // regions[i] == bank entry i
            std::vector <atlas_placement> m_rects; // m_rects[i] == where picture i landed
            std::vector <std::pair <uint32_t, std::size_t>> m_by_id; // (id, position), sorted, for find_by_id
    };
}


