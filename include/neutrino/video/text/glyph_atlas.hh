//
// Created by igor on 05/11/2026.
//

#pragma once

#include <optional>
#include <unordered_set>
#include <vector>
#include <limits>
#include <algorithm>
#include <cstdint>

#include <neutrino/neutrino_export.h>
#include <neutrino/video/atlas/image_atlas.hh>
#include <neutrino/video/image/image_bank.hh>
#include <neutrino/video/image/image_builder.hh>
#include <neutrino/video/color.hh>
#include <neutrino/video/palette.hh>

#include <failsafe/enforce.hh>

namespace neutrino {
    namespace details {
        struct glyph_atlas_access;
    }

    /**
     * @brief Metrics of one glyph (font_plan.md F6, P5).
     *
     * Coordinates: x right, y down. The decoder states only @ref glyph and @ref advance (in
     * `start_glyph`); `end_glyph` derives the rest from the pixels written, which are given
     * relative to the pen on the baseline.
     */
    struct NEUTRINO_EXPORT glyph_properties {
        /// The game's id for the glyph (character code, glyph number); the image bank entry id.
        std::uint32_t glyph = 0;

        /// How far the pen moves right after drawing the glyph.
        std::int16_t advance = 0;

        /// Pen to the left edge of the glyph's picture. May be negative (ink left of the pen).
        std::int16_t bearing_x = 0;

        /// Top of the line cell to the top edge of the glyph's picture (the cell top is
        /// `baseline` above the pen). Ink above the cell gives a negative offset.
        std::int16_t y_offset = 0;
    };

    /**
     * @brief Metrics of the whole font (font_plan.md F6).
     *
     * Given to the builder as they are; the builder does not derive any of them.
     */
    struct NEUTRINO_EXPORT font_properties {
        /// Distance from one line's top to the next line's top.
        std::int16_t line_height = 0;

        /// Distance from the top of the line cell down to the baseline the text sits on.
        std::int16_t baseline = 0;

        /// Advance of a space, for fonts without a space glyph.
        std::int16_t space_advance = 0;

        /// Extra gap the format adds after every glyph, on top of its advance.
        std::int16_t letter_spacing = 0;

        /// Glyph drawn for code points the font does not map.
        std::uint32_t default_glyph = 0;
    };

    enum class glyph_atlas_format {
        alpha,
        indexed,
        rgba
    };

    template<glyph_atlas_format format>
    class glyph_atlas_builder;

    class NEUTRINO_EXPORT glyph_atlas {
        friend struct details::glyph_atlas_access;

        template<glyph_atlas_format format>
        friend class glyph_atlas_builder;

        public:
            [[nodiscard]] std::size_t glyph_count() const noexcept;
            /// @throws std::runtime_error when @p idx is not less than @ref glyph_count.
            [[nodiscard]] const glyph_properties& get_glyph_properties(std::size_t idx) const;

            [[nodiscard]] font_properties get_font_properties() const noexcept;

        private:
            glyph_atlas(const font_properties& font_props, image_atlas&& atlas, std::vector <glyph_properties>&& props);

        private:
            font_properties m_font_props;
            image_atlas m_atlas;
            std::vector <glyph_properties> m_props;
    };

    namespace details {
        template<glyph_atlas_format format>
        struct glyph_canvas_pixel;

        template<>
        struct glyph_canvas_pixel <glyph_atlas_format::alpha> {
            using type = std::uint8_t;
            static constexpr auto kind = image_kind::rgba;
        };

        template<>
        struct glyph_canvas_pixel <glyph_atlas_format::indexed> {
            using type = std::uint8_t;
        };

        template<>
        struct glyph_canvas_pixel <glyph_atlas_format::rgba> {
            using type = color;
            static constexpr auto kind = image_kind::rgba;
        };

        template<glyph_atlas_format format>
        class glyph_canvas_base {
            public:
                using pixel_type = typename glyph_canvas_pixel <format>::type;

            public:
                glyph_canvas_base()
                    : m_min_x(std::numeric_limits <int>::max()),
                      m_min_y(std::numeric_limits <int>::max()),
                      m_max_x(std::numeric_limits <int>::min()),
                      m_max_y(std::numeric_limits <int>::min()) {
                }

                void put(int x, int y, pixel_type pixel) {
                    m_min_x = std::min(x, m_min_x);
                    m_min_y = std::min(y, m_min_y);
                    m_max_x = std::max(x, m_max_x);
                    m_max_y = std::max(y, m_max_y);

                    m_commands.emplace_back(x, y, pixel);
                }

            protected:
                void clear() {
                    m_min_x = std::numeric_limits <int>::max();
                    m_max_x = std::numeric_limits <int>::min();
                    m_min_y = std::numeric_limits <int>::max();
                    m_max_y = std::numeric_limits <int>::min();
                    m_commands.clear();
                }

                [[nodiscard]] dim get_dims() const noexcept {
                    const int w = this->m_max_x - this->m_min_x + 1;
                    const int h = this->m_max_y - this->m_min_y + 1;
                    return {w, h};
                }

                template<typename Builder>
                void apply(Builder& builder) {
                    for (const auto& c : m_commands) {
                        if constexpr (format == glyph_atlas_format::alpha) {
                            builder.set(c.x - m_min_x, c.y - m_min_y, color{0xFF, 0xFF, 0xFF, c.value});
                        } else {
                            builder.set(c.x - m_min_x, c.y - m_min_y, c.value);
                        }
                    }
                }

                [[nodiscard]] bool empty() const noexcept {
                    return m_commands.empty();
                }

            protected:
                struct command {
                    int x;
                    int y;
                    pixel_type value;
                };

                int m_min_x;
                int m_min_y;
                int m_max_x;
                int m_max_y;
                std::vector <command> m_commands;
        };

        template<glyph_atlas_format format>
        class glyph_canvas : public glyph_canvas_base <format> {
            protected:
                image create() {
                    image_builder <glyph_canvas_pixel <format>::kind> builder(this->get_dims());
                    if constexpr (format == glyph_atlas_format::alpha) {
                        builder.fill({0xFF, 0xFF, 0xFF, 0});
                    }
                    this->apply(builder);
                    return std::move(builder).build();
                }
        };

        template<>
        class glyph_canvas <glyph_atlas_format::indexed> : public glyph_canvas_base <glyph_atlas_format::indexed> {
            public:
                explicit glyph_canvas(std::optional <std::uint8_t> transparent_index = {})
                    : m_transparent_index(transparent_index),
                      m_palette(nullptr) {
                }

                explicit glyph_canvas(const palette& pal, std::optional <std::uint8_t> transparent_index = {})
                    : m_transparent_index(transparent_index),
                      m_palette(&pal) {
                }

            protected:
                image create() {
                    if (m_palette) {
                        image_builder <image_kind::indexed> builder(this->get_dims(), *m_palette, m_transparent_index);
                        return finish(std::move(builder));
                    }
                    image_builder <image_kind::indexed> builder(this->get_dims(), m_transparent_index);
                    return finish(std::move(builder));
                }

            private:
                // Pixels not written are transparent: the key index when there is one
                // (a fresh image is index 0, which may be a visible colour).
                image finish(image_builder <image_kind::indexed>&& builder) {
                    if (m_transparent_index) {
                        builder.fill(*m_transparent_index);
                    }
                    this->apply(builder);
                    return std::move(builder).build();
                }

            private:
                std::optional <std::uint8_t> m_transparent_index;
                const palette* m_palette;
        };
    }

    template<glyph_atlas_format format>
    class glyph_atlas_builder : public details::glyph_canvas <format> {
        public:
            explicit glyph_atlas_builder(font_properties props)
                : m_font_props(props) {
            }

            glyph_atlas_builder(font_properties props, std::optional <std::uint8_t> transparent_index)
                requires (format == glyph_atlas_format::indexed)
                : details::glyph_canvas <glyph_atlas_format::indexed>(transparent_index),
                  m_font_props(props) {
            }

            glyph_atlas_builder(font_properties props, const palette& pal,
                                std::optional <std::uint8_t> transparent_index)
                requires (format == glyph_atlas_format::indexed)
                : details::glyph_canvas <glyph_atlas_format::indexed>(pal, transparent_index),
                  m_font_props(props) {
            }

            /**
             * @brief Start glyph @p glyph_id. Pixels written until @ref end_glyph belong to it.
             *
             * @param glyph_id The game's id for the glyph; also its picture's bank entry id.
             * @param props The glyph's @c advance; @c glyph, @c bearing_x and @c y_offset are
             *        set by the builder.
             * @throws std::runtime_error when a glyph is already started.
             */
            void start_glyph(std::uint32_t glyph_id, glyph_properties props = {}) {
                ENFORCE(!m_in_glyph, "start_glyph(", glyph_id, "): the previous glyph is not ended");
                this->clear();
                props.glyph = glyph_id;
                m_props.push_back(props);
                m_in_glyph = true;
            }

            /**
             * @brief Finish the started glyph: crop its pixels to their bounds and derive
             *        @c bearing_x and @c y_offset. A glyph without pixels gets an empty picture.
             *
             * @throws std::runtime_error when no glyph is started, or the glyph lies further than
             *         an @c int16_t from the pen.
             */
            void end_glyph() {
                ENFORCE(m_in_glyph, "end_glyph() without start_glyph()");
                m_in_glyph = false;
                auto& p = m_props.back();
                if (this->empty()) {
                    m_bank.entries.push_back({image{}, p.glyph}); // blank: region of size 0
                    return;
                }
                p.bearing_x = to_int16(this->m_min_x, p.glyph);
                // Pixels are relative to the pen on the baseline; the cell top is baseline above it.
                p.y_offset = to_int16(this->m_min_y + m_font_props.baseline, p.glyph);
                m_bank.entries.push_back({this->create(), p.glyph});
            }

            /// @brief A glyph without pixels (e.g. space): only advances the pen.
            void empty_glyph(std::uint32_t glyph_id, glyph_properties props = {}) {
                start_glyph(glyph_id, props);
                end_glyph();
            }

            /**
             * @brief Pack the glyphs into an atlas. Glyph @c i is picture @c i.
             *
             * @throws std::runtime_error when a glyph is started but not ended, or a glyph id
             *         is used twice.
             */
            glyph_atlas build(std::optional <pixel_format> atlas_texture_format = {},
                              std::optional <dim> max_page = {}) && {
                ENFORCE(!m_in_glyph, "build(): the last glyph is not ended");
                std::unordered_set <std::uint32_t> ids;
                ids.reserve(m_props.size());
                for (const auto& p : m_props) {
                    ENFORCE(ids.insert(p.glyph).second, "build(): glyph", p.glyph, "is defined twice");
                }
                return {
                    m_font_props,
                    build_atlas(m_bank, atlas_texture_format, max_page),
                    std::move(m_props)
                };
            }

        private:
            static std::int16_t to_int16(int v, std::uint32_t glyph) {
                ENFORCE(v >= std::numeric_limits <std::int16_t>::min() && v <= std::numeric_limits <std::int16_t>::max(),
                        "Glyph", glyph, "is too far from the pen:", v);
                return static_cast <std::int16_t>(v);
            }

            font_properties m_font_props;
            image_bank m_bank;
            std::vector <glyph_properties> m_props;
            bool m_in_glyph = false;
    };
}
